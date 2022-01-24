/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#define LOG_TAG "hf_stm"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"
#include "stack_adapter_hfp.h"
#include "stack_adapter_service_base.h"

#include "btm_hfp_hf.h"
#include "bts_hf_client_event.h"
#include "bts_hf_client_state_machine.h"
#include "utils/log.h"
#include "utils/utils.h"

#define HF_CONNECT_TIMEOUT 4 * 1000
#define HF_SERVICE_CBACK(P_CB, P_CBACK, ...)                     \
    do {                                                         \
        if ((P_CB) && (P_CB)->P_CBACK) {                         \
            (P_CB)->P_CBACK(__VA_ARGS__);                        \
        }                                                        \
    } while (0)

typedef struct _hf_state_machine {
    state_machine_t         sm;
    bt_address              addr;
    uint16_t                sco_conn_handle;
    uv_timer_t*             connect_timer;
    bool                    recognition_active;
    uint8_t                 spk_volume;
    uint8_t                 mic_volume;
    uint8_t                 codec;
    struct list_node        pending_actions;
    hf_client_service_t*    service;
} hf_state_machine_t;

typedef struct {
    struct list_node node;
    uint32_t cmd_code;
} hf_at_cmd_t;

static void disconnected_enter(state_machine_t* sm);
static void disconnected_exit(state_machine_t* sm);
static void connecting_enter(state_machine_t* sm);
static void connecting_exit(state_machine_t* sm);
static void connected_enter(state_machine_t* sm);
static void connected_exit(state_machine_t* sm);
static void audio_on_enter(state_machine_t* sm);
static void audio_on_exit(state_machine_t* sm);

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool audio_on_process_event(state_machine_t* sm, uint32_t event, void* p_data);

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

static const state_t connected_state = {
    .state_name = "Connected",
    .enter = connected_enter,
    .exit = connected_exit,
    .process_event = connected_process_event,
};

static const state_t audio_on_state = {
    .state_name = "AduioOn",
    .enter = audio_on_enter,
    .exit = audio_on_exit,
    .process_event = audio_on_process_event,
};

static char* stack_event_to_string(hf_client_event_t event)
{
    switch (event) {
        CASE_RETURN_STR(CONNECT)
        CASE_RETURN_STR(DISCONNECT)
        CASE_RETURN_STR(CONNECT_AUDIO)
        CASE_RETURN_STR(DISCONNECT_AUDIO)
        CASE_RETURN_STR(VOICE_RECOGNITION_START)
        CASE_RETURN_STR(VOICE_RECOGNITION_STOP)
        CASE_RETURN_STR(SET_MIC_VOLUME)
        CASE_RETURN_STR(SET_SPEAKER_VOLUME)
        CASE_RETURN_STR(DIAL_NUMBER)
        CASE_RETURN_STR(DIAL_MEMORY)
        CASE_RETURN_STR(DIAL_LAST)
        CASE_RETURN_STR(ACCEPT_CALL)
        CASE_RETURN_STR(REJECT_CALL)
        CASE_RETURN_STR(HOLD_CALL)
        CASE_RETURN_STR(TERMINATE_CALL)
        CASE_RETURN_STR(QUERY_CURRENT_CALLS)
        CASE_RETURN_STR(UPDATE_BATTERY_LEVEL)
        CASE_RETURN_STR(SEND_AT_COMMAND)
        CASE_RETURN_STR(TIMEOUT)
        CASE_RETURN_STR(STACK_EVENT)
        CASE_RETURN_STR(STACK_EVENT_AUDIO_REQ)
        CASE_RETURN_STR(STACK_EVENT_CONNECTION_STATE_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_AUDIO_STATE_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_VR_STATE_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_CALL)
        CASE_RETURN_STR(STACK_EVENT_CALLSETUP)
        CASE_RETURN_STR(STACK_EVENT_CALLHELD)
        CASE_RETURN_STR(STACK_EVENT_CLIP)
        CASE_RETURN_STR(STACK_EVENT_CALL_WAITING)
        CASE_RETURN_STR(STACK_EVENT_CURRENT_CALLS)
        CASE_RETURN_STR(STACK_EVENT_VOLUME_CHANGED)
        CASE_RETURN_STR(STACK_EVENT_CMD_RESPONSE)
        CASE_RETURN_STR(STACK_EVENT_CMD_RESULT)
        CASE_RETURN_STR(STACK_EVENT_RING_INDICATION)
        CASE_RETURN_STR(STACK_EVENT_CODEC_CHANGED)
    default:
        return "UNKNOWN_EVENT";
    }
}

static void add_pending_action(hf_state_machine_t* hfsm, uint32_t cmd_code)
{
    hf_at_cmd_t* cmd = malloc(sizeof(hf_at_cmd_t));

    cmd->cmd_code = cmd_code;
    list_add_tail(&hfsm->pending_actions, &cmd->node);
}

static uint32_t first_pending_action(hf_state_machine_t* hfsm)
{
    struct list_node* node;

    node = list_remove_head(&hfsm->pending_actions);
    if (node) {
        uint32_t code = ((hf_at_cmd_t*)node)->cmd_code;
        free(node);
        return code;
    }

    return 0;
}

static void notify_connection_state_changed(hf_client_service_t* service,
    bt_address addr,
    hf_client_connection_state_t state)
{
    if (state == HF_CLIENT_CONNECTION_STATE_CONNECTED)
        BT_LOGD("PERFORMANCE-HF-BTM-CONNECTED");
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
    HF_SERVICE_CBACK(service->callbacks, connection_state_cb, addr, state);
}

static void notify_audio_state_changed(hf_client_service_t* service,
    bt_address addr,
    hf_client_audio_state_t state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
    HF_SERVICE_CBACK(service->callbacks, audio_state_cb, addr, state);
}

static void notify_vr_state_changed(hf_client_service_t* service,
    bt_address addr,
    hf_client_vr_state_t state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
    HF_SERVICE_CBACK(service->callbacks, vr_cmd_cb, addr, state);
}

static void disconnected_enter(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(hfsm->addr));
}
static void disconnected_exit(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(hfsm->addr));
}

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hf_event_data_t* data = (hf_event_data_t*)p_data;
    SERVICE_BT_STATUS status;

    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(hfsm->addr));
    switch (event) {
    case CONNECT:
        //check bonded state
        //check address (hfsm->addr == data->bd_addr)
        BT_LOGD("PERFORMANCE-HF-BLUELET-CONNECT_START");
        status = service_adapter_hfp_connect(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Connect failed for %s", addr_str(hfsm->addr));
            notify_connection_state_changed(hfsm->service, hfsm->addr,
                HF_CLIENT_CONNECTION_STATE_DISCONNECTED);
            break;
        }

        hsm_transition_to(sm, &connecting_state);
        break;

    case STACK_EVENT_CONNECTION_STATE_CHANGED: {
        //check bonded state;
        hf_client_connection_state_t state = data->valueint1;

        switch (state) {
        case HF_CLIENT_CONNECTION_STATE_CONNECTED:
            notify_connection_state_changed(hfsm->service, hfsm->addr, state);
            hsm_transition_to(sm, &connected_state);
            break;
        case HF_CLIENT_CONNECTION_STATE_CONNECTING:
        case HF_CLIENT_CONNECTION_STATE_DISCONNECTED:
        case HF_CLIENT_CONNECTION_STATE_DISCONNECTING:
            BT_LOGW("Ignored connection state:%d", state);
            break;
        default:
            break;
        }
        break;
    }

    case STACK_EVENT_CODEC_CHANGED:
        hfsm->codec = data->valueint1;
        break;

    default:
        BT_LOGE("Disconnected: Unexpected stack event: %s", stack_event_to_string(event));
        break;
    }

    return true;
}

static void hf_connect_timeout_callback(char* data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)data;

    hf_client_msg_t* msg = hf_client_msg_new(TIMEOUT, hfsm->addr);
    hf_client_state_machine_handle_msg(hfsm, msg);
    hf_client_msg_destory(msg);
}

static void connecting_enter(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(hfsm->addr));
    //start connecting timeout timer
    hfsm->connect_timer = start_timer(HF_CONNECT_TIMEOUT, 0, hf_connect_timeout_callback, hfsm);
}

static void connecting_exit(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(hfsm->addr));
    //stop timer
    stop_timer(hfsm->connect_timer);
    hfsm->connect_timer = NULL;
}

static bool connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hf_event_data_t* data = (hf_event_data_t*)p_data;

    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(hfsm->addr));
    switch (event) {
    case STACK_EVENT_CONNECTION_STATE_CHANGED: {
        hf_client_connection_state_t state = data->valueint1;

        switch (state) {
        case HF_CLIENT_CONNECTION_STATE_DISCONNECTED:
            notify_connection_state_changed(hfsm->service, hfsm->addr, state);
            hsm_transition_to(sm, &disconnected_state);
            break;
        case HF_CLIENT_CONNECTION_STATE_CONNECTED:
            notify_connection_state_changed(hfsm->service, hfsm->addr, state);
            service_adapter_hfp_set_volume(hfsm->addr, VOLUME_MIC, 5);
            service_adapter_hfp_set_volume(hfsm->addr, VOLUME_SPEAKER, 5);
            hsm_transition_to(sm, &connected_state);
            break;
        case HF_CLIENT_CONNECTION_STATE_CONNECTING:
        case HF_CLIENT_CONNECTION_STATE_DISCONNECTING:
            BT_LOGW("Ignored connection state:%d", state);
            break;
        default:
            break;
        }
        break;
    }

    case STACK_EVENT_CODEC_CHANGED:
        hfsm->codec = data->valueint1;
        break;

    case TIMEOUT:
        BT_LOGD("Connection timeout");
        //slc connection callback
        hsm_transition_to(sm, &disconnected_state);
        break;

    default:
        break;
    }
    return true;
}

static void connected_enter(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(hfsm->addr));
    service_adapter_hfp_send_battery_value(hfsm->addr, 100);
}

static void connected_exit(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(hfsm->addr));
}

static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hf_client_service_t* service = hfsm->service;
    hf_event_data_t* data = (hf_event_data_t*)p_data;
    SERVICE_BT_STATUS status;

    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(hfsm->addr));
    switch (event) {
    case CONNECT:
        //no handle
        break;

    case DISCONNECT:
        //do disconnect
        status = service_adapter_hfp_disconnect(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect failed for :%s", addr_str(hfsm->addr));
        }
        break;

    case CONNECT_AUDIO:
        status = service_adapter_hfp_create_sco(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            //callback audio state disconnted
            BT_LOGE("Connect audio failed for :%s", addr_str(hfsm->addr));
        }
        break;

    case DISCONNECT_AUDIO:
        status = service_adapter_hfp_disconnect_sco(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect audio failed for :%s", addr_str(hfsm->addr));
        }
        break;

    case VOICE_RECOGNITION_START:
        if (!hfsm->recognition_active) {
            status = service_adapter_hfp_enable_voice_recognition(hfsm->addr);
            if (status != SERVICE_BT_STATUS_SUCCESS) {
                BT_LOGE("Could not start voice recognition");
            }
        }
        break;

    case VOICE_RECOGNITION_STOP:
        if (hfsm->recognition_active) {
            status = service_adapter_hfp_disable_voice_recognition(hfsm->addr);
            if (status != SERVICE_BT_STATUS_SUCCESS) {
                BT_LOGE("Could not stop voice recognition");
            }
        }
        break;

    case SET_MIC_VOLUME: {
        uint8_t vol = data->valueint1;
        vol = vol > 15 ? 15 : vol;
        //transfer to hf volume
        service_adapter_hfp_set_volume(hfsm->addr, VOLUME_MIC, vol);
        break;
    }

    case SET_SPEAKER_VOLUME: {
        uint8_t vol = data->valueint1;
        vol = vol > 15 ? 15 : vol;
        //transfer to hf volume
        service_adapter_hfp_set_volume(hfsm->addr, VOLUME_SPEAKER, vol);
        break;
    }

    case DIAL_NUMBER:
        status = service_adapter_hfp_dial_number(hfsm->addr, data->string1);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Dial number: %s failed", data->string1);
        }
        break;

    case DIAL_MEMORY: {
        int memory = data->valueint1;
        BT_LOGD("Dial memory :%d", memory);
        status = service_adapter_hfp_dial_memory(hfsm->addr, memory);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Dial memory: %d failed", memory);
        }
        add_pending_action(hfsm, HFP_ATCC_ATD);
        break;
    }

    case DIAL_LAST: {
        status = service_adapter_hfp_dial_number(hfsm->addr, NULL);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Dial Last failed");
        }
        break;
    }

    case ACCEPT_CALL:
        status = service_adapter_hfp_answer_call(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Answer call failed");
        }
        break;

    case REJECT_CALL:
        status = service_adapter_hfp_reject_call(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Reject call failed");
        }
        break;

    case HOLD_CALL:
        status = service_adapter_hfp_call_control(hfsm->addr, HFP_CALL_CONTROL_CHLD_2, 0);
        //status = service_adapter_hfp_hold_call(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Hold call failed");
        }
        break;

    case TERMINATE_CALL:
        status = service_adapter_hfp_hangup_call(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Terminate call failed");
        }
        break;

    case QUERY_CURRENT_CALLS:
        status = service_adapter_hfp_get_current_calls(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Query current call failed");
        }
        break;

    case SEND_AT_COMMAND: {
        SERVICE_HFP_AT_CMD_S atcmd;

        atcmd.at_string = data->string1;
        atcmd.at_length = strlen(data->string1);
        status = service_adapter_hfp_send_at_cmd(hfsm->addr, &atcmd, NULL);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Send at command failed");
        }
        break;
    }

    case UPDATE_BATTERY_LEVEL:
        status = service_adapter_hfp_send_battery_value(hfsm->addr, (uint8_t)data->valueint1);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Update battery level failed");
        }
        break;

    case STACK_EVENT_AUDIO_REQ:
        status = service_adapter_gap_accept_sco_link(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Accept Sco connection failed");
        }
        break;

    case STACK_EVENT_CONNECTION_STATE_CHANGED: {
        hf_client_connection_state_t state = data->valueint1;

        switch (state) {
        case HF_CLIENT_CONNECTION_STATE_DISCONNECTED:
            notify_connection_state_changed(hfsm->service, hfsm->addr, state);
            hsm_transition_to(sm, &disconnected_state);
            break;
        case HF_CLIENT_CONNECTION_STATE_CONNECTED:
            //service_adapter_hfp_set_volume(remote_addr, VOLUME_SPEAKER, vol);
            notify_connection_state_changed(hfsm->service, hfsm->addr, state);
            hsm_transition_to(sm, &connected_state);
            break;
        case HF_CLIENT_CONNECTION_STATE_CONNECTING:
        case HF_CLIENT_CONNECTION_STATE_DISCONNECTING:
            break;
        }
        break;
    }

    case STACK_EVENT_AUDIO_STATE_CHANGED: {
        hf_client_audio_state_t state = data->valueint1;

        switch (state) {
        case HF_CLIENT_AUDIO_STATE_CONNECTED:
            //set audio focus, route audio channel
            if (hfsm->codec == SERVICE_HFP_CODEC_MSBC)
                state = HF_CLIENT_AUDIO_STATE_CONNECTED_MSBC;
            notify_audio_state_changed(hfsm->service, hfsm->addr, state);
            hfsm->sco_conn_handle = data->valueint2;
            hsm_transition_to(sm, &audio_on_state);
            break;
        case HF_CLIENT_AUDIO_STATE_DISCONNECTED:
        default:
            break;
        }
        break;
    }

    case STACK_EVENT_VR_STATE_CHANGED: {
        hf_client_vr_state_t state = data->valueint1;

        notify_vr_state_changed(hfsm->service, hfsm->addr, state);
        if (state == HF_CLIENT_VR_STATE_STOPPED)
            hfsm->recognition_active = false;
        else
            hfsm->recognition_active = true;
        break;
    }

    case STACK_EVENT_CALL: {
        hf_client_call_t call = data->valueint1;

        HF_SERVICE_CBACK(service->callbacks, call_cb, hfsm->addr, call);
        break;
    }

    case STACK_EVENT_CALLSETUP: {
        hf_client_callsetup_t setup = data->valueint1;

        HF_SERVICE_CBACK(service->callbacks, callsetup_cb, hfsm->addr, setup);
        break;
    }

    case STACK_EVENT_CALLHELD: {
        hf_client_callheld_t held = data->valueint1;

        HF_SERVICE_CBACK(service->callbacks, callheld_cb, hfsm->addr, held);
        break;
    }

    case STACK_EVENT_CLIP: {
        char* number = data->string1;
        char* name = data->string2;

        BT_LOGD("CLIP:number :%s, name: %s", number, name == NULL ? "NULL" : name);
        HF_SERVICE_CBACK(service->callbacks, clip_cb, hfsm->addr, number, name);
        break;
    }

    case STACK_EVENT_CALL_WAITING:
        //not support
        break;

    case STACK_EVENT_CURRENT_CALLS: {
        int index = data->valueint1;
        hf_client_call_direction_t dir = data->valueint2;
        hf_client_call_state_t state = data->valueint3;
        hf_client_call_mpty_type_t mpty = data->valueint4;
        char* number = data->string1;
        if (index == 0) {
            BT_LOGD("Query current call final");
        } else {
            BT_LOGD("Current Call[%d]: dir:%d, state:%d, mpty:%d, number:%s", index, dir, state, mpty, number);
        }
        HF_SERVICE_CBACK(service->callbacks, current_calls_cb, hfsm->addr, index, dir, state, mpty, (const char*)number);
        break;
    }

    case STACK_EVENT_VOLUME_CHANGED: {
        hf_client_volume_type_t type = data->valueint1;
        int vol = data->valueint2;
        //set media volume, need call media interface
        BT_LOGD("Volume changed, %s:%d", type ? "Mic" : "Spk", vol);
        HF_SERVICE_CBACK(service->callbacks, volume_change_cb, hfsm->addr, type, vol);
        break;
    }

    case STACK_EVENT_CMD_RESPONSE: {
        const char* resp = data->string1;

        HF_SERVICE_CBACK(service->callbacks, cmd_complete_cb, hfsm->addr, resp);
        break;
    }

    case STACK_EVENT_CMD_RESULT: {
        uint32_t cmd_code = data->valueint1;
        uint32_t cmd_result = data->valueint2;
        uint32_t pending;

        pending = first_pending_action(hfsm);
        if (pending == cmd_code) {
            switch (cmd_code) {
            case HFP_ATCC_ATD:
                if (cmd_result != HFP_ATC_RESULT_OK) {
                    BT_LOGE("Dial memory failed:%lu", cmd_result);
                }
                break;
            }
        }
        break;
    }

    case STACK_EVENT_RING_INDICATION: {
        int active = data->valueint1;
        hf_client_in_band_ring_state_t ring_state = data->valueint2;
        if (active)
            HF_SERVICE_CBACK(service->callbacks, ring_indication_cb, hfsm->addr, ring_state);
        break;
    }

    case STACK_EVENT_CODEC_CHANGED:
        hfsm->codec = data->valueint1;
        break;

    default:
        break;
    }
    return true;
}

static void audio_on_enter(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(hfsm->addr));
}

static void audio_on_exit(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(hfsm->addr));
}

static bool audio_on_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hf_client_service_t* service = hfsm->service;
    hf_event_data_t* data = (hf_event_data_t*)p_data;
    SERVICE_BT_STATUS status;

    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(hfsm->addr));
    switch (event) {
    case DISCONNECT:
        break;

    case DISCONNECT_AUDIO:
        status = service_adapter_hfp_disconnect_sco(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect Sco connection failed");
        }
        break;

    case VOICE_RECOGNITION_STOP:
        if (hfsm->recognition_active) {
            status = service_adapter_hfp_disable_voice_recognition(hfsm->addr);
            if (status != SERVICE_BT_STATUS_SUCCESS) {
                BT_LOGE("Could not stop voice recognition");
            }
        }
        break;

    case SET_MIC_VOLUME: {
        uint8_t vol = data->valueint1;
        vol = vol > 15 ? 15 : vol;
        //transfer to hf volume
        BT_LOGD("Set Mic Volume :%d", vol);
        service_adapter_hfp_set_volume(hfsm->addr, VOLUME_MIC, vol);
        break;
    }

    case SET_SPEAKER_VOLUME: {
        uint8_t vol = data->valueint1;
        vol = vol > 15 ? 15 : vol;

        BT_LOGD("Set Speaker Volume :%d", vol);
        //transfer to hf volume
        service_adapter_hfp_set_volume(hfsm->addr, VOLUME_SPEAKER, vol);
        break;
    }

    case REJECT_CALL:
        status = service_adapter_hfp_call_control(hfsm->addr, HFP_CALL_CONTROL_CHLD_0, 0);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Reject call failed");
        }
        break;

    case HOLD_CALL:
        status = service_adapter_hfp_call_control(hfsm->addr, HFP_CALL_CONTROL_CHLD_2, 0);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Hold call failed");
        }
        break;

    case TERMINATE_CALL:
        status = service_adapter_hfp_hangup_call(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Terminate call failed");
        }
        break;

    case QUERY_CURRENT_CALLS:
        status = service_adapter_hfp_get_current_calls(hfsm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Query current call failed");
        }
        break;

    case UPDATE_BATTERY_LEVEL:
        status = service_adapter_hfp_send_battery_value(hfsm->addr, (uint8_t)data->valueint1);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Update battery level failed");
        }
        break;

    case STACK_EVENT_VR_STATE_CHANGED: {
        hf_client_vr_state_t state = data->valueint1;

        notify_vr_state_changed(hfsm->service, hfsm->addr, state);
        if (state == HF_CLIENT_VR_STATE_STOPPED)
            hfsm->recognition_active = false;
        else
            hfsm->recognition_active = true;
        break;
    }

    case STACK_EVENT_CALL: {
        hf_client_call_t call = data->valueint1;

        HF_SERVICE_CBACK(service->callbacks, call_cb, hfsm->addr, call);
        break;
    }

    case STACK_EVENT_CALLSETUP: {
        hf_client_callsetup_t setup = data->valueint1;

        HF_SERVICE_CBACK(service->callbacks, callsetup_cb, hfsm->addr, setup);
        break;
    }

    case STACK_EVENT_CALLHELD: {
        hf_client_callheld_t held = data->valueint1;

        HF_SERVICE_CBACK(service->callbacks, callheld_cb, hfsm->addr, held);
        break;
    }

    case STACK_EVENT_CURRENT_CALLS: {
        int index = data->valueint1;
        hf_client_call_direction_t dir = data->valueint2;
        hf_client_call_state_t state = data->valueint3;
        hf_client_call_mpty_type_t mpty = data->valueint4;
        char* number = data->string1;
        if (index == 0) {
            BT_LOGD("Query current call final");
        } else {
            BT_LOGD("Current Call[%d]: dir:%d, state:%d, mpty:%d, number:%s", index, dir, state, mpty, number);
        }

        HF_SERVICE_CBACK(service->callbacks, current_calls_cb, hfsm->addr, index, dir, state, mpty, (const char*)number);
        break;
    }

    case STACK_EVENT_VOLUME_CHANGED: {
        hf_client_volume_type_t type = data->valueint1;
        int vol = data->valueint2;
        //set media volume, need call media interface
        BT_LOGD("Volume changed, %s:%d", type ? "Mic" : "Spk", vol);
        HF_SERVICE_CBACK(service->callbacks, volume_change_cb, hfsm->addr, type, vol);
        break;
    }

    case STACK_EVENT_CONNECTION_STATE_CHANGED: {
        hf_client_connection_state_t state = data->valueint1;

        switch (state) {
        case HF_CLIENT_CONNECTION_STATE_DISCONNECTED:
            //set audio focus, route audio
            notify_audio_state_changed(hfsm->service, hfsm->addr, state);
            notify_connection_state_changed(hfsm->service, hfsm->addr, state);
            hsm_transition_to(sm, &disconnected_state);
            break;
        case HF_CLIENT_CONNECTION_STATE_DISCONNECTING:
        case HF_CLIENT_CONNECTION_STATE_CONNECTED:
        case HF_CLIENT_CONNECTION_STATE_CONNECTING:
            BT_LOGE("Receive state change in unexpect state: %d", state);
            break;
        }
        break;
    }

    case STACK_EVENT_AUDIO_STATE_CHANGED: {
        hf_client_audio_state_t state = data->valueint1;

        switch (state) {
        case HF_CLIENT_AUDIO_STATE_DISCONNECTED:
            //set audio focus, route audio
            notify_audio_state_changed(hfsm->service, hfsm->addr, state);
            hsm_transition_to(sm, &connected_state);
            break;
        case HF_CLIENT_AUDIO_STATE_CONNECTED:
            break;
        default:
            break;
        }
        break;
    }

    case STACK_EVENT_CMD_RESPONSE: {
        const char* resp = data->string1;

        HF_SERVICE_CBACK(service->callbacks, cmd_complete_cb, hfsm->addr, resp);
        break;
    }

    case STACK_EVENT_CMD_RESULT:
        break;

    default:
        break;
    }

    return true;
}

static void hf_client_event_dispatch(hf_state_machine_t* hfsm, hf_client_msg_t* msg)
{
    if (!msg)
        return;

    hsm_dispatch_event(&hfsm->sm, msg->event, &msg->event_data);
}

hf_state_machine_t* hf_client_state_machine_new(hf_client_service_t* context,
    bt_address bd_addr)
{
    hf_state_machine_t* hfsm;

    hfsm = (hf_state_machine_t*)malloc(sizeof(hf_state_machine_t));
    if (!hfsm)
        return NULL;

    hfsm->connect_timer = NULL;
    hfsm->recognition_active = false;
    hfsm->service = context;
    list_initialize(&hfsm->pending_actions);
    hsm_ctor(&hfsm->sm, (state_t*)&disconnected_state);
    memcpy(hfsm->addr, bd_addr, sizeof(bt_address));

    return hfsm;
}

void hf_client_state_machine_destory(hf_state_machine_t* hfsm)
{
    if (!hfsm)
        return;

    if (hfsm->connect_timer)
        stop_timer(hfsm->connect_timer);
    hsm_dtor(&hfsm->sm);
    free((void*)hfsm);
}

void hf_client_state_machine_handle_msg(hf_state_machine_t* sm,
    hf_client_msg_t* msg)
{
    hf_client_event_dispatch(sm, msg);
}

hf_client_connection_state_t hf_client_get_conn_state(hf_state_machine_t* sm)
{
    state_t* state = hsm_get_current_state(&sm->sm);

    if (state == (state_t*)&disconnected_state) {
        return HF_CLIENT_CONNECTION_STATE_DISCONNECTED;
    } else if (state == (state_t*)&connecting_state) {
        return HF_CLIENT_CONNECTION_STATE_CONNECTING;
    } else if (state == (state_t*)&connected_state || state == (state_t*)&audio_on_state) {
        return HF_CLIENT_CONNECTION_STATE_CONNECTED;
    } else {
        BT_LOGE("%s:Unknow State", __func__);
        return HF_CLIENT_CONNECTION_STATE_DISCONNECTED;
    }
}
