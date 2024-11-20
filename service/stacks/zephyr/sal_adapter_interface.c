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
#include "power_manager.h"
#include "service_loop.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_err.h>

#include <settings/settings.h>

#include "sal_interface.h"

#include "utils/log.h"

#define BT_INVALID_CONNECTION_HANDLE 0xFFFF

#define STACK_CALL(func) zblue_##func

typedef void (*sal_func_t)(void* args);

typedef union {
    char name[BT_LOC_NAME_MAX_LEN];
    bt_io_capability_t cap;
    uint32_t cod;
    struct {
        bt_scan_mode_t scan_mode;
        bool bondable;
    } scanmode;
    uint32_t timeout;
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
    bt_pm_mode_t mode;
    bt_link_role_t role;
    bt_link_policy_t policy;
    struct {
        uint16_t central_frequency;
        uint16_t band_width;
        uint16_t number;
    } afh;
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
static void zblue_on_connect_req(struct bt_conn* conn, uint8_t link_type, uint8_t* cod);
static void zblue_on_connected(struct bt_conn* conn, uint8_t err);
static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason);
static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err);
#ifdef CONFIG_BT_REMOTE_INFO
static void zblue_on_remote_info_available(struct bt_conn* conn,
    struct bt_conn_remote_info* remote_info);
#endif
static void zblue_on_link_mode_changed(struct bt_conn* conn, uint8_t mode, uint16_t interval);
static void zblue_on_role_changed(struct bt_conn* conn, uint8_t role);
static void zblue_on_pairing_request(struct bt_conn* conn);
static void zblue_on_passkey_display(struct bt_conn* conn, unsigned int passkey);
static void zblue_on_passkey_entry(struct bt_conn* conn);
static void zblue_on_passkey_confirm(struct bt_conn* conn, unsigned int passkey);
static void zblue_on_cancel(struct bt_conn* conn);
static void zblue_on_pairing_confirm(struct bt_conn* conn);
static void zblue_on_pincode_entry(struct bt_conn* conn, bool highsec);
static void zblue_on_link_key_notify(struct bt_conn* conn, uint8_t* key, uint8_t key_type);
static void zblue_on_pairing_complete(struct bt_conn* conn, bool bonded);
static void zblue_on_pairing_failed(struct bt_conn* conn, enum bt_security_err reason);
static void zblue_on_bond_deleted(uint8_t id, const bt_addr_le_t* peer);

static struct bt_conn_cb g_conn_cbs = {
    .connect_req = zblue_on_connect_req,
    .connected = zblue_on_connected,
    .disconnected = zblue_on_disconnected,
    .security_changed = zblue_on_security_changed,
#ifdef CONFIG_BT_REMOTE_INFO
    .remote_info_available = zblue_on_remote_info_available,
#endif
    .link_mode_changed = zblue_on_link_mode_changed,
    .role_changed = zblue_on_role_changed,
};

static struct bt_conn_auth_info_cb g_conn_auth_info_cbs = {
    .link_key_notify = zblue_on_link_key_notify,
    .pairing_complete = zblue_on_pairing_complete,
    .pairing_failed = zblue_on_pairing_failed,
    .bond_deleted = zblue_on_bond_deleted,
};

static struct bt_conn_auth_cb g_conn_auth_cbs = {
    .pairing_request = zblue_on_pairing_request,
    .cancel = zblue_on_cancel,
    .pairing_confirm = zblue_on_pairing_confirm,
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

    if (!service_loop_work((void*)req, sal_invoke_async, NULL))
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

static void zblue_conn_get_addr(struct bt_conn* conn, bt_address_t* addr)
{
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);
    bt_addr_set(addr, info.br.dst->val);
}

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

static void zblue_on_connected(struct bt_conn* conn, uint8_t err)
{
    acl_state_param_t state = {
        .transport = BT_TRANSPORT_BREDR,
        .connection_state = CONNECTION_STATE_CONNECTED
    };

    zblue_conn_get_addr(conn, &state.addr);
    adapter_on_connection_state_changed(&state);
}

static void zblue_on_disconnected(struct bt_conn* conn, uint8_t reason)
{
    acl_state_param_t state = {
        .transport = BT_TRANSPORT_BREDR,
        .connection_state = CONNECTION_STATE_DISCONNECTED
    };

    zblue_conn_get_addr(conn, &state.addr);
    adapter_on_connection_state_changed(&state);
}

static void zblue_on_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err)
{
    bt_address_t addr;
    bool encrypted = false;

    zblue_conn_get_addr(conn, &addr);

    if (err) {
        adapter_on_bond_state_changed(&addr, BOND_STATE_NONE, BT_TRANSPORT_BREDR, BT_STATUS_FAIL, false);
    }

    if (level >= BT_SECURITY_L2 && err == BT_SECURITY_ERR_SUCCESS) {
        encrypted = true;
    }

    adapter_on_encryption_state_changed(&addr, encrypted, BT_TRANSPORT_BREDR);
}

#ifdef CONFIG_BT_REMOTE_INFO
static void zblue_on_remote_info_available(struct bt_conn* conn,
    struct bt_conn_remote_info* remote_info)
{
}
#endif

static void zblue_on_link_mode_changed(struct bt_conn* conn, uint8_t mode, uint16_t interval)
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

static void zblue_on_pairing_request(struct bt_conn* conn)
{
    bt_address_t addr;

    zblue_conn_get_addr(conn, &addr);
    adapter_on_pairing_request(&addr, false, true);
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

static void zblue_on_link_key_notify(struct bt_conn* conn, uint8_t* key, uint8_t key_type)
{
    bt_address_t addr;

    zblue_conn_get_addr(conn, &addr);
    adapter_on_link_key_update(&addr, key, key_type);
    adapter_on_bond_state_changed(&addr, BOND_STATE_BONDED, BT_TRANSPORT_BREDR, BT_STATUS_SUCCESS, false);
}

static void zblue_on_pairing_complete(struct bt_conn* conn, bool bonded)
{
    bt_address_t addr;
    bond_state_t state;

    if (bonded) {
        state = BOND_STATE_BONDED;
        /* Start timer, waiting for linkkey notify */
    } else {
        state = BOND_STATE_NONE;
        zblue_conn_get_addr(conn, &addr);
        adapter_on_bond_state_changed(&addr, state, BT_TRANSPORT_BREDR, BT_STATUS_AUTH_FAILURE, false);
    }
}

static void zblue_on_pairing_failed(struct bt_conn* conn, enum bt_security_err reason)
{
    bt_address_t addr;

    zblue_conn_get_addr(conn, &addr);
    adapter_on_bond_state_changed(&addr, BOND_STATE_NONE, BT_TRANSPORT_BREDR, BT_STATUS_AUTH_FAILURE, false);
    bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static void zblue_on_bond_deleted(uint8_t id, const bt_addr_le_t* peer)
{
    bt_address_t addr;

    if (id == 0 && peer->type == BT_ADDR_LE_PUBLIC) {
        bt_addr_set(&addr, peer->a.val);
        adapter_on_link_key_removed(&addr, BT_STATUS_SUCCESS);
    } /* else: Ignore it*/
}

static void zblue_on_ready_cb(int err)
{
    uint8_t state = BT_BREDR_STACK_STATE_OFF;

    if (IS_ENABLED(CONFIG_SETTINGS)) {
        settings_load();
    }

    if (err) {
        BT_LOGD("zblue init failed (err %d)\n", err);
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);
        return;
    }

    bt_conn_set_auto(false);

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

/* service adapter layer for BREDR */
bt_status_t bt_sal_init(const bt_vhal_interface* vhal)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    zblue_main();

    bt_conn_cb_register(&g_conn_cbs);
    bt_conn_auth_cb_register(&g_conn_auth_cbs);
    bt_conn_auth_info_cb_register(&g_conn_auth_info_cbs);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    bt_conn_auth_cb_register(NULL);
    bt_conn_auth_info_cb_unregister(&g_conn_auth_info_cbs);
#endif
}

/* Adapter power */
bt_status_t bt_sal_enable(bt_controller_id_t id)
{
    UNUSED(id);

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (bt_is_ready()) {
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_ON);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(bt_enable(zblue_on_ready_cb), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_disable(bt_controller_id_t id)
{
    UNUSED(id);

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (!bt_is_ready()) {
        adapter_on_adapter_state_changed(BT_BREDR_STACK_STATE_OFF);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(bt_disable(), 0);
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
        break;
    case BT_IO_CAPABILITY_DISPLAYYESNO:
        g_conn_auth_cbs.passkey_display = zblue_on_passkey_display;
        g_conn_auth_cbs.passkey_entry = NULL;
        g_conn_auth_cbs.passkey_confirm = zblue_on_passkey_confirm;
        break;
    case BT_IO_CAPABILITY_KEYBOARDONLY:
        g_conn_auth_cbs.passkey_display = NULL;
        g_conn_auth_cbs.passkey_entry = zblue_on_passkey_entry;
        g_conn_auth_cbs.passkey_confirm = NULL;
        break;
    case BT_IO_CAPABILITY_KEYBOARDDISPLAY:
        g_conn_auth_cbs.passkey_display = zblue_on_passkey_display;
        g_conn_auth_cbs.passkey_entry = zblue_on_passkey_entry;
        g_conn_auth_cbs.passkey_confirm = zblue_on_passkey_confirm;
        break;
    case BT_IO_CAPABILITY_NOINPUTNOOUTPUT:
    default:
        g_conn_auth_cbs.passkey_display = NULL;
        g_conn_auth_cbs.passkey_entry = NULL;
        g_conn_auth_cbs.passkey_confirm = NULL;
        break;
    }

    bt_conn_auth_cb_register(NULL);
    bt_conn_auth_cb_register(&g_conn_auth_cbs);

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

    BT_LOGD("%s: %lu", __func__, req->adpt.cod);
    SAL_CHECK(bt_set_class_of_device(req->adpt.cod), 0);
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

    int ret = bt_br_set_connectable(pscan);
    if (ret != 0 && ret != -EALREADY) {
        BT_LOGE("%s set connectable failed:%d", __func__, ret);
        return;
    }

    if (iscan) {
        ret = bt_br_set_discoverable(iscan);
        if (ret != 0 && ret != -EALREADY) {
            BT_LOGE("%s set discoverable failed:%d", __func__, ret);
            return;
        }
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

static bool zblue_inquiry_eir_name(uint8_t* eir, int len, char* name)
{
    while (len) {
        if (len < 2) {
            false;
        }

        /* Look for early termination */
        if (!eir[0]) {
            false;
        }

        /* Check if field length is correct */
        if (eir[0] > len - 1) {
            false;
        }

        switch (eir[1]) {
        case BT_DATA_NAME_SHORTENED:
        case BT_DATA_NAME_COMPLETE:
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

static void zblue_on_discovery_complete_cb(struct bt_br_discovery_result* results,
    size_t count)
{
    bt_discovery_result_t device;

    if (results == NULL || count == 0) {
        adapter_on_discovery_state_changed(BT_DISCOVERY_STOPPED);
        return;
    }

    for (size_t i = 0; i < count; i++) {
        memcpy(device.addr.addr, &results[i].addr, 6);
        device.rssi = results[i].rssi;
        device.cod = (results[i].cod[2] << 16) | (results[i].cod[1] << 8) | results[i].cod[0];
        zblue_inquiry_eir_name(results[i].eir, sizeof(results[i].eir), device.name);

        /* report discovery result to service */
        adapter_on_device_found(&device);
    }
}

static void STACK_CALL(start_discovery)(void* args)
{
#define DISCOVERY_DEVICE_MAX 30
    sal_adapter_req_t* req = args;
    struct bt_br_discovery_param param;
    static struct bt_br_discovery_result g_discovery_results[DISCOVERY_DEVICE_MAX];

    /* unlimited number of responses. */
    param.limited = false;
    param.length = req->adpt.timeout;

    if (bt_br_discovery_start(&param, g_discovery_results,
            SAL_ARRAY_SIZE(g_discovery_results), zblue_on_discovery_complete_cb)
        == 0)
        adapter_on_discovery_state_changed(BT_DISCOVERY_STARTED);
}
#endif

bt_status_t bt_sal_start_discovery(bt_controller_id_t id, uint32_t timeout)
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

    req->adpt.timeout = timeout;

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
    bt_conn_set_auto(enable);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(sco_connection_reply)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_sco((bt_addr_t*)&req->addr);

    if (req->adpt.accept) {
        SAL_CHECK(bt_conn_accept_sco_conn(conn), 0);
    } else {
        SAL_CHECK(bt_conn_reject_sco_conn(conn, BT_HCI_ERR_INSUFFICIENT_RESOURCES), 0);
    }

    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_sco_connection_reply(bt_controller_id_t id, bt_address_t* addr, bool accept)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(sco_connection_reply));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.accept = accept;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(acl_connection_reply)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (req->adpt.accept) {
        SAL_CHECK(bt_conn_accept_acl_conn(conn), 0);
    } else {
        SAL_CHECK(bt_conn_reject_acl_conn(conn, BT_HCI_ERR_INSUFFICIENT_RESOURCES), 0);
    }

    bt_conn_unref(conn);
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

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(pair_reply)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (req->adpt.reason == BT_HCI_ERR_SUCCESS) {
        SAL_CHECK(bt_conn_auth_pairing_accept(conn), 0);
    } else {
        SAL_CHECK(bt_conn_auth_pairing_reject(conn, req->adpt.reason), 0);
    }
}
#endif

bt_status_t bt_sal_pair_reply(bt_controller_id_t id, bt_address_t* addr, uint8_t reason)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(pair_reply));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.reason = reason;

    return sal_send_req(req);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(ssp_reply)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

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
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    struct bt_conn_info info;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    bt_conn_get_info(conn, &info);
    bt_conn_unref(conn);

    return info.handle;
#else
    return BT_INVALID_CONNECTION_HANDLE;
#endif
}

uint16_t bt_sal_get_sco_connection_handle(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    struct bt_conn_info info;
    struct bt_conn* conn = bt_conn_lookup_addr_sco((bt_addr_t*)addr);

    bt_conn_get_info(conn, &info);
    bt_conn_unref(conn);

    return info.handle;
#else
    return BT_INVALID_CONNECTION_HANDLE;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(connect)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;

    conn = bt_conn_create_br((const bt_addr_t*)&req->addr, BT_BR_CONN_PARAM_DEFAULT);
    if (!conn) {
        BT_LOGW("bt_conn_create_br Connection failed");
        return;
    }

    bt_conn_unref(conn);
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

    SAL_CHECK(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN), 0);
    bt_conn_unref(conn);
}
#endif

bt_status_t bt_sal_disconnect(bt_controller_id_t id, bt_address_t* addr, uint8_t reason)
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

    conn = bt_conn_pair((bt_addr_t*)&req->addr, BT_SECURITY_L3);
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
static void STACK_CALL(cancel_bond)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    SAL_CHECK(bt_conn_auth_cancel(conn), 0);
    SAL_CHECK(bt_br_unpair((bt_addr_t*)&req->addr), 0);
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

    req = sal_adapter_req(id, addr, STACK_CALL(remove_bond));
    if (!req)
        return BT_STATUS_NOMEM;

    req->adpt.bond.transport = transport;

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

bt_status_t bt_sal_set_bonded_devices(bt_controller_id_t id, remote_device_properties_t* props, int cnt)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    UNUSED(id);
    struct bt_bond_info_br bondinfo;

    for (int i = 0; i < cnt; i++) {
        memcpy(&bondinfo.addr, &props->addr, 6);
        memcpy(&bondinfo.key, &props->link_key, 16);
        bondinfo.key_type = props->link_key_type;
        if (bt_br_set_bond_info(&bondinfo))
            break;
    }

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void get_bonded_devices(const struct bt_bond_info_br* info,
    void* user_data)
{
    struct device_context* ctx = user_data;

    if (ctx->got < ctx->cnt) {
        memcpy(&ctx->props->addr, &info->addr, 6);
        memcpy(&ctx->props->link_key, &info->key, 16);
        ctx->props->link_key_type = info->key_type;
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

    bt_br_foreach_bond(get_bonded_devices, &ctx);
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
static void STACK_CALL(set_power_mode)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_pm_mode_t* pm = &req->adpt.mode;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

    if (pm->mode == BT_LINK_MODE_ACTIVE) {
        SAL_CHECK(bt_conn_check_exit_sniff(conn), 0);
    } else {
        SAL_CHECK(bt_conn_check_enter_sniff(conn, pm->min, pm->max, pm->attempt, pm->timeout), 0);
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

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
static void STACK_CALL(set_link_role)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)&req->addr);

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

/* VSC */
bt_status_t bt_sal_send_hci_command(bt_controller_id_t id, uint8_t ogf, uint16_t ocf, uint8_t length, uint8_t* buf,
    bt_hci_event_callback_t cb, void* context)
{
    UNUSED(id);
    SAL_NOT_SUPPORT;
}
