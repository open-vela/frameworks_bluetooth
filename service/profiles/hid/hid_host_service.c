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
#define LOG_TAG "hidh"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "bt_profile.h"
#include "callbacks_list.h"
#include "hid_host_service.h"
#include "sal_hid_host_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_hidh.started)              \
            return BT_STATUS_NOT_ENABLED; \
    }

/* Internal SCI policy values (HOGP SCI CR: HID Control Point) */
#define SCI_POLICY_NONE 0x00
#define SCI_POLICY_DEFAULT 0x02 /* 7.5~15ms, latency=0 */
#define SCI_POLICY_FAST 0x03 /* 1.25~5ms, latency=0 */
#define SCI_POLICY_LOW_POWER 0x04 /* 7.5~15ms, latency=100 */
#define SCI_POLICY_FULL_RANGE 0x05 /* 1.25~15ms, latency=0 */

#define HOGP_MAX_SUPPORTED_INTERVALS 9

#define HIDH_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, hid_host_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    bool started;
    callbacks_list_t* callbacks;
} hid_host_handle_t;

static hid_host_handle_t g_hidh;

/* ---- Event types for service loop ---- */

typedef struct {
    enum {
        HIDH_CONN_STATE_EVT = 0,
        HIDH_REPORT_MAP_EVT,
        HIDH_INPUT_REPORT_EVT,
        HIDH_GET_REPORT_EVT,
        HIDH_PNP_ID_EVT,
        HIDH_BATTERY_LEVEL_EVT,
        HIDH_MODE_CHANGED_EVT,
    } event;
    union {
        struct {
            bt_address_t addr;
            bt_transport_t transport;
            profile_connection_state_t state;
        } conn_state;
        struct {
            bt_address_t addr;
            uint8_t service_index;
            uint8_t* data;
            uint16_t len;
        } report_map;
        struct {
            bt_address_t addr;
            uint8_t service_index;
            uint8_t report_id;
            uint8_t* data;
            uint16_t len;
        } input_report;
        struct {
            bt_address_t addr;
            uint8_t report_id;
            uint8_t report_type;
            uint8_t* data;
            uint16_t len;
        } get_report;
        struct {
            bt_address_t addr;
            uint8_t vid_src;
            uint16_t vid;
            uint16_t pid;
            uint16_t version;
        } pnp_id;
        struct {
            bt_address_t addr;
            uint8_t bat_index;
            uint8_t level;
        } battery_level;
        struct {
            bt_address_t addr;
            uint8_t mode;
            int status;
        } mode_changed;
    };
} hidh_msg_t;

/* ---- Event processing ---- */

static void hid_host_event_process(void* data)
{
    hidh_msg_t* msg = data;

    switch (msg->event) {
    case HIDH_CONN_STATE_EVT:
        HIDH_CALLBACK_FOREACH(g_hidh.callbacks, connection_state_cb,
            &msg->conn_state.addr, msg->conn_state.transport,
            msg->conn_state.state);
        break;
    case HIDH_REPORT_MAP_EVT:
        HIDH_CALLBACK_FOREACH(g_hidh.callbacks, report_map_cb,
            &msg->report_map.addr, msg->report_map.service_index,
            msg->report_map.data, msg->report_map.len);
        free(msg->report_map.data);
        break;
    case HIDH_INPUT_REPORT_EVT:
        HIDH_CALLBACK_FOREACH(g_hidh.callbacks, input_report_cb,
            &msg->input_report.addr, msg->input_report.service_index,
            msg->input_report.report_id,
            msg->input_report.data, msg->input_report.len);
        free(msg->input_report.data);
        break;
    case HIDH_GET_REPORT_EVT:
        HIDH_CALLBACK_FOREACH(g_hidh.callbacks, get_report_cb,
            &msg->get_report.addr, msg->get_report.report_id,
            msg->get_report.report_type,
            msg->get_report.data, msg->get_report.len);
        free(msg->get_report.data);
        break;
    case HIDH_PNP_ID_EVT:
        HIDH_CALLBACK_FOREACH(g_hidh.callbacks, pnp_id_cb,
            &msg->pnp_id.addr, msg->pnp_id.vid_src,
            msg->pnp_id.vid, msg->pnp_id.pid, msg->pnp_id.version);
        break;
    case HIDH_BATTERY_LEVEL_EVT:
        HIDH_CALLBACK_FOREACH(g_hidh.callbacks, battery_level_cb,
            &msg->battery_level.addr, msg->battery_level.bat_index,
            msg->battery_level.level);
        break;
    case HIDH_MODE_CHANGED_EVT:
        HIDH_CALLBACK_FOREACH(g_hidh.callbacks, mode_changed_cb,
            &msg->mode_changed.addr, msg->mode_changed.mode,
            msg->mode_changed.status);
        break;
    }

    free(msg);
}

/* ---- SAL callbacks ---- */

void hid_host_on_connection_state_changed(bt_address_t* addr,
    bt_transport_t transport, profile_connection_state_t state)
{
    hidh_msg_t* msg = malloc(sizeof(hidh_msg_t));
    if (!msg)
        return;

    msg->event = HIDH_CONN_STATE_EVT;
    memcpy(&msg->conn_state.addr, addr, sizeof(bt_address_t));
    msg->conn_state.transport = transport;
    msg->conn_state.state = state;
    do_in_service_loop(hid_host_event_process, msg);
}

void hid_host_on_report_map(bt_address_t* addr, uint8_t service_index,
    const uint8_t* data, uint16_t len)
{
    hidh_msg_t* msg = malloc(sizeof(hidh_msg_t));
    if (!msg)
        return;

    msg->event = HIDH_REPORT_MAP_EVT;
    memcpy(&msg->report_map.addr, addr, sizeof(bt_address_t));
    msg->report_map.service_index = service_index;
    msg->report_map.len = len;
    msg->report_map.data = NULL;
    if (len > 0 && data) {
        msg->report_map.data = malloc(len);
        if (!msg->report_map.data) {
            BT_LOGE("%s malloc report_map data failed", __func__);
            free(msg);
            return;
        }
        memcpy(msg->report_map.data, data, len);
    }

    do_in_service_loop(hid_host_event_process, msg);
}

void hid_host_on_input_report(bt_address_t* addr, uint8_t service_index,
    uint8_t report_id, const uint8_t* data, uint16_t len)
{
    hidh_msg_t* msg = malloc(sizeof(hidh_msg_t));
    if (!msg)
        return;

    msg->event = HIDH_INPUT_REPORT_EVT;
    memcpy(&msg->input_report.addr, addr, sizeof(bt_address_t));
    msg->input_report.service_index = service_index;
    msg->input_report.report_id = report_id;
    msg->input_report.len = len;
    msg->input_report.data = NULL;
    if (len > 0 && data) {
        msg->input_report.data = malloc(len);
        if (!msg->input_report.data) {
            BT_LOGE("%s malloc input_report data failed", __func__);
            free(msg);
            return;
        }
        memcpy(msg->input_report.data, data, len);
    }
    do_in_service_loop(hid_host_event_process, msg);
}

void hid_host_on_get_report_result(bt_address_t* addr, uint8_t report_id,
    uint8_t report_type, const uint8_t* data, uint16_t len)
{
    hidh_msg_t* msg = malloc(sizeof(hidh_msg_t));
    if (!msg)
        return;

    msg->event = HIDH_GET_REPORT_EVT;
    memcpy(&msg->get_report.addr, addr, sizeof(bt_address_t));
    msg->get_report.report_id = report_id;
    msg->get_report.report_type = report_type;
    msg->get_report.len = len;
    msg->get_report.data = NULL;
    if (len > 0 && data) {
        msg->get_report.data = malloc(len);
        if (!msg->get_report.data) {
            BT_LOGE("%s malloc get_report data failed", __func__);
            free(msg);
            return;
        }
        memcpy(msg->get_report.data, data, len);
    }
    do_in_service_loop(hid_host_event_process, msg);
}

void hid_host_on_pnp_id(bt_address_t* addr, uint8_t vid_src,
    uint16_t vid, uint16_t pid, uint16_t version)
{
    hidh_msg_t* msg = malloc(sizeof(hidh_msg_t));
    if (!msg)
        return;

    msg->event = HIDH_PNP_ID_EVT;
    memcpy(&msg->pnp_id.addr, addr, sizeof(bt_address_t));
    msg->pnp_id.vid_src = vid_src;
    msg->pnp_id.vid = vid;
    msg->pnp_id.pid = pid;
    msg->pnp_id.version = version;
    do_in_service_loop(hid_host_event_process, msg);
}

void hid_host_on_battery_level(bt_address_t* addr, uint8_t bat_index,
    uint8_t level)
{
    hidh_msg_t* msg = malloc(sizeof(hidh_msg_t));
    if (!msg)
        return;

    msg->event = HIDH_BATTERY_LEVEL_EVT;
    memcpy(&msg->battery_level.addr, addr, sizeof(bt_address_t));
    msg->battery_level.bat_index = bat_index;
    msg->battery_level.level = level;
    do_in_service_loop(hid_host_event_process, msg);
}

void hid_host_on_mode_changed(bt_address_t* addr, uint8_t mode, int status)
{
    hidh_msg_t* msg = malloc(sizeof(hidh_msg_t));
    if (!msg) {
        return;
    }

    msg->event = HIDH_MODE_CHANGED_EVT;
    memcpy(&msg->mode_changed.addr, addr, sizeof(bt_address_t));
    msg->mode_changed.mode = mode;
    msg->mode_changed.status = status;

    do_in_service_loop(hid_host_event_process, msg);
}

/* ---- Profile interface ---- */

static void* hidh_register_callbacks(void* remote, const hid_host_callbacks_t* callbacks)
{
    if (!g_hidh.callbacks) {
        g_hidh.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
    }

    return bt_remote_callbacks_register(g_hidh.callbacks, remote, (void*)callbacks);
}

static bool hidh_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_hidh.callbacks, remote, cookie);
}

static bt_status_t hidh_connect(bt_address_t* addr, bt_transport_t transport)
{
    CHECK_ENABLED();

    return bt_sal_hid_host_connect(addr, transport);
}

static bt_status_t hidh_disconnect(bt_address_t* addr)
{
    CHECK_ENABLED();
    return bt_sal_hid_host_disconnect(addr);
}

static bt_status_t hidh_get_report(bt_address_t* addr, uint8_t report_id,
    uint8_t report_type)
{
    CHECK_ENABLED();
    return bt_sal_hid_host_get_report(addr, report_id, report_type);
}

static bt_status_t hidh_set_report(bt_address_t* addr, uint8_t report_id,
    uint8_t report_type, const uint8_t* data, uint16_t len)
{
    CHECK_ENABLED();
    return bt_sal_hid_host_set_report(addr, report_id, report_type, data, len);
}

static bt_status_t hidh_set_protocol(bt_address_t* addr, uint8_t protocol_mode)
{
    CHECK_ENABLED();
    return bt_sal_hid_host_set_protocol(addr, protocol_mode);
}

static bt_status_t hidh_suspend(bt_address_t* addr)
{
    CHECK_ENABLED();
    return bt_sal_hid_host_suspend(addr);
}

static bt_status_t hidh_exit_suspend(bt_address_t* addr)
{
    CHECK_ENABLED();
    return bt_sal_hid_host_exit_suspend(addr);
}

static uint16_t hidh_select_iso_interval(bt_address_t* addr, uint8_t level)
{
    uint16_t intervals[HOGP_MAX_SUPPORTED_INTERVALS];
    int count;

    count = bt_sal_hid_host_get_supported_intervals(addr, intervals, HOGP_MAX_SUPPORTED_INTERVALS);
    BT_LOGD("hidh_select_iso_interval: count=%d level=%u", count, level);
    if (count <= 0)
        return 5000;

    switch (level) {
    case BT_HID_HOST_LEVEL_HIGH:
        return intervals[0];
    case BT_HID_HOST_LEVEL_LOW:
        return intervals[count - 1];
    case BT_HID_HOST_LEVEL_MEDIUM:
        return intervals[count / 2];
    default: /* auto - prefer 5ms or closest */
        for (int i = 0; i < count; i++) {
            if (intervals[i] >= 5000)
                return intervals[i];
        }
        return intervals[count - 1];
    }
}

static bt_status_t hidh_set_mode(bt_address_t* addr, uint8_t mode,
    uint8_t level)
{
    static const uint8_t level_to_sci[] = {
        [BT_HID_HOST_LEVEL_LOW] = SCI_POLICY_LOW_POWER,
        [BT_HID_HOST_LEVEL_MEDIUM] = SCI_POLICY_DEFAULT,
        [BT_HID_HOST_LEVEL_HIGH] = SCI_POLICY_FAST,
        [BT_HID_HOST_LEVEL_AUTO] = SCI_POLICY_FULL_RANGE,
    };
    uint8_t policy = SCI_POLICY_NONE;
    uint16_t iso_interval = 0;

    CHECK_ENABLED();

    if ((mode & BT_HID_HOST_MODE_SCI) && level <= BT_HID_HOST_LEVEL_AUTO)
        policy = level_to_sci[level];

    if (mode & BT_HID_HOST_MODE_ISO) {
        iso_interval = hidh_select_iso_interval(addr, level);
        BT_LOGD("hidh_set_mode: iso_interval=%u level=%u", iso_interval, level);
    }

    return bt_sal_hid_host_set_mode(addr, mode, policy, iso_interval);
}

static const hid_host_interface_t g_hidh_interface = {
    .size = sizeof(hid_host_interface_t),
    .register_callbacks = hidh_register_callbacks,
    .unregister_callbacks = hidh_unregister_callbacks,
    .connect = hidh_connect,
    .disconnect = hidh_disconnect,
    .get_report = hidh_get_report,
    .set_report = hidh_set_report,
    .set_protocol = hidh_set_protocol,
    .suspend = hidh_suspend,
    .exit_suspend = hidh_exit_suspend,
    .set_mode = hidh_set_mode,
};

static const void* get_hidh_profile_interface(void)
{
    return &g_hidh_interface;
}

/* ---- Profile service lifecycle ---- */

static bt_status_t hid_host_init(void)
{
    memset(&g_hidh, 0, sizeof(g_hidh));
    return BT_STATUS_SUCCESS;
}

static void hidh_do_startup(void* data)
{
    profile_on_startup_t cb = (profile_on_startup_t)data;
    bt_status_t ret = bt_sal_hid_host_init();

    g_hidh.started = (ret == BT_STATUS_SUCCESS);
    cb(PROFILE_HID_HOST, g_hidh.started);
}

static bt_status_t hid_host_startup(profile_on_startup_t cb)
{
    do_in_service_loop(hidh_do_startup, cb);
    return BT_STATUS_SUCCESS;
}

static void hidh_do_shutdown(void* data)
{
    profile_on_shutdown_t cb = (profile_on_shutdown_t)data;

    bt_sal_hid_host_cleanup();
    g_hidh.started = false;
    cb(PROFILE_HID_HOST, true);
}

static bt_status_t hid_host_shutdown(profile_on_shutdown_t cb)
{
    do_in_service_loop(hidh_do_shutdown, cb);
    return BT_STATUS_SUCCESS;
}

static void hid_host_cleanup(void)
{
    if (g_hidh.callbacks) {
        bt_callbacks_list_free(g_hidh.callbacks);
        g_hidh.callbacks = NULL;
    }
}

static int hid_host_get_state(void)
{
    return g_hidh.started ? SERVICE_ENABLED : SERVICE_DISABLED;
}

static int hid_host_dump(void)
{
    return 0;
}

static void hid_host_process_msg(profile_msg_t* msg)
{
    (void)msg;
}

static const profile_service_t hid_host_service = {
    .auto_start = true,
    .name = PROFILE_HID_HOST_NAME,
    .id = PROFILE_HID_HOST,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = hid_host_init,
    .startup = hid_host_startup,
    .shutdown = hid_host_shutdown,
    .process_msg = hid_host_process_msg,
    .get_state = hid_host_get_state,
    .get_profile_interface = get_hidh_profile_interface,
    .cleanup = hid_host_cleanup,
    .dump = hid_host_dump,
};

void register_hid_host_service(void)
{
    register_service(&hid_host_service);
}
