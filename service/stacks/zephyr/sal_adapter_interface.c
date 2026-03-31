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
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#define LOG_TAG "sal_adapter"
#include <stdint.h>

#include "bluetooth.h"
#include "bt_adapter.h"

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_status.h"

#include "adapter_internel.h"
#include "bluetooth_define.h"
#include "hci_h4.h"
#include "power_manager.h"
#include "service_loop.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>

#include <zephyr/settings/settings.h>

#include "sal_adapter_le_interface.h"
#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "sal_zephyr_interface.h"

#include <settings_zblue.h>
#include <zephyr/settings/settings.h>

#include "keys.h"

#include "utils/log.h"

#define STACK_CALL(func) zblue_##func

static void STACK_CALL(pending_connect_complete)(void* args);

typedef void (*sal_func_t)(void* args);

typedef union {
    char name[BT_LOC_NAME_MAX_LEN];
    bt_io_capability_t cap;
    uint32_t cod;
    struct {
        bt_scan_mode_t scan_mode;
        bool bondable;
    } scanmode;
    struct {
        uint32_t timeout;
        bool limited;
    } discovery;
    struct {
        bool inquiry;
        bt_scan_type_t type;
        uint16_t interval;
        uint16_t window;
    } sp;
    bool accept;
    uint8_t reason;
    struct {
        bool accept;
        bt_pair_type_t type;
        uint32_t passkey;
    } ssp;
    struct {
        bool accept;
        char* pincode;
        int len;
    } pin;
    struct {
        bt_transport_t transport;
        bt_addr_type_t type;
    } bond;
    bt_transport_t transport;
    bt_pm_mode_t mode;
    bt_link_role_t role;
    bt_link_policy_t policy;
    struct {
        uint16_t central_frequency;
        uint16_t band_width;
        uint16_t number;
    } afh;
    int security_level;
    uint8_t map[10];
} sal_adapter_args_t;

typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    sal_func_t func;
    sal_adapter_args_t adpt;
} sal_adapter_req_t;

struct device_context {
    remote_device_properties_t* props;
    int got;
    int cnt;
};

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
extern int zblue_main(void);
static void sal_pending_connect_init(void);
static void sal_pending_connect_cleanup(void);
#ifndef CONFIG_BT_CONN_REQ_AUTO_HANDLE
static void zblue_on_connect_req(struct bt_conn* conn, uint8_t link_type, uint8_t* cod);
#endif
static void zblue_on_connected(struct bt_conn* conn, uint8_t err);
static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason);
static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err);
#ifdef CONFIG_BT_REMOTE_INFO
static void zblue_on_remote_info_available(struct bt_conn* conn,
    struct bt_conn_remote_info* remote_info);
#endif
#ifdef CONFIG_BT_POWER_MODE_CONTROL
static void zblue_on_mode_changed(struct bt_conn* conn, uint8_t mode, uint16_t interval);
#endif
static void zblue_on_role_changed(struct bt_conn* conn, uint8_t role);
static void zblue_on_passkey_display(struct bt_conn* conn, unsigned int passkey);
static void zblue_on_passkey_entry(struct bt_conn* conn);
static void zblue_on_passkey_confirm(struct bt_conn* conn, unsigned int passkey);
static void zblue_on_cancel(struct bt_conn* conn);
static void zblue_on_pairing_confirm(struct bt_conn* conn);
static void zblue_on_pincode_entry(struct bt_conn* conn, bool highsec);
static void zblue_on_br_pairing_complete_ctkd(struct bt_conn* conn, bool is_link_key);
static void zblue_on_br_pairing_complete(struct bt_conn* conn, bool bonding_flag);
static void zblue_on_br_pairing_failed(struct bt_conn* conn, enum bt_security_err reason);
static void zblue_on_br_bond_deleted(uint8_t id, const bt_addr_le_t* peer);
bt_status_t bt_sal_disconnect_internal(bt_controller_id_t id, bt_address_t* addr, uint8_t reason);
bt_status_t bt_sal_remove_bond_internal(bt_controller_id_t id, bt_address_t* addr);
static void zblue_register_callback(void);
static void zblue_unregister_callback(void);
#if defined(CONFIG_SETTINGS_ZBLUE)
static int zblue_on_link_key_notify(uint8_t dev_id, bt_addr_le_t* addr, const char* key_value, uint8_t value_len);
static int zblue_on_link_key_load(bt_addr_le_t* addr, uint8_t* key_value, uint8_t value_len);
#endif

static bt_security_t g_security_level = BT_SECURITY_L2;

static struct bt_conn_cb g_conn_cbs = {
#ifndef CONFIG_BT_CONN_REQ_AUTO_HANDLE
    .connect_req = zblue_on_connect_req,
#endif /* CONFIG_BT_CONN_REQ_AUTO_HANDLE */
    .connected = zblue_on_connected,
    .disconnected = zblue_on_disconnected,
    .security_changed = zblue_on_security_changed,
#ifdef CONFIG_BT_REMOTE_INFO
    .remote_info_available = zblue_on_remote_info_available,
#endif
#ifdef CONFIG_BT_POWER_MODE_CONTROL
    .mode_changed = zblue_on_mode_changed,
#endif
    .role_changed = zblue_on_role_changed,
};

#if defined(CONFIG_SETTINGS_ZBLUE)
static struct bt_settings_zblue_cb g_setting_cbs = {
    .linkkey_notify = zblue_on_link_key_notify,
    .linkkey_load = zblue_on_link_key_load,
};
#endif

static struct bt_conn_auth_info_cb g_conn_auth_info_cbs = {
    .pairing_complete = zblue_on_br_pairing_complete,
    .pairing_complete_ctkd = zblue_on_br_pairing_complete_ctkd,
    .pairing_failed = zblue_on_br_pairing_failed,
    .bond_deleted = zblue_on_br_bond_deleted,
};

static struct bt_conn_auth_cb g_conn_auth_cbs = {
    .cancel = zblue_on_cancel,
    .pincode_entry = zblue_on_pincode_entry
};

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
    if (!req)
        return BT_STATUS_PARM_INVALID;

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void zblue_conn_get_addr(struct bt_conn* conn, bt_address_t* addr)
{
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);
    bt_addr_set(addr, info.br.dst->val);
}

#ifndef CONFIG_BT_CONN_REQ_AUTO_HANDLE
static void zblue_on_connect_req(struct bt_conn* conn, uint8_t link_type, uint8_t* cod)
{
    if (link_type == BT_HCI_ACL) {
        acl_state_param_t state = {
            .transport = BT_TRANSPORT_BREDR,
            .connection_state = CONNECTION_STATE_CONNECTING
        };
        uint32_t class = ((uint32_t)cod[2] << 16) | ((uint32_t)cod[1] << 8) | (uint32_t)cod[0];

        zblue_conn_get_addr(conn, &state.addr);
        adapter_on_connect_request(&state.addr, class);
        adapter_on_connection_state_changed(&state);
    } else {
        // Ignore
    }
}
#endif

static void zblue_on_connected(struct bt_conn* conn, uint8_t err)
{
    if (!bt_conn_get_dst_br(conn)) {
        return;
    }

    bt_conn_info_t* slot;
    acl_state_param_t state = {
        .transport = BT_TRANSPORT_BREDR,
        .connection_state = CONNECTION_STATE_CONNECTED
    };

    zblue_conn_get_addr(conn, &state.addr);
    if (err) {
        state.connection_state = CONNECTION_STATE_DISCONNECTED;
        state.status = err;
        slot = bt_conn_find(&state.addr, BT_TRANSPORT_BREDR);
        if (slot) {
            bt_conn_remove(&state.addr, BT_TRANSPORT_BREDR);
        }
        bt_sal_cm_acl_disconnected_callback(cm_data_new(&state.addr, PROFILE_UNKOWN, CONN_ID_DEFAULT));
        goto error;
    }

    bt_sal_get_remote_name(BT_TRANSPORT_BREDR, &state.addr);
    bt_sal_cm_acl_connected_callback(cm_data_new(&state.addr, PROFILE_UNKOWN, CONN_ID_DEFAULT));

error:
    adapter_on_connection_state_changed(&state);

    /* Dispatch to worker thread to safely operate on pending connect list */
    sal_send_req(sal_adapter_req(PRIMARY_ADAPTER, &state.addr,
        STACK_CALL(pending_connect_complete)));
}

static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason)
{
    if (!bt_conn_get_dst_br(conn)) {
        return;
    }

    bt_conn_info_t* slot;
    acl_state_param_t state = {
        .transport = BT_TRANSPORT_BREDR,
        .connection_state = CONNECTION_STATE_DISCONNECTED,
        .hci_reason_code = reason,
    };

    zblue_conn_get_addr(conn, &state.addr);
    adapter_on_connection_state_changed(&state);
    slot = bt_conn_find(&state.addr, BT_TRANSPORT_BREDR);
    if (slot) {
        bt_conn_remove(&state.addr, BT_TRANSPORT_BREDR);
    }

    bt_sal_cm_acl_disconnected_callback(cm_data_new(&state.addr, PROFILE_UNKOWN, CONN_ID_DEFAULT));
}

static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err)
{
    bt_address_t addr;
    struct bt_conn_info info;
    bt_status_t ret;
    bool encrypted = false;

    if (bt_conn_get_info(conn, &info) < 0) {
        return;
    }

    if (info.type != BT_CONN_TYPE_BR) {
        return;
    }

    bt_addr_set(&addr, info.br.dst->val);

    BT_LOGD("%s, state: %d, level: %d, required level: %d, err: %d",
        __func__, info.state, level, g_security_level, err);

    if (level >= g_security_level && err == BT_SECURITY_ERR_SUCCESS) {
        encrypted = true;
        adapter_on_encryption_state_changed(&addr, encrypted, BT_TRANSPORT_BREDR);
        return;
    }

    if ((level < g_security_level) && (err == BT_SECURITY_ERR_AUTH_FAIL || err == BT_SECURITY_ERR_PIN_OR_KEY_MISSING)) {
        adapter_on_bond_state_changed(&addr, BOND_STATE_NONE, BT_TRANSPORT_BREDR, BT_STATUS_FAIL, false);
        BT_LOGD("%s, err: %d, remove old key async", __func__, err);
        ret = bt_sal_remove_bond_internal(PRIMARY_ADAPTER, &addr);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGE("%s, Failed to remove old BR key async: %d", __func__, ret);
        }
    } else if (err != BT_SECURITY_ERR_SUCCESS) {
        BT_LOGW("%s, preserve bond on BR security failure, state: %d, level: %d, required: %d, err: %d",
            __func__, info.state, level, g_security_level, err);
    }

    if (err == BT_SECURITY_ERR_AUTH_FAIL || err == BT_SECURITY_ERR_PIN_OR_KEY_MISSING || (err == BT_SECURITY_ERR_SUCCESS && level < g_security_level)) {
        ret = bt_sal_disconnect_internal(PRIMARY_ADAPTER, &addr, BT_HCI_ERR_AUTH_FAIL);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGE("%s, disconnect async failed: %d", __func__, ret);
        }
        return;
    }

    adapter_on_encryption_state_changed(&addr, encrypted, BT_TRANSPORT_BREDR);
}

#ifdef CONFIG_BT_REMOTE_INFO
static void zblue_on_remote_info_available(struct bt_conn* conn,
    struct bt_conn_remote_info* remote_info)
{
}
#endif

#ifdef CONFIG_BT_POWER_MODE_CONTROL
static void zblue_on_mode_changed(struct bt_conn* conn, uint8_t mode, uint16_t interval)
{
    bt_link_mode_t linkmode;
    bt_address_t addr;

    if (mode == BT_ACTIVE_MODE) {
        linkmode = BT_LINK_MODE_ACTIVE;
    } else {
        linkmode = BT_LINK_MODE_SNIFF;
    }

    zblue_conn_get_addr(conn, &addr);
    adapter_on_link_mode_changed(&addr, linkmode, interval);
}
#endif

static void zblue_on_role_changed(struct bt_conn* conn, uint8_t role)
{
    bt_link_role_t linkrole;
    bt_address_t addr;

    if (role == BT_CONN_ROLE_PERIPHERAL) {
        linkrole = BT_LINK_ROLE_SLAVE;
    } else {
        linkrole = BT_LINK_ROLE_MASTER;
    }

    zblue_conn_get_addr(conn, &addr);
    adapter_on_link_role_changed(&addr, linkrole);
}

static void zblue_on_passkey_display(struct bt_conn* conn, unsigned int passkey)
{
    bt_address_t addr;

    zblue_conn_get_addr(conn, &addr);
    adapter_on_ssp_request(&addr, BT_TRANSPORT_BREDR, 0, PAIR_TYPE_PASSKEY_NOTIFICATION, passkey, NULL);
}

static void zblue_on_passkey_entry(struct bt_conn* conn)
{
    bt_address_t addr;

    zblue_conn_get_addr(conn, &addr);
    adapter_on_ssp_request(&addr, BT_TRANSPORT_BREDR, 0, PAIR_TYPE_PASSKEY_ENTRY, 0, NULL);
}

static void zblue_on_passkey_confirm(struct bt_conn* conn, unsigned int passkey)
{
    bt_address_t addr;

    zblue_conn_get_addr(conn, &addr);
    adapter_on_ssp_request(&addr, BT_TRANSPORT_BREDR, 0, PAIR_TYPE_PASSKEY_CONFIRMATION, passkey, NULL);
}

static void zblue_on_cancel(struct bt_conn* conn)
{
}

static void zblue_on_pairing_confirm(struct bt_conn* conn)
{
    bt_address_t addr;

    zblue_conn_get_addr(conn, &addr);
    /* it's justworks */
    adapter_on_ssp_request(&addr, BT_TRANSPORT_BREDR, 0, PAIR_TYPE_CONSENT, 0, NULL);
}

static void zblue_on_pincode_entry(struct bt_conn* conn, bool highsec)
{
    bt_address_t addr;

    zblue_conn_get_addr(conn, &addr);
    adapter_on_pin_request(&addr, 0, true, NULL);
}

static void zblue_on_br_pairing_complete_ctkd(struct bt_conn* conn, bool is_link_key)
{
    bt_address_t addr;
    const bt_addr_le_t* dst;

    if (!is_link_key) {
        return;
    }

    dst = bt_conn_get_dst(conn);
    if (!dst) {
        return;
    }

    memcpy(addr.addr, dst->a.val, sizeof(addr.addr));

    adapter_on_bond_state_changed(&addr, BOND_STATE_BONDED, BT_TRANSPORT_BREDR, BT_STATUS_SUCCESS, true);
}

#ifdef CONFIG_SETTINGS_ZBLUE
static int zblue_on_link_key_notify(uint8_t dev_id, bt_addr_le_t* addr, const char* key_value, uint8_t value_len)
{
    bt_address_t br_addr;
    bt_128key_t key;
    bt_link_key_type_t key_type = 0;
    struct bt_keys_link_key* link_key;

    memcpy(br_addr.addr, addr->a.val, sizeof(br_addr.addr));
    if (!key_value) {
        BT_LOGD("%s delete key_value", __func__);
        return 0;
    }

    link_key = (struct bt_keys_link_key*)zalloc(sizeof(struct bt_keys_link_key));
    if (!link_key) {
        BT_LOGE("%s link_key malloc fail", __func__);
        return -ENOSPC;
    }

    memcpy(link_key->storage_start, key_value, value_len);
    memcpy(key, link_key->val, 16);
    key_type = link_key->key_type;
    free(link_key);

    adapter_on_link_key_update(&br_addr, key, key_type, false);
    return 0;
}

static int zblue_on_link_key_load(bt_addr_le_t* addr, uint8_t* key_value, uint8_t value_len)
{
    struct bt_keys_link_key* link_key;
    bt_address_t br_addr;
    uint8_t* key;

    memcpy(br_addr.addr, addr->a.val, sizeof(br_addr.addr));

    link_key = (struct bt_keys_link_key*)zalloc(sizeof(struct bt_keys_link_key));
    if (!link_key) {
        BT_LOGE("%s link_key malloc fail", __func__);
        return -ENOSPC;
    }

    key = adapter_get_link_key(&br_addr);
    if (!key) {
        BT_LOGE("%s link_key load fail", __func__);
        free(link_key);
        return -EINVAL;
    }

    memcpy(link_key->val, key, 16);

    link_key->key_type = adapter_get_link_key_type(&br_addr);
    switch (link_key->key_type) {
    case BT_LK_COMBINATION:
    case BT_LK_AUTH_COMBINATION_P192:
        link_key->flags |= BT_LINK_KEY_AUTHENTICATED;
        break;
    case BT_LK_AUTH_COMBINATION_P256:
        link_key->flags |= BT_LINK_KEY_AUTHENTICATED | BT_LINK_KEY_SC;
        break;
    default:
        break;
    }

    memcpy(key_value, link_key->storage_start, value_len);
    free(link_key);
    return value_len;
}
#endif

static void zblue_on_br_pairing_complete(struct bt_conn* conn, bool bonding_flag)
{
    bt_address_t addr;

    if (!bt_conn_get_dst_br(conn)) {
        return;
    }

    BT_LOGD("%s bonding_flag: %s", __func__, bonding_flag ? "true" : "false");

    zblue_conn_get_addr(conn, &addr);
    adapter_on_bond_state_changed(&addr, BOND_STATE_BONDED, BT_TRANSPORT_BREDR, BT_STATUS_SUCCESS, false);
}

static void zblue_on_br_pairing_failed(struct bt_conn* conn, enum bt_security_err reason)
{
    bt_address_t addr;

    if (!bt_conn_get_dst_br(conn)) {
        return;
    }

    zblue_conn_get_addr(conn, &addr);
    adapter_on_bond_state_changed(&addr, BOND_STATE_NONE, BT_TRANSPORT_BREDR, BT_STATUS_AUTH_FAILURE, false);
    bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static void zblue_on_br_bond_deleted(uint8_t id, const bt_addr_le_t* peer)
{
    bt_address_t addr;

    if (id == 0 && peer->type == BT_ADDR_LE_PUBLIC) {
        bt_addr_set(&addr, peer->a.val);
        adapter_on_link_key_removed(&addr, BT_STATUS_SUCCESS);
    } /* else: Ignore it*/
}

static void zblue_on_ready_cb(bt_controller_id_t dev_id, int err)
{
    uint8_t state = BT_BREDR_STACK_STATE_OFF;

    UNUSED(dev_id);
#if !defined(CONFIG_BLUETOOTH_BLE_SUPPORT)
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }
#endif

    if (err) {
        BT_LOGD("zblue init failed (err %d)\n", err);
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);
        return;
    }

    zblue_register_callback();

#if defined(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE) && !defined(CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
    state = BT_BREDR_STACK_STATE_ON;
#else
    switch (adapter_get_state()) {
    case BT_ADAPTER_STATE_BLE_TURNING_ON:
        state = BLE_STACK_STATE_ON;
        break;
    case BT_ADAPTER_STATE_TURNING_ON:
        state = BT_BREDR_STACK_STATE_ON;
        break;
    default:
        break;
    }
#endif
    adapter_on_adapter_state_changed(state);
}
#endif

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static bool zblue_inquiry_eir_name(const uint8_t* eir, int len, char* name)
{
    while (len) {
        if (len < 2) {
            return false;
        }

        /* Look for early termination */
        if (!eir[0]) {
            return false;
        }

        /* Check if field length is correct */
        if (eir[0] > len - 1) {
            return false;
        }

        switch (eir[1]) {
        case BT_DATA_NAME_SHORTENED:
        case BT_DATA_NAME_COMPLETE:
            if (eir[0] <= 1) {
                return false;
            }

            memset(name, 0, BT_REM_NAME_MAX_LEN);
            if (eir[0] > BT_REM_NAME_MAX_LEN - 1) {
                memcpy(name, &eir[2], BT_REM_NAME_MAX_LEN - 1);
            } else {
                memcpy(name, &eir[2], eir[0] - 1);
            }
            return true;
        default:
            break;
        }

        /* Parse next AD Structure */
        len -= eir[0] + 1;
        eir += eir[0] + 1;
    }

    return false;
}

static void zblue_on_discovery_recv_cb(const struct bt_br_discovery_result* results)
{
    bt_discovery_result_t device = { 0 };

    memcpy(device.addr.addr, &results->addr, 6);
    device.rssi = results->rssi;
    device.cod = (results->cod[2] << 16) | (results->cod[1] << 8) | results->cod[0];
    zblue_inquiry_eir_name(results->eir, sizeof(results->eir), device.name);

    /* report discovery result to service */
    adapter_on_device_found(&device);
}

static void zblue_on_discovery_complete_cb(const struct bt_br_discovery_result* results,
    size_t count)
{
    adapter_on_discovery_state_changed(BT_DISCOVERY_STOPPED);
}

static struct bt_br_discovery_cb g_br_discovery_cb = {
    .recv = zblue_on_discovery_recv_cb,
    .timeout = zblue_on_discovery_complete_cb
};

static void zblue_register_callback(void)
{
    bt_br_discovery_cb_register(&g_br_discovery_cb);
    bt_conn_cb_register(&g_conn_cbs);
    bt_conn_auth_cb_register(&g_conn_auth_cbs);
    bt_conn_auth_info_cb_register(&g_conn_auth_info_cbs);
#ifdef CONFIG_SETTINGS_ZBLUE
    bt_setting_cb_register(&g_setting_cbs);
#endif
}

static void zblue_unregister_callback(void)
{
    bt_br_discovery_cb_unregister(&g_br_discovery_cb);
    bt_conn_auth_cb_register(NULL);
    bt_conn_auth_info_cb_unregister(&g_conn_auth_info_cbs);
}
#endif

/* service adapter layer for BREDR */
bt_status_t bt_sal_init(const bt_vhal_interface* vhal)
{
    bt_sal_hci_transport_init(vhal);

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    extern void z_sys_init(void);
    z_sys_init();
    bt_sal_cm_conn_init();
    sal_pending_connect_init();

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    sal_pending_connect_cleanup();
    bt_sal_cm_conn_cleanup();
#endif

    bt_sal_hci_transport_cleanup();
}

/* Adapter power */
bt_status_t bt_sal_enable(bt_controller_id_t id)
{
    UNUSED(id);

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (bt_is_ready()) {
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_ON);
        zblue_register_callback();
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(bt_enable(zblue_on_ready_cb), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#if defined(CONFIG_BLUETOOTH_BREDR_SUPPORT)
static void STACK_CALL(brder_disable)(void* args)
{
    UNUSED(args);

    zblue_unregister_callback();
    bt_br_set_visibility(false, false);
#ifndef CONFIG_BLUETOOTH_BLE_SUPPORT
    bt_br_set_visibility(false, false);
    bt_disable();
#endif
}
#endif

bt_status_t bt_sal_disable(bt_controller_id_t id)
{
    UNUSED(id);

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    sal_adapter_req_t* req;

    if (!bt_is_ready()) {
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);
        return BT_STATUS_SUCCESS;
    }

    req = sal_adapter_req(id, NULL, STACK_CALL(brder_disable));
    if (!req) {
        return BT_STATUS_NOMEM;
    }
    sal_send_req(req);

    adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bool bt_sal_is_enabled(bt_controller_id_t id)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);

    return bt_is_ready();
#else
    return false;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_name)(void* args)
{
    sal_adapter_req_t* req = args;

    BT_LOGD("%s: %s", __func__, req->adpt.name);
    SAL_CHECK(bt_set_name(req->adpt.name), 0);
}
#endif

bt_status_t bt_sal_set_name(bt_controller_id_t id, char* name)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_name));
    if (!req)
        return BT_STATUS_NOMEM;

    strlcpy(req->adpt.name, name, BT_LOC_NAME_MAX_LEN);

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

const char* bt_sal_get_name(bt_controller_id_t id)
{
    UNUSED(id);

    return bt_get_name();
}

bt_status_t bt_sal_get_address(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    bt_addr_le_t got = { 0 };
    size_t count = 1;

    SAL_CHECK_PARAM(addr);

    bt_id_get(&got, &count);
    bt_addr_set(addr, (uint8_t*)&got.a);

    SAL_ASSERT(got.type == BT_ADDR_LE_PUBLIC);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_io_capability(bt_controller_id_t id, bt_io_capability_t cap)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);

    switch (cap) {
    case BT_IO_CAPABILITY_DISPLAYONLY:
        g_conn_auth_cbs.passkey_display = zblue_on_passkey_display;
        g_conn_auth_cbs.passkey_entry = NULL;
        g_conn_auth_cbs.passkey_confirm = NULL;
        g_conn_auth_cbs.pairing_confirm = NULL;
        break;
    case BT_IO_CAPABILITY_DISPLAYYESNO:
        g_conn_auth_cbs.passkey_display = zblue_on_passkey_display;
        g_conn_auth_cbs.passkey_entry = NULL;
        g_conn_auth_cbs.passkey_confirm = zblue_on_passkey_confirm;
        g_conn_auth_cbs.pairing_confirm = zblue_on_pairing_confirm;
        break;
    case BT_IO_CAPABILITY_KEYBOARDONLY:
        g_conn_auth_cbs.passkey_display = NULL;
        g_conn_auth_cbs.passkey_entry = zblue_on_passkey_entry;
        g_conn_auth_cbs.passkey_confirm = NULL;
        g_conn_auth_cbs.pairing_confirm = NULL;
        break;
    case BT_IO_CAPABILITY_KEYBOARDDISPLAY:
        g_conn_auth_cbs.passkey_display = zblue_on_passkey_display;
        g_conn_auth_cbs.passkey_entry = zblue_on_passkey_entry;
        g_conn_auth_cbs.passkey_confirm = zblue_on_passkey_confirm;
        g_conn_auth_cbs.pairing_confirm = zblue_on_pairing_confirm;
        break;
    case BT_IO_CAPABILITY_NOINPUTNOOUTPUT:
    default:
        g_conn_auth_cbs.passkey_display = NULL;
        g_conn_auth_cbs.passkey_entry = NULL;
#ifdef CONFIG_HCI_AUTO_REPLY_IN_JUST_WORK
        g_conn_auth_cbs.passkey_confirm = NULL;
#else
        g_conn_auth_cbs.passkey_confirm = zblue_on_passkey_confirm;
#endif
        g_conn_auth_cbs.pairing_confirm = NULL;
        break;
    }

    bt_conn_auth_cb_register(NULL);
    bt_conn_auth_cb_register(&g_conn_auth_cbs);

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    /* Keep consistency with legacy stack: sync LE IO capability with BR/EDR IO capability */
    bt_sal_le_set_io_capability(id, cap);
#endif

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_io_capability_t bt_sal_get_io_capability(bt_controller_id_t id)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    return BT_IO_CAPABILITY_UNKNOW;
#else
    return BT_IO_CAPABILITY_UNKNOW;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_device_class)(void* args)
{
    sal_adapter_req_t* req = args;

    SAL_CHECK(bt_br_set_class_of_device(req->adpt.cod), 0);
}
#endif

bt_status_t bt_sal_set_device_class(bt_controller_id_t id, uint32_t cod)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_device_class));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.cod = cod;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t bt_sal_get_device_class(bt_controller_id_t id)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_scan_mode)(void* args)
{
    sal_adapter_req_t* req = args;
    bool iscan = false;
    bool pscan = false;
    int ret;

    switch (req->adpt.scanmode.scan_mode) {
    case BT_SCAN_MODE_NONE:
        break;
    case BT_SCAN_MODE_CONNECTABLE: {
        pscan = true;
        break;
    }
    case BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE: {
        iscan = true;
        pscan = true;
        break;
    }
    default:
        break;
    }

    ret = bt_br_set_visibility(iscan, pscan);
    if (ret != 0 && ret != -EALREADY) {
        BT_LOGE("%s set scanmode failed:%d", __func__, ret);
        return;
    }

    if (ret == 0)
        adapter_on_scan_mode_changed(req->adpt.scanmode.scan_mode);
}
#endif

bt_status_t bt_sal_set_scan_mode(bt_controller_id_t id, bt_scan_mode_t scan_mode, bool bondable)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_scan_mode));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.scanmode.scan_mode = scan_mode;
    req->adpt.scanmode.bondable = bondable;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_scan_mode_t bt_sal_get_scan_mode(bt_controller_id_t id)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}

bool bt_sal_get_bondable(bt_controller_id_t id)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
/* Inquiry/page and inquiry/page scan */
static void STACK_CALL(start_discovery)(void* args)
{
#define DISCOVERY_DEVICE_MAX 30
    sal_adapter_req_t* req = args;
    struct bt_br_discovery_param param;
    static struct bt_br_discovery_result g_discovery_results[DISCOVERY_DEVICE_MAX];

    /* unlimited number of responses. */
    param.limited = req->adpt.discovery.limited;
    param.length = req->adpt.discovery.timeout;

    if (bt_br_discovery_start(&param, g_discovery_results,
            SAL_ARRAY_SIZE(g_discovery_results))
        == 0)
        adapter_on_discovery_state_changed(BT_DISCOVERY_STARTED);
}
#endif

bt_status_t bt_sal_start_discovery(bt_controller_id_t id, uint32_t timeout, bool is_limited)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    /* Range(timeout * 1.28s) --> 1.28 to 61.44 s */
    if (!timeout || timeout > 0x30)
        return BT_STATUS_PARM_INVALID;

    req = sal_adapter_req(id, NULL, STACK_CALL(start_discovery));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.discovery.timeout = timeout;
    req->adpt.discovery.limited = is_limited;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(stop_discovery)(void* args)
{
    SAL_CHECK(bt_br_discovery_stop(), 0);
    adapter_on_discovery_state_changed(BT_DISCOVERY_STOPPED);
}
#endif

bt_status_t bt_sal_stop_discovery(bt_controller_id_t id)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);

    return sal_send_req(sal_adapter_req(id, NULL, STACK_CALL(stop_discovery)));
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_scan_parameters)(void* args)
{
    sal_adapter_req_t* req = args;

    if (req->adpt.sp.type == BT_BR_SCAN_TYPE_STANDARD || req->adpt.sp.type == BT_BR_SCAN_TYPE_INTERLACED) {
        if (req->adpt.sp.inquiry) {
            SAL_CHECK(bt_br_write_inquiry_scan_type(req->adpt.sp.type), 0);
        } else {
            SAL_CHECK(bt_br_write_page_scan_type(req->adpt.sp.type), 0);
        }
    }

    if (req->adpt.sp.window <= 0x1000 && req->adpt.sp.interval >= 0x11 && (req->adpt.sp.interval > req->adpt.sp.window)) {
        if (req->adpt.sp.inquiry) {
            SAL_CHECK(bt_br_write_inquiry_scan_activity(req->adpt.sp.interval, req->adpt.sp.window), 0);
        } else {
            SAL_CHECK(bt_br_write_page_scan_activity(req->adpt.sp.interval, req->adpt.sp.window), 0);
        }
    }
}
#endif

bt_status_t bt_sal_set_page_scan_parameters(bt_controller_id_t id, bt_scan_type_t type,
    uint16_t interval, uint16_t window)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_scan_parameters));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.sp.inquiry = false;
    req->adpt.sp.type = type;
    req->adpt.sp.interval = interval;
    req->adpt.sp.window = window;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_inquiry_scan_parameters(bt_controller_id_t id, bt_scan_type_t type,
    uint16_t interval, uint16_t window)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_scan_parameters));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.sp.inquiry = true;
    req->adpt.sp.type = type;
    req->adpt.sp.interval = interval;
    req->adpt.sp.window = window;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
/* Remote device RNR/connection/bond/properties */
static void zblue_on_remote_name_req_cb(const bt_addr_t* bdaddr, const char* name, uint8_t status)
{
    if (status == BT_HCI_ERR_SUCCESS) {
        adapter_on_remote_name_recieved((bt_address_t*)bdaddr, name);
    } else {
        BT_LOGE("%s error: %02" PRIu8, __func__, status);
    }
}

static void STACK_CALL(get_remote_name)(void* args)
{
    sal_adapter_req_t* req = args;

    SAL_CHECK(bt_br_remote_name_request((bt_addr_t*)&req->addr, zblue_on_remote_name_req_cb), 0);
}
#endif

bt_status_t bt_sal_get_remote_name(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);

    return sal_send_req(sal_adapter_req(id, addr, STACK_CALL(get_remote_name)));
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_auto_accept_connection(bt_controller_id_t id, bool enable)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_sco_connection_reply(bt_controller_id_t id, bt_address_t* addr, bool accept)
{
    SAL_NOT_SUPPORT;
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(acl_connection_reply)(void* args)
{
#ifndef CONFIG_BT_CONN_REQ_AUTO_HANDLE
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    if (req->adpt.accept) {
        SAL_CHECK(bt_conn_accept_acl_conn(conn), 0);
    } else {
        SAL_CHECK(bt_conn_reject_acl_conn(conn, BT_HCI_ERR_INSUFFICIENT_RESOURCES), 0);
    }

    bt_conn_unref(conn);
#endif
}
#endif

bt_status_t bt_sal_acl_connection_reply(bt_controller_id_t id, bt_address_t* addr, bool accept)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(acl_connection_reply));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.accept = accept;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_pair_reply(bt_controller_id_t id, bt_address_t* addr, uint8_t reason)
{
    SAL_NOT_SUPPORT;
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(ssp_reply)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    if (req->adpt.ssp.accept) {
        switch (req->adpt.ssp.type) {
        case PAIR_TYPE_PASSKEY_CONFIRMATION:
        case PAIR_TYPE_CONSENT:
            SAL_CHECK(bt_conn_auth_passkey_confirm(conn), 0);
            break;
        case PAIR_TYPE_PASSKEY_ENTRY:
            SAL_CHECK(bt_conn_auth_passkey_entry(conn, req->adpt.ssp.passkey), 0);
            break;
        default:
            break;
        }
    } else {
        SAL_CHECK(bt_conn_auth_cancel(conn), 0);
    }

    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_ssp_reply(bt_controller_id_t id, bt_address_t* addr, bool accept,
    bt_pair_type_t type, uint32_t passkey)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(ssp_reply));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.ssp.accept = accept;
    req->adpt.ssp.type = type;
    req->adpt.ssp.passkey = passkey;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(pin_reply)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    if (req->adpt.pin.accept) {
        SAL_CHECK(bt_conn_auth_pincode_entry(conn, req->adpt.pin.pincode), 0);
    } else {
        SAL_CHECK(bt_conn_auth_cancel(conn), 0);
    }

    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_pin_reply(bt_controller_id_t id, bt_address_t* addr,
    bool accept, char* pincode, int len)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(pin_reply));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.pin.accept = accept;
    req->adpt.pin.pincode = malloc(len + 1);
    memcpy(req->adpt.pin.pincode, pincode, len);
    req->adpt.pin.pincode[len] = '\0';
    req->adpt.pin.len = len;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

connection_state_t bt_sal_get_connection_state(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    struct bt_conn_info info;
    connection_state_t state = CONNECTION_STATE_DISCONNECTED;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return CONNECTION_STATE_DISCONNECTED;
    }

    bt_conn_get_info(conn, &info);
    switch (info.state) {
    case BT_CONN_STATE_DISCONNECTED: {
        state = CONNECTION_STATE_DISCONNECTED;
        break;
    }
    case BT_CONN_STATE_CONNECTING: {
        state = CONNECTION_STATE_CONNECTING;
        break;
    }
    case BT_CONN_STATE_CONNECTED: {
        state = CONNECTION_STATE_CONNECTED;
        break;
    }
    case BT_CONN_STATE_DISCONNECTING: {
        state = CONNECTION_STATE_DISCONNECTING;
        break;
    }
    default:
        break;
    }

    bt_conn_unref(conn);
    return state;
#else
    return CONNECTION_STATE_DISCONNECTED;
#endif
}

uint16_t bt_sal_get_acl_connection_handle(bt_controller_id_t id, bt_address_t* addr, bt_transport_t trasnport)
{
    UNUSED(id);
    struct bt_conn_info info;
    struct bt_conn* conn = NULL;

    if (trasnport == BT_TRANSPORT_BLE) {
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        conn = get_le_conn_from_addr(addr);
        if (!conn) {
            BT_LOGE("%s, conn null", __func__);
            return BT_INVALID_CONNECTION_HANDLE;
        }

        if (bt_conn_get_info(conn, &info)) {
            BT_LOGE("%s, bt_conn_get_info fail", __func__);
            return BT_INVALID_CONNECTION_HANDLE;
        }

        return info.handle;
#endif
    } else if (trasnport == BT_TRANSPORT_BREDR) {
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
        conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
        if (!conn) {
            BT_LOGE("%s, conn null", __func__);
            return BT_INVALID_CONNECTION_HANDLE;
        }

        if (bt_conn_get_info(conn, &info)) {
            BT_LOGE("%s, bt_conn_get_info fail", __func__);
            bt_conn_unref(conn);
            return BT_INVALID_CONNECTION_HANDLE;
        }

        bt_conn_unref(conn);
        return info.handle;
#endif
    }

    return BT_INVALID_CONNECTION_HANDLE;
}

uint16_t bt_sal_get_sco_connection_handle(bt_controller_id_t id, bt_address_t* addr)
{
    return BT_INVALID_CONNECTION_HANDLE;
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static bt_list_t* g_pending_connect_list = NULL;

static void sal_pending_connect_init(void)
{
    if (!g_pending_connect_list)
        g_pending_connect_list = bt_list_new(free);
}

static void sal_pending_connect_cleanup(void)
{
    if (g_pending_connect_list) {
        bt_list_free(g_pending_connect_list);
        g_pending_connect_list = NULL;
    }
}

static void sal_pending_connect_done(bt_address_t* addr)
{
    bt_list_node_t* node;
    sal_adapter_req_t* pending;

    if (!g_pending_connect_list)
        return;

    for (node = bt_list_head(g_pending_connect_list); node;
         node = bt_list_next(g_pending_connect_list, node)) {
        pending = (sal_adapter_req_t*)bt_list_node(node);
        if (pending && !memcmp(&pending->addr, addr, sizeof(bt_address_t))) {
            bt_list_remove(g_pending_connect_list, pending);
            return;
        }
    }
}

static bool sal_start_connect(void)
{
    bt_list_node_t* node;
    sal_adapter_req_t* pending;
    struct bt_conn* conn;
    acl_state_param_t state;

    if (!g_pending_connect_list)
        return false;

    node = bt_list_head(g_pending_connect_list);
    if (!node)
        return false;

    pending = (sal_adapter_req_t*)bt_list_node(node);
    if (!pending)
        return false;

    conn = bt_conn_create_br((const bt_addr_t*)&pending->addr, BT_BR_CONN_PARAM_DEFAULT);
    if (!conn) {
        bt_list_remove(g_pending_connect_list, pending);
        BT_LOGW("bt_conn_create_br failed");
        return false;
    }

    memset(&state, 0, sizeof(state));
    state.transport = BT_TRANSPORT_BREDR;
    state.connection_state = CONNECTION_STATE_CONNECTING;
    memcpy(&state.addr, &pending->addr, sizeof(bt_address_t));
    adapter_on_connection_state_changed(&state);

    bt_conn_unref(conn);
    return true;
}

static void STACK_CALL(pending_connect_complete)(void* args)
{
    sal_adapter_req_t* req = args;

    sal_pending_connect_done(&req->addr);
    sal_start_connect();
}

static bool sal_pending_connect_exists(bt_address_t* addr)
{
    bt_list_node_t* node;
    sal_adapter_req_t* pending;

    if (!g_pending_connect_list)
        return false;

    for (node = bt_list_head(g_pending_connect_list); node;
         node = bt_list_next(g_pending_connect_list, node)) {
        pending = (sal_adapter_req_t*)bt_list_node(node);
        if (pending && !memcmp(&pending->addr, addr, sizeof(bt_address_t)))
            return true;
    }

    return false;
}

static void STACK_CALL(connect)(void* args)
{
    sal_adapter_req_t* req = args;
    sal_adapter_req_t* pending;

    if (sal_pending_connect_exists(&req->addr)) {
        BT_LOGD("bt_sal_connect: already connecting or queued");
        return;
    }

    pending = zalloc(sizeof(sal_adapter_req_t));
    if (!pending)
        return;

    memcpy(&pending->addr, &req->addr, sizeof(bt_address_t));
    bt_list_add_tail(g_pending_connect_list, pending);

    if (bt_list_length(g_pending_connect_list) > 1) {
        BT_LOGD("bt_sal_connect: queued pending connect");
        return;
    }

    if (!sal_start_connect())
        BT_LOGW("bt_sal_connect: first connection failed");
}
#endif

bt_status_t bt_sal_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);

    return sal_send_req(sal_adapter_req(id, addr, STACK_CALL(connect)));
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(disconnect)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);
    if (conn == NULL) {
        return;
    }

    SAL_CHECK(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN), 0);
    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_disconnect(bt_controller_id_t id, bt_address_t* addr, uint8_t reason)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;
    bt_status_t status;

    /* disconnect profile, then disconnect acl */
    status = bt_sal_cm_try_disconnect_profiles(addr, false);

    if (status == BT_STATUS_SUCCESS) {
        return status;
    }

    req = sal_adapter_req(id, addr, STACK_CALL(disconnect));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.reason = reason;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_disconnect_internal(bt_controller_id_t id, bt_address_t* addr, uint8_t reason)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(disconnect));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.reason = reason;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(create_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    bond_state_t state = BOND_STATE_NONE;
    struct bt_conn* conn;

    conn = bt_conn_pair_br((bt_addr_t*)&req->addr, g_security_level);
    if (conn) {
        state = BOND_STATE_BONDING;
        bt_conn_unref(conn);
    }

    adapter_on_bond_state_changed(&req->addr, state, BT_TRANSPORT_BREDR, BT_STATUS_SUCCESS, false);
}
#endif

bt_status_t bt_sal_create_bond(bt_controller_id_t id, bt_address_t* addr, bt_transport_t transport, bt_addr_type_t type)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(create_bond));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.bond.transport = transport;
    req->adpt.bond.type = type;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_security_level)(void* args)
{
    sal_adapter_req_t* req = args;

    g_security_level = req->adpt.security_level;
}
#endif

bt_status_t bt_sal_set_security_level(bt_controller_id_t id, uint8_t level)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_security_level));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    req->adpt.security_level = level;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(cancel_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    SAL_CHECK(bt_conn_auth_cancel(conn), 0);
    SAL_CHECK(bt_br_unpair((bt_addr_t*)&req->addr), 0);

    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_cancel_bond(bt_controller_id_t id, bt_address_t* addr, bt_transport_t transport)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(cancel_bond));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.bond.transport = transport;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(remove_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    SAL_CHECK(bt_br_unpair((bt_addr_t*)&req->addr), 0);
}
#endif

bt_status_t bt_sal_remove_bond(bt_controller_id_t id, bt_address_t* addr, bt_transport_t transport)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;
    bt_status_t status;

    status = bt_sal_cm_try_disconnect_profiles(addr, true);

    if (status == BT_STATUS_SUCCESS) {
        return status;
    }

    req = sal_adapter_req(id, addr, STACK_CALL(remove_bond));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.bond.transport = transport;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_remove_bond_internal(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(remove_bond));
    if (!req)
        return BT_STATUS_NOMEM;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_set_remote_oob_data(bt_controller_id_t id, bt_address_t* addr,
    bt_oob_data_t* p192_val, bt_oob_data_t* p256_val)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_remove_remote_oob_data(bt_controller_id_t id, bt_address_t* addr)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_get_local_oob_data(bt_controller_id_t id)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_get_remote_device_info(bt_controller_id_t id, bt_address_t* addr, remote_device_properties_t* properties)
{
    UNUSED(id);
    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(set_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t le_addr;

    memcpy(le_addr.a.val, req->addr.addr, sizeof(le_addr.a.val));
    le_addr.type = BT_LE_ADDR_TYPE_PUBLIC;

#ifdef CONFIG_SETTINGS_ZBLUE
    bt_settings_load(req->id, 0, "link_key", &le_addr);
#endif
}

bt_status_t bt_sal_set_bonded_devices(bt_controller_id_t id, remote_device_properties_t* props, int cnt)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    sal_adapter_req_t* req;
    bt_status_t status;

    for (int i = 0; i < cnt; i++) {
        req = sal_adapter_req(id, &props->addr, STACK_CALL(set_bond));
        if (!req) {
            BT_LOGE("%s, req null", __func__);
            return BT_STATUS_NOMEM;
        }

        status = sal_send_req(req);
        if (status) {
            BT_LOGE("%s send req error, ret: %d", __func__, status);
            return status;
        }

        props++;
    }

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void get_bonded_devices(const struct bt_bond_info* info,
    void* user_data)
{
    struct device_context* ctx = user_data;
    uint8_t* link_key;

    if (ctx->got < ctx->cnt) {
        memcpy(&ctx->props->addr, &info->addr, 6);
        link_key = adapter_get_link_key(&ctx->props->addr);
        if (link_key) {
            memcpy(ctx->props->link_key, link_key, 16);
            ctx->props->link_key_type = adapter_get_link_key_type(&ctx->props->addr);
        }

        ctx->props++;
        ctx->got++;
    }
}
#endif

bt_status_t bt_sal_get_bonded_devices(bt_controller_id_t id, remote_device_properties_t* props, int* cnt)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    struct device_context ctx;

    ctx.props = props;
    ctx.cnt = *cnt;
    ctx.got = 0;

    bt_foreach_bond_br(get_bonded_devices, &ctx);
    *cnt = ctx.got;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void get_connected_devices(struct bt_conn* conn, void* data)
{
    struct device_context* ctx = data;
    struct bt_conn_info info;

    if (ctx->got < ctx->cnt) {
        bt_conn_get_info(conn, &info);
        memcpy(&ctx->props->addr, info.br.dst->val, 6);
        ctx->props++;
        ctx->got++;
    }
}
#endif

bt_status_t bt_sal_get_connected_devices(bt_controller_id_t id, remote_device_properties_t* props, int* cnt)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    struct device_context ctx;

    ctx.props = props;
    ctx.cnt = *cnt;
    ctx.got = 0;

    bt_conn_foreach(BT_CONN_TYPE_BR, get_connected_devices, &ctx);
    *cnt = ctx.got;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

/* Service discovery */
bt_status_t bt_sal_start_service_discovery(bt_controller_id_t id, bt_address_t* addr, bt_uuid_t* uuid)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(stop_service_discovery)(void* args)
{
}
#endif

bt_status_t bt_sal_stop_service_discovery(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);

    return sal_send_req(sal_adapter_req(id, addr, STACK_CALL(stop_service_discovery)));
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
/* Link policy */
#ifdef CONFIG_BT_POWER_MODE_CONTROL
static void STACK_CALL(set_power_mode)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_pm_mode_t* pm = &req->adpt.mode;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    if (pm->mode == BT_LINK_MODE_ACTIVE) {
        SAL_CHECK(bt_conn_exit_sniff_mode(conn), 0);
    } else {
        SAL_CHECK(bt_conn_enter_sniff_mode(conn, pm->min, pm->max, pm->attempt, pm->timeout), 0);
    }

    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_set_power_mode(bt_controller_id_t id, bt_address_t* addr, bt_pm_mode_t* mode)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(set_power_mode));
    if (!req)
        return BT_STATUS_NOMEM;

    memcpy(&req->adpt.mode, mode, sizeof(*mode));

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}
#else /* CONFIG_BT_POWER_MODE_CONTROL */
bt_status_t bt_sal_set_power_mode(bt_controller_id_t id, bt_address_t* addr, bt_pm_mode_t* mode)
{
    UNUSED(id);
    UNUSED(addr);
    UNUSED(mode);

    return BT_STATUS_NOT_SUPPORTED;
}
#endif /* CONFIG_BT_POWER_MODE_CONTROL */

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_link_role)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    SAL_CHECK(bt_conn_switch_role(conn, req->adpt.role), 0);
    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_set_link_role(bt_controller_id_t id, bt_address_t* addr, bt_link_role_t role)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(set_link_role));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.role = role;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_link_policy)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    uint16_t policy = 0;

    switch (req->adpt.policy) {
    case BT_BR_LINK_POLICY_DISABLE_ALL:
        break;
    case BT_BR_LINK_POLICY_ENABLE_ROLE_SWITCH:
        policy = 1 << BT_HCI_POLICY_ROLE_SWITCH;
        break;
    case BT_BR_LINK_POLICY_ENABLE_SNIFF:
        policy = 1 << BT_HCI_POLICY_SNIFF_MODE;
        break;
    case BT_BR_LINK_POLICY_ENABLE_ROLE_SWITCH_AND_SNIFF:
        policy = (1 << BT_HCI_POLICY_ROLE_SWITCH) | (1 << BT_HCI_POLICY_SNIFF_MODE);
        break;
    default:
        break;
    }

    if (!bt_conn_set_link_policy_settings(conn, policy)) {
        adapter_on_link_policy_changed(&req->addr, req->adpt.policy);
    }

    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_set_link_policy(bt_controller_id_t id, bt_address_t* addr, bt_link_policy_t policy)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(set_link_policy));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.policy = policy;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_afh_channel_classification)(void* args)
{
}
#endif

bt_status_t bt_sal_set_afh_channel_classification(bt_controller_id_t id, uint16_t central_frequency,
    uint16_t band_width, uint16_t number)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_afh_channel_classification));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.afh.central_frequency = central_frequency;
    req->adpt.afh.band_width = band_width;
    req->adpt.afh.number = number;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_afh_channel_classification_1)(void* args)
{
}
#endif

bt_status_t bt_sal_set_afh_channel_classification_1(bt_controller_id_t id, uint8_t* map)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_afh_channel_classification_1));
    if (!req)
        return BT_STATUS_NOMEM;

    memcpy(req->adpt.map, map, 10);

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

static void STACK_CALL(read_rssi)(void* args)
{
    int err;
    int8_t rssi;
    struct bt_conn* conn = NULL;
    sal_adapter_req_t* req = args;

    switch (req->adpt.transport) {
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    case BT_TRANSPORT_BREDR:
        conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);
        break;
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    case BT_TRANSPORT_BLE:
        conn = get_le_conn_from_addr(&req->addr);
        if (conn)
            bt_conn_ref(conn);

        break;
#endif
    default:
        BT_LOGW("%s, unsupported transport: %d", __func__, req->adpt.transport);
        return;
    }

    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    err = bt_conn_read_rssi(conn, &rssi);
    bt_conn_unref(conn);
    if (err) {
        BT_LOGE("%s, failed to read rssi, err = %d", __func__, err);
        return;
    }

    adapter_on_rssi_read(&req->addr, rssi, req->adpt.transport);
}

bt_status_t bt_sal_read_rssi(bt_controller_id_t id, bt_address_t* addr, bt_transport_t transport)
{
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(read_rssi));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.transport = transport;
    return sal_send_req(req);
}

/* VSC */
bt_status_t bt_sal_send_hci_command(bt_controller_id_t id, uint8_t ogf, uint16_t ocf, uint8_t length, uint8_t* buf,
    bt_hci_event_callback_t cb, void* context)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}
