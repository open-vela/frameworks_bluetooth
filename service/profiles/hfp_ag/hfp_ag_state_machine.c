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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "audio_control.h"
#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_device.h"
#include "bt_hfp_ag.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "bt_vendor.h"
#include "hci_parser.h"
#include "hfp_ag_event.h"
#include "hfp_ag_service.h"
#include "hfp_ag_state_machine.h"
#include "hfp_ag_tele_service.h"
#include "media_system.h"
#include "power_manager.h"
#include "sal_hfp_ag_interface.h"
#include "sal_interface.h"
#include "utils/log.h"

#define HFP_AG_RETRY_MAX 1
#define HFP_FAKE_NUMBER "10000000"

typedef struct _ag_state_machine {
    state_machine_t sm;
    bt_address_t addr;
    uint16_t sco_conn_handle;
    uint32_t remote_features;
    bool recognition_active;
    bool virtual_call_started;
    bool offloading;
    void* service;
    uint8_t codec;
    uint8_t spk_volume;
    uint8_t mic_volume;
    uint8_t retry_cnt;
    int media_volume;
    void* volume_listener;
    uint32_t set_volume_cnt;
    pending_state_t pending;
    service_timer_t* connect_timer;
    service_timer_t* audio_timer;
    service_timer_t* dial_out_timer;
    service_timer_t* offload_timer;
    service_timer_t* retry_timer;
} ag_state_machine_t;

typedef struct vendor_specific_at_prefix {
    char* at_prefix;
    uint16_t company_id;
} vendor_specific_at_prefix_t;

static const vendor_specific_at_prefix_t company_id_map[] = {
    { "+XIAOMI", BLUETOOTH_COMPANY_ID_XIAOMI },
    { "+ANDROID", BLUETOOTH_COMPANY_ID_GOOGLE },
};

#define AG_TIMEOUT 10000
#define AG_OFFLOAD_TIMEOUT 500
#define AG_STM_DEBUG 1
#if AG_STM_DEBUG
static void ag_stm_trans_debug(state_machine_t* sm, bt_address_t* addr, const char* action);
static void ag_stm_event_debug(state_machine_t* sm, bt_address_t* addr, uint32_t event);
static const char* stack_event_to_string(hfp_ag_event_t event);

#define AG_DBG_ENTER(__sm, __addr) ag_stm_trans_debug(__sm, __addr, "Enter")
#define AG_DBG_EXIT(__sm, __addr) ag_stm_trans_debug(__sm, __addr, "Exit ")
#define AG_DBG_EVENT(__sm, __addr, __event) ag_stm_event_debug(__sm, __addr, __event);
#else
#define AG_DBG_ENTER(__sm, __addr)
#define AG_DBG_EXIT(__sm, __addr)
#define AG_DBG_EVENT(__sm, __addr, __event)
#endif

extern bt_status_t hfp_ag_send_event(bt_address_t* addr, hfp_ag_event_t evt);
extern bt_status_t hfp_ag_send_message(hfp_ag_msg_t* msg);

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
static void set_virtual_call_started(state_machine_t* sm, bool started);

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
static void ag_stm_trans_debug(state_machine_t* sm, bt_address_t* addr, const char* action)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s State=%s, Peer=[%s]", action, hsm_get_current_state_name(sm), addr_str);
}

static void ag_stm_event_debug(state_machine_t* sm, bt_address_t* addr, uint32_t event)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(sm),
        addr_str, stack_event_to_string(event));
}

static const char* stack_event_to_string(hfp_ag_event_t event)
{
    static char ag_evt[32] = { 0 };

    switch (event) {
        CASE_RETURN_STR(AG_CONNECT)
        CASE_RETURN_STR(AG_DISCONNECT)
        CASE_RETURN_STR(AG_CONNECT_AUDIO)
        CASE_RETURN_STR(AG_DISCONNECT_AUDIO)
        CASE_RETURN_STR(AG_START_VIRTUAL_CALL)
        CASE_RETURN_STR(AG_STOP_VIRTUAL_CALL)
        CASE_RETURN_STR(AG_VOICE_RECOGNITION_START)
        CASE_RETURN_STR(AG_VOICE_RECOGNITION_STOP)
        CASE_RETURN_STR(AG_PHONE_STATE_CHANGE)
        CASE_RETURN_STR(AG_DEVICE_STATUS_CHANGED)
        CASE_RETURN_STR(AG_SET_VOLUME)
        CASE_RETURN_STR(AG_SET_INBAND_RING_ENABLE)
        CASE_RETURN_STR(AG_DIALING_RESULT)
        CASE_RETURN_STR(AG_SEND_AT_COMMAND)
        CASE_RETURN_STR(AG_SEND_VENDOR_SPECIFIC_AT_COMMAND)
        CASE_RETURN_STR(AG_STARTUP)
        CASE_RETURN_STR(AG_SHUTDOWN)
        CASE_RETURN_STR(AG_CONNECT_TIMEOUT)
        CASE_RETURN_STR(AG_AUDIO_TIMEOUT)
        CASE_RETURN_STR(AG_OFFLOAD_START_REQ)
        CASE_RETURN_STR(AG_OFFLOAD_STOP_REQ)
        CASE_RETURN_STR(AG_OFFLOAD_START_EVT)
        CASE_RETURN_STR(AG_OFFLOAD_STOP_EVT)
        CASE_RETURN_STR(AG_OFFLOAD_TIMEOUT_EVT)
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
        CASE_RETURN_STR(AG_STACK_EVENT_NREC_REQ)
    default:
        snprintf(ag_evt, 32, "UNKNOWN_AG_EVENT:%d", event);
        return (const char*)ag_evt;
    }
}
#endif

static bool flag_isset(ag_state_machine_t* agsm, pending_state_t flag)
{
    return (bool)(agsm->pending & flag);
}

static void flag_set(ag_state_machine_t* agsm, pending_state_t flag)
{
    agsm->pending |= flag;
}

static void flag_clear(ag_state_machine_t* agsm, pending_state_t flag)
{
    agsm->pending &= ~flag;
}

static bool at_cmd_check_test(bt_address_t* addr, const char* atcmd)
{
    if (!strcmp(atcmd, "AT+TEST\r\n")) {
        bt_sal_hfp_ag_send_at_cmd(addr, "\r\n+TEST:0\r\n", strlen("\r\n+TEST:0\r\n"));
        return true;
    }

    return false;
}

static void connect_timeout(service_timer_t* timer, void* data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)data;

    hfp_ag_send_event(&agsm->addr, AG_CONNECT_TIMEOUT);
}

static void dial_out_timeout(service_timer_t* timer, void* data)
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

static void process_cind_request(ag_state_machine_t* agsm)
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

static void update_remote_features(ag_state_machine_t* agsm, uint32_t remote_features)
{
    BT_LOGD("%s, remote features:0x%" PRIu32, __func__, remote_features);
    agsm->remote_features = remote_features;
}

static void disconnected_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    if (hsm_get_previous_state(sm)) {
        bt_media_remove_listener(agsm->volume_listener);
        agsm->spk_volume = 0;
        agsm->mic_volume = 0;
        agsm->media_volume = INVALID_MEDIA_VOLUME;
        agsm->volume_listener = NULL;
        agsm->set_volume_cnt = 0;
        agsm->virtual_call_started = false;
        bt_media_set_anc_enable(true);
        bt_pm_conn_close(PROFILE_HFP_AG, &agsm->addr);
        flag_clear(agsm, PENDING_DISCONNECT);
        ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_DISCONNECTED);
    }
}

static void disconnected_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_data_t* data = (hfp_ag_data_t*)p_data;
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
            update_remote_features(agsm, data->valueint3);
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
    case AG_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case AG_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    case AG_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static void connecting_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    agsm->connect_timer = service_loop_timer_no_repeating(AG_TIMEOUT, connect_timeout, agsm);
    ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_CONNECTING);

    bt_pm_busy(PROFILE_HFP_AG, &agsm->addr);
    bt_pm_idle(PROFILE_HFP_AG, &agsm->addr);
}

static void connecting_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
    service_loop_cancel_timer(agsm->connect_timer);
    agsm->connect_timer = NULL;
}

static void ag_retry_callback(service_timer_t* timer, void* data)
{
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    state_machine_t* sm = (state_machine_t*)data;
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_state_t state;

    assert(agsm);

    bt_addr_ba2str(&agsm->addr, _addr_str);
    state = ag_state_machine_get_state(agsm);
    BT_LOGD("%s: device=[%s], state=%d, retry_cnt=%d", __func__, _addr_str, state, agsm->retry_cnt);
    if (state == HFP_AG_STATE_DISCONNECTED) {
        if (bt_sal_hfp_ag_connect(&agsm->addr) == BT_STATUS_SUCCESS) {
            hsm_transition_to(sm, &connecting_state);
        } else {
            BT_LOGI("failed to connect %s", _addr_str);
        }
    }

    agsm->retry_timer = NULL;
}

static void process_vendor_specific_at(bt_address_t* addr, const char* at_string)
{
    uint8_t company_id_index;
    uint16_t company_id;
    size_t prefix_size;
    char command[HFP_COMPANY_PREFIX_LEN_MAX + 1] = { 0 };
    const char* value;
    const vendor_specific_at_prefix_t* prefix;

    if (!at_string)
        return;

    for (company_id_index = 0; company_id_index < ARRAY_SIZE(company_id_map); company_id_index++) {
        prefix = &company_id_map[company_id_index];
        prefix_size = strlen(prefix->at_prefix);
        if (strlen(at_string) <= prefix_size + 3 /* "AT" and "=" */) {
            continue;
        }
        if (strncmp(at_string + 2 /* "AT" */, prefix->at_prefix, prefix_size)) {
            continue;
        }
        value = at_string + strlen(prefix->at_prefix) + 3;  /* The value is the string after "AT+XIAOMI=" */
        if (value[0] == '\r' || value[0] == '\n') {
            break;
        }
        strlcpy(command, prefix->at_prefix, sizeof(command));
        company_id = prefix->company_id;

        BT_LOGD("%s, command:%s, company_id:0x%04X, value:%s", __FUNCTION__, command, company_id, value);
        ag_service_notify_vendor_specific_cmd(addr, command, company_id, value);
        bt_sal_hfp_ag_send_at_cmd(addr, "\r\nOK\r\n", sizeof("\r\nOK\r\n"));
        return;
    }
    BT_LOGD("unknown AT command:%s", at_string);
    bt_sal_hfp_ag_error_response(addr, HFP_ATCMD_RESULT_CMEERR_OPERATION_NOTSUPPORTED);
}

static void hfp_ag_send_vendor_specific_at_cmd(bt_address_t* addr, const char* command, const char* value) {
    char at_command[HFP_AT_LEN_MAX+1] = "\r\n";
    if (!command || !value)
        return;

    strlcat(at_command, command, sizeof(at_command));
    strlcat(at_command, ": ", sizeof(at_command));
    strlcat(at_command, value, sizeof(at_command));
    strlcat(at_command, "\r\n", sizeof(at_command));
    BT_LOGD("%s, command:%s, value:%s", __FUNCTION__, command, value);
    bt_sal_hfp_ag_send_at_cmd(addr, at_command, strlen(at_command));
}

static bool connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_data_t* data = (hfp_ag_data_t*)p_data;
    uint32_t random_timeout;

    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
        /* handle ? */
        break;
    case AG_CONNECT_TIMEOUT:
        bt_sal_hfp_ag_disconnect(&agsm->addr);
        hsm_transition_to(sm, &disconnected_state);
        break;
    case AG_SEND_AT_COMMAND:
        bt_sal_hfp_ag_send_at_cmd(&agsm->addr, data->string1, strlen(data->string1));
        break;
    case AG_SEND_VENDOR_SPECIFIC_AT_COMMAND:
        hfp_ag_send_vendor_specific_at_cmd(&agsm->addr, data->string1, data->string2);
        break;
    case AG_STACK_EVENT_CONNECTION_STATE_CHANGED: {
        profile_connection_state_t state = data->valueint1;
        profile_connection_reason_t reason = data->valueint2;

        switch (state) {
        case PROFILE_STATE_CONNECTED:
            hsm_transition_to(sm, &connected_state);
            update_remote_features(agsm, data->valueint3);
            break;
        case PROFILE_STATE_DISCONNECTED:
            if (reason == PROFILE_REASON_COLLISION && agsm->retry_cnt < HFP_AG_RETRY_MAX) {
                /* failed to establish HFP connection, retry for up to HFP_AG_RETRY_MAX times */
                if (agsm->retry_timer == NULL) {
                    srand(time(NULL)); /* set random seed */
                    random_timeout = 100 + (rand() % 800);
                    BT_LOGD("retry HFP connection with device:[%s], delay=%" PRIu32 "ms",
                        bt_addr_str(&agsm->addr), random_timeout);
                    agsm->retry_timer = service_loop_timer(random_timeout, 0, ag_retry_callback, sm);
                    agsm->retry_cnt++;
                }
            }
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
    case AG_STACK_EVENT_AT_COMMAND:
        process_vendor_specific_at(&agsm->addr, data->string1);
        break;
    case AG_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case AG_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    case AG_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        BT_LOGW("Unexpected event:%" PRId32 "", event);
        break;
    }
    return true;
}

static void disconnecting_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_DISCONNECTING);
}

static void disconnecting_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool disconnecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_data_t* data = (hfp_ag_data_t*)p_data;
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
    case AG_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case AG_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    case AG_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static void handle_ag_set_voice_call_volume(state_machine_t* sm, hfp_volume_type_t type, uint8_t volume)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    bt_status_t status = BT_STATUS_SUCCESS;
    int new_volume = INVALID_MEDIA_VOLUME;

    if (type == HFP_VOLUME_TYPE_SPK) {
        agsm->spk_volume = volume;

        new_volume = bt_media_volume_hfp_to_media(volume);
        if (new_volume == agsm->media_volume) {
            return;
        }

        status = bt_media_set_voice_call_volume(new_volume);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Set media voice call volume failed");
        } else if (agsm->set_volume_cnt < UINT32_MAX) {
            agsm->set_volume_cnt++;
        }

    } else if (type == HFP_VOLUME_TYPE_MIC) {
        agsm->mic_volume = volume;
    }
}

static bool default_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_data_t* data = (hfp_ag_data_t*)p_data;

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
    case AG_PHONE_STATE_CHANGE: {
        uint8_t num_active = data->valueint1;
        uint8_t num_held = data->valueint2;
        hfp_ag_call_state_t call_state = data->valueint3;
        hfp_call_addrtype_t type = data->valueint4;
        if (((num_active + num_held) > 0)
            || (call_state != HFP_AG_CALL_STATE_IDLE && call_state != HFP_AG_CALL_STATE_DISCONNECTED)) {
            set_virtual_call_started(sm, false);
        }

        bt_sal_hfp_ag_phone_state_change(&agsm->addr, num_active, num_held, call_state, type,
            data->string1, data->string2);
    } break;
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
    case AG_SEND_VENDOR_SPECIFIC_AT_COMMAND:
        hfp_ag_send_vendor_specific_at_cmd(&agsm->addr, data->string1, data->string2);
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
    case AG_STACK_EVENT_VOLUME_CHANGED: {
        hfp_volume_type_t type = data->valueint1;
        uint8_t ag_vol = data->valueint2;
        handle_ag_set_voice_call_volume(sm, type, ag_vol);
        BT_LOGD("Volume changed, %s:%" PRIu8, type == HFP_VOLUME_TYPE_MIC ? "Mic" : "Spk", ag_vol);
        ag_service_notify_volume_changed(&agsm->addr, type, ag_vol);
        break;
    }
    case AG_STACK_EVENT_AT_CIND_REQUEST:
        process_cind_request(agsm);
        break;
    case AG_STACK_EVENT_AT_CLCC_REQUEST:
        if (agsm->virtual_call_started) {
            bt_sal_hfp_ag_clcc_response(&agsm->addr, 1, HFP_CALL_DIRECTION_INCOMING,
                HFP_AG_CALL_STATE_ACTIVE, HFP_CALL_MODE_VOICE, HFP_CALL_MPTY_TYPE_SINGLE,
                HFP_CALL_ADDRTYPE_UNKNOWN, HFP_FAKE_NUMBER);
            bt_sal_hfp_ag_clcc_response(&agsm->addr, 0, 0, 0, 0, 0, 0, NULL);
        } else {
            /* system call interface */
            tele_service_query_current_call(&agsm->addr);
        }
        break;
    case AG_STACK_EVENT_AT_COPS_REQUEST: {
        /* system call interface */
        char* operation_name = NULL;
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
        ag_service_notify_call_answered(&agsm->addr);
        break;
    case AG_STACK_EVENT_REJECT_CALL:
        /* system call interface */
        tele_service_reject_call();
        ag_service_notify_call_rejected(&agsm->addr);
        break;
    case AG_STACK_EVENT_HANGUP_CALL:
        /* system call interface */
        tele_service_hangup_call();
        ag_service_notify_call_hangup(&agsm->addr);
        break;
    case AG_STACK_EVENT_DIAL_NUMBER: {
        set_virtual_call_started(sm, false);
        if (data->string1) {
            BT_LOGD("Dial number:%s", data->string1);
            /* system call interface */
            if (tele_service_dial_number(data->string1) != BT_STATUS_SUCCESS)
                bt_sal_hfp_ag_dial_response(&agsm->addr, HFP_ATCMD_RESULT_ERROR);
            else
                agsm->dial_out_timer = service_loop_timer_no_repeating(5000, dial_out_timeout, NULL);
        } else {
            BT_LOGD("Redial last number, currently not supported");
            bt_sal_hfp_ag_dial_response(&agsm->addr, HFP_ATCMD_RESULT_ERROR);
        }
        ag_service_notify_call_dial(&agsm->addr, data->string1 ? data->string1 : NULL);
    } break;
    case AG_STACK_EVENT_DIAL_MEMORY:
        /* system call interface */
        break;
    case AG_STACK_EVENT_CALL_CONTROL: {
        hfp_call_control_t chld = data->valueint1;
        /* system call interface */
        tele_service_call_control(chld);
    } break;
    case AG_STACK_EVENT_AT_COMMAND: {
        const char* at_cmd = data->string1;

        if (at_cmd_check_test(&agsm->addr, at_cmd)) {
            break;
        } else {
            process_vendor_specific_at(&agsm->addr, at_cmd);
        }
    } break;
    case AG_STACK_EVENT_SEND_DTMF:
        /* system call interface */
        break;
    case AG_STACK_EVENT_NREC_REQ:
        /* disable local ANC */
        if (data->valueint1 == 0)
            bt_media_set_anc_enable(false);
        break;
    case AG_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }
    return true;
}

static void default_connection_event_process(state_machine_t* sm, hfp_ag_data_t* data)
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

static void hfp_ag_voice_volume_change_callback(void* cookie, int volume)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)cookie;
    hfp_ag_msg_t* msg;

    agsm->media_volume = volume;
    if (agsm->set_volume_cnt) {
        agsm->set_volume_cnt--;
        return;
    }

    msg = hfp_ag_msg_new(AG_SET_VOLUME, &agsm->addr);
    if (!msg) {
        BT_LOGE("New ag message alloc failed");
        return;
    }

    msg->data.valueint1 = HFP_VOLUME_TYPE_SPK;
    msg->data.valueint2 = volume;
    hfp_ag_send_message(msg);
}

static void set_virtual_call_started(state_machine_t* sm, bool started)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    if (agsm->virtual_call_started == started)
        return;

    BT_LOGD("%s, started = %d", __func__, started);
    agsm->virtual_call_started = started;

    if (started) {
        bt_sal_hfp_ag_phone_state_change(&agsm->addr, 0, 0, HFP_AG_CALL_STATE_DIALING, HFP_CALL_ADDRTYPE_UNKNOWN, "", "");
        bt_sal_hfp_ag_phone_state_change(&agsm->addr, 0, 0, HFP_AG_CALL_STATE_ALERTING, HFP_CALL_ADDRTYPE_UNKNOWN, "", "");
        bt_sal_hfp_ag_phone_state_change(&agsm->addr, 1, 0, HFP_AG_CALL_STATE_ACTIVE, HFP_CALL_ADDRTYPE_UNKNOWN, "", "");
    } else {
        bt_sal_hfp_ag_phone_state_change(&agsm->addr, 0, 0, HFP_AG_CALL_STATE_DISCONNECTED, HFP_CALL_ADDRTYPE_UNKNOWN, "", "");
        bt_sal_hfp_ag_phone_state_change(&agsm->addr, 0, 0, HFP_AG_CALL_STATE_IDLE, HFP_CALL_ADDRTYPE_UNKNOWN, "", "");
    }
}

static void connected_enter(state_machine_t* sm)
{
    hfp_ag_msg_t* msg;
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    uint8_t previous_state = hsm_get_state_value(hsm_get_previous_state(sm));

    bt_pm_conn_open(PROFILE_HFP_AG, &agsm->addr);

    if (previous_state < HFP_AG_STATE_CONNECTED) {
        if (bt_media_get_voice_call_volume(&agsm->media_volume) != BT_STATUS_SUCCESS) {
            BT_LOGE("Get voice call volume failed");
        }
        agsm->volume_listener = bt_media_listen_voice_call_volume_change(hfp_ag_voice_volume_change_callback, agsm);
        agsm->retry_cnt = 0;
        if (!agsm->volume_listener) {
            BT_LOGE("Start to listen voice call volume failed");
        }
        ag_service_notify_connection_state_changed(&agsm->addr, PROFILE_STATE_CONNECTED);
    } else {
        ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
    }
    if (flag_isset(agsm, PENDING_DISCONNECT)) {
        msg = hfp_ag_msg_new(AG_DISCONNECT, &agsm->addr);
        if (msg) {
            ag_state_machine_dispatch(agsm, msg);
            hfp_ag_msg_destory(msg);
        } else {
            BT_LOGE("message alloc failed");
        }
    }
}

static void connected_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static void bt_hci_event_callback(bt_hci_event_t* hci_event, void* context)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)context;
    hfp_ag_msg_t* msg;
    hfp_ag_event_t event;

    BT_LOGD("%s, evt_code:0x%x, len:%d", __func__, hci_event->evt_code,
        hci_event->length);
    BT_DUMPBUFFER("vsc", (uint8_t*)hci_event->params, hci_event->length);

    if (flag_isset(agsm, PENDING_OFFLOAD_START)) {
        event = AG_OFFLOAD_START_EVT;
        flag_clear(agsm, PENDING_OFFLOAD_START);
    } else if (flag_isset(agsm, PENDING_OFFLOAD_STOP)) {
        event = AG_OFFLOAD_STOP_EVT;
        flag_clear(agsm, PENDING_OFFLOAD_STOP);
    } else {
        return;
    }

    msg = hfp_ag_event_new_ext(event, &agsm->addr, hci_event, sizeof(bt_hci_event_t) + hci_event->length);
    hfp_ag_send_message(msg);
}

static bt_status_t ag_offload_send_cmd(ag_state_machine_t* agsm, bool is_start)
{
    uint8_t ogf;
    uint16_t ocf;
    size_t size;
    uint8_t* payload;
    hfp_offload_config_t config = { 0 };
    uint8_t offload[CONFIG_VSC_MAX_LEN];

    config.sco_hdl = agsm->sco_conn_handle;
    config.sco_codec = agsm->codec;
    if (is_start) {
        if (!hfp_offload_start_builder(&config, offload, &size)) {
            BT_LOGE("HFP AG offload start builder failed");
            assert(0);
        }
    } else {
        if (!hfp_offload_stop_builder(&config, offload, &size)) {
            BT_LOGE("HFP AG offload stop builder failed");
            assert(0);
        }
    }

    payload = offload;
    STREAM_TO_UINT8(ogf, payload);
    STREAM_TO_UINT16(ocf, payload);
    size -= sizeof(ogf) + sizeof(ocf);

    return bt_sal_send_hci_command(PRIMARY_ADAPTER, ogf, ocf, size, payload, bt_hci_event_callback, agsm);
}

static bool is_virtual_call_allowed(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    uint32_t state;
    uint8_t num_active, num_held, call_state;

    state = ag_state_machine_get_state(agsm);
    if (state != HFP_AG_STATE_CONNECTED)
        return false;

    if (agsm->virtual_call_started)
        return false;

    tele_service_get_phone_state(&num_active, &num_held, &call_state);
    if (num_active || num_held
        || (call_state != HFP_AG_CALL_STATE_IDLE && call_state != HFP_AG_CALL_STATE_DISCONNECTED))
        return false;

    return true;
}

static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_data_t* data = (hfp_ag_data_t*)p_data;
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
    case AG_START_VIRTUAL_CALL:
        if (!is_virtual_call_allowed(sm)) {
            ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
            return false;
        }

        set_virtual_call_started(sm, true);

        if (bt_sal_hfp_ag_connect_audio(&agsm->addr) != BT_STATUS_SUCCESS) {
            set_virtual_call_started(sm, false);
            ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
            return false;
        }
        hsm_transition_to(sm, &audio_connecting_state);
        break;
    case AG_STACK_EVENT_AUDIO_REQ:
        if (bt_sal_sco_connection_reply(PRIMARY_ADAPTER, &agsm->addr, true) != BT_STATUS_SUCCESS) {
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
            agsm->sco_conn_handle = data->valueint2;
            hsm_transition_to(sm, &audio_on_state);
            break;
        case HFP_AUDIO_STATE_DISCONNECTED:
        default:
            BT_LOGW("Ignored audio connection state:%d", state);
            break;
        }
    } break;
    case AG_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case AG_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    case AG_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        default_process_event(sm, event, p_data);
        break;
    }
    return true;
}

static void audio_connecting_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_CONNECTING);
}

static void audio_connecting_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool audio_connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_data_t* data = (hfp_ag_data_t*)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
        /* Temporary solution: disconnect SLC directly. TODO: defer disconnect message */
        if (bt_sal_hfp_ag_disconnect(&agsm->addr) != BT_STATUS_SUCCESS) {
            ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
        }
        hsm_transition_to(sm, &disconnecting_state);
        break;
    case AG_DISCONNECT_AUDIO:
        /* TODO: handle */
        BT_LOGD("defer DISCONNECT_AUDIO message");
        break;
    case AG_STOP_VIRTUAL_CALL:
        /* TODO: handle */
        BT_LOGD("defer STOP_VITRUAL_CALL message");
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
            agsm->sco_conn_handle = data->valueint2;
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
    case AG_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case AG_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    case AG_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        default_process_event(sm, event, p_data);
        break;
    }
    return true;
}

static void hfp_ag_offload_timeout_callback(service_timer_t* timer, void* data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)data;
    hfp_ag_msg_t* msg;

    msg = hfp_ag_msg_new(AG_OFFLOAD_TIMEOUT_EVT, &agsm->addr);
    ag_state_machine_dispatch(agsm, msg);
    hfp_ag_msg_destory(msg);
}

static void audio_on_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, &agsm->addr);

    bt_pm_sco_open(PROFILE_HFP_AG, &agsm->addr);

    /* TODO: get volume */
    /* TODO: set remote volume */
    bt_media_set_hfp_samplerate(agsm->codec == HFP_CODEC_MSBC ? 16000 : 8000);
    bt_media_set_sco_available();
    ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_CONNECTED);
}

static void audio_on_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_msg_t* msg;

    AG_DBG_EXIT(sm, &agsm->addr);

    bt_pm_sco_close(PROFILE_HFP_AG, &agsm->addr);
    /* set sco device unavaliable */
    bt_media_set_sco_unavailable();

    set_virtual_call_started(sm, false);
    if (agsm->offloading) {
        /* In case that AUDIO_CTRL_CMD_STOP is not received on time */
        msg = hfp_ag_msg_new(AG_OFFLOAD_STOP_REQ, &agsm->addr);
        if (msg) {
            ag_state_machine_dispatch(agsm, msg);
            hfp_ag_msg_destory(msg);
        } else {
            BT_LOGE("message alloc failed");
        }
    }
}

static bool audio_on_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_data_t* data = (hfp_ag_data_t*)p_data;
    uint8_t status;

    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
        if (bt_sal_hfp_ag_disconnect_audio(&agsm->addr) == BT_STATUS_SUCCESS) {
            flag_set(agsm, PENDING_DISCONNECT);
            hsm_transition_to(sm, &audio_disconnecting_state);
            BT_LOGD("Wait for SCO disconnetion first");
            return false;
        }
        if (bt_sal_hfp_ag_disconnect(&agsm->addr) != BT_STATUS_SUCCESS) {
            ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
        }
        hsm_transition_to(sm, &disconnecting_state);
        break;
    case AG_DISCONNECT_AUDIO:
        if (bt_sal_hfp_ag_disconnect_audio(&agsm->addr) != BT_STATUS_SUCCESS) {
            ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
            hsm_transition_to(sm, &connected_state);
            return false;
        }
        hsm_transition_to(sm, &audio_disconnecting_state);
        break;
    case AG_STOP_VIRTUAL_CALL:
        if (!agsm->virtual_call_started) {
            BT_LOGW("Virtual call not started");
            return false;
        }

        set_virtual_call_started(sm, false);

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
        hfp_volume_type_t type = data->valueint1;
        uint8_t ag_vol = bt_media_volume_media_to_hfp(data->valueint2);
        if ((type == HFP_VOLUME_TYPE_SPK) && (ag_vol != agsm->spk_volume)) {
            status = bt_sal_hfp_ag_set_volume(&agsm->addr, type, ag_vol);
            if (status != BT_STATUS_SUCCESS) {
                BT_LOGE("Could not set speaker volume");
                break;
            }
            agsm->spk_volume = ag_vol;
        } else if ((type == HFP_VOLUME_TYPE_MIC) && (ag_vol != agsm->mic_volume)) {
            status = bt_sal_hfp_ag_set_volume(&agsm->addr, type, ag_vol);
            if (status != BT_STATUS_SUCCESS) {
                BT_LOGE("Could not set mic volume");
                break;
            }
            agsm->mic_volume = ag_vol;
        }
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
    case AG_OFFLOAD_START_REQ:
        if (ag_offload_send_cmd(agsm, true) != BT_STATUS_SUCCESS) {
            BT_LOGE("failed to start offload");
            audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
            break;
        }
        flag_set(agsm, PENDING_OFFLOAD_START);
        agsm->offload_timer = service_loop_timer(AG_OFFLOAD_TIMEOUT, 0, hfp_ag_offload_timeout_callback, agsm);
        break;
    case AG_OFFLOAD_START_EVT: {
        bt_hci_event_t* hci_event;
        hci_error_t result;

        if (agsm->offload_timer) {
            service_loop_cancel_timer(agsm->offload_timer);
            agsm->offload_timer = NULL;
        }

        hci_event = data->data;
        result = hci_get_result(hci_event);
        if (result != HCI_SUCCESS) {
            BT_LOGE("AG_OFFLOAD_START fail, status:0x%0x", result);
            audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
            if (bt_sal_hfp_ag_disconnect_audio(&agsm->addr) != BT_STATUS_SUCCESS) {
                BT_ADDR_LOG("Terminate audio failed for :%s", &agsm->addr);
            }
            break;
        }

        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STARTED);
        break;
    }
    case AG_OFFLOAD_TIMEOUT_EVT: {
        flag_clear(agsm, PENDING_OFFLOAD_START);
        agsm->offload_timer = NULL;
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
        if (bt_sal_hfp_ag_disconnect_audio(&agsm->addr) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("Terminate audio failed for :%s", &agsm->addr);
        }
        break;
    } break;
    case AG_OFFLOAD_STOP_REQ:
        if (agsm->offload_timer) {
            service_loop_cancel_timer(agsm->offload_timer);
            agsm->offload_timer = NULL;
            audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
        }
        if (ag_offload_send_cmd(agsm, false) != BT_STATUS_SUCCESS) {
            BT_LOGE("failed to stop offload");
            audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
            break;
        }
        flag_set(agsm, PENDING_OFFLOAD_STOP);
        break;
    case AG_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        default_process_event(sm, event, p_data);
        break;
    }
    return true;
}

static void audio_disconnecting_enter(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_ENTER(sm, &agsm->addr);
    ag_service_notify_audio_state_changed(&agsm->addr, HFP_AUDIO_STATE_DISCONNECTING);
}

static void audio_disconnecting_exit(state_machine_t* sm)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    AG_DBG_EXIT(sm, &agsm->addr);
}

static bool audio_disconnecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ag_state_machine_t* agsm = (ag_state_machine_t*)sm;
    hfp_ag_data_t* data = (hfp_ag_data_t*)p_data;
    AG_DBG_EVENT(sm, &agsm->addr, event);

    switch (event) {
    case AG_DISCONNECT:
        /* TODO: defer disconnect message */
        bt_sal_hfp_ag_disconnect(&agsm->addr);
        hsm_transition_to(sm, &disconnecting_state);
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
    case AG_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case AG_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    case AG_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_AG, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        default_process_event(sm, event, p_data);
        break;
    }
    return true;
}

ag_state_machine_t* ag_state_machine_new(bt_address_t* addr, void* context)
{
    ag_state_machine_t* agsm;

    agsm = (ag_state_machine_t*)malloc(sizeof(ag_state_machine_t));
    if (!agsm)
        return NULL;

    memset(agsm, 0, sizeof(ag_state_machine_t));
    agsm->recognition_active = false;
    agsm->virtual_call_started = false;
    agsm->service = context;
    agsm->connect_timer = NULL;
    agsm->audio_timer = NULL;
    agsm->dial_out_timer = NULL;
    agsm->codec = HFP_CODEC_CVSD;
    agsm->media_volume = INVALID_MEDIA_VOLUME;
    memcpy(&agsm->addr, addr, sizeof(bt_address_t));
    hsm_ctor(&agsm->sm, (state_t*)&disconnected_state);

    return agsm;
}

void ag_state_machine_destory(ag_state_machine_t* agsm)
{
    if (!agsm)
        return;

    if (agsm->connect_timer)
        service_loop_cancel_timer(agsm->connect_timer);

    if (agsm->retry_timer)
        service_loop_cancel_timer(agsm->retry_timer);

    bt_media_remove_listener(agsm->volume_listener);
    agsm->volume_listener = NULL;
    hsm_dtor(&agsm->sm);
    free((void*)agsm);
}

void ag_state_machine_dispatch(ag_state_machine_t* agsm, hfp_ag_msg_t* msg)
{
    if (!agsm || !msg)
        return;

    hsm_dispatch_event(&agsm->sm, msg->event, &msg->data);
}

uint32_t ag_state_machine_get_state(ag_state_machine_t* agsm)
{
    return hsm_get_current_state_value(&agsm->sm);
}

uint16_t ag_state_machine_get_sco_handle(ag_state_machine_t* agsm)
{
    return agsm->sco_conn_handle;
}

void ag_state_machine_set_sco_handle(ag_state_machine_t* agsm, uint16_t sco_hdl)
{
    agsm->sco_conn_handle = sco_hdl;
}

uint8_t ag_state_machine_get_codec(ag_state_machine_t* agsm)
{
    return agsm->codec;
}

void ag_state_machine_set_offloading(ag_state_machine_t* agsm, bool offloading)
{
    agsm->offloading = offloading;
}
