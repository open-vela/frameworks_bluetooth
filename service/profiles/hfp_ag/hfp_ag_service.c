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
#define LOG_TAG "hfp_ag"
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <sys/types.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "bt_hfp_ag.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "hfp_ag_event.h"
#include "hfp_ag_service.h"
#include "hfp_ag_state_machine.h"
#include "hfp_ag_tele_service.h"
#include "sal_hfp_ag_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifndef CONFIG_HFP_AG_MAX_CONNECTIONS
#define CONFIG_HFP_AG_MAX_CONNECTIONS 1
#endif

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_ag_service.started)        \
            return BT_STATUS_NOT_ENABLED; \
    }

#define AG_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, hfp_ag_callbacks_t, _cback, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct
{
    bool started;
    uint8_t max_connections;
    bt_list_t *ag_devices;
    callbacks_list_t *callbacks;
    pthread_mutex_t device_lock;
} ag_service_t;

typedef struct
{
    bt_address_t addr;
    ag_state_machine_t *agsm;
} ag_device_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static ag_service_t g_ag_service = {
    .started = false,
    .ag_devices = NULL,
    .callbacks = NULL,
};

static uint32_t ag_support_features = HFP_BRSF_AG_HFINDICATORS | HFP_BRSF_AG_ENHANCED_CALLSTATUS |
                                      HFP_BRSF_AG_3WAYCALL | HFP_BRSF_AG_ENHANCED_CALLCONTROL |
                                      HFP_BRSF_AG_REJECT_CALL | HFP_BRSF_AG_EXTENDED_ERRORRESULT |
                                      HFP_BRSF_AG_CODEC_NEGOTIATION | HFP_BRSF_AG_eSCO_S4T2_SETTING;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool ag_device_cmp(void *device, void *addr)
{
    return bt_addr_compare(&((ag_device_t *)device)->addr, addr) == 0;
}

static ag_device_t *find_ag_device_by_addr(bt_address_t *addr)
{
    return bt_list_find(g_ag_service.ag_devices, ag_device_cmp, addr);
}

static ag_device_t *ag_device_new(bt_address_t *addr, ag_state_machine_t *agsm)
{
    ag_device_t *device = malloc(sizeof(ag_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->agsm = agsm;

    return device;
}

static void ag_device_delete(ag_device_t *device)
{
    if (!device)
        return;

    hfp_ag_msg_t *msg = hfp_ag_msg_new(DISCONNECT, &device->addr);
    if (msg == NULL)
        return;

    ag_state_machine_dispatch(device->agsm, msg);
    ag_state_machine_destory(device->agsm);
    hfp_ag_msg_destory(msg);
    free(device);
}

static ag_state_machine_t *get_state_machine(bt_address_t *addr)
{
    ag_state_machine_t *agsm;
    ag_device_t *device;

    if (!g_ag_service.started)
        return NULL;

    device = find_ag_device_by_addr(addr);
    if (device)
        return device->agsm;

    agsm = ag_state_machine_new(addr, (void *)&g_ag_service);
    if (!agsm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    device = ag_device_new(addr, agsm);
    if (!device) {
        BT_LOGE("New device alloc failed");
        ag_state_machine_destory(agsm);
        return NULL;
    }

    bt_list_add_tail(g_ag_service.ag_devices, device);

    return agsm;
}

static uint8_t get_current_connnection_cnt(void)
{
    bt_list_t *list = g_ag_service.ag_devices;
    bt_list_node_t *node;
    uint8_t cnt = 0;

    pthread_mutex_lock(&g_ag_service.device_lock);
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        ag_device_t *device = bt_list_node(node);
        if (ag_state_machine_get_state(device->agsm) >= HFP_AG_STATE_CONNECTED || ag_state_machine_get_state(device->agsm) == HFP_AG_STATE_CONNECTING)
            cnt++;
    }
    pthread_mutex_unlock(&g_ag_service.device_lock);

    return cnt;
}

static uint32_t get_ag_features(void)
{
#ifdef CONFIG_KVDB
    return property_get_int32("persist.bluetooth.hfp.ag_features", ag_support_features);
#else
    return ag_support_features;
#endif
}

static void ag_shutdown(void)
{
    if (!g_ag_service.started)
        return;

    pthread_mutex_lock(&g_ag_service.device_lock);
    g_ag_service.started = false;
    bt_list_free(g_ag_service.ag_devices);
    g_ag_service.ag_devices = NULL;
    pthread_mutex_unlock(&g_ag_service.device_lock);
    pthread_mutex_destroy(&g_ag_service.device_lock);
    bt_callbacks_list_free(g_ag_service.callbacks);
    g_ag_service.callbacks = NULL;
    bt_sal_hfp_ag_cleanup();
}

static void ag_dispatch_msg_foreach(void *data, void *context)
{
    ag_device_t *device = (ag_device_t *)data;

    ag_state_machine_dispatch(device->agsm, (hfp_ag_msg_t *)context);
}

static void hfp_ag_process_message(void *data)
{
    hfp_ag_msg_t *msg = (hfp_ag_msg_t *)data;

    switch (msg->event) {
    case SHUTDOWN:
        ag_shutdown();
        break;
    case DEVICE_STATUS_CHANGED:
    case PHONE_STATE_CHANGE:
    case SET_VOLUME:
    case SET_INBAND_RING_ENABLE:
    case DIALING_RESULT:
        pthread_mutex_lock(&g_ag_service.device_lock);
        bt_list_foreach(g_ag_service.ag_devices, ag_dispatch_msg_foreach, msg);
        pthread_mutex_unlock(&g_ag_service.device_lock);
        break;
    default: {
        pthread_mutex_lock(&g_ag_service.device_lock);
        ag_state_machine_t *agsm = get_state_machine(&msg->data.addr);
        if (agsm)
            ag_state_machine_dispatch(agsm, msg);
        pthread_mutex_unlock(&g_ag_service.device_lock);
        break;
    }
    }

    hfp_ag_msg_destory(msg);
}

static bt_status_t hfp_ag_send_message(hfp_ag_msg_t *msg)
{
    assert(msg);

    do_in_service_loop(hfp_ag_process_message, msg);

    return BT_STATUS_SUCCESS;
}

bt_status_t hfp_ag_send_event(bt_address_t *addr, hfp_ag_event_t evt)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(evt, addr);

    if (!msg)
        return BT_STATUS_NOMEM;

    return hfp_ag_send_message(msg);
}

static bt_status_t hfp_ag_init(void)
{
    BT_LOGD("%s", __func__);
    return BT_STATUS_SUCCESS;
}

static bt_status_t hfp_ag_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    pthread_mutexattr_t attr;
    ag_service_t *service = &g_ag_service;

    BT_LOGD("%s", __func__);
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->max_connections = CONFIG_HFP_AG_MAX_CONNECTIONS;
    service->ag_devices = bt_list_new((bt_list_free_cb_t)ag_device_delete);
    service->callbacks = bt_callbacks_list_new(2);
    if (!service->ag_devices || !service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->device_lock, &attr);

    status = bt_sal_hfp_ag_init(get_ag_features(), service->max_connections);
    if (status != BT_STATUS_SUCCESS)
        goto fail;

    tele_service_init();
    service->started = true;
    return BT_STATUS_SUCCESS;
fail:
    bt_list_free(service->ag_devices);
    bt_callbacks_list_free(service->callbacks);
    pthread_mutex_destroy(&service->device_lock);
    return status;
}

static bt_status_t hfp_ag_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);

    return hfp_ag_send_event(NULL, SHUTDOWN);
}

static void hfp_ag_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static void *hfp_ag_register_callbacks(void *remote, const hfp_ag_callbacks_t *callbacks)
{
    if (!g_ag_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_ag_service.callbacks, remote, (void *)callbacks);
}

static bool hfp_ag_unregister_callbacks(void **remote, void *cookie)
{
    if (!g_ag_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_ag_service.callbacks, remote, cookie);
}

static bool hfp_ag_is_connected(bt_address_t *addr)
{
    pthread_mutex_lock(&g_ag_service.device_lock);
    ag_device_t *device = find_ag_device_by_addr(addr);

    if (!device) {
        pthread_mutex_unlock(&g_ag_service.device_lock);
        return false;
    }

    bool connected = ag_state_machine_get_state(device->agsm) >= HFP_AG_STATE_CONNECTED;
    pthread_mutex_unlock(&g_ag_service.device_lock);

    return connected;
}

static bool hfp_ag_is_audio_connected(bt_address_t *addr)
{
    pthread_mutex_lock(&g_ag_service.device_lock);
    ag_device_t *device = find_ag_device_by_addr(addr);

    if (!device) {
        pthread_mutex_unlock(&g_ag_service.device_lock);
        return false;
    }

    bool connected = ag_state_machine_get_state(device->agsm) == HFP_AG_STATE_AUDIO_CONNECTED;
    pthread_mutex_unlock(&g_ag_service.device_lock);

    return connected;
}

static profile_connection_state_t hfp_ag_get_connection_state(bt_address_t *addr)
{
    ag_device_t *device = find_ag_device_by_addr(addr);
    profile_connection_state_t conn_state;
    uint32_t state;

    if (!device)
        return PROFILE_STATE_DISCONNECTED;

    pthread_mutex_lock(&g_ag_service.device_lock);
    state = ag_state_machine_get_state(device->agsm);
    if (state == HFP_AG_STATE_DISCONNECTED)
        conn_state = PROFILE_STATE_DISCONNECTED;
    else if (state == HFP_AG_STATE_CONNECTING)
        conn_state = PROFILE_STATE_CONNECTING;
    else if (state == HFP_AG_STATE_DISCONNECTING)
        conn_state = PROFILE_STATE_DISCONNECTING;
    else
        conn_state = PROFILE_STATE_CONNECTED;
    pthread_mutex_unlock(&g_ag_service.device_lock);

    return conn_state;
}

static bt_status_t hfp_ag_connect(bt_address_t *addr)
{
    CHECK_ENABLED();
    if (get_current_connnection_cnt() >= g_ag_service.max_connections)
        return BT_STATUS_NO_RESOURCES;

    return hfp_ag_send_event(addr, CONNECT);
}

static bt_status_t hfp_ag_disconnect(bt_address_t *addr)
{
    CHECK_ENABLED();
    profile_connection_state_t state = hfp_ag_get_connection_state(addr);
    if (state == PROFILE_STATE_DISCONNECTED || state == PROFILE_STATE_DISCONNECTING)
        return BT_STATUS_FAIL;

    return hfp_ag_send_event(addr, DISCONNECT);
}

static bt_status_t hfp_ag_connect_audio(bt_address_t *addr)
{
    CHECK_ENABLED();
    if (!hfp_ag_is_connected(addr) || hfp_ag_is_audio_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_ag_send_event(addr, CONNECT_AUDIO);
}

static bt_status_t hfp_ag_disconnect_audio(bt_address_t *addr)
{
    CHECK_ENABLED();
    if (!hfp_ag_is_audio_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_ag_send_event(addr, DISCONNECT_AUDIO);
}

static bt_status_t hfp_ag_start_voice_recognition(bt_address_t *addr)
{
    CHECK_ENABLED();
    if (!hfp_ag_is_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_ag_send_event(addr, VOICE_RECOGNITION_START);
}

static bt_status_t hfp_ag_stop_voice_recognition(bt_address_t *addr)
{
    CHECK_ENABLED();
    if (!hfp_ag_is_connected(addr))
        return BT_STATUS_FAIL;

    return hfp_ag_send_event(addr, VOICE_RECOGNITION_STOP);
}

bt_status_t hfp_ag_phone_state_change(uint8_t num_active, uint8_t num_held,
                                      hfp_ag_call_state_t call_state,
                                      hfp_call_addrtype_t type, const char *number,
                                      const char *name)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(PHONE_STATE_CHANGE, NULL);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = num_active;
    msg->data.valueint2 = num_held;
    msg->data.valueint3 = call_state;
    msg->data.valueint4 = type;
    AG_MSG_ADD_STR(msg, 1, number, strlen(number));
    AG_MSG_ADD_STR(msg, 2, name, strlen(name));

    return hfp_ag_send_message(msg);
}

bt_status_t hfp_ag_device_status_changed(hfp_network_state_t network,
                                         hfp_roaming_state_t roam,
                                         uint8_t signal, uint8_t battery)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(DEVICE_STATUS_CHANGED, NULL);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = network;
    msg->data.valueint2 = roam;
    msg->data.valueint3 = signal;
    msg->data.valueint4 = battery;

    return hfp_ag_send_message(msg);
}

bt_status_t hfp_ag_dial_result(uint8_t result)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(DIALING_RESULT, NULL);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = result;

    return hfp_ag_send_message(msg);
}

static const hfp_ag_interface_t agInterface = {
    .size = sizeof(agInterface),
    .register_callbacks = hfp_ag_register_callbacks,
    .unregister_callbacks = hfp_ag_unregister_callbacks,
    .is_connected = hfp_ag_is_connected,
    .is_audio_connected = hfp_ag_is_audio_connected,
    .get_connection_state = hfp_ag_get_connection_state,
    .connect = hfp_ag_connect,
    .disconnect = hfp_ag_disconnect,
    .connect_audio = hfp_ag_connect_audio,
    .disconnect_audio = hfp_ag_disconnect_audio,
    .start_voice_recognition = hfp_ag_start_voice_recognition,
    .stop_voice_recognition = hfp_ag_stop_voice_recognition,
    .phone_state_change = hfp_ag_phone_state_change,
    .device_status_changed = hfp_ag_device_status_changed,
    .dial_response = hfp_ag_dial_result,
};

static const void *get_ag_profile_interface(void)
{
    return &agInterface;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void ag_service_notify_connection_state_changed(bt_address_t *addr, profile_connection_state_t state)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_CALLBACK_FOREACH(g_ag_service.callbacks, connection_state_cb, addr, state);
}

void ag_service_notify_audio_state_changed(bt_address_t *addr, hfp_audio_state_t state)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_CALLBACK_FOREACH(g_ag_service.callbacks, audio_state_cb, addr, state);
}

void ag_service_notify_vr_state_changed(bt_address_t *addr, bool started)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_CALLBACK_FOREACH(g_ag_service.callbacks, vr_cmd_cb, addr, started);
}

void ag_service_notify_hf_battery_update(bt_address_t *addr, uint8_t value)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_CALLBACK_FOREACH(g_ag_service.callbacks, hf_battery_update_cb, addr, value);
}

void hfp_ag_on_connection_state_changed(bt_address_t *addr, profile_connection_state_t state)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_CONNECTION_STATE_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    hfp_ag_send_message(msg);
}

void hfp_ag_on_audio_state_changed(bt_address_t *addr, hfp_audio_state_t state)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_AUDIO_STATE_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    hfp_ag_send_message(msg);
}

void hfp_ag_on_codec_changed(bt_address_t *addr, hfp_codec_config_t *config)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_CODEC_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = config->codec;
    hfp_ag_send_message(msg);
}

void hfp_ag_on_volume_changed(bt_address_t *addr, hfp_volume_type_t type, uint8_t volume)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_VOLUME_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = type;
    msg->data.valueint2 = volume;
    hfp_ag_send_message(msg);
}

void hfp_ag_on_received_cind_request(bt_address_t *addr)
{
    hfp_ag_send_event(addr, STACK_EVENT_AT_CIND_REQUEST);
}

void hfp_ag_on_received_clcc_request(bt_address_t *addr)
{
    hfp_ag_send_event(addr, STACK_EVENT_AT_CLCC_REQUEST);
}

void hfp_ag_on_received_cops_request(bt_address_t *addr)
{
    hfp_ag_send_event(addr, STACK_EVENT_AT_COPS_REQUEST);
}

void hfp_ag_on_voice_recognition_state_changed(bt_address_t *addr, bool started)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_VR_STATE_CHANGED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = started;
    hfp_ag_send_message(msg);
}

void hfp_ag_on_remote_battery_level_update(bt_address_t *addr, uint8_t value)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_BATTERY_UPDATE, addr);
    if (!msg)
        return;

    msg->data.valueint1 = value;
    hfp_ag_send_message(msg);
}

void hfp_ag_on_answer_call(bt_address_t *addr)
{
    hfp_ag_send_event(addr, STACK_EVENT_ANSWER_CALL);
}

void hfp_ag_on_reject_call(bt_address_t *addr)
{
    hfp_ag_send_event(addr, STACK_EVENT_REJECT_CALL);
}

void hfp_ag_on_hangup_call(bt_address_t *addr)
{
    hfp_ag_send_event(addr, STACK_EVENT_HANGUP_CALL);
}

void hfp_ag_on_received_at_cmd(bt_address_t *addr, char *at_string, uint16_t at_length)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_AT_COMMAND, addr);
    if (!msg)
        return;

    AG_MSG_ADD_STR(msg, 1, at_string, at_length);
    hfp_ag_send_message(msg);
}

void hfp_ag_on_audio_connect_request(bt_address_t *addr)
{
    hfp_ag_send_event(addr, STACK_EVENT_AUDIO_REQ);
}

void hfp_ag_on_dial_number(bt_address_t *addr, char *number, uint32_t length)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_DIAL_NUMBER, addr);
    if (!msg)
        return;

    AG_MSG_ADD_STR(msg, 1, number, length);
    hfp_ag_send_message(msg);
}

void hfp_ag_on_dial_memory(bt_address_t *addr, uint32_t location)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_DIAL_MEMORY, addr);
    if (!msg)
        return;

    msg->data.valueint1 = location;
    hfp_ag_send_message(msg);
}

void hfp_ag_on_call_control(bt_address_t *addr, hfp_call_control_t control)
{
    hfp_ag_msg_t *msg = hfp_ag_msg_new(STACK_EVENT_CALL_CONTROL, addr);
    if (!msg)
        return;

    msg->data.valueint1 = control;
    hfp_ag_send_message(msg);
}

void hfp_ag_on_received_dtmf(bt_address_t *addr, char tone)
{
}

void hfp_ag_on_received_manufacture_request(bt_address_t *addr)
{
    bt_sal_hfp_ag_manufacture_id_response(addr, "xiaomi", strlen("xiaomi"));
}

void hfp_ag_on_received_model_id_request(bt_address_t *addr)
{
    bt_sal_hfp_ag_model_id_response(addr, "2109119BC", strlen("2109119BC"));
}

static const profile_service_t hfp_ag_service = {
    .auto_start = true,
    .name = "hfp_ag",
    .id = PROFILE_HFP_AG,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = hfp_ag_init,
    .startup = hfp_ag_startup,
    .shutdown = hfp_ag_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_ag_profile_interface,
    .cleanup = hfp_ag_cleanup,
    .dump = NULL,
};

void register_hfp_ag_service(void)
{
    register_service(&hfp_ag_service);
}