/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#define LOG_TAG "cs_rap_gattc"

#include <stdlib.h>
#include <string.h>

#include "bt_addr.h"
#include "bt_gatt_defs.h"
#include "bt_list.h"
#include "gattc_service.h"
#include "cs_rap_gattc.h"
#include "cs_msg.h"
#include "cs_service.h"
#include "sal_le_scan_interface.h"
#include "service_manager.h"
#include "utils/log.h"

/**
 * @brief CS RAP GATTC Connection structure
 */
typedef struct {
    struct list_node node;
    bt_address_t addr;
    void* conn_handle;
    cs_rap_gattc_handles_t handles;
    bool discovering;
    bool handles_valid;
    uint16_t ras_start_handle;
    uint16_t ras_end_handle;
} cs_rap_gattc_conn_t;

/**
 * @brief CS RAP GATTC Environment
 */
typedef struct {
    struct list_node conn_list;
    const cs_rap_gattc_callbacks_t* callbacks;
} cs_rap_gattc_env_t;

static cs_rap_gattc_env_t* g_cs_rap_gattc = NULL;

/* Forward declarations */
static void cs_rap_gattc_connection_cb(void* conn_handle, bt_address_t* addr);
static void cs_rap_gattc_disconnection_cb(void* conn_handle, bt_address_t* addr);
static void cs_rap_gattc_discover_cb(void* conn_handle, gatt_status_t status,
    bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle);
static void cs_rap_gattc_read_cb(void* conn_handle, gatt_status_t status,
    uint16_t handle, uint8_t* value, uint16_t length);
static void cs_rap_gattc_write_cb(void* conn_handle, gatt_status_t status,
    uint16_t handle);
static void cs_rap_gattc_subscribe_cb(void* conn_handle, gatt_status_t status,
    uint16_t handle, bool enable);
static void cs_rap_gattc_notify_cb(void* conn_handle, uint16_t handle,
    uint8_t* value, uint16_t length);
static void cs_rap_gattc_mtu_cb(void* conn_handle, gatt_status_t status, uint32_t mtu);

static gattc_callbacks_t gattc_cbs = {
    .size = sizeof(gattc_callbacks_t),
    .on_connected = cs_rap_gattc_connection_cb,
    .on_disconnected = cs_rap_gattc_disconnection_cb,
    .on_discovered = cs_rap_gattc_discover_cb,
    .on_read = cs_rap_gattc_read_cb,
    .on_written = cs_rap_gattc_write_cb,
    .on_subscribed = cs_rap_gattc_subscribe_cb,
    .on_notified = cs_rap_gattc_notify_cb,
    .on_mtu_updated = cs_rap_gattc_mtu_cb,
    .on_phy_read = NULL,
    .on_phy_updated = NULL,
    .on_rssi_read = NULL,
    .on_conn_param_updated = NULL,
};

/* Helper to get GATTC interface */
static gattc_interface_t* get_gattc_interface(void)
{
    return (gattc_interface_t*)service_manager_get_profile(PROFILE_GATTC);
}

/* Helper functions */
static cs_rap_gattc_conn_t* find_conn_by_addr(bt_address_t* addr)
{
    if (!g_cs_rap_gattc || !addr) {
        return NULL;
    }

    struct list_node* node;
    list_for_every(&g_cs_rap_gattc->conn_list, node)
    {
        cs_rap_gattc_conn_t* conn = (cs_rap_gattc_conn_t*)node;
        if (memcmp(&conn->addr, addr, sizeof(bt_address_t)) == 0) {
            return conn;
        }
    }

    return NULL;
}

static cs_rap_gattc_conn_t* find_conn_by_handle(void* conn_handle)
{
    if (!g_cs_rap_gattc || !conn_handle) {
        return NULL;
    }

    struct list_node* node;
    list_for_every(&g_cs_rap_gattc->conn_list, node)
    {
        cs_rap_gattc_conn_t* conn = (cs_rap_gattc_conn_t*)node;
        if (conn->conn_handle == conn_handle) {
            return conn;
        }
    }

    return NULL;
}

static cs_rap_gattc_conn_t* create_conn(bt_address_t* addr)
{
    cs_rap_gattc_conn_t* conn = (cs_rap_gattc_conn_t*)malloc(sizeof(cs_rap_gattc_conn_t));
    if (!conn) {
        BT_LOGE("Failed to allocate connection");
        return NULL;
    }

    memset(conn, 0, sizeof(cs_rap_gattc_conn_t));
    memcpy(&conn->addr, addr, sizeof(bt_address_t));
    list_add_tail(&g_cs_rap_gattc->conn_list, &conn->node);

    return conn;
}

static void remove_conn(cs_rap_gattc_conn_t* conn)
{
    if (conn) {
        list_delete(&conn->node);
        free(conn);
    }
}

/* GATTC Callbacks */
static void cs_rap_gattc_connection_cb(void* conn_handle, bt_address_t* addr)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGW("CS RAP GATTC connected, addr=%s, conn_handle=%p", addr_str, conn_handle);

    cs_rap_gattc_conn_t* conn = find_conn_by_handle(conn_handle);
    if (!conn) {
        conn = find_conn_by_addr(addr);
        if (!conn) {
            conn = create_conn(addr);
            BT_LOGW("CS RAP GATTC created new conn for %s", addr_str);
        }
    }

    if (conn) {
        conn->conn_handle = conn_handle;
    }

    /* NOTE: Do NOT dispatch CONNECTED_EVT here.
     * The state machine already transitioned to Connected before calling
     * cs_rap_gattc_connect(). Sending another CONNECTED_EVT would cause
     * the state machine to re-enter Connected and disrupt the CS flow.
     */

    if (g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_connected) {
        g_cs_rap_gattc->callbacks->on_connected(addr, 0);
    }

    /* BLE ACL connection established. Start RAS service discovery first.
     * CONNECTED_EVT will be dispatched after discovery completes. */
    BT_LOGW("CS RAP GATTC connected: starting auto-discover for %s", addr_str);
    bt_status_t ret = cs_rap_gattc_discover(addr);
    BT_LOGW("CS RAP GATTC auto-discover returned %d", ret);
}

static void cs_rap_gattc_disconnection_cb(void* conn_handle, bt_address_t* addr)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("CS RAP GATTC disconnected, addr=%s", addr_str);

    cs_rap_gattc_conn_t* conn = find_conn_by_handle(conn_handle);
    if (conn) {
        remove_conn(conn);
    }

    /* Dispatch DISCONNECTED_EVT to CS state machine */
    cs_msg_t* msg = cs_msg_new(DISCONNECTED_EVT, addr);
    bt_sal_cs_event_callback(msg);

    if (g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_disconnected) {
        g_cs_rap_gattc->callbacks->on_disconnected(addr, 0);
    }
}

static void cs_rap_gattc_parse_characteristic(cs_rap_gattc_conn_t* conn,
    uint16_t handle, uint16_t uuid16)
{
    switch (uuid16) {
    case BT_UUID_RANGE_FEAT_VAL:
        conn->handles.features_handle = handle;
        BT_LOGD("Found RAS Features: handle=%d", handle);
        break;
    case BT_UUID_RANGE_RTT_DT_VAL:
        conn->handles.real_time_data_handle = handle;
        BT_LOGD("Found RAS Real-time Data: handle=%d", handle);
        break;
    case BT_UUID_RANGE_ON_DEM_DT_VAL:
        conn->handles.on_demand_data_handle = handle;
        BT_LOGD("Found RAS On-demand Data: handle=%d", handle);
        break;
    case BT_UUID_RANGE_RAS_CTR_POINT_VAL:
        conn->handles.control_point_handle = handle;
        BT_LOGD("Found RAS Control Point: handle=%d", handle);
        break;
    case BT_UUID_RANGE_DT_RD_VAL:
        conn->handles.data_ready_handle = handle;
        BT_LOGD("Found RAS Data Ready: handle=%d", handle);
        break;
    case BT_UUID_RANGE_DT_OV_WR_VAL:
        conn->handles.data_overwritten_handle = handle;
        BT_LOGD("Found RAS Data Overwritten: handle=%d", handle);
        break;
    case BT_UUID_RANGE_COUNTER_VAL:
        conn->handles.ranging_counter_handle = handle;
        BT_LOGD("Found RAS Ranging Counter: handle=%d", handle);
        break;
    default:
        break;
    }
}

static void cs_rap_gattc_parse_ras_attributes(cs_rap_gattc_conn_t* conn)
{
    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc || !conn->ras_start_handle || !conn->ras_end_handle) {
        return;
    }

    gatt_attr_desc_t attr_desc;

    for (uint16_t h = conn->ras_start_handle; h <= conn->ras_end_handle; h++) {
        if (gattc->get_attribute_by_handle(conn->conn_handle, h, &attr_desc) != BT_STATUS_SUCCESS) {
            continue;
        }

        if (attr_desc.uuid.type != BT_UUID16_TYPE) {
            continue;
        }

        if (attr_desc.type == GATT_CHARACTERISTIC) {
            cs_rap_gattc_parse_characteristic(conn, h, attr_desc.uuid.val.u16);
        }
    }

    conn->handles_valid = (conn->handles.features_handle != 0);
}

static void cs_rap_gattc_discover_cb(void* conn_handle, gatt_status_t status,
    bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle)
{
    BT_LOGD("discover_cb: status=%d, start=%d, end=%d, uuid=%p, conn_handle=%p",
        status, start_handle, end_handle, uuid, conn_handle);

    cs_rap_gattc_conn_t* conn = find_conn_by_handle(conn_handle);
    if (!conn) {
        BT_LOGE("discover_cb: conn not found for handle=%p", conn_handle);
        return;
    }

    /* Discovery complete when uuid is NULL or type is 0 */
    if (!uuid || !uuid->type) {
        BT_LOGD("discover_cb: completed, ras_start=%d, ras_end=%d",
            conn->ras_start_handle, conn->ras_end_handle);
        conn->discovering = false;

        cs_rap_gattc_parse_ras_attributes(conn);

        if (g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_discover_complete) {
            g_cs_rap_gattc->callbacks->on_discover_complete(&conn->addr, conn->handles_valid, &conn->handles);
        }

        if (conn->handles_valid && conn->handles.real_time_data_handle) {
            BT_LOGD("Post-encryption discovery done, auto-enabling real-time data notification");
            cs_rap_gattc_enable_real_time_data(&conn->addr, true);
        }

        BT_LOGD("discover_cb: dispatching CONNECTED_EVT to state machine");
        cs_msg_t* evt_msg = cs_msg_new(CONNECTED_EVT, &conn->addr);
        bt_sal_cs_event_callback(evt_msg);

        return;
    }

    /* Extract 16-bit UUID value, handling both UUID16 and UUID128 forms */
    uint16_t uuid16_val = 0;
    bool uuid_matched = false;
    if (uuid->type == BT_UUID16_TYPE) {
        uuid16_val = uuid->val.u16;
        uuid_matched = true;
    } else if (uuid->type == BT_UUID128_TYPE) {
        /* Bluetooth base UUID: xxxxxxxx-0000-1000-8000-00805F9B34FB
         * The 16-bit value is at bytes [12..13] (little-endian) */
        uuid16_val = uuid->val.u128[12] | (uuid->val.u128[13] << 8);
        uuid_matched = true;
    }

    /* Check if this is RAS service */
    if (status == GATT_STATUS_SUCCESS && uuid_matched &&
        uuid16_val == BT_UUID_RANGING_VAL) {
        BT_LOGD("Found RAS service: handles %d-%d", start_handle, end_handle);
        conn->ras_start_handle = start_handle;
        conn->ras_end_handle = end_handle;
    } else {
        char uuid_str[BT_UUID_STR_LENGTH] = { 0 };
        bt_uuid_to_string(uuid, uuid_str, sizeof(uuid_str));
        BT_LOGD("discover_cb: service uuid=%s NOT RAS, status=%d", uuid_str, status);
    }
}

static void cs_rap_gattc_read_cb(void* conn_handle, gatt_status_t status,
    uint16_t handle, uint8_t* value, uint16_t length)
{
    BT_LOGD("CS RAP GATTC read: status=%d, handle=%d, len=%d", status, handle, length);

    cs_rap_gattc_conn_t* conn = find_conn_by_handle(conn_handle);
    if (!conn) {
        return;
    }

    if (status == GATT_STATUS_SUCCESS && handle == conn->handles.features_handle) {
        if (length >= 4 && g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_features_read) {
            uint32_t features = value[0] | (value[1] << 8) | (value[2] << 16) | (value[3] << 24);
            g_cs_rap_gattc->callbacks->on_features_read(&conn->addr, features);
        }
    }
}

static void cs_rap_gattc_write_cb(void* conn_handle, gatt_status_t status, uint16_t handle)
{
    BT_LOGD("CS RAP GATTC write: status=%d, handle=%d", status, handle);

    cs_rap_gattc_conn_t* conn = find_conn_by_handle(conn_handle);
    if (!conn) {
        return;
    }

    if (g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_write_complete) {
        g_cs_rap_gattc->callbacks->on_write_complete(&conn->addr, handle, status);
    }
}

static void cs_rap_gattc_subscribe_cb(void* conn_handle, gatt_status_t status,
    uint16_t handle, bool enable)
{
    BT_LOGD("CS RAP GATTC subscribe: status=%d, handle=%d, enable=%d", status, handle, enable);
}

static void cs_rap_gattc_notify_cb(void* conn_handle, uint16_t handle,
    uint8_t* value, uint16_t length)
{
    BT_LOGW("CS RAP GATTC notify: handle=%d, len=%d, conn_handle=%p", handle, length, conn_handle);

    cs_rap_gattc_conn_t* conn = find_conn_by_handle(conn_handle);
    if (!conn) {
        /* Fallback: notify may come on old conn_handle after reconnect.
         * Try to find any valid conn in the list. */
        if (g_cs_rap_gattc && !list_is_empty(&g_cs_rap_gattc->conn_list)) {
            conn = (cs_rap_gattc_conn_t*)list_peek_head(&g_cs_rap_gattc->conn_list);
            BT_LOGW("CS RAP GATTC notify: conn_handle mismatch, using fallback conn=%p", conn);
        }
    }
    if (!conn || !value || length == 0) {
        BT_LOGW("CS RAP GATTC notify: no conn found, dropping notify");
        return;
    }

    BT_LOGW("CS RAP GATTC notify: rt_data_handle=%d, od_data_handle=%d, dr_handle=%d, cp_handle=%d, do_handle=%d",
        conn->handles.real_time_data_handle, conn->handles.on_demand_data_handle,
        conn->handles.data_ready_handle, conn->handles.control_point_handle,
        conn->handles.data_overwritten_handle);

    if (handle == conn->handles.data_ready_handle) {
        /* Data Ready notification: contains ranging counter (2 bytes) */
        if (length >= 2 && g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_data_ready) {
            uint16_t ranging_counter = value[0] | (value[1] << 8);
            g_cs_rap_gattc->callbacks->on_data_ready(&conn->addr, ranging_counter);
        }
    } else if (handle == conn->handles.real_time_data_handle ||
               handle == conn->handles.on_demand_data_handle) {
        /* Ranging data notification */
        if (g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_ranging_data) {
            g_cs_rap_gattc->callbacks->on_ranging_data(&conn->addr, value, length);
        }
    } else if (handle == conn->handles.control_point_handle) {
        /* Control point response */
        if (length >= 1 && g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_control_point_rsp) {
            uint8_t opcode = value[0];
            g_cs_rap_gattc->callbacks->on_control_point_rsp(&conn->addr, opcode, &value[1], length - 1);
        }
    } else if (handle == conn->handles.data_overwritten_handle) {
        /* Data overwritten notification */
        if (length >= 2 && g_cs_rap_gattc->callbacks && g_cs_rap_gattc->callbacks->on_data_overwritten) {
            uint16_t ranging_counter = value[0] | (value[1] << 8);
            g_cs_rap_gattc->callbacks->on_data_overwritten(&conn->addr, ranging_counter);
        }
    }
}

static void cs_rap_gattc_mtu_cb(void* conn_handle, gatt_status_t status, uint32_t mtu)
{
    BT_LOGD("CS RAP GATTC MTU: status=%d, mtu=%lu", status, mtu);
}

/* Public API implementations */
int cs_rap_gattc_init(const cs_rap_gattc_callbacks_t* callbacks)
{
    BT_LOGD("CS RAP GATTC init called, callbacks=%p", callbacks);
    if (g_cs_rap_gattc) {
        BT_LOGD("CS RAP GATTC already initialized");
        return 0;
    }

    g_cs_rap_gattc = (cs_rap_gattc_env_t*)malloc(sizeof(cs_rap_gattc_env_t));
    if (!g_cs_rap_gattc) {
        BT_LOGE("Failed to allocate CS RAP GATTC environment");
        return -1;
    }

    memset(g_cs_rap_gattc, 0, sizeof(cs_rap_gattc_env_t));
    list_initialize(&g_cs_rap_gattc->conn_list);
    g_cs_rap_gattc->callbacks = callbacks;

    BT_LOGD("CS RAP GATTC initialized");
    return 0;
}

void cs_rap_gattc_deinit(void)
{
    if (!g_cs_rap_gattc) {
        return;
    }

    /* Clean up connections */
    struct list_node* node;
    struct list_node* tmp;
    list_for_every_safe(&g_cs_rap_gattc->conn_list, node, tmp)
    {
        cs_rap_gattc_conn_t* conn = (cs_rap_gattc_conn_t*)node;
        gattc_interface_t* gattc = get_gattc_interface();
        if (gattc && conn->conn_handle) {
            gattc->delete_connect(conn->conn_handle);
        }
        remove_conn(conn);
    }

    free(g_cs_rap_gattc);
    g_cs_rap_gattc = NULL;

    BT_LOGD("CS RAP GATTC deinitialized");
}

bt_status_t cs_rap_gattc_connect(bt_address_t* addr)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    if (addr) {
        bt_addr_ba2str(addr, addr_str);
    }
    BT_LOGW("cs_rap_gattc_connect: ENTER, addr=%s", addr_str);

    if (!g_cs_rap_gattc || !addr) {
        BT_LOGE("cs_rap_gattc_connect: INVALID PARAMS, g_cs_rap_gattc=%p, addr=%p", g_cs_rap_gattc, addr);
        return BT_STATUS_PARM_INVALID;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        BT_LOGE("cs_rap_gattc_connect: GATTC interface not available");
        return BT_STATUS_NOT_ENABLED;
    }
    BT_LOGW("cs_rap_gattc_connect: got gattc interface, connect=%p", gattc->connect);

    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn) {
        conn = create_conn(addr);
        if (!conn) {
            BT_LOGE("cs_rap_gattc_connect: create_conn failed");
            return BT_STATUS_NOMEM;
        }
        BT_LOGW("cs_rap_gattc_connect: created new conn for %s", addr_str);
    } else {
        BT_LOGW("cs_rap_gattc_connect: found existing conn for %s", addr_str);
    }

    /* Stop any ongoing LE scan before creating connection.
     * HCI does not allow LE Create Connection while scanning is active
     * (returns Command Disallowed 0x0c). */
    BT_LOGW("cs_rap_gattc_connect: stopping LE scan before connection");
    bt_status_t scan_ret = bt_sal_le_stop_scan(PRIMARY_ADAPTER);
    BT_LOGW("cs_rap_gattc_connect: bt_sal_le_stop_scan returned %d", scan_ret);

    /* Create GATTC connection */
    BT_LOGW("cs_rap_gattc_connect: calling create_connect");
    bt_status_t status = gattc->create_connect(if_gattc_get_remote(NULL), &conn->conn_handle, &gattc_cbs);
    BT_LOGW("cs_rap_gattc_connect: create_connect returned %d, conn_handle=%p", status, conn->conn_handle);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("cs_rap_gattc_connect: Failed to create GATTC connection: %d", status);
        remove_conn(conn);
        return status;
    }

    /* Connect to device - use random address type (1) since nRF54L15 DK
     * uses random addresses. TODO: make addr_type configurable.
     */
    BT_LOGW("cs_rap_gattc_connect: calling gattc->connect(handle=%p, addr=%s, addr_type=1)", conn->conn_handle, addr_str);
    status = gattc->connect(conn->conn_handle, addr, 1);
    BT_LOGW("cs_rap_gattc_connect: gattc->connect returned %d", status);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("cs_rap_gattc_connect: Failed to connect: %d", status);
        gattc->delete_connect(conn->conn_handle);
        remove_conn(conn);
        return status;
    }

    BT_LOGW("cs_rap_gattc_connect: EXIT SUCCESS");
    return BT_STATUS_SUCCESS;
}

bt_status_t cs_rap_gattc_disconnect(bt_address_t* addr)
{
    if (!g_cs_rap_gattc || !addr) {
        return BT_STATUS_PARM_INVALID;
    }

    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->conn_handle) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    return gattc->disconnect(conn->conn_handle);
}

bt_status_t cs_rap_gattc_discover(bt_address_t* addr)
{
    if (!g_cs_rap_gattc || !addr) {
        return BT_STATUS_PARM_INVALID;
    }

    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->conn_handle) {
        return BT_STATUS_NOT_FOUND;
    }

    if (conn->discovering) {
        BT_LOGD("cs_rap_gattc_discover: discovery already in progress");
        return BT_STATUS_BUSY;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    conn->discovering = true;

    bt_uuid_t ras_uuid = BT_UUID_DECLARE_16(BT_UUID_RANGING_VAL);

    BT_LOGD("cs_rap_gattc_discover: searching for RAS service UUID=0x%04x, conn_handle=%p",
        BT_UUID_RANGING_VAL, conn->conn_handle);
    return gattc->discover_service(conn->conn_handle, &ras_uuid);
}

bt_status_t cs_rap_gattc_read_features(bt_address_t* addr)
{
    if (!g_cs_rap_gattc || !addr) {
        return BT_STATUS_PARM_INVALID;
    }

    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.features_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    return gattc->read(conn->conn_handle, conn->handles.features_handle);
}

bt_status_t cs_rap_gattc_enable_real_time_data(bt_address_t* addr, bool enable)
{
    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.real_time_data_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    if (enable) {
        return gattc->subscribe(conn->conn_handle, conn->handles.real_time_data_handle, 0x01);
    } else {
        return gattc->unsubscribe(conn->conn_handle, conn->handles.real_time_data_handle);
    }
}

bt_status_t cs_rap_gattc_enable_on_demand_data(bt_address_t* addr, bool enable)
{
    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.on_demand_data_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    if (enable) {
        return gattc->subscribe(conn->conn_handle, conn->handles.on_demand_data_handle, 0x01);
    } else {
        return gattc->unsubscribe(conn->conn_handle, conn->handles.on_demand_data_handle);
    }
}

bt_status_t cs_rap_gattc_enable_data_ready(bt_address_t* addr, bool enable)
{
    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.data_ready_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    if (enable) {
        return gattc->subscribe(conn->conn_handle, conn->handles.data_ready_handle, 0x01);
    } else {
        return gattc->unsubscribe(conn->conn_handle, conn->handles.data_ready_handle);
    }
}

bt_status_t cs_rap_gattc_enable_control_point(bt_address_t* addr, bool enable)
{
    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.control_point_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    /* Control point uses indication (0x02) */
    if (enable) {
        return gattc->subscribe(conn->conn_handle, conn->handles.control_point_handle, 0x02);
    } else {
        return gattc->unsubscribe(conn->conn_handle, conn->handles.control_point_handle);
    }
}

bt_status_t cs_rap_gattc_get_ranging_data(bt_address_t* addr, uint16_t ranging_counter)
{
    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.control_point_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    uint8_t data[3];
    data[0] = RAS_CP_OP_GET_RANGING_DATA;
    data[1] = ranging_counter & 0xFF;
    data[2] = (ranging_counter >> 8) & 0xFF;

    return gattc->write(conn->conn_handle, conn->handles.control_point_handle, data, 3);
}

bt_status_t cs_rap_gattc_ack_ranging_data(bt_address_t* addr, uint16_t ranging_counter)
{
    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.control_point_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    uint8_t data[3];
    data[0] = RAS_CP_OP_ACK_RANGING_DATA;
    data[1] = ranging_counter & 0xFF;
    data[2] = (ranging_counter >> 8) & 0xFF;

    return gattc->write(conn->conn_handle, conn->handles.control_point_handle, data, 3);
}

bt_status_t cs_rap_gattc_retrieve_lost_segments(bt_address_t* addr, uint16_t ranging_counter,
    uint8_t first_seg_idx, uint8_t last_seg_idx)
{
    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.control_point_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    uint8_t data[5];
    data[0] = RAS_CP_OP_RETRIEVE_LOST_DATA_SEG;
    data[1] = ranging_counter & 0xFF;
    data[2] = (ranging_counter >> 8) & 0xFF;
    data[3] = first_seg_idx;
    data[4] = last_seg_idx;

    return gattc->write(conn->conn_handle, conn->handles.control_point_handle, data, 5);
}

bt_status_t cs_rap_gattc_abort_operation(bt_address_t* addr)
{
    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.control_point_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    uint8_t data[1];
    data[0] = RAS_CP_OP_ABORT_OPERATION;

    return gattc->write(conn->conn_handle, conn->handles.control_point_handle, data, 1);
}

bt_status_t cs_rap_gattc_set_filter(bt_address_t* addr, const cs_filter_config_t* config)
{
    if (!config) {
        return BT_STATUS_PARM_INVALID;
    }

    cs_rap_gattc_conn_t* conn = find_conn_by_addr(addr);
    if (!conn || !conn->handles_valid || conn->handles.control_point_handle == 0) {
        return BT_STATUS_NOT_FOUND;
    }

    gattc_interface_t* gattc = get_gattc_interface();
    if (!gattc) {
        return BT_STATUS_NOT_ENABLED;
    }

    uint8_t data[3];
    data[0] = RAS_CP_OP_SET_FILTER;
    /* Filter configuration: bits 0-1 = mode, bits 2-15 = filter mask */
    uint16_t filter_config = (config->mode & 0x03) | ((config->filter_mask << 2) & 0xFFFC);
    data[1] = filter_config & 0xFF;
    data[2] = (filter_config >> 8) & 0xFF;

    return gattc->write(conn->conn_handle, conn->handles.control_point_handle, data, 3);
}

