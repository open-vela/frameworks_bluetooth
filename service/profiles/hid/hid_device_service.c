/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#define LOG_TAG "hidd"
/****************************************************************************
 * Included Files
 ****************************************************************************/
// stdlib
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "bt_profile.h"
#include "callbacks_list.h"
#include "power_manager.h"
#include "sal_hid_device_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_hidd_handle.started)       \
            return BT_STATUS_NOT_ENABLED; \
    }

#define HIDD_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, hid_device_callbacks_t, _cback, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct {
    bool started;
    hid_app_state_t app_state;
    bt_address_t peer_addr;
    profile_connection_state_t conn_state;
    pthread_mutex_t hid_lock;
    callbacks_list_t* callbacks;
} hid_device_handle_t;

typedef struct {
    enum {
        APP_REGISTER_EVT = 0,
        CONNECT_CHANGE_EVT,
        GET_REPORT_EVT,
        SET_REPORT_EVT,
        RECEIVE_REPORT_EVT,
        VIRTUAL_UNPLUG_EVT,
    } event;

    union {
        /**
         * @brief APP_REGISTER_EVENT
         */
        struct app_register_evt_param {
            hid_app_state_t state;
        } app_register;

        /**
         * @brief CONNECT_CHANGE_EVENT
         */
        struct connect_change_evt_param {
            bt_address_t addr;
            bool le_hid;
            profile_connection_state_t state;
        } connect_change;

        /**
         * @brief GET_REPORT_EVENT
         */
        struct get_report_evt_param {
            bt_address_t addr;
            uint8_t rpt_type;
            uint8_t rpt_id;
            uint16_t buffer_size;
        } get_report;

        /**
         * @brief SET_REPORT_EVENT
         */
        struct set_report_evt_param {
            bt_address_t addr;
            uint8_t rpt_type;
            uint16_t rpt_size;
            uint8_t rpt_data[0];
        } set_report;

        /**
         * @brief RECEIVE_REPORT_EVENT
         */
        struct recv_report_evt_param {
            bt_address_t addr;
            uint8_t rpt_type;
            uint16_t rpt_size;
            uint8_t rpt_data[0];
        } recv_report;

        /**
         * @brief VIRTUAL_UNPLUG_EVENT
         */
        struct virtual_unplug_evt_param {
            bt_address_t addr;
        } unplug;
    };

} hidd_msg_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static hid_device_handle_t g_hidd_handle = { .started = false };
static bool hid_device_unregister_callbacks(void** remote, void* cookie);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void hid_device_handle_connection_event(bt_address_t* addr, profile_connection_state_t state)
{
    hid_device_handle_t* handle = &g_hidd_handle;

    handle->conn_state = state;
    switch (state) {
    case PROFILE_STATE_CONNECTING:
        memcpy(&handle->peer_addr, addr, sizeof(bt_address_t));
        break;
    case PROFILE_STATE_CONNECTED:
        bt_pm_conn_open(PROFILE_HID_DEV, addr);
        break;
    case PROFILE_STATE_DISCONNECTED:
        bt_pm_conn_close(PROFILE_HID_DEV, addr);
        bt_addr_set_empty(&handle->peer_addr);
        break;
    default:
        break;
    }
}

static void hid_device_event_process(void* data)
{
    hidd_msg_t* msg = (hidd_msg_t*)data;
    if (!msg)
        return;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        BT_LOGW("%s HID device is stopped, msg id:%d", __func__, msg->event);
        goto end;
    }

    switch (msg->event) {
    case APP_REGISTER_EVT:
        if (msg->app_register.state == HID_APP_STATE_NOT_REGISTERED)
            g_hidd_handle.app_state = HID_APP_STATE_NOT_REGISTERED;
        HIDD_CALLBACK_FOREACH(g_hidd_handle.callbacks, app_state_cb, msg->app_register.state);
        break;
    case CONNECT_CHANGE_EVT: {
        BT_ADDR_LOG("HID-DEVICE-CONNECTION-STATE-EVENT from:%s, state:%d", &msg->connect_change.addr, msg->connect_change.state);
        hid_device_handle_connection_event(&msg->connect_change.addr, msg->connect_change.state);
        HIDD_CALLBACK_FOREACH(g_hidd_handle.callbacks, connection_state_cb, &msg->connect_change.addr, msg->connect_change.le_hid, msg->connect_change.state);
    } break;
    case GET_REPORT_EVT:
        HIDD_CALLBACK_FOREACH(g_hidd_handle.callbacks, get_report_cb, &msg->get_report.addr, msg->get_report.rpt_type, msg->get_report.rpt_id, msg->get_report.buffer_size);
        break;
    case SET_REPORT_EVT:
        HIDD_CALLBACK_FOREACH(g_hidd_handle.callbacks, set_report_cb, &msg->set_report.addr, msg->set_report.rpt_type, msg->set_report.rpt_size, msg->set_report.rpt_data);
        break;
    case RECEIVE_REPORT_EVT:
        HIDD_CALLBACK_FOREACH(g_hidd_handle.callbacks, receive_report_cb, &msg->recv_report.addr, msg->recv_report.rpt_type, msg->recv_report.rpt_size, msg->recv_report.rpt_data);
        break;
    case VIRTUAL_UNPLUG_EVT:
        HIDD_CALLBACK_FOREACH(g_hidd_handle.callbacks, virtual_unplug_cb, &msg->unplug.addr);
        break;
    default:
        break;
    }

end:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    free(msg);
}

static bt_status_t hid_device_init(void)
{
    bt_status_t status;
    pthread_mutexattr_t attr;

    memset(&g_hidd_handle, 0, sizeof(g_hidd_handle));
    g_hidd_handle.started = false;

    g_hidd_handle.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
    if (!g_hidd_handle.callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_hidd_handle.hid_lock, &attr) < 0) {
        status = BT_STATUS_FAIL;
        goto fail;
    }

    return BT_STATUS_SUCCESS;

fail:
    if (g_hidd_handle.callbacks) {
        bt_callbacks_list_free(g_hidd_handle.callbacks);
        g_hidd_handle.callbacks = NULL;
    }

    return status;
}

static bt_status_t hid_device_startup(profile_on_startup_t cb)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (g_hidd_handle.started) {
        pthread_mutex_unlock(&g_hidd_handle.hid_lock);
        cb(PROFILE_HID_DEV, true);
        return BT_STATUS_SUCCESS;
    }

    status = bt_sal_hid_device_init();
    if (status != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_hidd_handle.hid_lock);
        cb(PROFILE_HID_DEV, false);
        return BT_STATUS_FAIL;
    }

    g_hidd_handle.started = true;
    g_hidd_handle.app_state = HID_APP_STATE_NOT_REGISTERED;
    g_hidd_handle.conn_state = PROFILE_STATE_DISCONNECTED;
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    cb(PROFILE_HID_DEV, true);

    return BT_STATUS_SUCCESS;
}

static bt_status_t hid_device_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        pthread_mutex_unlock(&g_hidd_handle.hid_lock);
        cb(PROFILE_HID_DEV, false);
        return BT_STATUS_NOT_ENABLED;
    }

    if (g_hidd_handle.conn_state == PROFILE_STATE_CONNECTED || g_hidd_handle.conn_state == PROFILE_STATE_CONNECTING) {
        bt_sal_hid_device_disconnect(&g_hidd_handle.peer_addr);
        bt_pm_conn_close(PROFILE_HID_DEV, &g_hidd_handle.peer_addr);
    }

    g_hidd_handle.started = false;
    g_hidd_handle.app_state = HID_APP_STATE_NOT_REGISTERED;
    g_hidd_handle.conn_state = PROFILE_STATE_DISCONNECTED;
    bt_addr_set_empty(&g_hidd_handle.peer_addr);
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    /* cleanup hid device stack */
    bt_sal_hid_device_cleanup();
    cb(PROFILE_HID_DEV, true);

    return BT_STATUS_SUCCESS;
}

static void hid_device_cleanup(void)
{
    bt_callbacks_list_free(g_hidd_handle.callbacks);
    g_hidd_handle.callbacks = NULL;
    pthread_mutex_destroy(&g_hidd_handle.hid_lock);
}

static void hid_device_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_REMOTE_DETACH: {
        bt_instance_t* ins = msg->data.data;

        if (ins->hidd_cookie) {
            BT_LOGD("%s PROFILE_EVT_REMOTE_DETACH", __func__);
            hid_device_unregister_callbacks((void**)&ins, ins->hidd_cookie);
            ins->hidd_cookie = NULL;
        }
        break;
    }
    default:
        break;
    }
}

static int hid_device_get_state(void)
{
    return 1;
}

static int hid_device_dump(void)
{
    hid_app_state_t app_state;
    profile_connection_state_t conn_state;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        pthread_mutex_unlock(&g_hidd_handle.hid_lock);
        BT_LOGI("HID device is stopped!");
        return 0;
    }

    app_state = g_hidd_handle.app_state;
    conn_state = g_hidd_handle.conn_state;
    bt_addr_ba2str(&g_hidd_handle.peer_addr, addr_str);
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);

    BT_LOGI("HID Device[0]:");
    BT_LOGI("\tApp state:%s", (app_state == HID_APP_STATE_REGISTERED) ? "registered" : "not registed");
    BT_LOGI("\tConnection state:%d, peer:%s", conn_state, addr_str);

    return 0;
}

static void* hid_device_register_callbacks(void* remote, const hid_device_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_hidd_handle.callbacks, remote, (void*)callbacks);
}

static bool hid_device_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_hidd_handle.callbacks, remote, cookie);
}

static bt_status_t hid_device_register_app(hid_device_sdp_settings_t* sdp, bool le_hid)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (g_hidd_handle.app_state == HID_APP_STATE_REGISTERED) {
        BT_LOGI("%s, HID app has registered!", __func__);
        status = BT_STATUS_NO_RESOURCES;
        goto exit;
    }

    status = bt_sal_hid_device_register_app(sdp, le_hid);
    if (status == BT_STATUS_SUCCESS) {
        g_hidd_handle.app_state = HID_APP_STATE_REGISTERED;
    }

exit:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    return status;
}

static bt_status_t hid_device_unregister_app(void)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (g_hidd_handle.app_state == HID_APP_STATE_NOT_REGISTERED) {
        status = BT_STATUS_NOT_FOUND;
        goto exit;
    }

    status = bt_sal_hid_device_unregister_app();

exit:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    return status;
}

static bt_status_t hid_device_connect(bt_address_t* addr)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (!bt_addr_is_empty(&g_hidd_handle.peer_addr)) {
        BT_ADDR_LOG("HID device has connected to %s, %s!", &g_hidd_handle.peer_addr, __func__);
        status = BT_STATUS_BUSY;
        goto exit;
    }

    status = bt_sal_hid_device_connect(addr);

exit:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    return status;
}

static bt_status_t hid_device_disconnect(bt_address_t* addr)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (bt_addr_compare(addr, &g_hidd_handle.peer_addr)) {
        BT_ADDR_LOG("%s is not current HID host, %s!", addr, __func__);
        status = BT_STATUS_DEVICE_NOT_FOUND;
        goto exit;
    }

    status = bt_sal_hid_device_disconnect(addr);

exit:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    return status;
}

static bt_status_t hid_device_send_report(bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (bt_addr_compare(addr, &g_hidd_handle.peer_addr)) {
        BT_ADDR_LOG("%s is not current HID host, %s!", addr, __func__);
        status = BT_STATUS_DEVICE_NOT_FOUND;
        goto exit;
    }

    bt_pm_busy(PROFILE_HID_DEV, addr);
    status = bt_sal_hid_device_send_report(addr, rpt_id, rpt_data, rpt_size);
    bt_pm_idle(PROFILE_HID_DEV, addr);

exit:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    return status;
}

static bt_status_t hid_device_response_report(bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (bt_addr_compare(addr, &g_hidd_handle.peer_addr)) {
        BT_ADDR_LOG("%s is not current HID host, %s!", addr, __func__);
        status = BT_STATUS_DEVICE_NOT_FOUND;
        goto exit;
    }

    bt_pm_busy(PROFILE_HID_DEV, addr);
    status = bt_sal_hid_device_get_report_response(addr, rpt_type, rpt_data, rpt_size);
    bt_pm_idle(PROFILE_HID_DEV, addr);

exit:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    return status;
}

static bt_status_t hid_device_report_error(bt_address_t* addr, hid_status_error_t error)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (bt_addr_compare(addr, &g_hidd_handle.peer_addr)) {
        BT_ADDR_LOG("%s is not current HID host, %s!", addr, __func__);
        status = BT_STATUS_DEVICE_NOT_FOUND;
        goto exit;
    }

    bt_pm_busy(PROFILE_HID_DEV, addr);
    status = bt_sal_hid_device_report_error(addr, error);
    bt_pm_idle(PROFILE_HID_DEV, addr);

exit:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    return status;
}

static bt_status_t hid_device_virtual_unplug(bt_address_t* addr)
{
    bt_status_t status;

    pthread_mutex_lock(&g_hidd_handle.hid_lock);
    if (!g_hidd_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    if (bt_addr_compare(addr, &g_hidd_handle.peer_addr)) {
        BT_ADDR_LOG("%s is not current HID host, %s!", addr, __func__);
        status = BT_STATUS_DEVICE_NOT_FOUND;
        goto exit;
    }

    bt_pm_busy(PROFILE_HID_DEV, addr);
    status = bt_sal_hid_device_virtual_unplug(addr);
    bt_pm_idle(PROFILE_HID_DEV, addr);

exit:
    pthread_mutex_unlock(&g_hidd_handle.hid_lock);
    return status;
}

static hid_device_interface_t deviceInterface = {
    .size = sizeof(deviceInterface),
    .register_callbacks = hid_device_register_callbacks,
    .unregister_callbacks = hid_device_unregister_callbacks,
    .register_app = hid_device_register_app,
    .unregister_app = hid_device_unregister_app,
    .connect = hid_device_connect,
    .disconnect = hid_device_disconnect,
    .send_report = hid_device_send_report,
    .response_report = hid_device_response_report,
    .report_error = hid_device_report_error,
    .virtual_unplug = hid_device_virtual_unplug,
};

static const void* get_device_profile_interface(void)
{
    return (void*)&deviceInterface;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void hid_device_on_app_state_changed(hid_app_state_t state)
{
    hidd_msg_t* msg = malloc(sizeof(hidd_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = APP_REGISTER_EVT;
    msg->app_register.state = state;

    do_in_service_loop(hid_device_event_process, msg);
}

void hid_device_on_connection_state_changed(bt_address_t* addr, bool le_hid, profile_connection_state_t state)
{
    hidd_msg_t* msg = malloc(sizeof(hidd_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = CONNECT_CHANGE_EVT;
    msg->connect_change.le_hid = le_hid;
    msg->connect_change.state = state;
    memcpy(&msg->connect_change.addr, addr, sizeof(bt_address_t));

    do_in_service_loop(hid_device_event_process, msg);
}

void hid_device_on_get_report(bt_address_t* addr, uint8_t rpt_type, uint8_t rpt_id, uint16_t buffer_size)
{
    hidd_msg_t* msg = malloc(sizeof(hidd_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = GET_REPORT_EVT;
    msg->get_report.rpt_type = rpt_type;
    msg->get_report.rpt_id = rpt_id;
    msg->get_report.buffer_size = buffer_size;
    memcpy(&msg->get_report.addr, addr, sizeof(bt_address_t));

    do_in_service_loop(hid_device_event_process, msg);
}

void hid_device_on_set_report(bt_address_t* addr, uint8_t rpt_type, uint16_t rpt_size, uint8_t* rpt_data)
{
    hidd_msg_t* msg = malloc(sizeof(hidd_msg_t) + rpt_size);
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = SET_REPORT_EVT;
    msg->set_report.rpt_type = rpt_type;
    msg->set_report.rpt_size = rpt_size;
    memcpy(&msg->set_report.rpt_data, rpt_data, rpt_size);
    memcpy(&msg->set_report.addr, addr, sizeof(bt_address_t));

    do_in_service_loop(hid_device_event_process, msg);
}

void hid_device_on_receive_report(bt_address_t* addr, uint8_t rpt_type, uint16_t rpt_size, uint8_t* rpt_data)
{
    hidd_msg_t* msg = malloc(sizeof(hidd_msg_t) + rpt_size);
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = RECEIVE_REPORT_EVT;
    msg->recv_report.rpt_type = rpt_type;
    msg->recv_report.rpt_size = rpt_size;
    memcpy(&msg->recv_report.rpt_data, rpt_data, rpt_size);
    memcpy(&msg->recv_report.addr, addr, sizeof(bt_address_t));

    do_in_service_loop(hid_device_event_process, msg);
}

void hid_device_on_virtual_cable_unplug(bt_address_t* addr)
{
    hidd_msg_t* msg = malloc(sizeof(hidd_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = VIRTUAL_UNPLUG_EVT;
    memcpy(&msg->unplug.addr, addr, sizeof(bt_address_t));

    do_in_service_loop(hid_device_event_process, msg);
}

static const profile_service_t hid_device_service = {
    .auto_start = true,
    .name = PROFILE_HID_DEV_NAME,
    .id = PROFILE_HID_DEV,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = hid_device_init,
    .startup = hid_device_startup,
    .shutdown = hid_device_shutdown,
    .process_msg = hid_device_process_msg,
    .get_state = hid_device_get_state,
    .get_profile_interface = get_device_profile_interface,
    .cleanup = hid_device_cleanup,
    .dump = hid_device_dump,
};

void register_hid_device_service(void)
{
    register_service(&hid_device_service);
}
