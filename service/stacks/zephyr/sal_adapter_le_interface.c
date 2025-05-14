/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for th specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#include "sal_adapter_le_interface.h"

#include "adapter_internel.h"
#include "gattc_service.h"
#include "gatts_service.h"
#include "sal_interface.h"
#include "service_loop.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>

#include <zephyr/settings/settings.h>

#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT

#define STACK_CALL(func) zblue_##func

typedef void (*sal_func_t)(void* args);

typedef union {
    struct bt_conn_le_phy_param phy_param;
    struct {
        struct bt_conn_le_create_param create;
        struct bt_le_conn_param conn;
    } conn_param;
} sal_adapter_args_t;

typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type;
    sal_func_t func;
    sal_adapter_args_t adpt;
} sal_adapter_req_t;

typedef struct {
    remote_device_le_properties_t* props;
    uint16_t* cnt;
} device_context_t;

extern int zblue_main(void);

static void zblue_on_connected(struct bt_conn* conn, uint8_t err);
static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason);
static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level, enum bt_security_err err);
static void zblue_on_pairing_complete(struct bt_conn* conn, bool bonded);
static void zblue_on_pairing_failed(struct bt_conn* conn, enum bt_security_err reason);
static void zblue_on_bond_deleted(uint8_t id, const bt_addr_le_t* peer);
static void zblue_on_phy_updated(struct bt_conn* conn, struct bt_conn_le_phy_info* info);
static void zblue_on_param_updated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout);

static void zblue_on_auth_passkey_display(struct bt_conn* conn, unsigned int passkey);
static void zblue_on_auth_passkey_confirm(struct bt_conn* conn, unsigned int passkey);
static void zblue_on_auth_passkey_entry(struct bt_conn* conn);
static void zblue_on_auth_cancel(struct bt_conn* conn);
static void zblue_on_auth_pairing_confirm(struct bt_conn* conn);
#ifdef CONFIG_BT_SMP_APP_PAIRING_ACCEPT
static enum bt_security_err zblue_on_pairing_accept(struct bt_conn* conn, const struct bt_conn_pairing_feat* const feat);
#endif

static struct bt_conn_cb g_conn_cbs = {
    .connected = zblue_on_connected,
    .disconnected = zblue_on_disconnected,
    .security_changed = zblue_on_security_changed,
    .le_param_updated = zblue_on_param_updated,
#if defined(CONFIG_BT_USER_PHY_UPDATE)
    .le_phy_updated = zblue_on_phy_updated,
#endif
};

static struct bt_conn_auth_info_cb g_conn_auth_info_cbs = {
    .pairing_complete = zblue_on_pairing_complete,
    .pairing_failed = zblue_on_pairing_failed,
    .bond_deleted = zblue_on_bond_deleted,
};

static struct bt_conn_auth_cb g_conn_auth_cbs;
static struct bt_conn* g_acl_conns[CONFIG_BT_MAX_CONN];

static void zblue_on_connected(struct bt_conn* conn, uint8_t err)
{
    struct bt_conn_info info;
    int i;
    acl_state_param_t state = {
        .transport = BT_TRANSPORT_BLE,
        .connection_state = CONNECTION_STATE_CONNECTED
    };

    BT_LOGD("%s, err:%d", __func__, err);
    bt_conn_get_info(conn, &info);

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    for (i = 0; i < ARRAY_SIZE(g_acl_conns); i++) {
        if (!g_acl_conns[i]) {
            g_acl_conns[i] = conn;
            break;
        }
    }

    memcpy(&state.addr, info.le.dst->a.val, sizeof(state.addr));
    adapter_on_connection_state_changed(&state);
    if (info.role == BT_HCI_ROLE_PERIPHERAL) {
        if_gatts_on_connection_state_changed(&state.addr, PROFILE_STATE_CONNECTED);
    } else if (info.role == BT_HCI_ROLE_CENTRAL) {
        if_gattc_on_connection_state_changed(&state.addr, PROFILE_STATE_CONNECTED);
    }
}

static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason)
{
    struct bt_conn_info info;
    int i;
    acl_state_param_t state = {
        .transport = BT_TRANSPORT_BLE,
        .connection_state = CONNECTION_STATE_DISCONNECTED
    };

    BT_LOGD("%s", __func__);
    bt_conn_get_info(conn, &info);

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    for (i = 0; i < ARRAY_SIZE(g_acl_conns); i++) {
        if (g_acl_conns[i] == conn) {
            g_acl_conns[i] = NULL;
            break;
        }
    }

    memcpy(&state.addr, info.le.dst->a.val, sizeof(state.addr));
    adapter_on_connection_state_changed(&state);
    if (info.role == BT_HCI_ROLE_PERIPHERAL) {
        if_gatts_on_connection_state_changed(&state.addr, PROFILE_STATE_DISCONNECTED);
    } else if (info.role == BT_HCI_ROLE_CENTRAL) {
        if_gattc_on_connection_state_changed(&state.addr, PROFILE_STATE_DISCONNECTED);
    }
}

static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err)
{
    struct bt_conn_info info;
    bt_address_t addr;
    bool encrypted = false;

    BT_LOGD("%s", __func__);
    bt_conn_get_info(conn, &info);

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    memcpy(&addr, info.le.dst->a.val, sizeof(addr));
    if (err) {
        adapter_on_bond_state_changed(&addr, BOND_STATE_NONE, BT_TRANSPORT_BLE, BT_STATUS_FAIL, false);
    }

    if (level >= BT_SECURITY_L2 && err == BT_SECURITY_ERR_SUCCESS) {
        encrypted = true;
    }

    adapter_on_encryption_state_changed(&addr, encrypted, BT_TRANSPORT_BLE);
}

static void zblue_on_param_updated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    struct bt_conn_info info;
    bt_address_t addr;

    bt_conn_get_info(conn, &info);
    memcpy(&addr, info.le.dst->a.val, sizeof(addr));

    BT_LOGD("%s, interval:%d, latency:%d, timeout:%d", __func__, interval, latency, timeout);

    if (info.role == BT_HCI_ROLE_CENTRAL) {
        if_gattc_on_connection_parameter_updated(&addr, interval, latency, timeout, BT_STATUS_SUCCESS);
    }
}

#if defined(CONFIG_BT_USER_PHY_UPDATE)
ble_phy_type_t le_phy_convert_from_stack(uint8_t mode)
{
    ble_phy_type_t phy;

    switch (mode) {
    case BT_GAP_LE_PHY_1M:
        phy = BT_LE_1M_PHY;
        break;
    case BT_GAP_LE_PHY_2M:
        phy = BT_LE_2M_PHY;
        break;
    case BT_GAP_LE_PHY_CODED:
        phy = BT_LE_CODED_PHY;
    default:
        BT_LOGE("%s, invalid phy:%d", __func__, mode);
        assert(0);
        break;
    }

    return phy;
}

uint8_t le_phy_convert_from_service(ble_phy_type_t mode)
{
    uint8_t phy;

    switch (mode) {
    case BT_LE_1M_PHY:
        phy = BT_GAP_LE_PHY_1M;
        break;
    case BT_LE_2M_PHY:
        phy = BT_GAP_LE_PHY_2M;
        break;
    case BT_LE_CODED_PHY:
        phy = BT_GAP_LE_PHY_CODED;
    default:
        BT_LOGE("%s, invalid phy:%d", __func__, mode);
        assert(0);
        break;
    }

    return phy;
}

static void zblue_on_phy_updated(struct bt_conn* conn, struct bt_conn_le_phy_info* phy)
{
    struct bt_conn_info info;
    bt_address_t addr;
    ble_phy_type_t tx_mode;
    ble_phy_type_t rx_mode;

    bt_conn_get_info(conn, &info);

    tx_mode = le_phy_convert_from_stack(phy->tx_phy);
    rx_mode = le_phy_convert_from_stack(phy->rx_phy);

    BT_LOGD("%s, tx phy:%d, rx phy:%d", __func__, tx_mode, rx_mode);
    memcpy(&addr, info.le.dst->a.val, sizeof(addr));

    if_gatts_on_phy_updated(&addr, tx_mode, rx_mode, GATT_STATUS_SUCCESS);

    if (info.role == BT_HCI_ROLE_PERIPHERAL) {
        if_gatts_on_phy_updated(&addr, tx_mode, rx_mode, GATT_STATUS_SUCCESS);
    } else if (info.role == BT_HCI_ROLE_CENTRAL) {
        if_gattc_on_phy_updated(&addr, tx_mode, rx_mode, GATT_STATUS_SUCCESS);
    }
}
#endif /*CONFIG_BT_USER_PHY_UPDATE*/

static void zblue_on_pairing_complete(struct bt_conn* conn, bool bonded)
{
    struct bt_conn_info info;
    bt_address_t addr;
    bond_state_t state;

    BT_LOGD("%s", __func__);
    bt_conn_get_info(conn, &info);

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    memcpy(&addr, info.le.dst->a.val, sizeof(addr));
    if (bonded) {
        state = BOND_STATE_BONDED;
    } else {
        state = BOND_STATE_NONE;
    }

    adapter_on_bond_state_changed(&addr, state, BT_TRANSPORT_BLE, BT_STATUS_SUCCESS, false);
}

static void zblue_on_pairing_failed(struct bt_conn* conn, enum bt_security_err reason)
{
    struct bt_conn_info info;
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    bt_conn_get_info(conn, &info);

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    memcpy(&addr, info.le.dst->a.val, sizeof(addr));
    adapter_on_bond_state_changed(&addr, BOND_STATE_NONE, BT_TRANSPORT_BLE, BT_STATUS_AUTH_FAILURE, false);
    bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static void zblue_on_bond_deleted(uint8_t id, const bt_addr_le_t* peer)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);

    if (id == 0 && peer->type == BT_ADDR_LE_PUBLIC) {
        memcpy(&addr, peer->a.val, sizeof(addr));
        adapter_on_link_key_removed(&addr, BT_STATUS_SUCCESS);
    }
}

static void zblue_on_ready_cb(int err)
{
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    if (err) {
        BT_LOGD("zblue init failed (err %d)\n", err);
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);
        return;
    }

    adapter_on_adapter_state_changed(BLE_STACK_STATE_ON);
}

static sal_adapter_req_t* sal_adapter_req(bt_controller_id_t id, bt_address_t* addr, sal_func_t func)
{
    sal_adapter_req_t* req = calloc(sizeof(sal_adapter_req_t), 1);

    if (req) {
        req->id = id;
        req->func = func;
        if (addr)
            memcpy(&req->addr, addr, sizeof(bt_address_t));
    }

    return req;
}

static void sal_invoke_async(service_work_t* work, void* userdata)
{
    sal_adapter_req_t* req = userdata;

    SAL_ASSERT(req);
    req->func(req);
    free(userdata);
}

static bt_status_t sal_send_req(sal_adapter_req_t* req)
{
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

struct bt_conn* get_le_conn_from_addr(bt_address_t* addr)
{
    for (int i = 0; i < ARRAY_SIZE(g_acl_conns); i++) {
        if (g_acl_conns[i]) {
            struct bt_conn_info info;

            bt_conn_get_info(g_acl_conns[i], &info);
            if (!memcmp(info.le.dst->a.val, addr, sizeof(bt_address_t))) {
                return g_acl_conns[i];
            }
        }
    }

    return NULL;
}

bt_status_t get_le_addr_from_conn(struct bt_conn* conn, bt_address_t* addr)

{
    struct bt_conn_info info;

    if (bt_conn_get_info(conn, &info)) {
        BT_LOGE("%s, get conn info fail", __func__);
        return BT_STATUS_FAIL;
    }

    memcpy(addr, info.le.dst->a.val, sizeof(bt_address_t));
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_le_init(const bt_vhal_interface* vhal)
{
    zblue_main();

    bt_conn_cb_register(&g_conn_cbs);
    bt_conn_auth_info_cb_register(&g_conn_auth_info_cbs);

    return BT_STATUS_SUCCESS;
}

void bt_sal_le_cleanup(void)
{
    bt_conn_cb_register(NULL);
    bt_conn_auth_info_cb_unregister(&g_conn_auth_info_cbs);
}

bt_status_t bt_sal_le_enable(bt_controller_id_t id)
{
    if (bt_is_ready()) {
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_ON);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(bt_enable(zblue_on_ready_cb), 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_le_disable(bt_controller_id_t id)
{
    if (!bt_is_ready()) {
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(bt_disable(), 0);
    adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);

    return BT_STATUS_SUCCESS;
}

static void zblue_on_auth_passkey_display(struct bt_conn* conn, unsigned int passkey)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    if (get_le_addr_from_conn(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_le_addr_from_conn failed", __func__);
        return;
    }

    adapter_on_ssp_request(&addr, BT_TRANSPORT_BLE, 0, PAIR_TYPE_PASSKEY_NOTIFICATION, passkey, NULL);
}

static void zblue_on_auth_passkey_confirm(struct bt_conn* conn, unsigned int passkey)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    if (get_le_addr_from_conn(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_le_addr_from_conn failed", __func__);
        return;
    }

    adapter_on_ssp_request(&addr, BT_TRANSPORT_BLE, 0, PAIR_TYPE_PASSKEY_CONFIRMATION, passkey, NULL);
}

static void zblue_on_auth_passkey_entry(struct bt_conn* conn)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    if (get_le_addr_from_conn(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_le_addr_from_conn failed", __func__);
        return;
    }

    adapter_on_ssp_request(&addr, BT_TRANSPORT_BLE, 0, PAIR_TYPE_PASSKEY_ENTRY, 0, NULL);
}

static void zblue_on_auth_cancel(struct bt_conn* conn)
{
    BT_LOGD("%s, conn: %p", __func__, conn);
}

static void zblue_on_auth_pairing_confirm(struct bt_conn* conn)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    if (get_le_addr_from_conn(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_le_addr_from_conn failed", __func__);
        return;
    }

    adapter_on_ssp_request(&addr, BT_TRANSPORT_BLE, 0, PAIR_TYPE_CONSENT, 0, NULL);
}

#ifdef CONFIG_BT_SMP_APP_PAIRING_ACCEPT
static enum bt_security_err zblue_on_pairing_accept(struct bt_conn* conn, const struct bt_conn_pairing_feat* const feat)
{
    BT_LOGD("Remote pairing features: IO: 0x%02x, OOB: %d, AUTH: 0x%02x, Key: %d, "
            "Init Kdist: 0x%02x, Resp Kdist: 0x%02x",
        feat->io_capability, feat->oob_data_flag,
        feat->auth_req, feat->max_enc_key_size,
        feat->init_key_dist, feat->resp_key_dist);

    if (!bt_addr_le_is_bonded(BT_ID_DEFAULT, &conn->le.dst)) {
        return BT_SECURITY_ERR_SUCCESS;
    }

    BT_LOGD("le bond lost");
    return BT_SECURITY_ERR_SUCCESS;
}
#endif /* CONFIG_BT_SMP_APP_PAIRING_ACCEPT */

bt_status_t bt_sal_le_set_io_capability(bt_controller_id_t id, bt_io_capability_t cap)
{
    BT_LOGD("Set IO capability: %d", cap);

    memset(&g_conn_auth_cbs, 0, sizeof(g_conn_auth_cbs));
    bt_conn_auth_cb_register(NULL);

    switch (cap) {
    case BT_IO_CAPABILITY_DISPLAYONLY:
        g_conn_auth_cbs.passkey_display = zblue_on_auth_passkey_display;
        g_conn_auth_cbs.cancel = zblue_on_auth_cancel;
        break;
    case BT_IO_CAPABILITY_DISPLAYYESNO:
        g_conn_auth_cbs.passkey_display = zblue_on_auth_passkey_display;
        g_conn_auth_cbs.passkey_confirm = zblue_on_auth_passkey_confirm;
        g_conn_auth_cbs.cancel = zblue_on_auth_cancel;
        break;
    case BT_IO_CAPABILITY_KEYBOARDONLY:
        g_conn_auth_cbs.passkey_entry = zblue_on_auth_passkey_entry;
        g_conn_auth_cbs.cancel = zblue_on_auth_cancel;
        break;
    case BT_IO_CAPABILITY_KEYBOARDDISPLAY:
        g_conn_auth_cbs.passkey_display = zblue_on_auth_passkey_display;
        g_conn_auth_cbs.passkey_entry = zblue_on_auth_passkey_entry;
        g_conn_auth_cbs.passkey_confirm = zblue_on_auth_passkey_confirm;
        g_conn_auth_cbs.cancel = zblue_on_auth_cancel;
        break;
    case BT_IO_CAPABILITY_NOINPUTNOOUTPUT:
        g_conn_auth_cbs.cancel = zblue_on_auth_cancel;
        break;
    default:
        BT_LOGE("Invalid IO capability: %d", cap);
        return BT_STATUS_FAIL;
    }

#ifdef CONFIG_BT_SMP_APP_PAIRING_ACCEPT
    g_conn_auth_cbs.zblue_on_pairing_accept = zblue_on_pairing_accept;
#endif
    g_conn_auth_cbs.pairing_confirm = zblue_on_auth_pairing_confirm;

    if (bt_conn_auth_cb_register(&g_conn_auth_cbs)) {
        BT_LOGE("Failed to register conn auth callbacks");
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void get_bonded_devices(const struct bt_bond_info* info, void* user_data)
{
    device_context_t* ctx = user_data;

    memcpy(&ctx->props->addr, &info->addr.a, sizeof(ctx->props->addr));
    ctx->props->addr_type = info->addr.type;
    ctx->props->device_type = BT_DEVICE_DEVTYPE_BLE;
    (*(ctx->cnt))++;
    ctx->props++;
}

bt_status_t bt_sal_le_get_bonded_devices(bt_controller_id_t id, remote_device_le_properties_t* props, uint16_t* prop_cnt)
{
    device_context_t ctx = { 0 };

    ctx.props = props;
    ctx.cnt = prop_cnt;

    bt_foreach_bond(BT_ID_DEFAULT, get_bonded_devices, &ctx);
    *prop_cnt = *ctx.cnt;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_le_set_static_identity(bt_controller_id_t id, bt_address_t* addr)
{
    /* stack handle this case: */
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_set_public_identity(bt_controller_id_t id, bt_address_t* addr)
{
    /* stack handle this case: */
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_set_address(bt_controller_id_t id, bt_address_t* addr)
{
    /* stack handle this case: */
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_get_address(bt_controller_id_t id)
{
    /* stack handle this case: */
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_set_bonded_devices(bt_controller_id_t id, remote_device_le_properties_t* props, uint16_t prop_cnt)
{
    /* stack handle this case: */
    SAL_NOT_SUPPORT;
}

static void STACK_CALL(conn_connect)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t address = { 0 };
    struct bt_conn* conn;
    int err;

    address.type = req->addr_type;
    memcpy(&address.a, &req->addr, sizeof(address.a));

    err = bt_conn_le_create(&address, &req->adpt.conn_param.create, &req->adpt.conn_param.conn, &conn);
    if (err) {
        BT_LOGE("%s, failed to create connection (%d)", __func__, err);
        return;
    }
}

bt_status_t bt_sal_le_connect(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type, ble_connect_params_t* params)
{
    sal_adapter_req_t* req;
    uint8_t type;

    req = sal_adapter_req(id, addr, STACK_CALL(conn_connect));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    req->adpt.conn_param.conn.interval_min = params->connection_interval_min;
    req->adpt.conn_param.conn.interval_max = params->connection_interval_max;
    req->adpt.conn_param.conn.latency = params->connection_latency;
    req->adpt.conn_param.conn.timeout = params->supervision_timeout;

    req->adpt.conn_param.create.options = BT_CONN_LE_OPT_NONE;
    req->adpt.conn_param.create.interval = params->scan_interval;
    req->adpt.conn_param.create.window = params->scan_window;

    switch (addr_type) {
    case BT_LE_ADDR_TYPE_PUBLIC:
        type = BT_ADDR_LE_PUBLIC;
        break;
    case BT_LE_ADDR_TYPE_RANDOM:
        type = BT_ADDR_LE_RANDOM;
        break;
    case BT_LE_ADDR_TYPE_PUBLIC_ID:
        type = BT_ADDR_LE_PUBLIC_ID;
        break;
    case BT_LE_ADDR_TYPE_RANDOM_ID:
        type = BT_ADDR_LE_RANDOM_ID;
        break;
    case BT_LE_ADDR_TYPE_ANONYMOUS:
        type = BT_ADDR_LE_ANONYMOUS;
        break;
    case BT_LE_ADDR_TYPE_UNKNOWN:
        type = BT_ADDR_LE_RANDOM;
        break;
    default:
        BT_LOGE("%s, invalid type:%d", __func__, addr_type);
        assert(0);
    }

    BT_LOGD("%s, addr_type:%d, type:%d", __func__, addr_type, type);
    req->addr_type = type;

    return sal_send_req(req);
}

static void STACK_CALL(conn_disconnect)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;
    int err;

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        BT_LOGE("%s, disconnect fail err:%d", __func__, err);
        return;
    }
}

bt_status_t bt_sal_le_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(conn_disconnect));
    if (!req) {
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

static void STACK_CALL(create_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;
    int err;

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    err = bt_conn_set_security(conn, BT_SECURITY_L4);
    if (err) {
        BT_LOGE("%s, bond fail err:%d", __func__, err);
        return;
    }
}

bt_status_t bt_sal_le_create_bond(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t type)
{
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(create_bond));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

static void STACK_CALL(remove_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;
    struct bt_conn_info info;
    int err;

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    bt_conn_get_info(conn, &info);
    err = bt_unpair(BT_ID_DEFAULT, info.le.dst);
    if (err < 0) {
        BT_LOGE("%s, unpair fail err:%d", __func__, err);
        return;
    }
}

bt_status_t bt_sal_le_remove_bond(bt_controller_id_t id, bt_address_t* addr)
{
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(remove_bond));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

bt_status_t bt_sal_le_smp_reply(bt_controller_id_t id, bt_address_t* addr, bool accept, bt_pair_type_t type, uint32_t passkey)
{
    struct bt_conn* conn;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    if (!accept) {
        BT_LOGD("%s, reject", __func__);
        SAL_CHECK(bt_conn_auth_cancel(conn), 0);
        return BT_STATUS_SUCCESS;
    }

    switch (type) {
    case PAIR_TYPE_PASSKEY_CONFIRMATION:
    case PAIR_TYPE_CONSENT:
        SAL_CHECK(bt_conn_auth_pairing_confirm(conn), 0);
        break;
    case PAIR_TYPE_PASSKEY_ENTRY:
        SAL_CHECK(bt_conn_auth_passkey_entry(conn, passkey), 0);
        break;
    default:
        BT_LOGE("%s, unsupported type:%d", __func__, type);
        return BT_STATUS_FAIL;
    }

    BT_LOGD("%s, accept", __func__);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_le_set_legacy_tk(bt_controller_id_t id, bt_address_t* addr, bt_128key_t tk_val)
{
    /* todo: */
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_set_remote_oob_data(bt_controller_id_t id, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
    /* todo: */
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_get_local_oob_data(bt_controller_id_t id, bt_address_t* addr)
{
    /* todo: */
    SAL_NOT_SUPPORT;
}

#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
static void STACK_CALL(add_white_list)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t addr;
    int err;

    addr.type = req->addr_type;
    memcpy(&addr.a, &req->addr, sizeof(addr.a));

    err = bt_le_filter_accept_list_add(&addr);
    if (err) {
        BT_LOGE("%s, add white list fail, err:%d", __func__, err);
        adapter_on_whitelist_update(&req->addr, true, BT_STATUS_FAIL);
        return;
    }

    adapter_on_whitelist_update(&req->addr, true, BT_STATUS_SUCCESS);
}
#endif

bt_status_t bt_sal_le_add_white_list(bt_controller_id_t id, bt_address_t* address, ble_addr_type_t addr_type)
{
#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, address, STACK_CALL(add_white_list));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    req->addr_type = addr_type;
    return sal_send_req(req);
#else
    SAL_NOT_SUPPORT;
#endif
}

#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
static void STACK_CALL(remove_white_list)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t addr;
    int err;

    addr.type = req->addr_type;
    memcpy(&addr.a, &req->addr, sizeof(addr.a));

    err = bt_le_filter_accept_list_add(&addr);
    if (err) {
        BT_LOGE("%s, remove white list fail, err:%d", __func__, err);
        adapter_on_whitelist_update(&req->addr, false, BT_STATUS_FAIL);
        return;
    }

    adapter_on_whitelist_update(&req->addr, false, BT_STATUS_SUCCESS);
}
#endif

bt_status_t bt_sal_le_remove_white_list(bt_controller_id_t id, bt_address_t* address, ble_addr_type_t addr_type)
{
#if defined(CONFIG_BT_FILTER_ACCEPT_LIST)
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, address, STACK_CALL(remove_white_list));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    req->addr_type = addr_type;
    return sal_send_req(req);
#else
    SAL_NOT_SUPPORT;
#endif
}

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static void STACK_CALL(set_phy)(void* args)
{
    sal_adapter_req_t* req = args;
    int err;
    struct bt_conn* conn;

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    err = bt_conn_le_phy_update(conn, &req->adpt.phy_param);
    if (err) {
        BT_LOGE("%s, phy update fail, err:%d", __func__, err);
        return;
    }
}
#endif

bt_status_t bt_sal_le_set_phy(bt_controller_id_t id, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
#if defined(CONFIG_BT_USER_PHY_UPDATE)
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(set_phy));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    req->adpt.phy_param.pref_tx_phy = le_phy_convert_from_service(tx_phy);
    req->adpt.phy_param.pref_rx_phy = le_phy_convert_from_service(rx_phy);
    req->adpt.phy_param.options = BT_CONN_LE_PHY_OPT_NONE;

    return sal_send_req(req);
#else
    SAL_NOT_SUPPORT;
#endif /*  CONFIG_BT_USER_PHY_UPDATE */
}

bt_status_t bt_sal_le_set_appearance(bt_controller_id_t id, uint16_t appearance)
{
#ifdef CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE
    SAL_CHECK_RET(bt_set_appearance(appearance), 0);
    return BT_STATUS_SUCCESS;
#else
    SAL_NOT_SUPPORT;
#endif /* CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE */
}

uint16_t bt_sal_le_get_appearance(bt_controller_id_t id)
{
#ifdef CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE
    return bt_get_appearance();
#else
    SAL_NOT_SUPPORT;
#endif /* CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE */
}

bt_status_t bt_sal_le_enable_key_derivation(bt_controller_id_t id, bool brkey_to_lekey, bool lekey_to_brkey)
{
    /* todo: */
    SAL_NOT_SUPPORT;
}

#endif /* CONFIG_BLUETOOTH_BLE_SUPPORT */