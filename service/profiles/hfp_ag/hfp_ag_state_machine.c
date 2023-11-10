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

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_hfp_ag.h"
#include "bt_list.h"
#include "hfp_ag_event.h"
#include "hfp_ag_service.h"
#include "hfp_ag_state_machine.h"
#include "hfp_ag_tele_service.h"
#include "sal_adapter_interface.h"
#include "sal_hfp_ag_interface.h"
#include "service_loop.h"

#include "bt_utils.h"
#include "utils/log.h"

typedef struct _ag_state_machine {
    state_machine_t sm;
    bt_address_t addr;
    uint16_t sco_conn_handle;
    bool recognition_active;
    void *service;
    uint8_t codec;
    uint8_t volume;
    service_timer_t *connect_timer;
    service_timer_t *audio_timer;
    service_timer_t *dial_out_timer;
} ag_state_machine_t;

#define AG_TIMEOUT   10000
#define AG_STM_DEBUG 1
#if AG_STM_DEBUG
static void ag_stm_trans_debug(state_machine_t *sm, bt_address_t *addr, const char *action);
static void ag_stm_event_debug(state_machine_t *sm, bt_address_t *addr, uint32_t event);
static const char *stack_event_to_string(hfp_ag_event_t event);

#define AG_DBG_ENTER(__sm, __addr)          ag_stm_trans_debug(__sm, __addr, "Enter")
#define AG_DBG_EXIT(__sm, __addr)           ag_stm_trans_debug(__sm, __addr, "Exit ")
#define AG_DBG_EVENT(__sm, __addr, __event) ag_stm_event_debug(__sm, __addr, __event);
#else
#define AG_DBG_ENTER(__sm, __addr)
#define AG_DBG_EXIT(__sm, __addr)
#define AG_DBG_EVENT(__sm, __addr, __event)
#endif
extern bt_status_t hfp_ag_send_event(bt_address_t *addr, hfp_ag_event_t evt);
static void disconnected_enter(state_machine_t *sm);
static void disconnected_exit(state_machine_t *sm);
static void connecting_enter(state_machine_t *sm);
static void connecting_exit(state_machine_t *sm);
static void disconnecting_enter(state_machine_t *sm);
static void disconnecting_exit(state_machine_t *sm);
static void connected_enter(state_machine_t *sm);
static void connected_exit(state_machine_t *sm);
static void audio_connecting_enter(state_machine_t *sm);
static void audio_connecting_exit(state_machine_t *sm);
static void audio_on_enter(state_machine_t *sm);
static void audio_on_exit(state_machine_t *sm);
static void audio_disconnecting_enter(state_machine_t *sm);
static void audio_disconnecting_exit(state_machine_t *sm);

static bool disconnected_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool connecting_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool disconnecting_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool connected_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool audio_connecting_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool audio_on_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool audio_disconnecting_process_event(state_machine_t *sm, uint32_t event, void *p_data);

static const state_t disconnected_state = {
    .state_name = "Disconnected",
    .state_value = HFP_AG_STATE_DISCONNECTED,
    .enter = disconnected_enter,
    .exit = disconnected_exit,
    .process_event = disconnected_process_event,
};

static const state_t connecting_state = {
    .state_name = "Connecting",
    .state_value = HFP_AG_STATE_CONNECTING,
    .enter = connecting_enter,
    .exit = connecting_exit,
    .process_event = connecting_process_event,
};

static const state_t disconnecting_state = {
    .state_name = "Disconnecting",
    .state_value = HFP_AG_STATE_DISCONNECTING,
    .enter = disconnecting_enter,
    .exit = disconnecting_exit,
    .process_event = disconnecting_process_event,
};

static const state_t connected_state = {
    .state_name = "Connected",
    .state_value = HFP_AG_STATE_CONNECTED,
    .enter = connected_enter,
    .exit = connected_exit,
    .process_event = connected_process_event,
};

static const state_t audio_connecting_state = {
    .state_name = "AudioConnecting",
    .state_value = HFP_AG_STATE_AUDIO_CONNECTING,
    .enter = audio_connecting_enter,
    .exit = audio_connecting_exit,
    .process_event = audio_connecting_process_event,
};

static const state_t audio_on_state = {
    .state_name = "AudioOn",
    .state_value = HFP_AG_STATE_AUDIO_CONNECTED,
    .enter = audio_on_enter,
    .exit = audio_on_exit,
    .process_event = audio_on_process_event,
};

static const state_t audio_disconnecting_state = {
    .state_name = "AudioDisconnecting",
    .state_value = HFP_AG_STATE_AUDIO_DISCONNECTING,
    .enter = audio_disconnecting_enter,
    .exit = audio_disconnecting_exit,
    .process_event = audio_disconnecting_process_event,
};

#if AG_STM_DEBUG
static void ag_stm_trans_debug(state_machine_t *sm, bt_address_t *addr, const char *action)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s State=%s, Peer=[%s]", action, hsm_get_current_state_name(sm), addr_str);
}

static void ag_stm_event_debug(state_machine_t *sm, bt_address_t *addr, uint32_t event)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(sm),
            addr_str, stack_event_to_string(event));
}

static const char *stack_event_to_string(hfp_ag_event_t event)
{
    static char ag_evt[32] = { 0 };

    switch (event) {
        CASE_RETURN_STR(AG_CONNECT)
        CASE_RETURN_STR(AG_DISCONNECT)
        CASE_RETURN_STR(AG_CONNECT_AUDIO)
        CASE_RETURN_STR(AG_DISCONNECT_AUDIO)
        CASE_RETURN_STR(AG_VOICE_RECOGNITION_START)
        CASE_RETURN_STR(AG_VOICE_RECOGNITION_STOP)
        CASE_RETURN_STR(AG_PHONE_STATE_CHANGE)
        CASE_RETURN_STR(AG_DEVICE_STATUS_CHANGED)
        CASE_RETURN_STR(AG_SET_VOLUME)
        CASE_RETURN_STR(AG_SET_INBAND_RING_ENABLE)
        CASE_RETURN_STR(AG_DIALING_RESULT)
        CASE_RETURN_STR(AG_SEND_AT_COMMAND)
        CASE_RETURN_STR(AG_STARTUP)
        CASE_RETURN_STR(AG_SHUTDOWN)
        CASE_RETURN_STR(AG_CONNECT_TIMEOUT)
        CASE_RETURN_STR(AG_AUDIO_TIMEOUT)
        CASE_RETURN_STR(AG_STACK_EVENT)
        CASE_RETURN_STR(AG_STACK_EVENT_AUDIO_REQ)
        CASE_RETURN_STR(AG_STACK_EVENT_CONNECTION_STATE_CHANGED)
        CASE_RETURN_STR(AG_STACK_EVENT_AUDIO_STATE_CHANGED)
        CASE_RETURN_STR(AG_STACK_EVENT_VR_STATE_CHANGED)
        CASE_RETURN_STR(AG_STACK_EVENT_CODEC_CHANGED)
        CASE_RETURN_STR(AG_STACK_EVENT_VOLUME_CHANGED)
        CASE_RETURN_STR(AG_STACK_EVENT_AT_CIND_REQUEST)
        CASE_RETURN_STR(AG_STACK_EVENT_AT_CLCC_REQUEST)
        CASE_RETURN_STR(AG_STACK_EVENT_AT_COPS_REQUEST)
        CASE_RETURN_STR(AG_STACK_EVENT_BATTERY_UPDATE)
        CASE_RETURN_STR(AG_STACK_EVENT_ANSWER_CALL)
        CASE_RETURN_STR(AG_STACK_EVENT_REJECT_CALL)
        CASE_RETURN_STR(AG_STACK_EVENT_HANGUP_CALL)
        CASE_RETURN_STR(AG_STACK_EVENT_DIAL_NUMBER)
        CASE_RETURN_STR(AG_STACK_EVENT_DIAL_MEMORY)
        CASE_RETURN_STR(AG_STACK_EVENT_CALL_CONTROL)
        CASE_RETURN_STR(AG_STACK_EVENT_AT_COMMAND)
        CASE_RETURN_STR(AG_STACK_EVENT_SEND_DTMF)
    default:
        snprintf(ag_evt, 32, "UNKNOWN_AG_EVENT:%d", event);
        return (const char *)ag_evt;
    }
}
#endif

static bool at_cmd_check_test(bt_address_t *addr, const char *atcmd)
{
    if (!strcmp(atcmd, "AT+TEST\r\n")) {
        bt_sal_hfp_ag_send_at_cmd(addr, "\r\n+TEST:0\r\n", strlen("\r\n+TEST:0\r\n"));
        return true;
    }

    return false;
}

static void connect_timeout(service_timer_t *timer, void *data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)data;

    hfp_ag_send_event(&agsm->addr, AG_CONNECT_TIMEOUT);
}

static void dial_out_timeout(service_timer_t *timer, void *data)
{
    // ag_state_machine_t* agsm = (ag_state_machine_t*)data;

    hfp_ag_dial_result(HFP_ATCMD_RESULT_TIMEOUT);
}

static uint8_t callstate_to_callsetup(hfp_ag_call_state_t call_state)
{
    switch (call_state) {
    case HFP_AG_CALL_STATE_INCOMING:
        return HFP_CALLSETUP_INCOMING;
    case HFP_AG_CALL_STATE_DIALING:
        return HFP_CALLSETUP_OUTGOING;
    case HFP_AG_CALL_STATE_ALERTING:
        return HFP_CALLSETUP_ALERTING;
    default:
        return HFP_CALLSETUP_NONE;
    }
}

static void process_cind_request(ag_state_machine_t *agsm)
{
    uint8_t num_active, num_held, call_state;
    hfp_ag_cind_resopnse_t resp;

    /* 1. system interface get calls */
    /* 2. get network state */
    /* 3. get battery */
    tele_service_get_network_info(&resp.network, &resp.roam, &resp.signal);
    resp.battery = 5;
    tele_service_get_phone_state(&num_active, &num_held, &call_state);
    resp.call = num_active ? HFP_CALL_CALLS_IN_PROGRESS : HFP_CALL_NO_CALLS_IN_PROGRESS;
    resp.call_held = num_held ? HFP_CALLHELD_HELD : HFP_CALLHELD_NONE;
    resp.call_setup = callstate_to_callsetup(call_state);
    BT_LOGD("AT+CIND=? response");
    bt_sal_hfp_ag_cind_response(&agsm->addr, &resp);
}

static void disconnected_enter(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    if (hsm_get_previous_state(sm))
        ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_DISCONNECTED);
}

static void disconnected_exit(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool disconnected_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    hfp_ag_data_t *data = (hfp_ag_data_t *)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_CONNECT:
        if (bt_sal_hfp_ag_connect(&agsm->addr) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("Connect failed for %s", &agsm->addr);
            ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_DISCONNECTED);
            return false;
        }
        hsm_transition_to(sm, &connecting_state);
        break;
    case AG_STACK_EVENT_CONNECTION_STATE_CHANGED: {
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
    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static void connecting_enter(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    agsm->connect_timer = service_loop_timer_no_repeating(AG_TIMEOUT, connect_timeout, agsm);
    ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_CONNECTING);
}

static void connecting_exit(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
    service_loop_cancel_timer(agsm->connect_timer);
    agsm->connect_timer = NULL;
}

static bool connecting_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    hfp_ag_data_t *data = (hfp_ag_data_t *)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
        /* handle ? */
        break;
    case AG_CONNECT_TIMEOUT:
        bt_sal_hfp_ag_disconnect(&agsm->addr);
        hsm_transition_to(sm, &disconnected_state);
        break;
    case AG_STACK_EVENT_CONNECTION_STATE_CHANGED: {
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
    case AG_STACK_EVENT_CODEC_CHANGED:
        agsm->codec = data->valueint1 == HFP_CODEC_MSBC ? HFP_CODEC_MSBC : HFP_CODEC_CVSD;
        break;
    case AG_STACK_EVENT_AT_CIND_REQUEST:
        process_cind_request(agsm);
        break;
    default:
        BT_LOGW("Unexpected event:%" PRId32 "", event);
        break;
    }
    return true;
}

static void disconnecting_enter(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_DISCONNECTING);
}

static void disconnecting_exit(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool disconnecting_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    hfp_ag_data_t *data = (hfp_ag_data_t *)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);
    switch (event) {
    case AG_STACK_EVENT_CONNECTION_STATE_CHANGED: {
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

static bool default_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    hfp_ag_data_t *data = (hfp_ag_data_t *)p_data;

    BT_LOGD("%s, event:%" PRIu32 "", __func__, event);
    switch (event) {
    case AG_VOICE_RECOGNITION_START:
        if (!agsm->recognition_active) {
            bt_sal_hfp_ag_start_voice_recognition(&agsm->addr);
        }
        break;
    case AG_VOICE_RECOGNITION_STOP:
        if (agsm->recognition_active) {
            bt_sal_hfp_ag_stop_voice_recognition(&agsm->addr);
        }
        break;
    case AG_PHONE_STATE_CHANGE:
        bt_sal_hfp_ag_phone_state_change(&agsm->addr, data->valueint1,
                                         data->valueint2, data->valueint3,
                                         data->valueint4, data->string1, data->string2);
        break;
    case AG_DEVICE_STATUS_CHANGED:
        bt_sal_hfp_ag_notify_device_status_changed(&agsm->addr, data->valueint1,
                                                   data->valueint2, data->valueint3,
                                                   data->valueint4);
        break;
    case AG_SET_INBAND_RING_ENABLE:
        bt_sal_hfp_ag_set_inband_ring_enable(&agsm->addr, true);
        break;
    case AG_SEND_AT_COMMAND:
        bt_sal_hfp_ag_send_at_cmd(&agsm->addr, data->string1, strlen(data->string1));
        break;
    case AG_DIALING_RESULT:
        if (agsm->dial_out_timer) {
            service_loop_cancel_timer(agsm->dial_out_timer);
            agsm->dial_out_timer = NULL;
            bt_sal_hfp_ag_dial_response(&agsm->addr, data->valueint1);
        }
        break;
    case AG_STACK_EVENT_VR_STATE_CHANGED:
        agsm->recognition_active = data->valueint1;
        ag_service_notify_vr_state_changed(&agsm->addr, data->valueint1);
        break;
    case AG_STACK_EVENT_CODEC_CHANGED:
        agsm->codec = data->valueint1 == HFP_CODEC_MSBC ? HFP_CODEC_MSBC : HFP_CODEC_CVSD;
        break;
    case AG_STACK_EVENT_VOLUME_CHANGED:
        /* set system volume */
        agsm->volume = data->valueint1;
        break;
    case AG_STACK_EVENT_AT_CIND_REQUEST:
        process_cind_request(agsm);
        break;
    case AG_STACK_EVENT_AT_CLCC_REQUEST:
        /* system call interface */
        tele_service_query_current_call(&agsm->addr);
        break;
    case AG_STACK_EVENT_AT_COPS_REQUEST: {
        /* system call interface */
        char *operation_name = NULL;
        operation_name = tele_service_get_operator();
        BT_LOGD("Operation name:%s", operation_name);
        bt_sal_hfp_ag_cops_response(&agsm->addr, operation_name, operation_name ? strlen(operation_name) : 0);
    } break;
    case AG_STACK_EVENT_BATTERY_UPDATE:
        ag_service_notify_hf_battery_update(&agsm->addr, data->valueint1);
        break;
    case AG_STACK_EVENT_ANSWER_CALL:
        /* system call interface */
        tele_service_answer_call();
        break;
    case AG_STACK_EVENT_REJECT_CALL:
        /* system call interface */
        tele_service_reject_call();
        break;
    case AG_STACK_EVENT_HANGUP_CALL:
        /* system call interface */
        tele_service_hangup_call();
        break;
    case AG_STACK_EVENT_DIAL_NUMBER: {
        if (data->string1) {
            BT_LOGD("Dial number:%s", data->string1);
            /* system call interface */
            if (tele_service_dial_number(data->string1) != BT_STATUS_SUCCESS)
                bt_sal_hfp_ag_dial_response(&agsm->addr, HFP_ATCMD_RESULT_ERROR);
            else
                agsm->dial_out_timer = service_loop_timer_no_repeating(5000, dial_out_timeout, NULL);
        }
        else {
            BT_LOGD("Redial last number, currently not supported");
            bt_sal_hfp_ag_dial_response(&agsm->addr, HFP_ATCMD_RESULT_ERROR);
        }
    } break;
    case AG_STACK_EVENT_DIAL_MEMORY:
        /* system call interface */
        break;
    case AG_STACK_EVENT_CALL_CONTROL: {
        hfp_call_control_t chld = data->valueint1;
        /* system call interface */
        tele_service_call_control(chld);
    } break;
    case AG_STACK_EVENT_AT_COMMAND:
        at_cmd_check_test(&agsm->addr, data->string1);
        /* TODO: need notify AT command? */
        break;
    case AG_STACK_EVENT_SEND_DTMF:
        /* system call interface */
        break;
    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static void default_connection_event_process(state_machine_t *sm, hfp_ag_data_t *data)
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

static void connected_enter(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    uint8_t previous_state = hsm_get_state_value(hsm_get_previous_state(sm));

    if (previous_state < HFP_AG_STATE_CONNECTED)
        ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_CONNECTED);
    else
        ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
}

static void connected_exit(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool connected_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    hfp_ag_data_t *data = (hfp_ag_data_t *)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
        if (bt_sal_hfp_ag_disconnect(&agsm->addr) != BT_STATUS_SUCCESS)
            BT_ADDR_LOG("Disconnect failed for :%s", &agsm->addr);

        hsm_transition_to(sm, &disconnecting_state);
        break;
    case AG_CONNECT_AUDIO:
        if (bt_sal_hfp_ag_connect_audio(&agsm->addr) != BT_STATUS_SUCCESS) {
            ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
            return false;
        }
        hsm_transition_to(sm, &audio_connecting_state);
        break;
    case AG_STACK_EVENT_AUDIO_REQ:
        if (bt_sal_reply_sco_link_request(&agsm->addr, true) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("Reply audio request fail:%s", &agsm->addr);
            return false;
        }
        hsm_transition_to(sm, &audio_connecting_state);
        break;
    case AG_STACK_EVENT_CONNECTION_STATE_CHANGED:
        default_connection_event_process(sm, data);
        break;
    case AG_STACK_EVENT_AUDIO_STATE_CHANGED: {
        hfp_audio_state_t state = data->valueint1;

        switch (state) {
        case HFP_AUDIO_STATE_CONNECTED:
            hsm_transition_to(sm, &audio_on_state);
            break;
        case HFP_AUDIO_STATE_DISCONNECTED:
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

static void audio_connecting_enter(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_CONNECTING);
}

static void audio_connecting_exit(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool audio_connecting_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    hfp_ag_data_t *data = (hfp_ag_data_t *)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
    case AG_DISCONNECT_AUDIO:
        /* TODO: handle */
        BT_LOGD("defer DISCONNECT/DISCONNECT_AUDIO message");
        break;
    case AG_STACK_EVENT_AUDIO_REQ:
        BT_LOGD("already in audio connecting state");
        break;
    case AG_AUDIO_TIMEOUT:
        /* TODO: handle */
        break;
    case AG_STACK_EVENT_CONNECTION_STATE_CHANGED:
        default_connection_event_process(sm, p_data);
        break;
    case AG_STACK_EVENT_AUDIO_STATE_CHANGED: {
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

static void audio_on_enter(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    /* TODO: get volume */
    /* TODO: set remote volume */
    /* TODO: set sample rate */
    /* TODO: set sco device avaliable */
    ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_CONNECTED);
}

static void audio_on_exit(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
    /* set sco device unavaliable */
}

static bool audio_on_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    hfp_ag_data_t *data = (hfp_ag_data_t *)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
        /* TODO: disconnect audio first */
        BT_LOGD("defer DISCONNECT message");
        break;
    case AG_DISCONNECT_AUDIO:
        if (bt_sal_hfp_ag_disconnect_audio(&agsm->addr) != BT_STATUS_SUCCESS) {
            ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
            hsm_transition_to(sm, &connected_state);
            return false;
        }
        hsm_transition_to(sm, &audio_disconnecting_state);
        break;
    case AG_VOICE_RECOGNITION_START:
    case AG_VOICE_RECOGNITION_STOP:
        /* TODO: should support VOICE_RECOGNITION_STOP */
        break;
    case AG_SET_VOLUME: {
        uint8_t vol = data->valueint1 > 15 ? 15 : data->valueint1;
        /* android don't support set Mic volume */
        bt_sal_hfp_ag_set_volume(&agsm->addr, HFP_VOLUME_TYPE_SPK, vol);
    } break;
    case AG_STACK_EVENT_CONNECTION_STATE_CHANGED:
        default_connection_event_process(sm, p_data);
        break;
    case AG_STACK_EVENT_AUDIO_STATE_CHANGED: {
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

static void audio_disconnecting_enter(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTING);
}

static void audio_disconnecting_exit(state_machine_t *sm)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool audio_disconnecting_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    ag_state_machine_t *agsm = (ag_state_machine_t *)sm;
    hfp_ag_data_t *data = (hfp_ag_data_t *)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
        /* TODO: handle */
        break;
    case AG_STACK_EVENT_CONNECTION_STATE_CHANGED:
        default_connection_event_process(sm, p_data);
        break;
    case AG_STACK_EVENT_AUDIO_STATE_CHANGED: {
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

ag_state_machine_t *ag_state_machine_new(bt_address_t *addr, void *context)
{
    ag_state_machine_t *agsm;

    agsm = (ag_state_machine_t *)malloc(sizeof(ag_state_machine_t));
    if (!agsm)
        return NULL;

    memset(agsm, 0, sizeof(ag_state_machine_t));
    agsm->recognition_active = false;
    agsm->service = context;
    agsm->connect_timer = NULL;
    agsm->audio_timer = NULL;
    agsm->dial_out_timer = NULL;
    memcpy(&agsm->addr, addr, sizeof(bt_address_t));
    hsm_ctor(&agsm->sm, (state_t *)&disconnected_state);

    return agsm;
}

void ag_state_machine_destory(ag_state_machine_t *agsm)
{
    if (!agsm)
        return;

    if (agsm->connect_timer)
        service_loop_cancel_timer(agsm->connect_timer);
    hsm_dtor(&agsm->sm);
    free((void *)agsm);
}

void ag_state_machine_dispatch(ag_state_machine_t *agsm, hfp_ag_msg_t *msg)
{
    if (!agsm || !msg)
        return;

    hsm_dispatch_event(&agsm->sm, msg->event, &msg->data);
}

uint32_t ag_state_machine_get_state(ag_state_machine_t *agsm)
{
    return hsm_get_current_state_value(&agsm->sm);
}
