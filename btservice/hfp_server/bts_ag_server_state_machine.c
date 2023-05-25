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
#define LOG_TAG "ag_stm"

#include <nuttx/list.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef CONFIG_UORB
#include <connectivity/bt.h>
#include <uORB/uORB.h>
#endif

#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"
#include "stack_adapter_hfp.h"
#include "stack_adapter_hfp_ag.h"

#include "stack_adapter_service_base.h"

#include "btm_hfp_ag.h"
#include "bts_ag_server_event.h"
#include "bts_ag_server_state_machine.h"
#include "utils/log.h"
#include "utils/utils.h"

#define BT_ADDR_STR_LENGTH 18

#define AG_SERVICE_CBACK(P_CB, P_CBACK, ...) \
    do {                                     \
        if ((P_CB) && (P_CB)->P_CBACK) {     \
            (P_CB)->P_CBACK(__VA_ARGS__);    \
        }                                    \
    } while (0)

typedef struct service_timer {
    uv_timer_t handle;
    service_timer_cb_t callback;
    void* userdata;
} service_timer_t;

typedef struct _ag_state_machine {
    state_machine_t sm;
    bt_address bd_addr;
    uint16_t sco_conn_handle;
    bool recognition_active;
    ag_server_service_t* service;
    uint8_t codec;
    uint8_t volume;
    uv_timer_t* connect_timer;
    uv_timer_t* audio_timer;
    uv_timer_t* dial_out_timer;
} ag_state_machine_t;

#define AG_TIMEOUT 10000
#define AG_STM_DEBUG 1
#define BT_ADDR_LOG(fmt, _addr, ...)                \
    do {                                            \
        char _addr_str[BT_ADDR_STR_LENGTH] = { 0 }; \
        ba2str(_addr, _addr_str);                   \
        BT_LOGD(fmt, _addr_str, ##__VA_ARGS__);     \
    } while (0);

#if AG_STM_DEBUG
static void ag_stm_trans_debug(state_machine_t* sm, bt_address bd_addr, const char* action);
static void ag_stm_event_debug(state_machine_t* sm, bt_address bd_addr, uint32_t event);
static const char* stack_event_to_string(ag_server_event_t event);

#define AG_DBG_ENTER(__sm, __addr) ag_stm_trans_debug(__sm, __addr, "Enter")
#define AG_DBG_EXIT(__sm, __addr) ag_stm_trans_debug(__sm, __addr, "Exit ")
#define AG_DBG_EVENT(__sm, __addr, __event) ag_stm_event_debug(__sm, __addr, __event);
#else
#define AG_DBG_ENTER(__sm, __addr)
#define AG_DBG_EXIT(__sm, __addr)
#define AG_DBG_EVENT(__sm, __addr, __event)
#endif

static void disconnected_enter(state_machine_t* sm);
static void disconnected_exit(state_machine_t* sm);
static void connecting_enter(state_machine_t* sm);
static void connecting_exit(state_machine_t* sm);
static void disconnecting_enter(state_machine_t* sm);
static void disconnecting_exit(state_machine_t* sm);
static void connected_enter(state_machine_t* sm);
static void connected_exit(state_machine_t* sm);
static void audio_connecting_enter(state_machine_t* sm);
static void audio_connecting_exit(state_machine_t* sm);
static void audio_on_enter(state_machine_t* sm);
static void audio_on_exit(state_machine_t* sm);
static void audio_disconnecting_enter(state_machine_t* sm);
static void audio_disconnecting_exit(state_machine_t* sm);

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool disconnecting_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool audio_connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool audio_on_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool audio_disconnecting_process_event(state_machine_t* sm, uint32_t event, void* p_data);

static const state_t disconnected_state = {
    .state_name = "Disconnected",
    .enter = disconnected_enter,
    .exit = disconnected_exit,
    .process_event = disconnected_process_event,
};

static const state_t connecting_state = {
    .state_name = "Connecting",
    .enter = connecting_enter,
    .exit = connecting_exit,
    .process_event = connecting_process_event,
};

static const state_t disconnecting_state = {
    .state_name = "Disconnecting",
    .enter = disconnecting_enter,
    .exit = disconnecting_exit,
    .process_event = disconnecting_process_event,
};

static const state_t connected_state = {
    .state_name = "Connected",
    .enter = connected_enter,
    .exit = connected_exit,
    .process_event = connected_process_event,
};

static const state_t audio_connecting_state = {
    .state_name = "AudioConnecting",
    .enter = audio_connecting_enter,
    .exit = audio_connecting_exit,
    .process_event = audio_connecting_process_event,
};

static const state_t audio_on_state = {
    .state_name = "AudioOn",
    .enter = audio_on_enter,
    .exit = audio_on_exit,
    .process_event = audio_on_process_event,
};

static const state_t audio_disconnecting_state = {
    .state_name = "AudioDisconnecting",
    .enter = audio_disconnecting_enter,
    .exit = audio_disconnecting_exit,
    .process_event = audio_disconnecting_process_event,
};

#if AG_STM_DEBUG
static void ag_stm_trans_debug(state_machine_t* sm, bt_address bd_addr, const char* action)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    ba2str(bd_addr, addr_str);
    BT_LOGD("%s State=%s, Peer=[%s]", action, hsm_get_current_state_name(sm), addr_str);
}

static void ag_stm_event_debug(state_machine_t* sm, bt_address bd_addr, uint32_t event)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    ba2str(bd_addr, addr_str);
    BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(sm),
        addr_str, stack_event_to_string(event));
}

static const char* stack_event_to_string(ag_server_event_t event)
{
    static char ag_evt[32] = { 0 };

    switch (event) {
        CASE_RETURN_STR(CONNECT)
        CASE_RETURN_STR(DISCONNECT)
        CASE_RETURN_STR(CONNECT_AUDIO)
        CASE_RETURN_STR(DISCONNECT_AUDIO)
        CASE_RETURN_STR(VOICE_RECOGNITION_START)
        CASE_RETURN_STR(VOICE_RECOGNITION_STOP)
        CASE_RETURN_STR(PHONE_STATE_CHANGE)
        CASE_RETURN_STR(DEVICE_STATUS_CHANGED)
        CASE_RETURN_STR(SET_VOLUME)
        CASE_RETURN_STR(SET_INBAND_RING_ENABLE)
        CASE_RETURN_STR(DIALING_RESULT)
        CASE_RETURN_STR(SEND_AT_COMMAND)
        CASE_RETURN_STR(STARTUP)
        CASE_RETURN_STR(CLEANUP)
        CASE_RETURN_STR(CONNECT_TIMEOUT)
        CASE_RETURN_STR(AUDIO_TIMEOUT)
        CASE_RETURN_STR(STACK_EVENT)
        CASE_RETURN_STR(STACK_EVENT_AUDIO_REQ)
        CASE_RETURN_STR(STACK_EVENT_CONNECTION_STATE_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_AUDIO_STATE_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_VR_STATE_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_CODEC_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_VOLUME_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_AT_CIND_REQUEST)
        CASE_RETURN_STR(STACK_EVENT_AT_CLCC_REQUEST)
        CASE_RETURN_STR(STACK_EVENT_AT_COPS_REQUEST)
        CASE_RETURN_STR(STACK_EVENT_BATTERY_UPDATE)
        CASE_RETURN_STR(STACK_EVENT_ANSWER_CALL)
        CASE_RETURN_STR(STACK_EVENT_REJECT_CALL)
        CASE_RETURN_STR(STACK_EVENT_HANGUP_CALL)
        CASE_RETURN_STR(STACK_EVENT_DIAL_NUMBER)
        CASE_RETURN_STR(STACK_EVENT_DIAL_MEMORY)
        CASE_RETURN_STR(STACK_EVENT_CALL_CONTROL)
        CASE_RETURN_STR(STACK_EVENT_AT_COMMAND)
        CASE_RETURN_STR(STACK_EVENT_SEND_DTMF)
        CASE_RETURN_STR(CIND_RESP)
        CASE_RETURN_STR(CLCC_RESP)
        CASE_RETURN_STR(COPS_RESP)
    default:
        snprintf(ag_evt, 32, "UNKNOWN_AG_EVENT:%d", event);
        return (const char*)ag_evt;
    }
}
#endif

#ifdef CONFIG_UORB
static void broadcast_hfp_state(int orb_fd, bt_address addr, int conn_state, int audio_state)
{
    struct hfp_state uORB_state;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    uORB_state.timestamp = ts.tv_sec * 1000 + ts.tv_nsec / 1000000UL;
    uORB_state.conn_state = conn_state;
    uORB_state.audio_state = audio_state;
    memcpy(uORB_state.addr, addr, 6);
    if (orb_fd > 0) {
        int ret = orb_publish(ORB_ID(hfp_state), orb_fd, &uORB_state);
        if (ret != 0)
            BT_LOGE("Failed to publish state");
    }
}
#endif

void ag_service_notify_connection_state_changed(ag_server_service_t* service,
    bt_address bd_addr,
    profile_connection_state_t state)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, connection_state_cb, bd_addr, state);
#ifdef CONFIG_UORB
    broadcast_hfp_state(service->orb_fd, bd_addr, state, HFP_AUDIO_STATE_DISCONNECTED);
#endif
}

void ag_service_notify_audio_state_changed(ag_server_service_t* service,
    bt_address bd_addr,
    hfp_audio_state_t state)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, audio_state_cb, bd_addr, state);
#ifdef CONFIG_UORB
    broadcast_hfp_state(service->orb_fd, bd_addr, PROFILE_STATE_CONNECTED, state);
#endif
}

void ag_service_notify_vr_state_changed(ag_server_service_t* service,
    bt_address bd_addr,
    bool started)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, vr_cmd_cb, bd_addr, started);
}

void ag_service_notify_ag_battery_update(ag_server_service_t* service,
    bt_address bd_addr,
    uint8_t value)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, ag_battery_update_cb, bd_addr, value);
}

void ag_service_notify_ag_answer_call(ag_server_service_t* service,
    bt_address bd_addr)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, answer_call_cb, bd_addr);
}

void ag_service_notify_ag_reject_call(ag_server_service_t* service,
    bt_address bd_addr)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, reject_call_cb, bd_addr);
}

void ag_service_notify_ag_hangup_call(ag_server_service_t* service,
    bt_address bd_addr)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, hangup_call_cb, bd_addr);
}

void ag_service_notify_ag_dial_number(ag_server_service_t* service,
    bt_address bd_addr,
    char* number)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, dial_number_cb, bd_addr, number);
}

void ag_service_notify_ag_call_control(ag_server_service_t* service,
    bt_address bd_addr,
    uint8_t chld)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, call_control_cb, bd_addr, chld);
}

void ag_service_notify_ag_cind(ag_server_service_t* service,
    bt_address bd_addr)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, cind_cb, bd_addr);
}

void ag_service_notify_ag_clcc(ag_server_service_t* service,
    bt_address bd_addr)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, clcc_cb, bd_addr);
}

void ag_service_notify_ag_cops(ag_server_service_t* service,
    bt_address bd_addr)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, cops_cb, bd_addr);
}

void ag_service_notify_ag_at_command(ag_server_service_t* service,
    bt_address bd_addr,
    char* at_command)
{
    BT_LOGD("%s", __FUNCTION__);
    AG_SERVICE_CBACK(service->callbacks, at_command_cb, bd_addr, at_command);
}

static void disconnected_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, agsm->bd_addr);
    if (hsm_get_previous_state(sm))
        ag_service_notify_connection_state_changed(agsm->service,
            agsm->bd_addr,
            PROFILE_STATE_DISCONNECTED);
}

static void disconnected_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, agsm->bd_addr);
}

static bool disconnected_process_event(state_machine_t* sm,
    uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    ag_server_data_t* data = (ag_server_data_t*)p_data;
    AG_DBG_EVENT(sm, agsm->bd_addr, event);

    switch (event) {
    case CONNECT:
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_connect %s", agsm->bd_addr);
        if (service_adapter_hfp_ag_connect(agsm->bd_addr) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("Connect failed for %s", agsm->bd_addr);
            ag_service_notify_connection_state_changed(agsm->service,
                agsm->bd_addr,
                PROFILE_STATE_DISCONNECTED);
            return false;
        }
        hsm_transition_to(sm, &connecting_state);
        break;
    case STACK_EVENT_CONNECTION_STATE_CHANGED: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_CONNECTED:
            hsm_transition_to(sm, &connected_state);
            break;
        case PROFILE_STATE_CONNECTING:
            hsm_transition_to(sm, &connecting_state);
            break;
        case PROFILE_STATE_DISCONNECTED:
        case PROFILE_STATE_DISCONNECTING:
            BT_LOGW("Ignored connection state:%d", state);
            break;
        }
    } break;
    case CIND_RESP: {
        BT_ADDR_LOG("stm hfp ag CIND_RESP %s", agsm->bd_addr);
        SERVICE_HFP_AG_CIND_RESPONSE_S resp = {};
        resp.device_status.service = data->valueint1;
        resp.device_status.signal = data->valueint2;
        resp.device_status.roam = data->valueint3;
        resp.device_status.battery = data->valueint4;

        resp.call = data->valueint5;
        resp.call_setup = data->valueint6;
        resp.call_held = data->valueint7;
        BT_ADDR_LOG("service_adapter_hfp_ag_cind_response %s", agsm->bd_addr);
        service_adapter_hfp_ag_cind_response(agsm->bd_addr, resp);
        break;
    }

    default:
        BT_LOGE("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static void ag_connect_timeout_callback(char* data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)data;

    ag_server_msg_t* msg = ag_server_msg_new(CONNECT_TIMEOUT, agsm->bd_addr);
    if (msg == NULL) {
        BT_LOGW("%s malloc failed", __func__);
        return;
    }

    ag_server_state_machine_handle_msg(agsm, msg);
    ag_server_msg_destory(msg);
}

static void connecting_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, agsm->bd_addr);
    agsm->connect_timer = start_timer(AG_TIMEOUT, 0, ag_connect_timeout_callback, agsm);

    ag_service_notify_connection_state_changed(agsm->service,
        agsm->bd_addr,
        PROFILE_STATE_CONNECTING);
}

static void connecting_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, agsm->bd_addr);
    stop_timer(agsm->connect_timer);
    agsm->connect_timer = NULL;
}

static bool connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    ag_server_data_t* data = (ag_server_data_t*)p_data;
    AG_DBG_EVENT(sm, agsm->bd_addr, event);

    switch (event) {
    case DISCONNECT:
        /* handle ? */
        break;
    case CONNECT_TIMEOUT:
        BT_ADDR_LOG("stm hfp ag CONNECT_TIMEOUT service_adapter_hfp_ag_disconnect %s", agsm->bd_addr);
        service_adapter_hfp_ag_disconnect(agsm->bd_addr);
        hsm_transition_to(sm, &disconnected_state);
        break;
    case STACK_EVENT_CONNECTION_STATE_CHANGED: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_CONNECTED:
            hsm_transition_to(sm, &connected_state);
            break;
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &disconnected_state);
            break;
        case PROFILE_STATE_CONNECTING:
        case PROFILE_STATE_DISCONNECTING:
            BT_LOGW("Ignored connection state:%d", state);
            break;
        }
    } break;
    case STACK_EVENT_CODEC_CHANGED:
        agsm->codec = data->valueint1;
        break;
    case STACK_EVENT_AT_CIND_REQUEST: {
        ag_service_notify_ag_cind(agsm->service, agsm->bd_addr);
        break;
    }
    case CIND_RESP: {
        BT_ADDR_LOG("stm hfp ag CIND_RESP %s", agsm->bd_addr);
        SERVICE_HFP_AG_CIND_RESPONSE_S resp = {};
        resp.device_status.service = data->valueint1;
        resp.device_status.signal = data->valueint2;
        resp.device_status.roam = data->valueint3;
        resp.device_status.battery = data->valueint4;

        resp.call = data->valueint5;
        resp.call_setup = data->valueint6;
        resp.call_held = data->valueint7;
        BT_ADDR_LOG("service_adapter_hfp_ag_cind_response %s", agsm->bd_addr);
        service_adapter_hfp_ag_cind_response(agsm->bd_addr, resp);
        break;
    }

    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static void disconnecting_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, agsm->bd_addr);
    ag_service_notify_connection_state_changed(agsm->service,
        agsm->bd_addr,
        PROFILE_STATE_DISCONNECTING);
}

static void disconnecting_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, agsm->bd_addr);
}

static bool disconnecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    ag_server_data_t* data = (ag_server_data_t*)p_data;
    AG_DBG_EVENT(sm, agsm->bd_addr, event);
    switch (event) {
    case STACK_EVENT_CONNECTION_STATE_CHANGED: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &disconnected_state);
            break;
        default:
            BT_LOGW("Ignored connection state:%d", state);
            break;
        }
    } break;
    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static bool default_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    ag_server_data_t* data = (ag_server_data_t*)p_data;

    BT_LOGD("%s, event:%" PRIu32 "", __func__, event);
    switch (event) {
    case VOICE_RECOGNITION_START:
        if (!agsm->recognition_active) {
            BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_enable_voice_recognition %s", agsm->bd_addr);
            service_adapter_hfp_ag_enable_voice_recognition(agsm->bd_addr);
        }
        break;
    case VOICE_RECOGNITION_STOP:
        if (agsm->recognition_active) {
            BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_disable_voice_recognition %s", agsm->bd_addr);
            service_adapter_hfp_ag_disable_voice_recognition(agsm->bd_addr);
        }
        break;
    case PHONE_STATE_CHANGE: {
        SERVICE_HFP_AG_PHONE_NUMBER_S* phone_number = NULL;
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_phone_state_change %s", agsm->bd_addr);
        if (data->string1) {
            uint8_t num_len = strlen(data->string1);
            phone_number = malloc(sizeof(SERVICE_HFP_AG_PHONE_NUMBER_S) + num_len);

            phone_number->type = data->valueint4;
            phone_number->number_length = num_len;
            memcpy(phone_number->number, data->string1, num_len);
        }
        service_adapter_hfp_ag_phone_state_change(agsm->bd_addr,
            data->valueint1,
            data->valueint2,
            data->valueint3,
            phone_number);
        free(phone_number);
        break;
    }
    case DEVICE_STATUS_CHANGED: {
        SERVICE_HFP_AG_DEVICE_STATUS_S status = {};
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_notify_device_status_changed %s", agsm->bd_addr);
        status.service = data->valueint1;
        status.signal = data->valueint3;
        status.roam = data->valueint2;
        status.battery = data->valueint4;
        service_adapter_hfp_ag_notify_device_status_changed(agsm->bd_addr, status);
        break;
    }
    case SET_INBAND_RING_ENABLE:
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_set_inband_ring_enable %s", agsm->bd_addr);
        service_adapter_hfp_ag_set_inband_ring_enable(agsm->bd_addr, true);
        break;
    case SEND_AT_COMMAND: {
        SERVICE_HFP_AG_AT_CMD_S cmd = {};
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_send_at_cmd %s", agsm->bd_addr);
        cmd.at_string = (char*)data->string1;
        cmd.at_length = strlen(data->string1);
        service_adapter_hfp_ag_send_at_cmd(agsm->bd_addr, &cmd);
        break;
    }
    case DIALING_RESULT:
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_dial_response %s", agsm->bd_addr);
        service_adapter_hfp_ag_dial_response(agsm->bd_addr, data->valueint1);
        break;
    case STACK_EVENT_VR_STATE_CHANGED:
        BT_ADDR_LOG("stm hfp ag STACK_EVENT_VR_STATE_CHANGED %s", agsm->bd_addr);
        agsm->recognition_active = data->valueint1;
        ag_service_notify_vr_state_changed(agsm->service, agsm->bd_addr, data->valueint1);
        break;
    case STACK_EVENT_CODEC_CHANGED:
        BT_ADDR_LOG("stm hfp ag STACK_EVENT_CODEC_CHANGED %s codec", agsm->bd_addr);
        agsm->codec = data->valueint1;
        break;
    case STACK_EVENT_VOLUME_CHANGED:
        /* set system volume */
        agsm->volume = data->valueint1;
        break;
    case STACK_EVENT_BATTERY_UPDATE:
        ag_service_notify_ag_battery_update(agsm->service, agsm->bd_addr, data->valueint1);
        break;
    case STACK_EVENT_ANSWER_CALL:
        /* system call interface */
        ag_service_notify_ag_answer_call(agsm->service, agsm->bd_addr);
        break;
    case STACK_EVENT_REJECT_CALL:
        /* system call interface */
        ag_service_notify_ag_reject_call(agsm->service, agsm->bd_addr);
        break;
    case STACK_EVENT_HANGUP_CALL:
        /* system call interface */
        ag_service_notify_ag_hangup_call(agsm->service, agsm->bd_addr);
        break;
    case STACK_EVENT_DIAL_NUMBER:
        BT_LOGD("Dial number:%s", data->string1);
        /* system call interface */
        ag_service_notify_ag_dial_number(agsm->service, agsm->bd_addr, data->string1);
        break;
    case STACK_EVENT_DIAL_MEMORY:
        /* system call interface */
        break;
    case STACK_EVENT_CALL_CONTROL: {
        ag_server_call_control_t chld = data->valueint1;
        /* system call interface */
        ag_service_notify_ag_call_control(agsm->service, agsm->bd_addr, chld);
        break;
    }
    case STACK_EVENT_AT_CIND_REQUEST:
        ag_service_notify_ag_cind(agsm->service, agsm->bd_addr);
        break;
    case STACK_EVENT_AT_CLCC_REQUEST:
        ag_service_notify_ag_clcc(agsm->service, agsm->bd_addr);
        break;
    case STACK_EVENT_AT_COPS_REQUEST:
        ag_service_notify_ag_cops(agsm->service, agsm->bd_addr);
        break;
    case STACK_EVENT_AT_COMMAND:
        ag_service_notify_ag_at_command(agsm->service, agsm->bd_addr, data->string1);
        break;
    case STACK_EVENT_SEND_DTMF:
        /* system call interface */
        break;
    case CIND_RESP: {
        BT_ADDR_LOG("stm hfp ag CLCC_RESP %s", agsm->bd_addr);
        SERVICE_HFP_AG_CIND_RESPONSE_S resp = {};
        resp.device_status.service = data->valueint1;
        resp.device_status.signal = data->valueint2;
        resp.device_status.roam = data->valueint3;
        resp.device_status.battery = data->valueint4;

        resp.call = data->valueint5;
        resp.call_setup = data->valueint6;
        resp.call_held = data->valueint7;
        BT_ADDR_LOG("service_adapter_hfp_ag_cind_response %s", agsm->bd_addr);
        service_adapter_hfp_ag_cind_response(agsm->bd_addr, resp);
        break;
    }
    case CLCC_RESP: {
        /* system call interface */
        BT_ADDR_LOG("stm hfp ag CLCC_RESP %s", agsm->bd_addr);
        uint8_t num_len = 0;
        SERVICE_HFP_AG_CLCC_RESPONSE_S* resp = NULL;

        if (index > 0) {
            if (data->string1)
                num_len = strlen(data->string1);

            resp = malloc(sizeof(SERVICE_HFP_AG_CLCC_RESPONSE_S) + num_len + 1);
            if (!resp)
                return BT_STATUS_NOMEM;

            resp->index = data->valueint1;
            resp->dir = data->valueint2;
            resp->status = data->valueint3;
            resp->mode = data->valueint4;
            resp->mpty = data->valueint5;
            resp->number.type = data->valueint6;
            if (num_len) {
                resp->number.number_length = num_len + 1;
                memcpy(resp->number.number, data->string1, num_len);
                resp->number.number[num_len] = '\0';
            }
        }
        service_adapter_hfp_ag_clcc_response(agsm->bd_addr, resp);
        free(resp);
        break;
    }
    case COPS_RESP: {
        char* operation_name = NULL;
        /* system call interface */
        BT_ADDR_LOG("stm hfp ag COPS_RESP %s", agsm->bd_addr);
        operation_name = data->string1;
        BT_LOGD("Operation name:%s", operation_name);
        service_adapter_hfp_ag_cops_response(agsm->bd_addr, (char*)operation_name,
            operation_name ? strlen(operation_name) : 0);
        break;
    }

    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static void default_connection_event_process(state_machine_t* sm, ag_server_data_t* data)
{
    profile_connection_state_t state = data->valueint1;

    switch (state) {
    case PROFILE_STATE_DISCONNECTED:
        hsm_transition_to(sm, &disconnected_state);
        break;
    case PROFILE_STATE_DISCONNECTING:
        hsm_transition_to(sm, &disconnecting_state);
        break;
    case PROFILE_STATE_CONNECTING:
    case PROFILE_STATE_CONNECTED:
        BT_LOGW("Ignored connection state:%d", state);
        break;
    }
}

static void connected_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, agsm->bd_addr);
    //uint8_t previous_state = hsm_get_state_value(hsm_get_previous_state(sm));
    //if (previous_state < HFP_AG_STATE_CONNECTED)
    ag_service_notify_connection_state_changed(agsm->service,
        agsm->bd_addr,
        PROFILE_STATE_CONNECTED);
    //else
    //ag_service_notify_audio_state_changed(agsm->service, &agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
}

static void connected_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, agsm->bd_addr);
}

static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    ag_server_data_t* data = (ag_server_data_t*)p_data;
    AG_DBG_EVENT(sm, agsm->bd_addr, event);

    switch (event) {
    case DISCONNECT:
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_disconnect %s", agsm->bd_addr);
        if (service_adapter_hfp_ag_disconnect(agsm->bd_addr) != BT_STATUS_SUCCESS)
            BT_ADDR_LOG("Disconnect failed for :%s", agsm->bd_addr);

        hsm_transition_to(sm, &disconnecting_state);
        break;
    case CONNECT_AUDIO:
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_create_sco %s", agsm->bd_addr);
        if (service_adapter_hfp_ag_create_sco(agsm->bd_addr) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("create_sco failed for :%s", agsm->bd_addr);
            ag_service_notify_audio_state_changed(agsm->service, agsm->bd_addr,
                HFP_AUDIO_STATE_DISCONNECTED);
            return false;
        }
        hsm_transition_to(sm, &audio_connecting_state);
        break;
    case STACK_EVENT_AUDIO_REQ:
        BT_ADDR_LOG("stm hfp ag service_adapter_gap_accept_sco_link %s", agsm->bd_addr);
        if (service_adapter_gap_accept_sco_link(agsm->bd_addr) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("Reply audio request fail:%s", agsm->bd_addr);
            return false;
        }
        hsm_transition_to(sm, &audio_connecting_state);
        break;
    case STACK_EVENT_CONNECTION_STATE_CHANGED:
        default_connection_event_process(sm, data);
        break;
    case STACK_EVENT_AUDIO_STATE_CHANGED: {
        hfp_audio_state_t state = data->valueint1;

        switch (state) {
        case HFP_AUDIO_STATE_CONNECTED:
            hsm_transition_to(sm, &audio_on_state);
            break;
        case HFP_AUDIO_STATE_DISCONNECTED:
            break;
        default:
            BT_LOGW("Ignored audio connection state:%d", state);
            break;
        }
    } break;
    default:
        default_process_event(sm, event, p_data);
        break;
    }
    return true;
}

static void audio_connecting_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, agsm->bd_addr);
    ag_service_notify_audio_state_changed(agsm->service, agsm->bd_addr, HFP_AUDIO_STATE_CONNECTING);
}

static void audio_connecting_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, agsm->bd_addr);
}

static bool audio_connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    ag_server_data_t* data = (ag_server_data_t*)p_data;
    AG_DBG_EVENT(sm, agsm->bd_addr, event);

    switch (event) {
    case DISCONNECT:
    case DISCONNECT_AUDIO:
        /* TODO: handle */
        BT_LOGD("defer DISCONNECT/DISCONNECT_AUDIO message");
        break;
    case STACK_EVENT_AUDIO_REQ:
        BT_LOGD("already in audio connecting state");
        break;
    case AUDIO_TIMEOUT:
        /* TODO: handle */
        break;
    case STACK_EVENT_CONNECTION_STATE_CHANGED:
        default_connection_event_process(sm, p_data);
        break;
    case STACK_EVENT_AUDIO_STATE_CHANGED: {
        hfp_audio_state_t state = data->valueint1;

        switch (state) {
        case HFP_AUDIO_STATE_CONNECTED:
            hsm_transition_to(sm, &audio_on_state);
            break;
        case HFP_AUDIO_STATE_DISCONNECTED:
            hsm_transition_to(sm, &connected_state);
            break;
        default:
            BT_LOGW("Ignored audio connection state:%d", state);
            break;
        }
    } break;
    default:
        default_process_event(sm, event, p_data);
        break;
    }
    return true;
}

static void audio_on_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, agsm->bd_addr);
    /* TODO: get volume */
    /* TODO: set remote volume */
    /* TODO: set sample rate */
    /* TODO: set sco device avaliable */
    if (agsm->codec == SERVICE_HFP_CODEC_MSBC) {
        BT_LOGD("audio_on_enter SERVICE_HFP_CODEC_MSBC");
        ag_service_notify_audio_state_changed(agsm->service, agsm->bd_addr, HFP_AUDIO_STATE_CONNECTED_MSBC);
    } else {
        BT_LOGD("audio_on_enter SERVICE_HFP_CODEC_CVSD");
        ag_service_notify_audio_state_changed(agsm->service, agsm->bd_addr, HFP_AUDIO_STATE_CONNECTED);
    }
}

static void audio_on_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, agsm->bd_addr);
    /* set sco device unavaliable */
}

static bool audio_on_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    ag_server_data_t* data = (ag_server_data_t*)p_data;
    AG_DBG_EVENT(sm, agsm->bd_addr, event);

    switch (event) {
    case DISCONNECT:
        /* TODO: disconnect audio first */
        BT_LOGD("defer DISCONNECT message");
        break;
    case DISCONNECT_AUDIO:
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_disconnect_sco %s", agsm->bd_addr);
        if (service_adapter_hfp_ag_disconnect_sco(agsm->bd_addr) != BT_STATUS_SUCCESS) {
            ag_service_notify_audio_state_changed(agsm->service, agsm->bd_addr, HFP_AUDIO_STATE_DISCONNECTED);
            hsm_transition_to(sm, &connected_state);
            return false;
        }
        hsm_transition_to(sm, &audio_disconnecting_state);
        break;
    case VOICE_RECOGNITION_START:
    case VOICE_RECOGNITION_STOP:
        /* TODO: should support VOICE_RECOGNITION_STOP */
        break;
    case SET_VOLUME: {
        BT_ADDR_LOG("stm hfp ag service_adapter_hfp_ag_set_volume %s", agsm->bd_addr);

        uint8_t vol = data->valueint1 > 15 ? 15 : data->valueint1;
        /* android don't support set Mic volume */
        //VOLUME_SPEAKER VOLUME_MIC;
        service_adapter_hfp_ag_set_volume(agsm->bd_addr, VOLUME_SPEAKER, vol);
    } break;
    case STACK_EVENT_CONNECTION_STATE_CHANGED:
        default_connection_event_process(sm, p_data);
        break;
    case STACK_EVENT_AUDIO_STATE_CHANGED: {
        hfp_audio_state_t state = data->valueint1;

        switch (state) {
        case HFP_AUDIO_STATE_DISCONNECTED:
            hsm_transition_to(sm, &connected_state);
            break;
        case HFP_AUDIO_STATE_CONNECTED:
        default:
            BT_LOGW("Ignored audio connection state:%d", state);
            break;
        }
    } break;
    default:
        default_process_event(sm, event, p_data);
        break;
    }
    return true;
}

static void audio_disconnecting_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, agsm->bd_addr);
    ag_service_notify_audio_state_changed(agsm->service, agsm->bd_addr, HFP_AUDIO_STATE_DISCONNECTING);
}

static void audio_disconnecting_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, agsm->bd_addr);
}

static bool audio_disconnecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    ag_server_data_t* data = (ag_server_data_t*)p_data;
    AG_DBG_EVENT(sm, agsm->bd_addr, event);

    switch (event) {
    case DISCONNECT:
        /* TODO: handle */
        break;
    case STACK_EVENT_CONNECTION_STATE_CHANGED:
        default_connection_event_process(sm, p_data);
        break;
    case STACK_EVENT_AUDIO_STATE_CHANGED: {
        hfp_audio_state_t state = data->valueint1;

        switch (state) {
        case HFP_AUDIO_STATE_DISCONNECTED:
            hsm_transition_to(sm, &connected_state);
            break;
        case HFP_AUDIO_STATE_CONNECTED:
        default:
            BT_LOGW("Ignored audio connection state:%d", state);
            break;
        }
    } break;
    default:
        default_process_event(sm, event, p_data);
        break;
    }
    return true;
}

ag_state_machine_t* ag_server_state_machine_new(void* context, bt_address bd_addr)
{
    ag_state_machine_t* agsm;

    agsm = (ag_state_machine_t*)malloc(sizeof(ag_state_machine_t));
    if (!agsm)
        return NULL;

    memset(agsm, 0, sizeof(ag_state_machine_t));
    agsm->recognition_active = false;
    agsm->service = context;
    agsm->connect_timer = NULL;
    agsm->audio_timer = NULL;
    agsm->dial_out_timer = NULL;
    memcpy(agsm->bd_addr, bd_addr, sizeof(bt_address));
    //list_initialize(&hfsm->pending_actions);
    hsm_ctor(&agsm->sm, (state_t*)&disconnected_state);

    return agsm;
}

void ag_server_state_machine_destory(ag_state_machine_t* agsm)
{
    if (!agsm)
        return;

    if (agsm->connect_timer)
        stop_timer(agsm->connect_timer);
    hsm_dtor(&agsm->sm);
    free((void*)agsm);
}

void ag_server_event_dispatch(ag_state_machine_t* agsm, ag_server_msg_t* msg)
{
    if (!agsm || !msg)
        return;

    hsm_dispatch_event(&agsm->sm, msg->event, &msg->data);
}

void ag_server_state_machine_handle_msg(ag_state_machine_t* agsm,
    ag_server_msg_t* msg)
{
    ag_server_event_dispatch(agsm, msg);
}

ag_server_state_t ag_server_state_machine_get_state(ag_state_machine_t* agsm)
{
    state_t* state = hsm_get_current_state(&agsm->sm);

    if (state == (state_t*)&disconnected_state) {
        return AG_SERVER_STATE_DISCONNECTED;
    } else if (state == (state_t*)&connecting_state) {
        return AG_SERVER_STATE_CONNECTING;
    } else if (state == (state_t*)&connected_state) {
        return AG_SERVER_STATE_CONNECTED;
    } else if (state == (state_t*)&disconnecting_state) {
        return AG_SERVER_STATE_DISCONNECTING;
    } else if (state == (state_t*)&audio_on_state) {
        return AG_SERVER_STATE_AUDIO_CONNECTED;
    } else if (state == (state_t*)&audio_connecting_state) {
        return AG_SERVER_STATE_AUDIO_CONNECTING;
    } else if (state == (state_t*)&audio_disconnecting_state) {
        return AG_SERVER_STATE_AUDIO_DISCONNECTING;
    } else {
        BT_LOGE("%s:Unknow State", __func__);
        return AG_SERVER_STATE_DISCONNECTED;
    }
}
