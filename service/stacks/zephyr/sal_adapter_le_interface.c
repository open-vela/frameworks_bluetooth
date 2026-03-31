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
#include "cs_service.h"
#include "gattc_service.h"
#include "gatts_service.h"
#include "sal_gatt_client_interface.h"
#include "sal_gatt_server_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "sal_zephyr_interface.h"
#include "service_loop.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>

#include <settings_zblue.h>
#include <zephyr/settings/settings.h>

#include "keys.h"

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
    struct {
        void* key;
        uint8_t id;
    } le_set_bond;
    struct {
        bool accept;
        bt_pair_type_t type;
        uint32_t passkey;
    } smp;
    int security_level;
    bool bondable;
    uint8_t ctkd_mode;
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

extern void z_sys_init(void);

static void zblue_on_connected(struct bt_conn* conn, uint8_t err);
static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason);
#ifdef CONFIG_BT_SMP
static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level, enum bt_security_err err);
static void zblue_on_pairing_complete_ctkd(struct bt_conn* conn, bool is_link_key);
#endif
static void zblue_on_pairing_complete(struct bt_conn* conn, bool bonding_flag);
static void zblue_on_pairing_failed(struct bt_conn* conn, enum bt_security_err reason);
static void zblue_on_bond_deleted(uint8_t id, const bt_addr_le_t* peer);
static void zblue_convert_le_addr(bt_address_t* addr, ble_addr_type_t type, bt_addr_le_t* le_addr);
#if defined(CONFIG_SETTINGS_ZBLUE)
static int zblue_on_irk_notify(uint8_t dev_id, const char* key_value, uint8_t value_len);
static int zblue_on_irk_load(uint8_t* key_value, uint8_t value_len);
static int zblue_on_ltk_notify(uint8_t dev_id, uint8_t id, bt_addr_le_t* addr, const char* key_value, uint8_t value_len);
static int zblue_on_ltk_load(bt_addr_le_t* addr, uint8_t* key_value, uint8_t value_len);
#endif
#if defined(CONFIG_BT_USER_PHY_UPDATE)
static void zblue_on_phy_updated(struct bt_conn* conn, struct bt_conn_le_phy_info* info);
#endif
static void zblue_on_param_updated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout);

#ifdef CONFIG_BT_SMP
static void zblue_on_auth_passkey_display(struct bt_conn* conn, unsigned int passkey);
static void zblue_on_auth_passkey_confirm(struct bt_conn* conn, unsigned int passkey);
static void zblue_on_auth_passkey_entry(struct bt_conn* conn);
static void zblue_on_auth_cancel(struct bt_conn* conn);
static void zblue_on_auth_pairing_confirm(struct bt_conn* conn);
#ifdef CONFIG_BT_SMP_APP_PAIRING_ACCEPT
static enum bt_security_err zblue_on_pairing_accept(struct bt_conn* conn, const struct bt_conn_pairing_feat* const feat);
#endif
#endif
static void zblue_register_callback(void);
static void zblue_unregister_callback(void);

#if defined(CONFIG_BLUETOOTH_LE_CS) && defined(CONFIG_BT_CHANNEL_SOUNDING)
static void zblue_on_cs_subevent(struct bt_conn* conn, struct bt_conn_le_cs_subevent_result* result);
static void zblue_on_cs_capabilities_available(struct bt_conn* conn,
    struct bt_conn_le_cs_capabilities* params);
static void zblue_on_cs_remote_fae_table_available(struct bt_conn* conn,
    struct bt_conn_le_cs_fae_table* params);
static void zblue_on_cs_config_created(struct bt_conn* conn, struct bt_conn_le_cs_config* config);
static void zblue_on_cs_config_removed(struct bt_conn* conn, uint8_t config_id);
static void zblue_on_cs_security_enabled(struct bt_conn* conn);
static void zblue_on_cs_procedure_enabled(struct bt_conn* conn,
    struct bt_conn_le_cs_procedure_enable_complete* params);
#endif /* CONFIG_BLUETOOTH_LE_CS &&  CONFIG_BT_CHANNEL_SOUNDING*/

static struct bt_conn_cb g_conn_cbs = {
    .connected = zblue_on_connected,
    .disconnected = zblue_on_disconnected,
#ifdef CONFIG_BT_SMP
    .security_changed = zblue_on_security_changed,
#endif
    .le_param_updated = zblue_on_param_updated,
#if defined(CONFIG_BT_USER_PHY_UPDATE)
    .le_phy_updated = zblue_on_phy_updated,
#endif
#if defined(CONFIG_BLUETOOTH_LE_CS) && defined(CONFIG_BT_CHANNEL_SOUNDING)
    .le_cs_remote_capabilities_available = zblue_on_cs_capabilities_available,
    .le_cs_remote_fae_table_available = zblue_on_cs_remote_fae_table_available,
    .le_cs_config_created = zblue_on_cs_config_created,
    .le_cs_config_removed = zblue_on_cs_config_removed,
    .le_cs_security_enabled = zblue_on_cs_security_enabled,
    .le_cs_procedure_enabled = zblue_on_cs_procedure_enabled,
    .le_cs_subevent_data_available = zblue_on_cs_subevent,
#endif /* CONFIG_BLUETOOTH_LE_CS && CONFIG_BT_CHANNEL_SOUNDING */
};

static struct bt_conn_auth_info_cb g_conn_auth_info_cbs = {
    .pairing_complete_ctkd = zblue_on_pairing_complete_ctkd,
    .pairing_complete = zblue_on_pairing_complete,
    .pairing_failed = zblue_on_pairing_failed,
    .bond_deleted = zblue_on_bond_deleted,
};

#if defined(CONFIG_SETTINGS_ZBLUE)
static struct bt_settings_zblue_cb g_setting_cbs = {
    .irk_notify = zblue_on_irk_notify,
    .irk_load = zblue_on_irk_load,
    .ltk_notify = zblue_on_ltk_notify,
    .ltk_load = zblue_on_ltk_load,
};
#endif

static struct bt_conn_auth_cb g_conn_auth_cbs;
static bt_security_t g_security_level = BT_SECURITY_L2;

static uint8_t zblue_convert_addr_type(ble_addr_type_t addr_type)
{
    uint8_t type;

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
        type = BT_ADDR_LE_PUBLIC;
        break;
    default:
        BT_LOGE("%s, invalid type:%d", __func__, addr_type);
        assert(0);
    }

    return type;
}

#if defined(CONFIG_SETTINGS_ZBLUE)
static int zblue_on_irk_notify(uint8_t dev_id, const char* key_value, uint8_t value_len)
{
    BT_LOGD("%s", __func__);

    adapter_on_irk_changed(key_value, value_len);

    return 0;
}

static bool irk_is_empty(const uint8_t* irk)
{
    for (int i = 0; i < 16; i++) {
        if (irk[i] != 0) {
            return false;
        }
    }

    return true;
}

static int zblue_on_irk_load(uint8_t* key_value, uint8_t value_len)
{
    uint8_t* irk;

    irk = adapter_get_local_irk();
    if (irk_is_empty(irk)) {
        return 0;
    }

    memcpy(key_value, irk, value_len);
    return value_len;
}
/**
 * struct smp_key {
 *     uint8_t id_addr[6];
 *     uint8_t id_addr_type;
 *     uint8_t id_num;
 *
 *     uint8_t enc_size;
 *
 *     uint8_t flags;
 *
 *     uint8_t ltk[16];
 *     uint8_t ediv[2];
 *     uint8_t rand[8];
 *
 *     uint8_t irk[16];
 *
 *     uint8_t csrk[16];
 *
 *     uint8_t rpa_addr[6];
 *     uint8_t rpa_addr_type;
 *     uint8_t id_num;
 *
 *     uint16_t keys;
 * };
 */
static int zblue_on_ltk_notify(uint8_t dev_id, uint8_t id, bt_addr_le_t* addr, const char* key_value, uint8_t value_len)
{
    remote_device_le_properties_t* prop;
    struct bt_keys* keys;
    bt_address_t le_addr;
    BT_LOGD("%s", __func__);

    if (!key_value) {
        BT_LOGD("%s, delete key_value", __func__);
        return 0;
    }

    prop = zalloc(sizeof(remote_device_le_properties_t));
    if (!prop) {
        BT_LOGD("%s, prop malloc failed", __func__);
        return -ENOSPC;
    }

    keys = (struct bt_keys*)zalloc(sizeof(struct bt_keys));
    if (!keys) {
        BT_LOGD("%s, keys malloc failed", __func__);
        free(prop);
        return -ENOSPC;
    }

    memcpy(keys->storage_start, key_value, value_len);

    memcpy(le_addr.addr, keys->irk.rpa.val, sizeof(le_addr.addr));
    if (!bt_addr_is_empty(&le_addr)) {
        memcpy(prop->addr.addr, keys->irk.rpa.val, sizeof(prop->addr.addr));
        prop->addr_type = BT_LE_ADDR_TYPE_RANDOM;
    } else {
        memcpy(prop->addr.addr, addr->a.val, sizeof(prop->addr.addr));
        prop->addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    }

    /**
     * smp[0 ~ 5]  id_addr
     * smp[6] id_addr type
     * smp[7] id_addr cap/id_num
     */
    memcpy(&prop->smp_key[0], addr->a.val, 6);
    prop->smp_key[6] = addr->type;

    /* SMP[8] LTK_len */
    prop->smp_key[8] = keys->enc_size;

    /* smp[9] LTK fea/flags */
    prop->smp_key[9] = keys->flags;
    /* smp[10 ~ 11] div[2](unused); */

    /**
     * smp[12 ~ 27] LTK key
     * smp[28 ~ 29] ediv(legacy)
     * smp[30 ~ 37] rand(legacy)
     */
    if (keys->keys & BT_KEYS_PERIPH_LTK) {
        memcpy(&prop->smp_key[12], keys->periph_ltk.val, 16);
        memcpy(&prop->smp_key[28], keys->periph_ltk.ediv, 2);
        memcpy(&prop->smp_key[30], keys->periph_ltk.rand, 8);
    } else {
        memcpy(&prop->smp_key[12], keys->ltk.val, 16);
        memcpy(&prop->smp_key[28], keys->ltk.ediv, 2);
        memcpy(&prop->smp_key[30], keys->ltk.rand, 8);
    }

    /* smp[38 ~ 53] IRK */
    memcpy(&prop->smp_key[38], keys->irk.val, 16);
    /* smp[54 ~ 69] CSRK(remote) */
    memcpy(&prop->smp_key[54], keys->remote_csrk.val, 16);

    // smp[70 ~ 77] addr { addr[6], type[1], cap[1]/id_num[1] };
    memcpy(&prop->smp_key[70], prop->addr.addr, sizeof(prop->addr.addr));
    prop->smp_key[76] = prop->addr_type;

    /* smp[78 ~ 79] RFU/keys; */
    memcpy(&prop->smp_key[78], &keys->keys, 2);

    memcpy(prop->local_csrk, keys->local_csrk.val, 16);

    adapter_on_le_bonded_device_update(prop, 1);
    free(prop);
    free(keys);

    return 0;
}

static int zblue_on_ltk_load(bt_addr_le_t* addr, uint8_t* key_value, uint8_t value_len)
{
    BT_LOGD("%s", __func__);
    uint8_t *smp_data, *local_csrk;
    bt_address_t le_addr, *remote_addr;
    struct bt_keys* keys;

    keys = (struct bt_keys*)zalloc(sizeof(struct bt_keys));
    if (!keys) {
        BT_LOGE("%s, malloc failed", __func__);
        return -ENOSPC;
    }

    /* get information */
    memcpy(&le_addr, addr->a.val, sizeof(le_addr.addr));
    remote_addr = adapter_get_le_remote_address(&le_addr, addr->type);
    local_csrk = adapter_get_local_csrk(remote_addr);
    smp_data = adapter_get_smp_data(remote_addr);
    if (!smp_data) {
        BT_LOGE("%s, smp_data is NULL", __func__);
        free(keys);
        return -EINVAL;
    }

    /* Rearrange data */
    keys->enc_size = smp_data[8];
    keys->flags = smp_data[9];

    memcpy(&keys->keys, &smp_data[78], 2);

    if (keys->keys & BT_KEYS_PERIPH_LTK) {
        memcpy(keys->periph_ltk.val, &smp_data[12], 16);
        memcpy(keys->periph_ltk.ediv, &smp_data[28], 2);
        memcpy(keys->periph_ltk.rand, &smp_data[30], 8);
    } else {
        memcpy(keys->ltk.val, &smp_data[12], 16);
        memcpy(keys->ltk.ediv, &smp_data[28], 2);
        memcpy(keys->ltk.rand, &smp_data[30], 8);
    }

    memcpy(keys->irk.val, &smp_data[38], 16);

    memcpy(keys->irk.rpa.val, remote_addr->addr, sizeof(remote_addr->addr));

    memcpy(keys->remote_csrk.val, &smp_data[54], 16);

    if (local_csrk)
        memcpy(keys->local_csrk.val, local_csrk, 16);

    memcpy(key_value, keys->storage_start, value_len);

    free(keys);

    return value_len;
}
#endif

static void zblue_on_connected(struct bt_conn* conn, uint8_t err)
{
    uint8_t role;
    struct bt_conn_info info;
    bt_conn_info_t* slot;
#if defined(CONFIG_BLUETOOTH_GATT_CLIENT) || defined(CONFIG_BLUETOOTH_GATT_SERVER)
    profile_connection_state_t profile_state = PROFILE_STATE_CONNECTED;
#endif

    bt_address_t le_addr;
    bt_address_t* remote_addr;
    acl_state_param_t state = {
        .transport = BT_TRANSPORT_BLE,
        .connection_state = CONNECTION_STATE_CONNECTED
    };

    BT_LOGD("%s, err:%d", __func__, err);
    bt_conn_get_info(conn, &info);

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    memcpy(&le_addr, info.le.dst->a.val, sizeof(le_addr.addr));
    remote_addr = adapter_get_le_remote_address(&le_addr, info.le.dst->type);
    if (remote_addr) {
        memcpy(&state.addr, remote_addr, sizeof(state.addr));
        state.addr_type = adapter_get_le_remote_address_type(remote_addr);
    } else {
        memcpy(&state.addr, &le_addr, sizeof(state.addr));
        state.addr_type = info.le.dst->type;
    }

    if (err) {
        state.connection_state = CONNECTION_STATE_DISCONNECTED;
        state.status = err;
#if defined(CONFIG_BLUETOOTH_GATT_CLIENT) || defined(CONFIG_BLUETOOTH_GATT_SERVER)
        profile_state = PROFILE_STATE_DISCONNECTED;
#endif

        if (info.role == BT_HCI_ROLE_CENTRAL) {
            bt_conn_unref(conn);
        }
    }

    slot = bt_conn_add(&state.addr, BT_TRANSPORT_BLE);

    if (!slot) {
        return;
    }

    if (!err) {
        slot->conn = conn;
        if (!slot->role) {
            slot->role |= GATT_ROLE_SERVER;
        }
    }

    role = slot->role;

    if (err || (slot->conn == NULL)) {
        bt_conn_remove(&state.addr, BT_TRANSPORT_BLE);
        slot = NULL;
    }

    adapter_on_connection_state_changed(&state);
#ifdef CONFIG_BLUETOOTH_GATT_SERVER
    if (role & GATT_ROLE_SERVER) {
        bt_sal_gatt_server_connection_state_changed_callback(PRIMARY_ADAPTER, &state.addr, profile_state);
    }
#endif

#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
    if (role & GATT_ROLE_CLIENT) {
        bt_sal_gatt_client_connection_state_changed_callback(PRIMARY_ADAPTER, &state.addr, profile_state);
    }
#endif
}

bt_status_t bt_sal_get_identity_addr(bt_address_t* addr, bt_address_t* id_addr)
{
    struct bt_conn_info info;
    struct bt_conn* conn;

    BT_LOGD("%s", __func__);
    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        bt_addr_set_empty(id_addr);
        return BT_STATUS_FAIL;
    }

    bt_conn_get_info(conn, &info);
    if (info.type != BT_CONN_TYPE_LE) {
        bt_addr_set_empty(id_addr);
        return BT_STATUS_FAIL;
    }

    memcpy(id_addr, info.le.dst->a.val, sizeof(id_addr->addr));
    return BT_STATUS_SUCCESS;
}

static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason)
{
    struct bt_conn_info info;
    bt_conn_info_t* slot;
    uint8_t role;
    bt_address_t le_addr;
    bt_address_t* remote_addr;
    acl_state_param_t state = {
        .transport = BT_TRANSPORT_BLE,
        .connection_state = CONNECTION_STATE_DISCONNECTED,
        .hci_reason_code = reason
    };

    BT_LOGD("%s", __func__);
    bt_conn_get_info(conn, &info);

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    if (info.role == BT_HCI_ROLE_CENTRAL) {
        bt_conn_unref(conn);
    }

    memcpy(&le_addr, info.le.dst->a.val, sizeof(le_addr.addr));
    remote_addr = adapter_get_le_remote_address(&le_addr, info.le.dst->type);
    if (remote_addr) {
        memcpy(&state.addr, remote_addr, sizeof(state.addr));
        state.addr_type = adapter_get_le_remote_address_type(remote_addr);
    } else {
        memcpy(&state.addr, info.le.remote->a.val, sizeof(state.addr));
        state.addr_type = info.le.remote->type;
    }

    slot = bt_conn_find(&state.addr, BT_TRANSPORT_BLE);

    if (!slot) {
        return;
    }

    role = slot->role;
    bt_conn_remove(&state.addr, BT_TRANSPORT_BLE);
    slot = NULL;

#ifdef CONFIG_BLUETOOTH_GATT_SERVER
    if (role & GATT_ROLE_SERVER) {
        bt_sal_gatt_server_connection_state_changed_callback(PRIMARY_ADAPTER, &state.addr, PROFILE_STATE_DISCONNECTED);
    }
#endif

#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
    if (role & GATT_ROLE_CLIENT) {
        bt_sal_gatt_client_connection_state_changed_callback(PRIMARY_ADAPTER, &state.addr, PROFILE_STATE_DISCONNECTED);
    }
#endif

    adapter_on_connection_state_changed(&state);
}

#ifdef CONFIG_BT_SMP
static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err)
{
    struct bt_conn_info info;
    bt_address_t addr;
    bt_address_t le_addr;
    bt_address_t* remote_addr;
    bool encrypted = false;

    bt_conn_get_info(conn, &info);

    if (info.type != BT_CONN_TYPE_LE) {
        return;
    }

    BT_LOGD("%s, state: %d, level: %d, required level: %d, err: %d",
        __func__, info.state, level, g_security_level, err);

    memcpy(&le_addr, info.le.dst->a.val, sizeof(le_addr.addr));
    remote_addr = adapter_get_le_remote_address(&le_addr, info.le.dst->type);
    if (remote_addr) {
        memcpy(&addr, remote_addr, sizeof(addr.addr));
    } else {
        memcpy(&addr, info.le.remote->a.val, sizeof(addr.addr));
    }

    if (level >= g_security_level && err == BT_SECURITY_ERR_SUCCESS) {
        encrypted = true;
        adapter_on_encryption_state_changed(&addr, encrypted, BT_TRANSPORT_BLE);
        return;
    }

    if (!adapter_get_pts_mode() && (level < g_security_level) && (err == BT_SECURITY_ERR_AUTH_FAIL || err == BT_SECURITY_ERR_PIN_OR_KEY_MISSING)) {
        adapter_on_bond_state_changed(&addr, BOND_STATE_NONE, BT_TRANSPORT_BLE, BT_STATUS_FAIL, false);
        BT_LOGD("%s, err: %d, remove old key async", __func__, err);
        bt_sal_le_remove_bond(PRIMARY_ADAPTER, &addr);
    } else if (err != BT_SECURITY_ERR_SUCCESS) {
        BT_LOGW("%s, preserve bond on LE security failure, state: %d, level: %d, required: %d, err: %d",
            __func__, info.state, level, g_security_level, err);
    } else if (level < g_security_level) {
        BT_LOGW("%s, security level insufficient: achieved %d, required %d",
            __func__, level, g_security_level);
    }

    adapter_on_encryption_state_changed(&addr, encrypted, BT_TRANSPORT_BLE);
}
#endif

static void zblue_on_param_updated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    bt_address_t addr;
    bt_conn_info_t* slot;
    uint8_t role;

    if (get_le_addr_from_conn(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_le_addr_from_conn failed", __func__);
        return;
    }

    BT_LOGD("%s, interval:%d, latency:%d, timeout:%d", __func__, interval, latency, timeout);

    slot = bt_conn_find(&addr, BT_TRANSPORT_BLE);
    if (!slot) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }
    role = slot->role;

#ifdef CONFIG_BLUETOOTH_GATT_SERVER
    if (role & GATT_ROLE_SERVER) {
        if_gatts_on_connection_parameter_changed(&addr, interval, latency, timeout);
    }
#endif

#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
    if (role & GATT_ROLE_CLIENT) {
        if_gattc_on_connection_parameter_updated(&addr, interval, latency, timeout, BT_STATUS_SUCCESS);
    }
#endif
}

#if defined(CONFIG_BLUETOOTH_LE_CS) && defined(CONFIG_BT_CHANNEL_SOUNDING)
static void zblue_dump_cs_capabilities(struct bt_conn_le_cs_capabilities* params)
{
    if (!params) {
        return;
    }

    BT_LOGD("num_config_supported:%d, max_consecutive_procedures_supported:%d",
        params->num_config_supported, params->max_consecutive_procedures_supported);
    BT_LOGD("num_antennas_supported:%d, max_antenna_paths_supported:%d.",
        params->num_antennas_supported, params->max_antenna_paths_supported);
    BT_LOGD("initiator_supported:%d, reflector_supported:%d",
        params->initiator_supported, params->reflector_supported);
    BT_LOGD("mode_3_supported:%d, rtt_aa_only_precision:%d",
        params->mode_3_supported, params->rtt_aa_only_precision);
    BT_LOGD("rtt_sounding_precision:%d, rtt_random_payload_precision:%d",
        params->rtt_sounding_precision, params->rtt_random_payload_precision);
    BT_LOGD("rtt_aa_only_n:%d, rtt_sounding_n:%d, rtt_random_payload_n:%d",
        params->rtt_aa_only_n, params->rtt_sounding_n, params->rtt_random_payload_n);
    BT_LOGD("phase_based_nadm_sounding_supported:%d, phase_based_nadm_random_supported:%d",
        params->phase_based_nadm_sounding_supported, params->phase_based_nadm_random_supported);
    BT_LOGD("cs_sync_2m_phy_supported:%d, cs_sync_2m_2bt_phy_supported:%d",
        params->cs_sync_2m_phy_supported, params->cs_sync_2m_2bt_phy_supported);
    BT_LOGD("cs_without_fae_supported:%d, chsel_alg_3c_supported:%d",
        params->cs_without_fae_supported, params->chsel_alg_3c_supported);
    BT_LOGD("pbr_from_rtt_sounding_seq_supported:%d, t_ip1_times_supported:%d",
        params->pbr_from_rtt_sounding_seq_supported, params->t_ip1_times_supported);
    BT_LOGD("t_ip2_times_supported:%d, t_fcs_times_supported:%d",
        params->t_ip2_times_supported, params->t_fcs_times_supported);
    BT_LOGD("t_pm_times_supported:%d, t_sw_time:%d, tx_snr_capability:%d",
        params->t_pm_times_supported, params->t_sw_time,
        params->tx_snr_capability);
}

static bt_srv_conn_le_cs_capabilities_t* zblue_convert_cs_capabilities_to_service(struct bt_conn_le_cs_capabilities* params)
{
    bt_srv_conn_le_cs_capabilities_t* capabilities;

    if (!params) {
        BT_LOGE("Invalid cs capabilities.");
        return NULL;
    }

    capabilities = (bt_srv_conn_le_cs_capabilities_t*)zalloc(sizeof(bt_srv_conn_le_cs_capabilities_t));

    if (!capabilities) {
        BT_LOGE("Failed to allocate memory for cs capabilities.");
        return NULL;
    }

    capabilities->num_config_supported = params->num_config_supported;
    capabilities->max_consecutive_procedures_supported = params->max_consecutive_procedures_supported;
    capabilities->num_antennas_supported = params->num_antennas_supported;
    capabilities->max_antenna_paths_supported = params->max_antenna_paths_supported;
    capabilities->initiator_supported = params->initiator_supported;
    capabilities->reflector_supported = params->reflector_supported;
    capabilities->mode_3_supported = params->mode_3_supported;

    switch (params->rtt_aa_only_precision) {
    case BT_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP:
        capabilities->rtt_aa_only_precision = CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP;
        break;
    case BT_CONN_LE_CS_RTT_AA_ONLY_10NS:
        capabilities->rtt_aa_only_precision = CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_10NS;
        break;
    case BT_CONN_LE_CS_RTT_AA_ONLY_150NS:
        capabilities->rtt_aa_only_precision = CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_150NS;
        break;
    default:
        BT_LOGE("Invalid rtt aa only precision: %d.", params->rtt_aa_only_precision);
        free(capabilities);
        return NULL;
    }

    switch (params->rtt_sounding_precision) {
    case BT_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP:
        capabilities->rtt_sounding_precision = CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP;
        break;
    case BT_CONN_LE_CS_RTT_SOUNDING_10NS:
        capabilities->rtt_sounding_precision = CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_10NS;
        break;
    case BT_CONN_LE_CS_RTT_SOUNDING_150NS:
        capabilities->rtt_sounding_precision = CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_150NS;
        break;
    default:
        BT_LOGE("Invalid rtt sounding precision: %d.", params->rtt_sounding_precision);
        free(capabilities);
        return NULL;
    }

    switch (params->rtt_random_payload_precision) {
    case BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP:
        capabilities->rtt_random_payload_precision = CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP;
        break;
    case BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS:
        capabilities->rtt_random_payload_precision = CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS;
        break;
    case BT_CONN_LE_CS_RTT_RANDOM_PAYLOAD_150NS:
        capabilities->rtt_random_payload_precision = CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_150NS;
        break;
    default:
        BT_LOGE("Invalid rtt random payload precision: %d.", params->rtt_random_payload_precision);
        free(capabilities);
        return NULL;
    }

    capabilities->rtt_aa_only_n = params->rtt_aa_only_n;
    capabilities->rtt_sounding_n = params->rtt_sounding_n;
    capabilities->rtt_random_payload_n = params->rtt_random_payload_n;
    capabilities->amplitude_based_nadm_sounding_supported = params->phase_based_nadm_sounding_supported;
    capabilities->amplitude_based_nadm_random_supported = params->phase_based_nadm_random_supported;
    capabilities->cs_sync_2m_phy_supported = params->cs_sync_2m_phy_supported;
    capabilities->cs_sync_2m_2bt_phy_supported = params->cs_sync_2m_2bt_phy_supported;
    capabilities->cs_without_fae_supported = params->cs_without_fae_supported;
    capabilities->chsel_alg_3c_supported = params->chsel_alg_3c_supported;
    capabilities->pbr_from_rtt_sounding_seq_supported = params->pbr_from_rtt_sounding_seq_supported;
    capabilities->t_ip1_times_supported = params->t_ip1_times_supported;
    capabilities->t_ip2_times_supported = params->t_ip2_times_supported;
    capabilities->t_fcs_times_supported = params->t_fcs_times_supported;
    capabilities->t_pm_times_supported = params->t_pm_times_supported;
    capabilities->t_sw_time = params->t_sw_time;
    capabilities->tx_snr_capability = params->tx_snr_capability;

    return capabilities;
}

void zblue_on_cs_capabilities_available(struct bt_conn* conn,
    struct bt_conn_le_cs_capabilities* params)
{
    cs_msg_t* msg = NULL;
    bt_srv_conn_le_cs_capabilities_t* capabilities = NULL;

    if (!conn) {
        BT_LOGE("Invalid connection handle.");
        return;
    }

    bt_address_t* addr = bt_conn_get_addr(conn);
    if (!addr) {
        BT_LOGE("Can't find address for conn:%p", conn);
        return;
    }

    msg = cs_msg_new(CAPABILITIES_RECEIVED_EVT, addr);
    if (!msg) {
        BT_LOGE("cs_msg_new failed.");
        return;
    }

    capabilities = zblue_convert_cs_capabilities_to_service(params);
    if (!capabilities) {
        BT_LOGE("zblue_convert_cs_capabilities_to_service failed.");
        cs_msg_destroy(msg);
        return;
    }

    msg->cs_data.data = (void*)capabilities;
    bt_sal_cs_event_callback(msg);

    BT_LOGD("CS capability exchange completed.");
    zblue_dump_cs_capabilities(params);
    return;
}

void zblue_on_cs_remote_fae_table_available(struct bt_conn* conn,
    struct bt_conn_le_cs_fae_table* params)
{
    return;
}

static void zblue_dump_cs_config(struct bt_conn_le_cs_config* config)
{
    if (!config) {
        return;
    }

    BT_LOGD("main_mode_type:%d, sub_mode_type:%d",
        config->main_mode_type, config->sub_mode_type);
    BT_LOGD("min_main_mode_steps:%d, max_main_mode_steps:%d",
        config->min_main_mode_steps, config->max_main_mode_steps);
    BT_LOGD("main_mode_repetition:%d, mode_0_steps:%d",
        config->main_mode_repetition, config->mode_0_steps);
    BT_LOGD("role:%d, rtt_type:%d, cs_sync_phy:%d",
        config->role, config->rtt_type, config->cs_sync_phy);
    BT_LOGD("channel_map_repetition:%d, channel_selection_type:%d",
        config->channel_map_repetition, config->channel_selection_type);
    BT_LOGD("ch3c_shape:%d, ch3c_jump:%d", config->ch3c_shape, config->ch3c_jump);
    BT_LOGD("t_ip1_time_us:%d, t_ip2_time_us:%d",
        config->t_ip1_time_us, config->t_ip2_time_us);
    BT_LOGD("t_fcs_time_us:%d, t_pm_time_us:%d", config->t_fcs_time_us, config->t_pm_time_us);
    BT_LOGD("channel_map:0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x.",
        config->channel_map[0], config->channel_map[1], config->channel_map[2],
        config->channel_map[3], config->channel_map[4], config->channel_map[5],
        config->channel_map[6], config->channel_map[7], config->channel_map[8],
        config->channel_map[9]);
}

static bt_srv_conn_le_cs_config_t* zblue_convert_cs_config_to_service(struct bt_conn_le_cs_config* config)
{
    bt_srv_conn_le_cs_config_t* srv_config;

    if (!config) {
        BT_LOGE("Invalid cs config.");
        return NULL;
    }

    srv_config = (bt_srv_conn_le_cs_config_t*)zalloc(sizeof(bt_srv_conn_le_cs_config_t));

    if (!srv_config) {
        BT_LOGE("Failed to allocate memory for cs config.");
        return NULL;
    }

    srv_config->id = config->id;
    switch (config->main_mode_type) {
    case BT_CONN_LE_CS_MAIN_MODE_1:
        srv_config->main_mode_type = CS_BT_SRV_CONN_LE_CS_MAIN_MODE_1;
        break;
    case BT_CONN_LE_CS_MAIN_MODE_2:
        srv_config->main_mode_type = CS_BT_SRV_CONN_LE_CS_MAIN_MODE_2;
        break;
    case BT_CONN_LE_CS_MAIN_MODE_3:
        srv_config->main_mode_type = CS_BT_SRV_CONN_LE_CS_MAIN_MODE_3;
        break;
    default:
        BT_LOGE("Invalid main mode type: %d.", config->main_mode_type);
        free(srv_config);
        return NULL;
    }

    switch (config->sub_mode_type) {
    case BT_CONN_LE_CS_SUB_MODE_1:
        srv_config->sub_mode_type = CS_BT_SRV_CONN_LE_CS_SUB_MODE_1;
        break;
    case BT_CONN_LE_CS_SUB_MODE_2:
        srv_config->sub_mode_type = CS_BT_SRV_CONN_LE_CS_SUB_MODE_2;
        break;
    case BT_CONN_LE_CS_SUB_MODE_3:
        srv_config->sub_mode_type = CS_BT_SRV_CONN_LE_CS_SUB_MODE_3;
        break;
    case BT_CONN_LE_CS_SUB_MODE_UNUSED:
        srv_config->sub_mode_type = CS_BT_SRV_CONN_LE_CS_SUB_MODE_UNUSED;
        break;
    default:
        BT_LOGE("Invalid sub mode type: %d.", config->sub_mode_type);
        free(srv_config);
        return NULL;
    }

    srv_config->min_main_mode_steps = config->min_main_mode_steps;
    srv_config->max_main_mode_steps = config->max_main_mode_steps;
    srv_config->main_mode_repetition = config->main_mode_repetition;
    srv_config->mode_0_steps = config->mode_0_steps;
    switch (config->role) {
    case BT_CONN_LE_CS_ROLE_INITIATOR:
        srv_config->role = CS_BT_SRV_CONN_LE_CS_ROLE_INITIATOR;
        break;
    case BT_CONN_LE_CS_ROLE_REFLECTOR:
        srv_config->role = CS_BT_SRV_CONN_LE_CS_ROLE_REFLECTOR;
        break;
    default:
        BT_LOGE("Invalid role: %d", config->role);
        free(srv_config);
        return NULL;
    }

    switch (config->rtt_type) {
    case BT_CONN_LE_CS_RTT_TYPE_AA_ONLY:
        srv_config->rtt_type = CS_BT_SRV_CONN_LE_CS_RTT_TYPE_AA_ONLY;
        break;
    case BT_CONN_LE_CS_RTT_TYPE_32_BIT_SOUNDING:
        srv_config->rtt_type = CS_BT_SRV_CONN_LE_CS_RTT_TYPE_32_BIT_SOUNDING;
        break;
    case BT_CONN_LE_CS_RTT_TYPE_96_BIT_SOUNDING:
        srv_config->rtt_type = CS_BT_SRV_CONN_LE_CS_RTT_TYPE_96_BIT_SOUNDING;
        break;
    case BT_CONN_LE_CS_RTT_TYPE_32_BIT_RANDOM:
        srv_config->rtt_type = CS_BT_SRV_CONN_LE_CS_RTT_TYPE_32_BIT_RANDOM;
        break;
    case BT_CONN_LE_CS_RTT_TYPE_64_BIT_RANDOM:
        srv_config->rtt_type = CS_BT_SRV_CONN_LE_CS_RTT_TYPE_64_BIT_RANDOM;
        break;
    case BT_CONN_LE_CS_RTT_TYPE_96_BIT_RANDOM:
        srv_config->rtt_type = CS_BT_SRV_CONN_LE_CS_RTT_TYPE_96_BIT_RANDOM;
        break;
    case BT_CONN_LE_CS_RTT_TYPE_128_BIT_RANDOM:
        srv_config->rtt_type = CS_BT_SRV_CONN_LE_CS_RTT_TYPE_128_BIT_RANDOM;
        break;
    default:
        BT_LOGE("Invalid rtt type: %d.", config->rtt_type);
        free(srv_config);
        return NULL;
    }

    switch (config->cs_sync_phy) {
    case BT_CONN_LE_CS_SYNC_1M_PHY:
        srv_config->cs_sync_phy = CS_BT_SRV_CONN_LE_CS_SYNC_1M_PHY;
        break;
    case BT_CONN_LE_CS_SYNC_2M_PHY:
        srv_config->cs_sync_phy = CS_BT_SRV_CONN_LE_CS_SYNC_2M_PHY;
        break;
    case BT_CONN_LE_CS_SYNC_2M_2BT_PHY:
        srv_config->cs_sync_phy = CS_BT_SRV_CONN_LE_CS_SYNC_2M_2BT_PHY;
        break;
    default:
        BT_LOGE("Invalid cs sync phy: %d.", config->cs_sync_phy);
        free(srv_config);
        return NULL;
    }

    srv_config->channel_map_repetition = config->channel_map_repetition;

    switch (config->channel_selection_type) {
    case BT_CONN_LE_CS_CHSEL_TYPE_3B:
        srv_config->channel_selection_type = CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3B;
        break;
    case BT_CONN_LE_CS_CHSEL_TYPE_3C:
        srv_config->channel_selection_type = CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3C;
        break;
    default:
        BT_LOGE("Invalid channel selection type: %d.", config->channel_selection_type);
        free(srv_config);
        return NULL;
    }

    switch (config->ch3c_shape) {
    case BT_CONN_LE_CS_CH3C_SHAPE_HAT:
        srv_config->ch3c_shape = CS_BT_SRV_CONN_LE_CS_CH3C_SHAPE_HAT;
        break;
    case BT_CONN_LE_CS_CH3C_SHAPE_X:
        srv_config->ch3c_shape = CS_BT_SRV_CONN_LE_CS_CH3C_SHAPE_X;
        break;
    default:
        BT_LOGE("Invalid ch3c shape: %d.", config->ch3c_shape);
        free(srv_config);
        return NULL;
    }

    srv_config->ch3c_jump = config->ch3c_jump;
    srv_config->t_ip1_time_us = config->t_ip1_time_us;
    srv_config->t_ip2_time_us = config->t_ip2_time_us;
    srv_config->t_fcs_time_us = config->t_fcs_time_us;
    srv_config->t_pm_time_us = config->t_pm_time_us;
    memcpy(srv_config->channel_map, config->channel_map, sizeof(config->channel_map));

    return srv_config;
}
void zblue_on_cs_config_created(struct bt_conn* conn, struct bt_conn_le_cs_config* config)
{
    cs_msg_t* msg = NULL;
    bt_srv_conn_le_cs_config_t* cs_config = NULL;
    bt_address_t* addr = bt_conn_get_addr(conn);
    if (!addr) {
        BT_LOGE("Can't find address for conn:%p", conn);
        return;
    }

    msg = cs_msg_new(CONFIG_DONE_EVT, addr);
    cs_config = zblue_convert_cs_config_to_service(config);
    msg->cs_data.data = (void*)cs_config;
    bt_sal_cs_event_callback(msg);

    BT_LOGD("CS config creation complete. ID: %d\n", config->id);
    zblue_dump_cs_config(config);
    return;
}

void zblue_on_cs_config_removed(struct bt_conn* conn, uint8_t config_id)
{
    return;
}

void zblue_on_cs_security_enabled(struct bt_conn* conn)
{
    cs_msg_t* msg = NULL;
    bt_address_t* addr = bt_conn_get_addr(conn);
    if (!addr) {
        BT_LOGE("Can't find address for conn:%p", conn);
        return;
    }

    msg = cs_msg_new(SECURITY_DONE_EVT, addr);
    bt_sal_cs_event_callback(msg);
    BT_LOGD("CS security enabled.\n");
    return;
}

static bt_srv_conn_le_cs_procedure_enable_complete_t* zblue_convert_procedure_enable_complete_struct_to_service(struct bt_conn_le_cs_procedure_enable_complete* params)
{
    bt_srv_conn_le_cs_procedure_enable_complete_t* procedure;

    if (!params) {
        BT_LOGE("Invalid procedure enable complete params.");
        return NULL;
    }

    procedure = (bt_srv_conn_le_cs_procedure_enable_complete_t*)zalloc(sizeof(bt_srv_conn_le_cs_procedure_enable_complete_t));

    if (!procedure) {
        BT_LOGE("Failed to allocate memory for procedure enable complete.");
        return NULL;
    }

    procedure->config_id = params->config_id;
    switch (params->state) {
    case BT_CONN_LE_CS_PROCEDURES_DISABLED:
        procedure->state = CS_BT_SRV_CONN_LE_CS_PROCEDURES_DISABLED;
        break;
    case BT_CONN_LE_CS_PROCEDURES_ENABLED:
        procedure->state = CS_BT_SRV_CONN_LE_CS_PROCEDURES_ENABLED;
        break;
    default:
        BT_LOGE("Invalid procedure state: %d.", params->state);
        free(procedure);
        return NULL;
    }

    switch (params->tone_antenna_config_selection) {
    case BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE:
        procedure->tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ZERO;
        break;
    case BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_TWO:
        procedure->tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE;
        break;
    case BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_THREE:
        procedure->tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_TWO;
        break;
    case BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FOUR:
        procedure->tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_THREE;
        break;
    case BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FIVE:
        procedure->tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FOUR;
        break;
    case BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SIX:
        procedure->tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FIVE;
        break;
    case BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SEVEN:
        procedure->tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SIX;
        break;
    case BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_EIGHT:
        procedure->tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SEVEN;
        break;
    default:
        BT_LOGE("Invalid tone antenna config selection: %d.", params->tone_antenna_config_selection);
        free(procedure);
        return NULL;
    }

    procedure->selected_tx_power = params->selected_tx_power;
    procedure->subevent_len = params->subevent_len;
    procedure->subevents_per_event = params->subevents_per_event;
    procedure->subevent_interval = params->subevent_interval;
    procedure->event_interval = params->event_interval;
    procedure->procedure_interval = params->procedure_interval;
    procedure->procedure_count = params->procedure_count;
    procedure->max_procedure_len = params->max_procedure_len;

    return procedure;
}

void zblue_on_cs_procedure_enabled(struct bt_conn* conn,
    struct bt_conn_le_cs_procedure_enable_complete* params)
{
    cs_msg_t* msg = NULL;
    bt_srv_conn_le_cs_procedure_enable_complete_t* procedure = NULL;
    bt_address_t* addr = bt_conn_get_addr(conn);
    if (!addr) {
        BT_LOGE("Can't find address for conn:%p", conn);
        return;
    }

    msg = cs_msg_new(PROCEDURE_DONE_EVT, addr);
    procedure = zblue_convert_procedure_enable_complete_struct_to_service(params);
    msg->cs_data.data = (void*)procedure;
    bt_sal_cs_event_callback(msg);

    if (params->state == 1) {
        BT_LOGD("CS procedures enabled.");
    } else {
        BT_LOGD("CS procedures disabled.");
    }

    BT_LOGD("config_id:%d, tone_antenna:%d, tx_power:%d, subevents_per_event:%d\n",
        params->config_id, params->tone_antenna_config_selection,
        params->selected_tx_power, params->subevents_per_event);
    BT_LOGD("subevent_interval:%d, event_interval:%d, procedure_interval:%d, procedure_count:%d, max_procedure_len:%d\n",
        params->subevent_interval, params->event_interval, params->procedure_interval,
        params->procedure_count, params->max_procedure_len);
    return;
}

static bt_srv_conn_le_cs_subevent_result_t* zblue_convert_subevent_result_struct_to_service(struct bt_conn_le_cs_subevent_result* params)
{
    bt_srv_conn_le_cs_subevent_result_t* subevent;

    if (!params) {
        BT_LOGE("Invalid subevent result params.");
        return NULL;
    }

    subevent = (bt_srv_conn_le_cs_subevent_result_t*)zalloc(sizeof(bt_srv_conn_le_cs_subevent_result_t));

    if (!subevent) {
        BT_LOGE("Failed to allocate memory for subevent result.");
        return NULL;
    }

    subevent->header.config_id = params->header.config_id;
    subevent->header.start_acl_conn_event_counter = params->header.start_acl_conn_event;
    subevent->header.procedure_counter = params->header.procedure_counter;
    subevent->header.frequency_compensation = params->header.frequency_compensation;
    subevent->header.reference_power_level = params->header.reference_power_level;
    switch (params->header.procedure_done_status) {
    case BT_CONN_LE_CS_PROCEDURE_COMPLETE:
        subevent->header.procedure_done_status = BT_LE_SRV_CS_PROCEDURE_COMPLETE;
        break;
    case BT_CONN_LE_CS_PROCEDURE_INCOMPLETE:
        subevent->header.procedure_done_status = BT_LE_SRV_CS_PROCEDURE_INCOMPLETE;
        break;
    case BT_CONN_LE_CS_PROCEDURE_ABORTED:
        subevent->header.procedure_done_status = BT_LE_SRV_CS_PROCEDURE_ABORTED;
        break;
    default:
        BT_LOGD("procedure_done_status is not valid: %d", params->header.procedure_done_status);
        free(subevent);
        return NULL;
    }

    switch (params->header.subevent_done_status) {
    case BT_CONN_LE_CS_SUBEVENT_COMPLETE:
        subevent->header.subevent_done_status = BT_LE_SRV_CS_SUBEVENT_COMPLETE;
        break;
    case BT_CONN_LE_CS_SUBEVENT_ABORTED:
        subevent->header.subevent_done_status = BT_LE_SRV_CS_SUBEVENT_ABORTED;
        break;
    default:
        BT_LOGD("subevent_done_status is not valid: %d", params->header.subevent_done_status);
        free(subevent);
        return NULL;
    }

    switch (params->header.procedure_abort_reason) {
    case BT_CONN_LE_CS_PROCEDURE_NOT_ABORTED:
        subevent->header.procedure_abort_reason = BT_LE_SRV_CS_PROCEDURE_NOT_ABORTED;
        break;
    case BT_CONN_LE_CS_PROCEDURE_ABORT_REQUESTED:
        subevent->header.procedure_abort_reason = BT_LE_SRV_CS_PROCEDURE_ABORT_REQUESTED;
        break;
    case BT_CONN_LE_CS_PROCEDURE_ABORT_TOO_FEW_CHANNELS:
        subevent->header.procedure_abort_reason = BT_LE_SRV_CS_PROCEDURE_ABORT_TOO_FEW_CHANNELS;
        break;
    case BT_CONN_LE_CS_PROCEDURE_ABORT_CHMAP_INSTANT_PASSED:
        subevent->header.procedure_abort_reason = BT_LE_SRV_CS_PROCEDURE_ABORT_CHMAP_INSTANT_PASSED;
        break;
    case BT_CONN_LE_CS_PROCEDURE_ABORT_UNSPECIFIED:
        subevent->header.procedure_abort_reason = BT_LE_SRV_CS_PROCEDURE_ABORT_UNSPECIFIED;
        break;
    default:
        BT_LOGD("procedure_abort_reason is not valid: %d", params->header.procedure_abort_reason);
        free(subevent);
        return NULL;
    }

    switch (params->header.subevent_abort_reason) {
    case BT_CONN_LE_CS_SUBEVENT_NOT_ABORTED:
        subevent->header.subevent_abort_reason = BT_LE_SRV_CS_SUBEVENT_NOT_ABORTED;
        break;
    case BT_CONN_LE_CS_SUBEVENT_ABORT_REQUESTED:
        subevent->header.subevent_abort_reason = BT_LE_SRV_CS_SUBEVENT_ABORT_REQUESTED;
        break;
    case BT_CONN_LE_CS_SUBEVENT_ABORT_NO_CS_SYNC:
        subevent->header.subevent_abort_reason = BT_LE_SRV_CS_SUBEVENT_ABORT_NO_CS_SYNC;
        break;
    case BT_CONN_LE_CS_SUBEVENT_ABORT_SCHED_CONFLICT:
        subevent->header.subevent_abort_reason = BT_LE_SRV_CS_SUBEVENT_ABORT_SCHED_CONFLICT;
        break;
    case BT_CONN_LE_CS_SUBEVENT_ABORT_UNSPECIFIED:
        subevent->header.subevent_abort_reason = BT_LE_SRV_CS_SUBEVENT_ABORT_UNSPECIFIED;
        break;
    default:
        BT_LOGD("subevent_abort_reason is not valid: %d", params->header.subevent_abort_reason);
        free(subevent);
        return NULL;
    }

    subevent->header.num_antenna_paths = params->header.num_antenna_paths;
    subevent->header.num_steps_reported = params->header.num_steps_reported;
    subevent->header.abort_step = params->header.abort_step;
    subevent->len = params->step_data_buf->len;
    subevent->step_data_buf = malloc(params->step_data_buf->len);
    if (!subevent->step_data_buf) {
        BT_LOGE("Failed to allocate memory for step_data_buf.");
        free(subevent);
        return NULL;
    }

    memcpy(subevent->step_data_buf, params->step_data_buf->data, subevent->len);
    return subevent;
}

void zblue_on_cs_subevent(struct bt_conn* conn, struct bt_conn_le_cs_subevent_result* result)
{
    cs_msg_t* msg = NULL;
    bt_srv_conn_le_cs_subevent_result_t* subevent = NULL;
    bt_address_t* addr = bt_conn_get_addr(conn);
    if (!addr) {
        BT_LOGE("Can't find address for conn:%p", conn);
        return;
    }

    if (!result->step_data_buf || !result->step_data_buf->len) {
        BT_LOGE("No data needs to be processed");
        return;
    }

    msg = cs_msg_new(SUBEVENT_RESULT_EVT, addr);
    subevent = zblue_convert_subevent_result_struct_to_service(result);
    msg->cs_data.data = (void*)subevent;
    bt_sal_cs_event_callback(msg);
}

#endif /* CONFIG_BLUETOOTH_LE_CS && CONFIG_BT_CHANNEL_SOUNDING */

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
    bt_address_t addr;
    ble_phy_type_t tx_mode;
    ble_phy_type_t rx_mode;
    bt_conn_info_t* slot;
    uint8_t role;

    tx_mode = le_phy_convert_from_stack(phy->tx_phy);
    rx_mode = le_phy_convert_from_stack(phy->rx_phy);

    BT_LOGD("%s, tx phy:%d, rx phy:%d", __func__, tx_mode, rx_mode);

    if (get_le_addr_from_conn(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_le_addr_from_conn failed", __func__);
        return;
    }

    adapter_on_le_phy_update(&addr, tx_mode, rx_mode, BT_STATUS_SUCCESS);

    slot = bt_conn_find(&addr, BT_TRANSPORT_BLE);
    if (!slot) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }
    role = slot->role;

#ifdef CONFIG_BLUETOOTH_GATT_SERVER
    if (role & GATT_ROLE_SERVER) {
        if_gatts_on_phy_updated(&addr, tx_mode, rx_mode, GATT_STATUS_SUCCESS);
    }
#endif

#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
    if (role & GATT_ROLE_CLIENT) {
        if_gattc_on_phy_updated(&addr, tx_mode, rx_mode, GATT_STATUS_SUCCESS);
    }
#endif
}
#endif /*CONFIG_BT_USER_PHY_UPDATE*/

static void zblue_on_pairing_complete_ctkd(struct bt_conn* conn, bool is_link_key)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    bt_address_t addr;
    const bt_addr_t* dst;

    if (is_link_key) {
        return;
    }

    BT_LOGD("%s", __func__);

    dst = bt_conn_get_dst_br(conn);
    if (!dst) {
        return;
    }

    memcpy(addr.addr, dst->val, sizeof(addr.addr));

    adapter_on_bond_state_changed(&addr, BOND_STATE_BONDED, BT_TRANSPORT_BLE, BT_STATUS_SUCCESS, true);
#endif
}

static void zblue_on_pairing_complete(struct bt_conn* conn, bool bonding_flag)
{
    bt_address_t addr;

    if (!bt_conn_get_dst(conn)) {
        return;
    }

    if (get_le_addr_from_conn(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_le_addr_from_conn failed", __func__);
        return;
    }

    BT_LOGD("%s bonding_flag: %s", __func__, bonding_flag ? "true" : "false");

    adapter_on_bond_state_changed(&addr, BOND_STATE_BONDED, BT_TRANSPORT_BLE, BT_STATUS_SUCCESS, false);
}

static void zblue_on_pairing_failed(struct bt_conn* conn, enum bt_security_err reason)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);

    if (get_le_addr_from_conn(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_le_addr_from_conn failed", __func__);
        return;
    }

    adapter_on_bond_state_changed(&addr, BOND_STATE_NONE, BT_TRANSPORT_BLE, BT_STATUS_AUTH_FAILURE, false);
    if (!adapter_get_pts_mode())
        bt_sal_le_disconnect(PRIMARY_ADAPTER, &addr);
}

static void zblue_on_bond_deleted(uint8_t id, const bt_addr_le_t* peer)
{
    bt_address_t addr;
    bool is_ctkd = false;
    bt_address_t* remote_addr;
    remote_device_le_properties_t* prop = zalloc(sizeof(remote_device_le_properties_t) * 0);

    BT_LOGD("%s", __func__);

    memcpy(&addr, peer->a.val, sizeof(addr.addr));
    remote_addr = adapter_get_le_remote_address(&addr, peer->type);
    if (!remote_addr) {
        BT_LOGE("%s, not found remote device", __func__);
        free(prop);
        return;
    }

    adapter_on_bond_state_changed(remote_addr, BOND_STATE_NONE, BT_TRANSPORT_BLE, BT_STATUS_SUCCESS, is_ctkd);
    adapter_on_le_bonded_device_update(prop, 0);
    free(prop);
}

static void zblue_register_callback(void)
{
    bt_conn_cb_register(&g_conn_cbs);
#ifdef CONFIG_BT_SMP
    bt_conn_le_auth_cb_register(&g_conn_auth_cbs);
    bt_conn_auth_info_cb_register(&g_conn_auth_info_cbs);
#endif
#ifdef CONFIG_SETTINGS_ZBLUE
    bt_setting_cb_register(&g_setting_cbs);
#endif
}

static void zblue_unregister_callback(void)
{
    bt_conn_cb_unregister(&g_conn_cbs);
#ifdef CONFIG_BT_SMP
    bt_conn_le_auth_cb_register(NULL);
    bt_conn_auth_info_cb_unregister(&g_conn_auth_info_cbs);
#endif
}

static void zblue_on_ready_cb(bt_controller_id_t dev_id, int err)
{
    UNUSED(dev_id);

    if (err) {
        BT_LOGD("zblue init failed (err %d)\n", err);
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);
        return;
    }

    zblue_register_callback();
    adapter_on_adapter_info_load();
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
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
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t get_le_addr_from_conn(struct bt_conn* conn, bt_address_t* addr)
{
    struct bt_conn_info info;
    bt_address_t *resolved_addr, *address;

    /* Check local connection info table first */
    address = bt_conn_get_addr(conn);
    if (address) {
        memcpy(addr, address, sizeof(bt_address_t));
        return BT_STATUS_SUCCESS;
    }

    /*
     * Fallback: g_conn_info may not be initialized yet if certain events
     * (e.g. MTU exchange) occur before the connected callback.
     * Use Zephyr's internal connection info as a fallback source.
     */
    if (bt_conn_get_info(conn, &info)) {
        BT_LOGE("%s: failed to get conn info", __func__);
        return BT_STATUS_FAIL;
    }

    if (info.type != BT_CONN_TYPE_LE || !info.le.dst) {
        BT_LOGE("%s: invalid LE connection or dst is null", __func__);
        return BT_STATUS_FAIL;
    }

    /* Attempt to resolve RPA to identity address */
    resolved_addr = adapter_get_le_remote_address((bt_address_t*)info.le.dst->a.val,
        info.le.dst->type);
    if (resolved_addr) {
        memcpy(addr, resolved_addr, sizeof(bt_address_t));
        BT_LOGD("%s: fallback to bt_conn_info and resolved RPA to identity address", __func__);
    } else {
        memcpy(addr, info.le.remote->a.val, sizeof(bt_address_t));
    }

    return BT_STATUS_SUCCESS;
}

struct bt_conn* get_le_conn_from_addr(bt_address_t* addr)
{
    bt_conn_info_t* info;

    info = bt_conn_find(addr, BT_TRANSPORT_BLE);

    return info ? info->conn : NULL;
}

bt_status_t bt_sal_le_init(const bt_vhal_interface* vhal)
{
#ifndef CONFIG_BLUETOOTH_BREDR_SUPPORT
    z_sys_init();
#endif

    return BT_STATUS_SUCCESS;
}

void bt_sal_le_cleanup(void)
{
}

bt_status_t bt_sal_le_enable(bt_controller_id_t id)
{
    if (bt_is_ready()) {
        adapter_on_adapter_state_changed(BLE_STACK_STATE_ON);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(bt_enable(zblue_on_ready_cb), 0);

    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(le_disable)(void* args)
{
    zblue_unregister_callback();
    bt_disable();
}

bt_status_t bt_sal_le_disable(bt_controller_id_t id)
{
    sal_adapter_req_t* req;

    if (!bt_is_ready()) {
        adapter_on_adapter_state_changed(BLE_STACK_STATE_OFF);
        return BT_STATUS_SUCCESS;
    }

    req = sal_adapter_req(id, NULL, STACK_CALL(le_disable));
    if (!req) {
        return BT_STATUS_NOMEM;
    }
    sal_send_req(req);
    adapter_on_adapter_state_changed(BLE_STACK_STATE_OFF);

    return BT_STATUS_SUCCESS;
}

#ifdef CONFIG_BT_SMP
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

    SAL_CHECK(bt_conn_auth_pairing_confirm(conn), 0);
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
#endif /* CONFIG_BT_SMP */

bt_status_t bt_sal_le_set_io_capability(bt_controller_id_t id, bt_io_capability_t cap)
{
#ifdef CONFIG_BT_SMP
    BT_LOGD("Set IO capability: %d", cap);

    memset(&g_conn_auth_cbs, 0, sizeof(g_conn_auth_cbs));
    bt_conn_le_auth_cb_register(NULL);

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
#ifndef CONFIG_HCI_AUTO_REPLY_IN_JUST_WORK
        g_conn_auth_cbs.passkey_confirm = zblue_on_auth_passkey_confirm;
#endif
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

    if (bt_conn_le_auth_cb_register(&g_conn_auth_cbs)) {
        BT_LOGE("Failed to register conn auth callbacks");
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
#else
    SAL_NOT_SUPPORT;
#endif
}

#ifdef CONFIG_BT_SMP
static void get_bonded_devices(const struct bt_bond_info* info, void* user_data)
{
    device_context_t* ctx = user_data;

    memcpy(&ctx->props->addr, &info->addr.a, sizeof(ctx->props->addr));
    ctx->props->addr_type = info->addr.type;
    ctx->props->device_type = BT_DEVICE_DEVTYPE_BLE;
    (*(ctx->cnt))++;
    ctx->props++;
}
#endif

bt_status_t bt_sal_le_get_bonded_devices(bt_controller_id_t id, remote_device_le_properties_t* props, uint16_t* prop_cnt)
{
#ifdef CONFIG_BT_SMP
    device_context_t ctx = { 0 };

    ctx.props = props;
    ctx.cnt = prop_cnt;

    bt_foreach_bond(BT_ID_DEFAULT, get_bonded_devices, &ctx);
    *prop_cnt = *ctx.cnt;

    return BT_STATUS_SUCCESS;
#else
    SAL_NOT_SUPPORT;
#endif
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

bt_status_t bt_sal_le_get_address(bt_controller_id_t id, bt_address_t* addr)
{
    UNUSED(id);
    bt_addr_le_t got = { 0 };
    size_t count = 1;

    SAL_CHECK_PARAM(addr);

    bt_id_get(&got, &count);
    bt_addr_set(addr, (uint8_t*)&got.a);

    SAL_ASSERT(got.type == BT_ADDR_LE_PUBLIC);
    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(le_set_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t le_addr;

    zblue_convert_le_addr(&req->addr, req->addr_type, &le_addr);

#ifdef CONFIG_SETTINGS_ZBLUE
    bt_settings_load(req->id, req->adpt.le_set_bond.id, req->adpt.le_set_bond.key, &le_addr);
    bt_settings_commit(req->id, req->adpt.le_set_bond.id, req->adpt.le_set_bond.key, &le_addr);
#endif
}

bt_status_t bt_sal_le_set_bonded_devices(bt_controller_id_t id, remote_device_le_properties_t* props, uint16_t prop_cnt)
{
    sal_adapter_req_t* req;
    bt_status_t status;

    for (int i = 0; i < prop_cnt; i++) {
        req = sal_adapter_req(id, (bt_address_t*)props->smp_key, STACK_CALL(le_set_bond));
        if (!req) {
            BT_LOGE("%s, req null", __func__);
            return BT_STATUS_NOMEM;
        }

        req->addr_type = props->smp_key[6];
        req->adpt.le_set_bond.id = BT_ID_DEFAULT;
        req->adpt.le_set_bond.key = "keys";

        status = sal_send_req(req);
        if (status) {
            BT_LOGE("%s send req error, ret: %d", __func__, status);
            return status;
        }

        props++;
    }

    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(security_connect)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;
    int err;

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    err = bt_conn_set_security(conn, g_security_level);
    if (err) {
        BT_LOGE("%s, start le encryption fail err:%d", __func__, err);
        return;
    }
}

static void STACK_CALL(conn_connect)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t address = { 0 };
    struct bt_conn* conn = NULL;
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

    if (get_le_conn_from_addr(addr)) {
        req = sal_adapter_req(id, addr, STACK_CALL(security_connect));
        if (!req) {
            BT_LOGE("%s, req null", __func__);
            return BT_STATUS_NOMEM;
        }

        return sal_send_req(req);
    }

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
        type = BT_ADDR_LE_PUBLIC;
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

static void STACK_CALL(set_bondable)(void* args)
{
    sal_adapter_req_t* req = args;

    bt_set_bondable_mc(req->id, req->adpt.bondable);
}

bt_status_t bt_sal_le_set_bondable(bt_controller_id_t id, bool enable)
{
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, NULL, STACK_CALL(set_bondable));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    req->adpt.bondable = enable;

    return sal_send_req(req);
}

#ifdef CONFIG_BT_SMP
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

    err = bt_conn_set_security(conn, g_security_level);
    if (err) {
        BT_LOGE("%s, bond fail err:%d", __func__, err);
        return;
    }
}
#endif

bt_status_t bt_sal_le_create_bond(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t type)
{
#ifdef CONFIG_BT_SMP
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(create_bond));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BT_SMP
static void STACK_CALL(set_security_level)(void* args)
{
    sal_adapter_req_t* req = args;

    g_security_level = req->adpt.security_level;
}
#endif

bt_status_t bt_sal_le_set_security_level(bt_controller_id_t id, uint8_t level)
{
#ifdef CONFIG_BT_SMP
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

#ifdef CONFIG_BT_SMP
static void zblue_convert_le_addr(bt_address_t* addr, ble_addr_type_t type, bt_addr_le_t* le_addr)
{
    le_addr->type = zblue_convert_addr_type(type);
    memcpy(le_addr->a.val, addr, sizeof(addr->addr));
}

static void STACK_CALL(remove_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_keys* keys;
    bt_addr_le_t le_addr;
    ble_addr_type_t type;
    int err;

    type = adapter_get_le_remote_address_type(&req->addr);
    if (type == BT_LE_ADDR_TYPE_UNKNOWN) {
        BT_LOGE("%s, unknown addr type", __func__);
        return;
    }

    zblue_convert_le_addr(&req->addr, type, &le_addr);
    keys = bt_keys_find_irk(BT_ID_DEFAULT, &le_addr);
    if (keys) {
        memcpy(&le_addr, &keys->addr, sizeof(bt_addr_le_t));
        err = bt_unpair(BT_ID_DEFAULT, &le_addr);
    } else {
        /* if peer device not support BT_PRIVACY, will not exchange IRK. */
        BT_LOGD("%s, not found irk", __func__);
        err = bt_unpair(BT_ID_DEFAULT, &le_addr);
    }

    if (err < 0) {
        BT_LOGE("%s, unpair fail err:%d", __func__, err);
        return;
    }
}
#endif

bt_status_t bt_sal_le_remove_bond(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BT_SMP
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(remove_bond));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BT_SMP
static void STACK_CALL(le_smp_reply)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    if (!req->adpt.smp.accept) {
        BT_LOGD("%s, reject", __func__);
        SAL_CHECK(bt_conn_auth_cancel(conn), 0);
        return;
    }

    switch (req->adpt.smp.type) {
    case PAIR_TYPE_PASSKEY_CONFIRMATION:
        SAL_CHECK(bt_conn_auth_passkey_confirm(conn), 0);
        break;
    case PAIR_TYPE_CONSENT:
        SAL_CHECK(bt_conn_auth_pairing_confirm(conn), 0);
        break;
    case PAIR_TYPE_PASSKEY_ENTRY:
        SAL_CHECK(bt_conn_auth_passkey_entry(conn, req->adpt.smp.passkey), 0);
        break;
    default:
        BT_LOGE("%s, unsupported type:%d", __func__, req->adpt.smp.type);
        return;
    }

    BT_LOGD("%s, accept", __func__);
}
#endif

bt_status_t bt_sal_le_smp_reply(bt_controller_id_t id, bt_address_t* addr, bool accept, bt_pair_type_t type, uint32_t passkey)
{
#ifdef CONFIG_BT_SMP
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(le_smp_reply));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.smp.accept = accept;
    req->adpt.smp.type = type;
    req->adpt.smp.passkey = passkey;

    return sal_send_req(req);
#else
    SAL_NOT_SUPPORT;
#endif
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
    bt_address_t id_addr;
    bt_addr_le_t addr;
    int err;

    if (adapter_get_remote_identity_address(&req->addr, &id_addr) == BT_STATUS_SUCCESS) {
        memcpy(&addr.a, &id_addr, sizeof(addr.a));
        /** TODO: consider random (static) identity address */
        addr.type = BT_LE_ADDR_TYPE_PUBLIC;
    } else {
        memcpy(&addr.a, &req->addr, sizeof(addr.a));
        addr.type = adapter_get_le_remote_address_type(&req->addr);
        BT_LOGD("%s, no public identity address", __func__);
    }

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
    bt_address_t id_addr;
    bt_addr_le_t addr;
    int err;

    if (adapter_get_remote_identity_address(&req->addr, &id_addr) == BT_STATUS_SUCCESS) {
        memcpy(&addr.a, &id_addr, sizeof(addr.a));
        addr.type = BT_LE_ADDR_TYPE_PUBLIC;
        /** TODO: consider random (static) identity address */
    } else {
        memcpy(&addr.a, &req->addr, sizeof(addr.a));
        addr.type = adapter_get_le_remote_address_type(&req->addr);
        BT_LOGD("%s, no public identity address", __func__);
    }

    err = bt_le_filter_accept_list_remove(&addr);
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

#ifdef CONFIG_BT_CLASSIC
static void STACK_CALL(enable_key_derivation)(void* args)
{
    sal_adapter_req_t* req = args;

    bt_smp_set_ctkd_mode_mc(req->id, req->adpt.ctkd_mode);
}
#endif

bt_status_t bt_sal_le_enable_key_derivation(bt_controller_id_t id, bool brkey_to_lekey, bool lekey_to_brkey)
{
#ifdef CONFIG_BT_CLASSIC
    sal_adapter_req_t* req;
    uint8_t ctkd_mode = 0;

    if (brkey_to_lekey) {
        ctkd_mode |= BT_SMP_CTKD_BR_TO_LE;
    }
    if (lekey_to_brkey) {
        ctkd_mode |= BT_SMP_CTKD_LE_TO_BR;
    }

    req = sal_adapter_req(id, NULL, STACK_CALL(enable_key_derivation));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    req->adpt.ctkd_mode = ctkd_mode;

    BT_LOGD("%s, brkey_to_lekey: %d, lekey_to_brkey: %d, mode: 0x%02x",
        __func__, brkey_to_lekey, lekey_to_brkey, ctkd_mode);

    return sal_send_req(req);
#else
    SAL_NOT_SUPPORT;
#endif
}

#endif /* CONFIG_BLUETOOTH_BLE_SUPPORT */
