/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#define LOG_TAG "hfp_hf"
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <sys/types.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "audio_control.h"
#include "bt_hfp_hf.h"
#include "bt_profile.h"
#include "bt_vendor.h"
#include "callbacks_list.h"
#include "hfp_hf_service.h"
#include "hfp_hf_state_machine.h"
#include "sal_hfp_hf_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifndef CONFIG_HFP_HF_MAX_CONNECTIONS
#define CONFIG_HFP_HF_MAX_CONNECTIONS 1
#endif

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_hfp_service.started)       \
            return BT_STATUS_NOT_ENABLED; \
    }

#define HF_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, hfp_hf_callbacks_t, _cback, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct
{
    bool started;
    bool offloading;
    uint8_t max_connections;
    bt_list_t* hf_devices;
    callbacks_list_t* callbacks;
} hf_service_t;

typedef struct
{
    bt_address_t addr;
    hf_state_machine_t* hfsm;
} hf_device_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
bt_status_t hfp_hf_send_message(hfp_hf_msg_t* msg);
static hf_state_machine_t* get_state_machine(bt_address_t* addr);
static bool hfp_hf_unregister_callbacks(void** remote, void* cookie);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static hf_service_t g_hfp_service = {
    .started = false,
    .hf_devices = NULL,
    .callbacks = NULL,
};

static uint32_t hf_support_features = HFP_BRSF_HF_HFINDICATORS | HFP_BRSF_HF_RMTVOLCTRL | HFP_BRSF_HF_ENHANCED_CALLSTATUS | HFP_BRSF_HF_CLIP | HFP_BRSF_HF_3WAYCALL | HFP_BRSF_HF_ENHANCED_CALLCONTROL | HFP_BRSF_HF_BVRA | HFP_BRSF_HF_CODEC_NEGOTIATION | HFP_BRSF_HF_ESCO_S4T2_SETTING;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool hf_device_cmp(void* device, void* addr)
{
    return bt_addr_compare(&((hf_device_t*)device)->addr, addr) == 0;
}

static hf_device_t* find_hf_device_by_addr(bt_address_t* addr)
{
    return bt_list_find(g_hfp_service.hf_devices, hf_device_cmp, addr);
}

static hf_device_t* find_hf_device_by_state(hfp_hf_state_t state)
{
    bt_list_t* list = g_hfp_service.hf_devices;
    bt_list_node_t* node;

    if (list == NULL) {
        return NULL;
    }

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        hf_device_t* device = bt_list_node(node);
        if (hf_state_machine_get_state(device->hfsm) == state) {
            return device;
        }
    }

    return NULL;
}

static hf_device_t* hf_device_new(bt_address_t* addr, hf_state_machine_t* hfsm)
{
    hf_device_t* device = malloc(sizeof(hf_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->hfsm = hfsm;

    return device;
}

static void hf_device_delete(hf_device_t* device)
{
    if (!device)
        return;

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_DISCONNECT, &device->addr);
    if (msg == NULL)
        return;

    hf_state_machine_dispatch(device->hfsm, msg);
    hfp_hf_msg_destroy(msg);
    hf_state_machine_destory(device->hfsm);
    free(device);
}

static hf_state_machine_t* get_state_machine(bt_address_t* addr)
{
    hf_state_machine_t* hfsm;
    hf_device_t* device;

    if (!g_hfp_service.started)
        return NULL;

    device = find_hf_device_by_addr(addr);
    if (device)
        return device->hfsm;

    hfsm = hf_state_machine_new(addr, (void*)&g_hfp_service);
    if (!hfsm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    hf_state_machine_set_offloading(hfsm, g_hfp_service.offloading);
    device = hf_device_new(addr, hfsm);
    if (!device) {
        BT_LOGE("New device alloc failed");
        hf_state_machine_destory(hfsm);
        return NULL;
    }

    bt_list_add_tail(g_hfp_service.hf_devices, device);

    return hfsm;
}

static uint32_t get_hf_features(void)
{
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    return property_get_int32("persist.bluetooth.hfp.hf_features", hf_support_features);
#else
    return hf_support_features;
#endif
}

static void hf_startup(profile_on_startup_t on_startup)
{
    bt_status_t status;
    hf_service_t* service = &g_hfp_service;

    if (service->started) {
        on_startup(PROFILE_HFP_HF, true);
        return;
    }

    service->max_connections = CONFIG_HFP_HF_MAX_CONNECTIONS;
    service->hf_devices = bt_list_new((bt_list_free_cb_t)hf_device_delete);
    service->callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
    if (!service->hf_devices || !service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    status = bt_sal_hfp_hf_init(get_hf_features(), CONFIG_HFP_HF_MAX_CONNECTIONS);
    if (status != BT_STATUS_SUCCESS)
        goto fail;

    service->started = true;
    on_startup(PROFILE_HFP_HF, true);

    return;
fail:
    bt_list_free(service->hf_devices);
    service->hf_devices = NULL;
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    on_startup(PROFILE_HFP_HF, false);
}

static void hf_shutdown(profile_on_shutdown_t on_shutdown)
{
    hf_service_t* service = &g_hfp_service;

    if (!service->started) {
        on_shutdown(PROFILE_HFP_HF, true);
        return;
    }

    service->started = false;
    bt_list_free(service->hf_devices);
    service->hf_devices = NULL;
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    bt_sal_hfp_hf_cleanup();
    on_shutdown(PROFILE_HFP_HF, true);
}

static void hf_dispatch_msg_foreach(void* data, void* context)
{
    hf_device_t* device = (hf_device_t*)data;

    hf_state_machine_dispatch(device->hfsm, (hfp_hf_msg_t*)context);
}

static void hfp_hf_process_message(void* data)
{
    hfp_hf_msg_t* msg = (hfp_hf_msg_t*)data;

    if (!g_hfp_service.started && msg->event != HF_STARTUP)
        return;

    switch (msg->event) {
    case HF_STARTUP:
        hf_startup(INT2PTR(profile_on_startup_t) msg->data.valueint1);
        break;
    case HF_SHUTDOWN:
        hf_shutdown(INT2PTR(profile_on_shutdown_t) msg->data.valueint1);
        break;
    case HF_UPDATE_BATTERY_LEVEL:
    case HF_SET_VOLUME:
        bt_list_foreach(g_hfp_service.hf_devices, hf_dispatch_msg_foreach, msg);
        break;
    default: {
        hf_state_machine_t* hfsm = get_state_machine(&msg->data.addr);
        if (!hfsm) {
            break;
        }

        if (msg->event == HF_STACK_EVENT_AUDIO_STATE_CHANGED
            && msg->data.valueint1 == HFP_AUDIO_STATE_CONNECTED) {
            /* Make this device active. TODO: set active device by App. */
            bt_list_move(g_hfp_service.hf_devices, g_hfp_service.hf_devices,
                find_hf_device_by_addr(&msg->data.addr), true);
        }

        hf_state_machine_dispatch(hfsm, msg);
        break;
    }
    }

    hfp_hf_msg_destroy(msg);
}

bt_status_t hfp_hf_send_message(hfp_hf_msg_t* msg)
{
    assert(msg);

    do_in_service_loop(hfp_hf_process_message, msg);

    return BT_STATUS_SUCCESS;
}

bt_status_t hfp_hf_send_event(bt_address_t* addr, hfp_hf_event_t evt)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(evt, addr);

    if (!msg)
        return BT_STATUS_NOMEM;

    return hfp_hf_send_message(msg);
}

static uint8_t get_current_connnection_cnt(void)
{
    bt_list_t* list = g_hfp_service.hf_devices;
    bt_list_node_t* node;
    uint8_t cnt = 0;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        hf_device_t* device = bt_list_node(node);
        if (hf_state_machine_get_state(device->hfsm) >= HFP_HF_STATE_CONNECTED || hf_state_machine_get_state(device->hfsm) == HFP_HF_STATE_CONNECTING)
            cnt++;
    }

    return cnt;
}

bool hfp_hf_on_sco_start(void)
{
    hf_device_t* device;

    device = find_hf_device_by_state(HFP_HF_STATE_AUDIO_CONNECTED);
    if (!device) {
        BT_LOGD("%s: sco not found", __func__);
        return false;
    }

    if (!g_hfp_service.offloading) {
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STARTED);
        return true;
    }

    if (hfp_hf_send_event(&device->addr, HF_OFFLOAD_START_REQ) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: failed to send msg", __func__);
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_START_FAIL);
        return true;
    }

    BT_LOGD("%s: send sco offload start", __func__);
    /* AUDIO_CTRL_EVT_STARTED would be generated at HF_OFFLOAD_START_EVT */
    return true;
}

bool hfp_hf_on_sco_stop(void)
{
    hf_device_t* device;

    device = find_hf_device_by_state(HFP_HF_STATE_AUDIO_CONNECTED);
    if (!device) {
        BT_LOGD("%s: sco not found", __func__);
        return false;
    }

    if (!g_hfp_service.offloading) {
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        return true;
    }

    if (hfp_hf_send_event(&device->addr, HF_OFFLOAD_STOP_REQ) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: failed to send msg", __func__);
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        return true;
    }

    BT_LOGD("%s: send sco offload stop", __func__);
    /* AUDIO_CTRL_EVT_STOPPED would be generated at HF_OFFLOAD_STOP_EVT */
    return true;
}

static bt_status_t hfp_hf_init(void)
{
    bt_status_t ret;

    ret = audio_ctrl_init(PROFILE_HFP_HF);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s: failed to start audio control channel", __func__);
        return ret;
    }

    return ret;
}

static void hfp_hf_cleanup(void)
{
    audio_ctrl_cleanup(PROFILE_HFP_HF);
}

static bt_status_t hfp_hf_startup(profile_on_startup_t cb)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STARTUP, NULL);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = PTR2INT(uint64_t) cb;

    return hfp_hf_send_message(msg);
}

static bt_status_t hfp_hf_shutdown(profile_on_shutdown_t cb)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_SHUTDOWN, NULL);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = PTR2INT(uint64_t) cb;

    return hfp_hf_send_message(msg);
}

static void hfp_hf_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_HFP_OFFLOADING:
        g_hfp_service.offloading = msg->data.valuebool;
        break;
    case PROFILE_EVT_REMOTE_DETACH: {
        bt_instance_t* ins = msg->data.data;

        if (ins->hfp_hf_cookie) {
            BT_LOGD("%s PROFILE_EVT_REMOTE_DETACH", __func__);
            hfp_hf_unregister_callbacks((void**)&ins, ins->hfp_hf_cookie);
            ins->hfp_hf_cookie = NULL;
        }
        break;
    }
    default:
        break;
    }
}

static int hfp_hf_get_state(void)
{
    return 1;
}

static void* hfp_hf_register_callbacks(void* remote, const hfp_hf_callbacks_t* callbacks)
{
    if (!g_hfp_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_hfp_service.callbacks, remote, (void*)callbacks);
}

static bool hfp_hf_unregister_callbacks(void** remote, void* cookie)
{
    if (!g_hfp_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_hfp_service.callbacks, remote, cookie);
}

static bool hfp_hf_is_connected(bt_address_t* addr)
{
    hf_device_t* device = find_hf_device_by_addr(addr);

    if (!device) {
        return false;
    }

    bool connected = hf_state_machine_get_state(device->hfsm) >= HFP_HF_STATE_CONNECTED;

    return connected;
}

static bool hfp_hf_is_audio_connected(bt_address_t* addr)
{
    hf_device_t* device = find_hf_device_by_addr(addr);

    if (!device) {
        return false;
    }

    bool connected = hf_state_machine_get_state(device->hfsm) == HFP_HF_STATE_AUDIO_CONNECTED;

    return connected;
}

static profile_connection_state_t hfp_hf_get_connection_state(bt_address_t* addr)
{
    hf_device_t* device = find_hf_device_by_addr(addr);
    profile_connection_state_t conn_state;
    uint32_t state;

    if (!device)
        return PROFILE_STATE_DISCONNECTED;

    state = hf_state_machine_get_state(device->hfsm);
    if (state == HFP_HF_STATE_DISCONNECTED)
        conn_state = PROFILE_STATE_DISCONNECTED;
    else if (state == HFP_HF_STATE_CONNECTING)
        conn_state = PROFILE_STATE_CONNECTING;
    else if (state == HFP_HF_STATE_DISCONNECTING)
        conn_state = PROFILE_STATE_DISCONNECTING;
    else
        conn_state = PROFILE_STATE_CONNECTED;

    return conn_state;
}

static bt_status_t hfp_hf_connect(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (get_current_connnection_cnt() >= g_hfp_service.max_connections)
        return BT_STATUS_NO_RESOURCES;

    return hfp_hf_send_event(addr, HF_CONNECT);
}

static bt_status_t hfp_hf_disconnect(bt_address_t* addr)
{
    CHECK_ENABLED();
    profile_connection_state_t state = hfp_hf_get_connection_state(addr);
    if (state == PROFILE_STATE_DISCONNECTED || state == PROFILE_STATE_DISCONNECTING)
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_DISCONNECT);
}

static bt_status_t hfp_hf_set_connection_policy(bt_address_t* addr, connection_policy_t policy)
{
    hf_state_machine_t* hfsm;
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    CHECK_ENABLED();

    hfsm = get_state_machine(addr);
    if (!hfsm) {
        bt_addr_ba2str(addr, _addr_str);
        BT_LOGE("Set policy fail, Device:%s not exist", _addr_str)
        return BT_STATUS_FAIL;
    }

    hf_state_machine_set_policy(hfsm, policy);

    if (policy == CONNECTION_POLICY_ALLOWED) {
        hfp_hf_connect(addr);
    } else if (policy == CONNECTION_POLICY_FORBIDDEN) {
        hfp_hf_disconnect(addr);
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t hfp_hf_connect_audio(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr) || hfp_hf_is_audio_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_CONNECT_AUDIO);
}

static bt_status_t hfp_hf_disconnect_audio(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_audio_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_DISCONNECT_AUDIO);
}

static bt_status_t hfp_hf_start_voice_recognition(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_VOICE_RECOGNITION_START);
}

static bt_status_t hfp_hf_stop_voice_recognition(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_VOICE_RECOGNITION_STOP);
}

static bt_status_t hfp_hf_dial(bt_address_t* addr, const char* number)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_DIAL_NUMBER, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    HF_MSG_ADD_STR(msg, 1, number, strlen(number));
    return hfp_hf_send_message(msg);
}

static bt_status_t hfp_hf_dial_memory(bt_address_t* addr, uint32_t memory)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_DIAL_MEMORY, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = memory;
    return hfp_hf_send_message(msg);
}

static bt_status_t hfp_hf_redial(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_DIAL_LAST);
}

static bt_status_t hfp_hf_accept_call(bt_address_t* addr, hfp_call_accept_t flag)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_ACCEPT_CALL, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = flag;
    return hfp_hf_send_message(msg);
}

static bt_status_t hfp_hf_reject_call(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_REJECT_CALL);
}

static bt_status_t hfp_hf_hold_call(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_HOLD_CALL);
}

static bt_status_t hfp_hf_terminate_call(bt_address_t* addr)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_hf_send_event(addr, HF_TERMINATE_CALL);
}

static bt_status_t hfp_hf_control_call(bt_address_t* addr, hfp_call_control_t chld, uint8_t index)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_CONTROL_CALL, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = chld;
    msg->data.valueint2 = index;

    return hfp_hf_send_message(msg);
}

static bt_status_t hfp_hf_query_current_calls(bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    hf_state_machine_t* hfsm = get_state_machine(addr);
    /* get call list from statemachine */
    bt_list_t* call_list = hf_state_machine_get_calls(hfsm);
    *num = bt_list_length(call_list);
    if (!(*num)) {
        return BT_STATUS_SUCCESS;
    }

    if (!allocator((void**)calls, sizeof(hfp_current_call_t) * (*num))) {
        return BT_STATUS_NOMEM;
    }

    bt_list_node_t* node;
    hfp_current_call_t* p = *calls;
    for (node = bt_list_head(call_list); node != NULL; node = bt_list_next(call_list, node)) {
        hfp_current_call_t* call = bt_list_node(node);
        memcpy(p, call, sizeof(hfp_current_call_t));
        p++;
    }

    return BT_STATUS_SUCCESS;
    // return hfp_hf_send_event(addr, QUERY_CURRENT_CALLS);
}

static bt_status_t hfp_hf_send_at_cmd(bt_address_t* addr, const char* cmd)
{
    CHECK_ENABLED();
    if (!hfp_hf_is_connected(addr))
        return BT_STATUS_FAIL;

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_SEND_AT_COMMAND, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    HF_MSG_ADD_STR(msg, 1, cmd, strlen(cmd));
    return hfp_hf_send_message(msg);
}

static bt_status_t hfp_hf_update_battery_level(bt_address_t* addr, uint8_t level)
{
    CHECK_ENABLED();

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_UPDATE_BATTERY_LEVEL, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = level;
    return hfp_hf_send_message(msg);
}

static bt_status_t hfp_hf_volume_control(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    CHECK_ENABLED();

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_SET_VOLUME, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = type;
    msg->data.valueint2 = volume;
    return hfp_hf_send_message(msg);
}

static bt_status_t hfp_hf_send_dtmf(bt_address_t* addr, char dtmf)
{
    CHECK_ENABLED();

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_SEND_DTMF, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = dtmf;
    return hfp_hf_send_message(msg);
}

static const hfp_hf_interface_t HfInterface = {
    sizeof(HfInterface),
    .register_callbacks = hfp_hf_register_callbacks,
    .unregister_callbacks = hfp_hf_unregister_callbacks,
    .is_connected = hfp_hf_is_connected,
    .is_audio_connected = hfp_hf_is_audio_connected,
    .get_connection_state = hfp_hf_get_connection_state,
    .connect = hfp_hf_connect,
    .disconnect = hfp_hf_disconnect,
    .set_connection_policy = hfp_hf_set_connection_policy,
    .connect_audio = hfp_hf_connect_audio,
    .disconnect_audio = hfp_hf_disconnect_audio,
    .start_voice_recognition = hfp_hf_start_voice_recognition,
    .stop_voice_recognition = hfp_hf_stop_voice_recognition,
    .dial = hfp_hf_dial,
    .dial_memory = hfp_hf_dial_memory,
    .redial = hfp_hf_redial,
    .accept_call = hfp_hf_accept_call,
    .reject_call = hfp_hf_reject_call,
    .hold_call = hfp_hf_hold_call,
    .terminate_call = hfp_hf_terminate_call,
    .control_call = hfp_hf_control_call,
    .query_current_calls = hfp_hf_query_current_calls,
    .send_at_cmd = hfp_hf_send_at_cmd,
    .update_battery_level = hfp_hf_update_battery_level,
    .volume_control = hfp_hf_volume_control,
    .send_dtmf = hfp_hf_send_dtmf,
};

static const void* get_hf_profile_interface(void)
{
    return &HfInterface;
}

static int hfp_hf_dump(void)
{
    printf("impl hfp hf dump");
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void hf_service_notify_connection_state_changed(bt_address_t* addr, profile_connection_state_t state)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, connection_state_cb, addr, state);
}

void hf_service_notify_audio_state_changed(bt_address_t* addr, hfp_audio_state_t state)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, audio_state_cb, addr, state);
}

void hf_service_notify_vr_state_changed(bt_address_t* addr, bool started)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, vr_cmd_cb, addr, started);
}

void hf_service_notify_call_state_changed(bt_address_t* addr, hfp_current_call_t* call)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, call_state_changed_cb, addr, call);
}

void hf_service_notify_cmd_complete(bt_address_t* addr, const char* resp)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, cmd_complete_cb, addr, resp);
}

void hf_service_notify_ring_indication(bt_address_t* addr, bool inband_ring_tone)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, ring_indication_cb, addr, inband_ring_tone);
}

void hf_service_notify_volume_changed(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, volume_changed_cb, addr, type, volume);
}

void hf_service_notify_call(bt_address_t* addr, hfp_call_t call)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, call_cb, addr, call);
}

void hf_service_notify_callsetup(bt_address_t* addr, hfp_callsetup_t callsetup)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, callsetup_cb, addr, callsetup);
}

void hf_service_notify_callheld(bt_address_t* addr, hfp_callheld_t callheld)
{
    BT_LOGD("%s", __func__);
    HF_CALLBACK_FOREACH(g_hfp_service.callbacks, callheld_cb, addr, callheld);
}

void hfp_hf_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state,
    profile_connection_reason_t reason, uint32_t remote_features)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CONNECTION_STATE_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    msg->data.valueint2 = reason;
    msg->data.valueint3 = remote_features;
    hfp_hf_send_message(msg);
}

void hfp_hf_on_audio_connection_state_changed(bt_address_t* addr,
    hfp_audio_state_t state,
    uint16_t sco_connection_handle)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_AUDIO_STATE_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    msg->data.valueint2 = sco_connection_handle;

    hfp_hf_send_message(msg);
}

void hfp_hf_on_codec_changed(bt_address_t* addr, hfp_codec_config_t* config)
{
    BT_LOGD("HF codec config [codec:%d][sample rate:%" PRIu32 "][bit width:%d]", config->codec,
        config->sample_rate, config->bit_width);

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CODEC_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = config->codec;
    hfp_hf_send_message(msg);
}

void hfp_hf_on_call_setup_state_changed(bt_address_t* addr, hfp_callsetup_t setup)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CALLSETUP, addr);
    if (!msg)
        return;

    msg->data.valueint1 = setup;
    hfp_hf_send_message(msg);
}

void hfp_hf_on_call_active_state_changed(bt_address_t* addr, hfp_call_t state)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CALL, addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    hfp_hf_send_message(msg);
}

void hfp_hf_on_call_held_state_changed(bt_address_t* addr, hfp_callheld_t state)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CALLHELD, addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    hfp_hf_send_message(msg);
}

void hfp_hf_on_volume_changed(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_VOLUME_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = type;
    msg->data.valueint2 = volume;

    hfp_hf_send_message(msg);
}

void hfp_hf_on_ring_active_state_changed(bt_address_t* addr, bool active, hfp_in_band_ring_state_t inband_ring)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_RING_INDICATION, addr);
    if (!msg)
        return;

    msg->data.valueint1 = active;
    msg->data.valueint2 = inband_ring;

    hfp_hf_send_message(msg);
}

void hfp_hf_on_voice_recognition_state_changed(bt_address_t* addr, bool started)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_VR_STATE_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = started;
    hfp_hf_send_message(msg);
}

void hfp_hf_on_received_at_cmd_resp(bt_address_t* addr, char* response, uint16_t response_length)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CMD_RESPONSE, addr);
    if (!msg)
        return;

    HF_MSG_ADD_STR(msg, 1, response, response_length);
    hfp_hf_send_message(msg);
}

void hfp_hf_on_received_sco_connection_req(bt_address_t* addr)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_AUDIO_REQ, addr);
    if (!msg)
        return;

    hfp_hf_send_message(msg);
}

void hfp_hf_on_clip(bt_address_t* addr, const char* number, const char* name)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CLIP, addr);
    if (!msg)
        return;

    HF_MSG_ADD_STR(msg, 1, number, strlen(number));
    HF_MSG_ADD_STR(msg, 2, name, strlen(name));

    hfp_hf_send_message(msg);
}

void hfp_hf_on_current_call_response(bt_address_t* addr, uint32_t idx,
    hfp_call_direction_t dir,
    hfp_hf_call_state_t status,
    hfp_call_mpty_type_t mpty,
    const char* number, uint32_t type)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CURRENT_CALLS, addr);
    if (!msg)
        return;

    msg->data.valueint1 = idx;
    msg->data.valueint2 = dir;
    msg->data.valueint3 = status;
    msg->data.valueint4 = mpty;
    HF_MSG_ADD_STR(msg, 1, number, strlen(number));

    hfp_hf_send_message(msg);
}

void hfp_hf_on_at_command_result_response(bt_address_t* addr, uint32_t at_cmd_code, uint32_t result)
{
    switch (at_cmd_code) {
    case HFP_ATCMD_CODE_ATD:
    case HFP_ATCMD_CODE_BLDN:
        break;
    default:
        return;
    }

    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STACK_EVENT_CMD_RESULT, addr);
    if (!msg)
        return;

    msg->data.valueint1 = at_cmd_code;
    msg->data.valueint2 = result;
    hfp_hf_send_message(msg);
}

static const profile_service_t hfp_hf_service = {
    .auto_start = true,
    .name = PROFILE_HFP_HF_NAME,
    .id = PROFILE_HFP_HF,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = hfp_hf_init,
    .startup = hfp_hf_startup,
    .shutdown = hfp_hf_shutdown,
    .process_msg = hfp_hf_process_msg,
    .get_state = hfp_hf_get_state,
    .get_profile_interface = get_hf_profile_interface,
    .cleanup = hfp_hf_cleanup,
    .dump = hfp_hf_dump,
};

void register_hfp_hf_service(void)
{
    register_service(&hfp_hf_service);
}
