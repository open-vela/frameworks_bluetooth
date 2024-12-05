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

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_err.h>

#include <settings/settings.h>

#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT

extern int zblue_main(void);

static void zblue_on_connected(struct bt_conn* conn, uint8_t err);
static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason);
static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level, enum bt_security_err err);
static void zblue_on_phy_updated(struct bt_conn* conn, struct bt_conn_le_phy_info* info);
static void zblue_on_param_updated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout);

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
}

static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err)
{
}

static void zblue_on_param_updated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
}

static void zblue_on_phy_updated(struct bt_conn* conn, struct bt_conn_le_phy_info* phy)
{
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

    return BT_STATUS_SUCCESS;
}

void bt_sal_le_cleanup(void)
{
    bt_conn_cb_register(NULL);
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

bt_status_t bt_sal_le_set_io_capability(bt_controller_id_t id, bt_io_capability_t cap)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_get_bonded_devices(bt_controller_id_t id, remote_device_le_properties_t* props, uint16_t* prop_cnt)
{
    SAL_NOT_SUPPORT;
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

bt_status_t bt_sal_le_connect(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type, ble_connect_params_t* params)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_create_bond(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t type)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_remove_bond(bt_controller_id_t id, bt_address_t* addr)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_smp_reply(bt_controller_id_t id, bt_address_t* addr, bool accept, bt_pair_type_t type, uint32_t passkey)
{
    SAL_NOT_SUPPORT;
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

bt_status_t bt_sal_le_add_white_list(bt_controller_id_t id, bt_address_t* address, ble_addr_type_t addr_type)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_remove_white_list(bt_controller_id_t id, bt_address_t* address, ble_addr_type_t addr_type)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_set_phy(bt_controller_id_t id, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_set_appearance(bt_controller_id_t id, uint16_t appearance)
{
    SAL_NOT_SUPPORT;
}

bt_status_t bt_sal_le_enable_key_derivation(bt_controller_id_t id, bool brkey_to_lekey, bool lekey_to_brkey)
{
    /* todo: */
    SAL_NOT_SUPPORT;
}

#endif /* CONFIG_BLUETOOTH_BLE_SUPPORT */
