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
#define LOG_TAG "hf_stm"

#include <nuttx/list.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_addr.h"
#include "bt_hfp_hf.h"
#include "bt_list.h"
#include "hfp_hf_service.h"
#include "hfp_hf_state_machine.h"
#include "sal_adapter_interface.h"
#include "sal_hfp_hf_interface.h"
#include "service_loop.h"

#include "bt_utils.h"
#include "utils/log.h"

typedef struct _hf_state_machine {
    state_machine_t sm;
    bt_address_t addr;
    uint16_t sco_conn_handle;
    service_timer_t *connect_timer;
    bool recognition_active;
    uint8_t spk_volume;
    uint8_t mic_volume;
    uint8_t codec;
    uint8_t call_in_progress;
    struct list_node pending_actions;
    bt_list_t *current_calls;
    bt_list_t *update_calls;
    uint8_t need_query;
    void *service;
} hf_state_machine_t;

typedef struct {
    struct list_node node;
    uint32_t cmd_code;
} hf_at_cmd_t;

#define HF_CONNECT_TIMEOUT 10 * 1000
#define HF_STM_DEBUG       1

#if HF_STM_DEBUG
static void hf_stm_trans_debug(state_machine_t *sm, bt_address_t *addr, const char *action);
static void hf_stm_event_debug(state_machine_t *sm, bt_address_t *addr, uint32_t event);
static const char *stack_event_to_string(hfp_hf_event_t event);

#define HF_DBG_ENTER(__sm, __addr)          hf_stm_trans_debug(__sm, __addr, "Enter")
#define HF_DBG_EXIT(__sm, __addr)           hf_stm_trans_debug(__sm, __addr, "Exit ")
#define HF_DBG_EVENT(__sm, __addr, __event) hf_stm_event_debug(__sm, __addr, __event);
#else
#define HF_DBG_ENTER(__sm, __addr)
#define HF_DBG_EXIT(__sm, __addr)
#define HF_DBG_EVENT(__sm, __addr, __event)
#endif

static void disconnected_enter(state_machine_t *sm);
static void disconnected_exit(state_machine_t *sm);
static void connecting_enter(state_machine_t *sm);
static void connecting_exit(state_machine_t *sm);
static void connected_enter(state_machine_t *sm);
static void connected_exit(state_machine_t *sm);
static void audio_on_enter(state_machine_t *sm);
static void audio_on_exit(state_machine_t *sm);

static bool disconnected_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool connecting_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool connected_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool audio_on_process_event(state_machine_t *sm, uint32_t event, void *p_data);

static const state_t disconnected_state = {
    .state_name = "Disconnected",
    .state_value = HFP_HF_STATE_DISCONNECTED,
    .enter = disconnected_enter,
    .exit = disconnected_exit,
    .process_event = disconnected_process_event,
};

static const state_t connecting_state = {
    .state_name = "Connecting",
    .state_value = HFP_HF_STATE_CONNECTING,
    .enter = connecting_enter,
    .exit = connecting_exit,
    .process_event = connecting_process_event,
};

static const state_t connected_state = {
    .state_name = "Connected",
    .state_value = HFP_HF_STATE_CONNECTED,
    .enter = connected_enter,
    .exit = connected_exit,
    .process_event = connected_process_event,
};

static const state_t audio_on_state = {
    .state_name = "AudioOn",
    .state_value = HFP_HF_STATE_AUDIO_CONNECTED,
    .enter = audio_on_enter,
    .exit = audio_on_exit,
    .process_event = audio_on_process_event,
};

#if HF_STM_DEBUG
static void hf_stm_trans_debug(state_machine_t *sm, bt_address_t *addr, const char *action)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s State=%s, Peer=[%s]", action, hsm_get_current_state_name(sm), addr_str);
}

static void hf_stm_event_debug(state_machine_t *sm, bt_address_t *addr, uint32_t event)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(sm),
            addr_str, stack_event_to_string(event));
}

static const char *stack_event_to_string(hfp_hf_event_t event)
{
    switch (event) {
        CASE_RETURN_STR(HF_CONNECT)
        CASE_RETURN_STR(HF_DISCONNECT)
        CASE_RETURN_STR(HF_CONNECT_AUDIO)
        CASE_RETURN_STR(HF_DISCONNECT_AUDIO)
        CASE_RETURN_STR(HF_VOICE_RECOGNITION_START)
        CASE_RETURN_STR(HF_VOICE_RECOGNITION_STOP)
        CASE_RETURN_STR(HF_SET_MIC_VOLUME)
        CASE_RETURN_STR(HF_SET_SPEAKER_VOLUME)
        CASE_RETURN_STR(HF_DIAL_NUMBER)
        CASE_RETURN_STR(HF_DIAL_MEMORY)
        CASE_RETURN_STR(HF_DIAL_LAST)
        CASE_RETURN_STR(HF_ACCEPT_CALL)
        CASE_RETURN_STR(HF_REJECT_CALL)
        CASE_RETURN_STR(HF_HOLD_CALL)
        CASE_RETURN_STR(HF_TERMINATE_CALL)
        CASE_RETURN_STR(HF_QUERY_CURRENT_CALLS)
        CASE_RETURN_STR(HF_UPDATE_BATTERY_LEVEL)
        CASE_RETURN_STR(HF_SEND_AT_COMMAND)
        CASE_RETURN_STR(HF_CONTROL_CALL)
        CASE_RETURN_STR(HF_TIMEOUT)
        CASE_RETURN_STR(HF_STACK_EVENT)
        CASE_RETURN_STR(HF_STACK_EVENT_AUDIO_REQ)
        CASE_RETURN_STR(HF_STACK_EVENT_CONNECTION_STATE_CHANGED)
        CASE_RETURN_STR(HF_STACK_EVENT_AUDIO_STATE_CHANGED)
        CASE_RETURN_STR(HF_STACK_EVENT_VR_STATE_CHANGED)
        CASE_RETURN_STR(HF_STACK_EVENT_CALL)
        CASE_RETURN_STR(HF_STACK_EVENT_CALLSETUP)
        CASE_RETURN_STR(HF_STACK_EVENT_CALLHELD)
        CASE_RETURN_STR(HF_STACK_EVENT_CLIP)
        CASE_RETURN_STR(HF_STACK_EVENT_CALL_WAITING)
        CASE_RETURN_STR(HF_STACK_EVENT_CURRENT_CALLS)
        CASE_RETURN_STR(HF_STACK_EVENT_VOLUME_CHANGED)
        CASE_RETURN_STR(HF_STACK_EVENT_CMD_RESPONSE)
        CASE_RETURN_STR(HF_STACK_EVENT_CMD_RESULT)
        CASE_RETURN_STR(HF_STACK_EVENT_RING_INDICATION)
        CASE_RETURN_STR(HF_STACK_EVENT_CODEC_CHANGED)
    default:
        return "UNKNOWN_HF_EVENT";
    }
}
#endif

static void add_pending_action(hf_state_machine_t *hfsm, uint32_t cmd_code)
{
    hf_at_cmd_t *cmd = malloc(sizeof(hf_at_cmd_t));

    cmd->cmd_code = cmd_code;
    list_add_tail(&hfsm->pending_actions, &cmd->node);
}

static uint32_t first_pending_action(hf_state_machine_t *hfsm)
{
    struct list_node *node;

    node = list_remove_head(&hfsm->pending_actions);
    if (node) {
        uint32_t code = ((hf_at_cmd_t *)node)->cmd_code;
        free(node);
        return code;
    }

    return 0;
}

static void set_current_call_name(hf_state_machine_t *hfsm, char *number, char *name)
{
    bt_list_node_t *cnode;
    bt_list_t *clist = hfsm->current_calls;

    if (number == NULL || name == NULL)
        return;

    for (cnode = bt_list_head(clist); cnode != NULL; cnode = bt_list_next(clist, cnode)) {
        hfp_current_call_t *call = bt_list_node(cnode);
        if (!strcmp(call->number, number)) {
            if (!strcmp(call->name, name))
                return;
            else {
                snprintf(call->name, HFP_NAME_DIGITS_MAX, "%s", name);
                hf_service_notify_call_state_changed(&hfsm->addr, call);
            }
        }
    }
}

static bool call_index_cmp(void *data, void *context)
{
    hfp_current_call_t *call = (hfp_current_call_t *)data;

    return call->index == *((uint32_t *)context);
}

static bool call_state_cmp(void *data, void *context)
{
    hfp_current_call_t *call = (hfp_current_call_t *)data;

    return call->state == *((uint32_t *)context);
}

static hfp_current_call_t *hf_call_new(uint32_t idx,
                                       hfp_call_direction_t dir,
                                       hfp_hf_call_state_t state,
                                       hfp_call_mpty_type_t mpty,
                                       char *number)
{
    hfp_current_call_t *call = malloc(sizeof(hfp_current_call_t));

    BT_LOGD("Current Call[%" PRIu32 "]: dir:%d, state:%d, mpty:%d, number:%s", idx, dir, state, mpty, number);
    call->index = idx;
    call->dir = dir;
    call->state = state;
    call->mpty = mpty;
    snprintf(call->number, HFP_PHONENUM_DIGITS_MAX, "%s", number);
    snprintf(call->name, HFP_NAME_DIGITS_MAX, "%s", "Unknown");

    return call;
}

static void hf_call_delete(void *data)
{
    hfp_current_call_t *call = (hfp_current_call_t *)data;

    free(call);
}

static hfp_current_call_t *get_call_by_state(hf_state_machine_t *hfsm, hfp_hf_call_state_t state)
{
    return bt_list_find(hfsm->current_calls, call_state_cmp, &state);
}

static void update_current_calls(hf_state_machine_t *hfsm, hfp_current_call_t *call)
{
    bt_list_add_tail(hfsm->update_calls, call);
}

static void query_current_calls_final(hf_state_machine_t *hfsm)
{
    BT_LOGD("Query current call final");
    bt_list_node_t *cnode, *unode;
    bt_list_t *clist = hfsm->current_calls;
    bt_list_t *ulist = hfsm->update_calls;

    for (cnode = bt_list_head(clist); cnode != NULL; cnode = bt_list_next(clist, cnode)) {
        hfp_current_call_t *ccall = bt_list_node(cnode);
        hfp_current_call_t *ucall = bt_list_find(ulist, call_index_cmp, &ccall->index);
        if (!ucall) {
            bt_list_node_t *tmp = bt_list_next(clist, cnode);
            /* call not found from update list, notify had terminated */
            ccall->state = HFP_HF_CALL_STATE_DISCONNECTED;
            hf_service_notify_call_state_changed(&hfsm->addr, ccall);
            /* resource free in bt_list_remove_node */
            bt_list_remove_node(clist, cnode);
            cnode = tmp;
            if (!cnode)
                break;
        } else {
            if (ucall->state != ccall->state || ucall->mpty != ccall->mpty || strcmp(ucall->number, ccall->number)) {
                /* call state or mutil part or number changed, notify changed */
                ccall->state = ucall->state;
                ccall->mpty = ucall->mpty;
                snprintf(ccall->number, HFP_PHONENUM_DIGITS_MAX, "%s", ucall->number);
                hf_service_notify_call_state_changed(&hfsm->addr, ccall);
            }
        }
    }

    for (unode = bt_list_head(ulist); unode != NULL; unode = bt_list_next(ulist, unode)) {
        hfp_current_call_t *ucall = bt_list_node(unode);
        hfp_current_call_t *ccall = bt_list_find(clist, call_index_cmp, &ucall->index);
        /* update new call to current call list */
        if (!ccall) {
            bt_list_add_tail(clist, ucall);
            hf_service_notify_call_state_changed(&hfsm->addr, ucall);
        }
    }

    bt_list_clear(ulist);
}

static void disconnected_enter(state_machine_t *sm)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;

    HF_DBG_ENTER(sm, &hfsm->addr);
    hfsm->need_query = false;
    if (hsm_get_previous_state(sm))
        hf_service_notify_connection_state_changed(&hfsm->addr, PROFILE_STATE_DISCONNECTED);
}

static void disconnected_exit(state_machine_t *sm)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;

    HF_DBG_EXIT(sm, &hfsm->addr);
}

static bool disconnected_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
    hfp_hf_data_t *data = (hfp_hf_data_t *)p_data;

    HF_DBG_EVENT(sm, &hfsm->addr, event);
    switch (event) {
    case HF_CONNECT:
        if (bt_sal_hfp_hf_connect(&hfsm->addr) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("Connect failed for %s", &hfsm->addr);
            hf_service_notify_connection_state_changed(&hfsm->addr, PROFILE_STATE_DISCONNECTED);
            break;
        }
        hsm_transition_to(sm, &connecting_state);
        break;
    case HF_STACK_EVENT_CONNECTION_STATE_CHANGED: {
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
        default:
            break;
        }
        break;
    }
    default:
        BT_LOGE("Disconnected: Unexpected stack event: %s", stack_event_to_string(event));
        break;
    }

    return true;
}

static void connect_timeout(service_timer_t *timer, void *data)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)data;

    hfp_hf_send_event(&hfsm->addr, HF_TIMEOUT);
}

static void connecting_enter(state_machine_t *sm)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;

    HF_DBG_ENTER(sm, &hfsm->addr);
    // start connecting timeout timer
    hfsm->connect_timer = service_loop_timer_no_repeating(HF_CONNECT_TIMEOUT, connect_timeout, hfsm);
    hf_service_notify_connection_state_changed(&hfsm->addr, PROFILE_STATE_CONNECTING);
}

static void connecting_exit(state_machine_t *sm)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;

    HF_DBG_EXIT(sm, &hfsm->addr);
    // stop timer
    service_loop_cancel_timer(hfsm->connect_timer);
    hfsm->connect_timer = NULL;
}

static bool connecting_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
    hfp_hf_data_t *data = (hfp_hf_data_t *)p_data;

    HF_DBG_EVENT(sm, &hfsm->addr, event);
    switch (event) {
    case HF_STACK_EVENT_CONNECTION_STATE_CHANGED: {
        profile_connection_state_t state = data->valueint1;
        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &disconnected_state);
            break;
        case PROFILE_STATE_CONNECTED:
            hsm_transition_to(sm, &connected_state);
            break;
        case PROFILE_STATE_CONNECTING:
        case PROFILE_STATE_DISCONNECTING:
            BT_LOGW("Ignored connection state: %d", state);
            break;
        default:
            break;
        }
        break;
    }
    case HF_STACK_EVENT_CODEC_CHANGED:
        hfsm->codec = data->valueint1;
        break;
    case HF_STACK_EVENT_CALL:
    case HF_STACK_EVENT_CALLSETUP:
    case HF_STACK_EVENT_CALLHELD:
    case HF_STACK_EVENT_CLIP:
        hfsm->need_query = true;
        break;
    case HF_TIMEOUT:
        BT_LOGI("Connection timeout");
        // try to disconnect peer device
        bt_sal_hfp_hf_disconnect(&hfsm->addr);
        hsm_transition_to(sm, &disconnected_state);
        break;
    default:
        break;
    }
    return true;
}

static void accept_call(hf_state_machine_t *hfsm, uint8_t flag)
{
    hfp_call_control_t ctrl;
    /* here process INCOMING call */
    if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_INCOMING) != NULL) {
        if (flag != HFP_HF_CALL_ACCEPT_NONE) {
            BT_LOGE("Have incoming call, error flag none");
            return;
        }

        BT_LOGI("Accept incoming call");
        if (bt_sal_hfp_hf_answer_call(&hfsm->addr) != BT_STATUS_SUCCESS) {
            BT_LOGE("Answer call failed");
        }
        /* here process WAITING call */
    } else if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_WAITING) != NULL) {
        if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_ACTIVE) == NULL && flag != HFP_HF_CALL_ACCEPT_NONE) {
            /* CHLD 1 CHLD2 only used for HLED call or WAITING call */
            BT_LOGE("When active call not exist, flag can't be hold or release");
            return;
        }

        if (flag == HFP_HF_CALL_ACCEPT_NONE || flag == HFP_HF_CALL_ACCEPT_HOLD) {
            ctrl = HFP_HF_CALL_CONTROL_CHLD_2;
        } else if (flag == HFP_HF_CALL_ACCEPT_RELEASE) {
            ctrl = HFP_HF_CALL_CONTROL_CHLD_1;
        } else {
            BT_LOGE("Accept with error flag");
            return;
        }
        BT_LOGI("Accept waiting call");
        if (bt_sal_hfp_hf_call_control(&hfsm->addr, ctrl, 0) != BT_STATUS_SUCCESS)
            BT_LOGE("Control call:%d error, line:%d", ctrl, __LINE__);
        /* here process HELD call */
    } else if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_HELD) != NULL) {
        if (flag == HFP_HF_CALL_ACCEPT_HOLD) {
            /* if flag want hold, hold active call and accpet hold call */
            ctrl = HFP_HF_CALL_CONTROL_CHLD_2;
        } else if (flag == HFP_HF_CALL_ACCEPT_RELEASE) {
            /* if flag want release, release all active call and accept hold call */
            ctrl = HFP_HF_CALL_CONTROL_CHLD_1;
        } else if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_ACTIVE) != NULL) {
            /* add held call to convesation */
            ctrl = HFP_HF_CALL_CONTROL_CHLD_3;
        } else
            ctrl = HFP_HF_CALL_CONTROL_CHLD_2;

        BT_LOGI("Accept held call");
        if (bt_sal_hfp_hf_call_control(&hfsm->addr, ctrl, 0) != BT_STATUS_SUCCESS)
            BT_LOGE("Control call:%d error, line:%d", ctrl, __LINE__);
    } else {
        BT_LOGE("No incoming/waiting/held call to accept");
    }
}

static void reject_call(hf_state_machine_t *hfsm)
{
    if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_INCOMING) != NULL) {
        BT_LOGI("Reject incoming call");
        if (bt_sal_hfp_hf_reject_call(&hfsm->addr) != BT_STATUS_SUCCESS) {
            BT_LOGE("Reject call failed");
        }
    } else if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_HELD) != NULL ||
               get_call_by_state(hfsm, HFP_HF_CALL_STATE_WAITING) != NULL) {
        BT_LOGI("Reject waiting call");
        if (bt_sal_hfp_hf_call_control(&hfsm->addr, HFP_HF_CALL_CONTROL_CHLD_0, 0) != BT_STATUS_SUCCESS)
            BT_LOGE("Reject waiting call(CHLD0) error, line:%d", __LINE__);
    } else {
        BT_LOGE("No call to reject");
    }
}

static void hangup_call(hf_state_machine_t *hfsm)
{
    if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_ACTIVE) != NULL ||
        get_call_by_state(hfsm, HFP_HF_CALL_STATE_DIALING) != NULL ||
        get_call_by_state(hfsm, HFP_HF_CALL_STATE_ALERTING) != NULL) {
        BT_LOGI("Terminate active/dialing/alerting call");
        if (bt_sal_hfp_hf_hangup_call(&hfsm->addr) != BT_STATUS_SUCCESS)
            BT_LOGE("Terminate call failed");
    } else if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_HELD) != NULL) {
        BT_LOGI("Release held call");
        if (bt_sal_hfp_hf_call_control(&hfsm->addr, HFP_HF_CALL_CONTROL_CHLD_0, 0) != BT_STATUS_SUCCESS)
            BT_LOGE("Release held call(CHLD0) error, line:%d", __LINE__);
    } else
        BT_LOGE("No call to terminate");
}

static void hold_call(hf_state_machine_t *hfsm)
{
    if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_ACTIVE) != NULL) {
        BT_LOGI("Hold active call");
        if (bt_sal_hfp_hf_call_control(&hfsm->addr, HFP_HF_CALL_CONTROL_CHLD_2, 0) != BT_STATUS_SUCCESS) {
            BT_LOGE("Hold active call(CHLD2) failed");
        }
    } else
        BT_LOGE("No call to hold");
}

static bool default_process_event(state_machine_t *sm, uint32_t event, hfp_hf_data_t *data)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
    bt_status_t status;
    BT_LOGD("%s, event=%" PRIu32 "", __func__, event);

    switch (event) {
    case HF_ACCEPT_CALL:
        accept_call(hfsm, data->valueint1);
        break;
    case HF_REJECT_CALL:
        reject_call(hfsm);
        break;
    case HF_HOLD_CALL:
        hold_call(hfsm);
        break;
    case HF_TERMINATE_CALL:
        hangup_call(hfsm);
        break;
    case HF_CONTROL_CALL: {
        hfp_call_control_t chld = data->valueint1;
        if (chld > 4) {
            BT_LOGE("Call control error code:%d, line:%d", chld, __LINE__);
            return false;
        }
        status = bt_sal_hfp_hf_call_control(&hfsm->addr, chld, 0);
        if (status != BT_STATUS_SUCCESS)
            BT_LOGE("Call control error:%d, line:%d", status, __LINE__);
    } break;
    case HF_QUERY_CURRENT_CALLS:
        status = bt_sal_hfp_hf_get_current_calls(&hfsm->addr);
        if (status != BT_STATUS_SUCCESS)
            BT_LOGE("Query current call failed");
        break;
    case HF_SEND_AT_COMMAND: {
        status = bt_sal_hfp_hf_send_at_cmd(&hfsm->addr, data->string1, strlen(data->string1));
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Send at command failed");
        }
        break;
    }
    case HF_UPDATE_BATTERY_LEVEL:
        status = bt_sal_hfp_hf_send_battery_level(&hfsm->addr, (uint8_t)data->valueint1);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Update battery level failed");
        }
        break;
    case HF_STACK_EVENT_VR_STATE_CHANGED: {
        hfp_hf_vr_state_t state = data->valueint1;

        hfsm->recognition_active = (state == HFP_HF_VR_STATE_STOPPED) ? false : true;
        hf_service_notify_vr_state_changed(&hfsm->addr, hfsm->recognition_active);
        break;
    }
    case HF_STACK_EVENT_CALL:
    case HF_STACK_EVENT_CALLSETUP:
    case HF_STACK_EVENT_CALLHELD:
        bt_sal_hfp_hf_get_current_calls(&hfsm->addr);
        break;
    case HF_STACK_EVENT_CLIP: {
        /* TODO: update call name */
        char *number = data->string1;
        char *name = data->string2;
        BT_LOGD("CLIP:number :%s, name: %s", number, name == NULL ? "NULL" : name);
        set_current_call_name(hfsm, number, name);
        break;
    }
    case HF_STACK_EVENT_CALL_WAITING:
        // not support
        break;
    case HF_STACK_EVENT_CURRENT_CALLS: {
        int index = data->valueint1;
        hfp_call_direction_t dir = data->valueint2;
        hfp_hf_call_state_t state = data->valueint3;
        hfp_call_mpty_type_t mpty = data->valueint4;
        char *number = data->string1;
        if (index == 0) {
            query_current_calls_final(hfsm);
        } else {
            update_current_calls(hfsm, hf_call_new(index, dir, state, mpty, number));
        }
        break;
    }
    case HF_STACK_EVENT_VOLUME_CHANGED: {
        hfp_volume_type_t type = data->valueint1;
        int vol = data->valueint2;
        // set media volume, need call media interface
        BT_LOGD("Volume changed, %s:%d", type ? "Mic" : "Spk", vol);
        break;
    }
    case HF_STACK_EVENT_CMD_RESPONSE: {
        const char *resp = data->string1;

        hf_service_notify_cmd_complete(&hfsm->addr, resp);
        break;
    }
    case HF_STACK_EVENT_CMD_RESULT: {
        uint32_t cmd_code = data->valueint1;
        uint32_t cmd_result = data->valueint2;
        uint32_t pending;

        pending = first_pending_action(hfsm);
        if (pending == cmd_code) {
            switch (cmd_code) {
            case HFP_ATCMD_CODE_ATD:
                if (cmd_result != HFP_ATCMD_RESULT_OK) {
                    BT_LOGE("Dial memory failed:%" PRIu32, cmd_result);
                }
                break;
            }
        }
        break;
    }
    case HF_STACK_EVENT_RING_INDICATION: {
        int active = data->valueint1;
        bool inband = data->valueint2 == HFP_IN_BAND_RINGTONE_PROVIDED;
        if (active)
            hf_service_notify_ring_indication(&hfsm->addr, inband);
        break;
    }
    case HF_STACK_EVENT_CODEC_CHANGED:
        hfsm->codec = data->valueint1;
        break;
    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }

    return true;
}

static void connected_enter(state_machine_t *sm)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;

    HF_DBG_ENTER(sm, &hfsm->addr);
    if (hfsm->need_query) {
        bt_sal_hfp_hf_get_current_calls(&hfsm->addr);
        hfsm->need_query = false;
    }
    if (hsm_get_previous_state(sm) != &audio_on_state)
        hf_service_notify_connection_state_changed(&hfsm->addr, PROFILE_STATE_CONNECTED);
}

static void connected_exit(state_machine_t *sm)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;

    HF_DBG_EXIT(sm, &hfsm->addr);
}

static bool connected_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
    hfp_hf_data_t *data = (hfp_hf_data_t *)p_data;
    bt_status_t status;

    HF_DBG_EVENT(sm, &hfsm->addr, event);
    switch (event) {
    case HF_DISCONNECT:
        // do disconnect
        if (bt_sal_hfp_hf_disconnect(&hfsm->addr) != BT_STATUS_SUCCESS)
            BT_ADDR_LOG("Disconnect failed for :%s", &hfsm->addr);

        hsm_transition_to(sm, &disconnected_state);
        break;
    case HF_CONNECT_AUDIO:
        if (bt_sal_hfp_hf_connect_audio(&hfsm->addr) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("Connect audio failed for :%s", &hfsm->addr);
            hf_service_notify_audio_state_changed(&hfsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
        }
        break;
    case HF_DISCONNECT_AUDIO:
        if (bt_sal_hfp_hf_disconnect_audio(&hfsm->addr) != BT_STATUS_SUCCESS)
            BT_ADDR_LOG("Disconnect audio failed for :%s", &hfsm->addr);
        break;
    case HF_VOICE_RECOGNITION_START:
        if (!hfsm->recognition_active) {
            if (bt_sal_hfp_hf_start_voice_recognition(&hfsm->addr) != BT_STATUS_SUCCESS)
                BT_LOGE("Could not start voice recognition");
        }
        break;
    case HF_VOICE_RECOGNITION_STOP:
        if (hfsm->recognition_active) {
            if (bt_sal_hfp_hf_stop_voice_recognition(&hfsm->addr) != BT_STATUS_SUCCESS)
                BT_LOGE("Could not stop voice recognition");
        }
        break;
    case HF_DIAL_NUMBER:
        status = bt_sal_hfp_hf_dial_number(&hfsm->addr, data->string1);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Dial number: %s failed", data->string1);
        }
        break;
    case HF_DIAL_MEMORY: {
        int memory = data->valueint1;
        BT_LOGD("Dial memory: %d", memory);
        status = bt_sal_hfp_hf_dial_memory(&hfsm->addr, memory);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Dial memory: %d failed", memory);
        }
        add_pending_action(hfsm, HFP_ATCMD_CODE_ATD);
        break;
    }
    case HF_DIAL_LAST: {
        status = bt_sal_hfp_hf_dial_number(&hfsm->addr, NULL);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Dial Last failed");
        }
        break;
    }
    case HF_STACK_EVENT_AUDIO_REQ:
        status = bt_sal_reply_sco_link_request(&hfsm->addr, true);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Accept Sco connection failed");
        }
        break;
    case HF_STACK_EVENT_CONNECTION_STATE_CHANGED: {
        profile_connection_state_t state = data->valueint1;

        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &disconnected_state);
            break;
        case PROFILE_STATE_CONNECTED:
        case PROFILE_STATE_CONNECTING:
        case PROFILE_STATE_DISCONNECTING:
            break;
        }
        break;
    }
    case HF_STACK_EVENT_AUDIO_STATE_CHANGED: {
        hfp_audio_state_t state = data->valueint1;

        switch (state) {
        case HFP_AUDIO_STATE_CONNECTED:
            hfsm->sco_conn_handle = data->valueint2;
            hsm_transition_to(sm, &audio_on_state);
            break;
        case HFP_AUDIO_STATE_DISCONNECTED:
        default:
            break;
        }
        break;
    }
    default:
        return default_process_event(sm, event, data);
    }
    return true;
}

static void audio_on_enter(state_machine_t *sm)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;

    HF_DBG_ENTER(sm, &hfsm->addr);
    /* TODO: get volume */
    /* TODO: set remote volume */
    /* TODO: set sco available */
    /* TODO: set samplerate */
    /* TODO: request audio focus */
    hf_service_notify_audio_state_changed(&hfsm->addr, HFP_AUDIO_STATE_CONNECTED);
}

static void audio_on_exit(state_machine_t *sm)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;

    HF_DBG_EXIT(sm, &hfsm->addr);
    /* TODO: set sco unavailable */
    /* TODO: abandon audio focus */
    hf_service_notify_audio_state_changed(&hfsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
}

static bool audio_on_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
    hfp_hf_data_t *data = (hfp_hf_data_t *)p_data;
    bt_status_t status;

    HF_DBG_EVENT(sm, &hfsm->addr, event);
    switch (event) {
    case HF_DISCONNECT:
        // do disconnect
        if (bt_sal_hfp_hf_disconnect(&hfsm->addr) != BT_STATUS_SUCCESS)
            BT_ADDR_LOG("Disconnect failed for :%s", &hfsm->addr);

        hsm_transition_to(sm, &disconnected_state);
        break;
    case HF_DISCONNECT_AUDIO:
        status = bt_sal_hfp_hf_disconnect_audio(&hfsm->addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect Sco connection failed");
        }
        break;
    case HF_VOICE_RECOGNITION_STOP:
        if (hfsm->recognition_active) {
            status = bt_sal_hfp_hf_stop_voice_recognition(&hfsm->addr);
            if (status != BT_STATUS_SUCCESS) {
                BT_LOGE("Could not stop voice recognition");
            }
        }
        break;
    case HF_SET_MIC_VOLUME: {
        uint8_t vol = data->valueint1;
        vol = vol > 15 ? 15 : vol;
        BT_LOGD("Set Mic Volume :%d", vol);
        bt_sal_hfp_hf_set_volume(&hfsm->addr, HFP_VOLUME_TYPE_MIC, vol);
        break;
    }
    case HF_SET_SPEAKER_VOLUME: {
        uint8_t vol = data->valueint1;
        vol = vol > 15 ? 15 : vol;

        BT_LOGD("Set Speaker Volume :%d", vol);
        bt_sal_hfp_hf_set_volume(&hfsm->addr, HFP_VOLUME_TYPE_SPK, vol);
        break;
    }
    case HF_STACK_EVENT_CONNECTION_STATE_CHANGED: {
        profile_connection_state_t state = data->valueint1;

        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            hsm_transition_to(sm, &disconnected_state);
            break;
        case PROFILE_STATE_DISCONNECTING:
        case PROFILE_STATE_CONNECTED:
        case PROFILE_STATE_CONNECTING:
            BT_LOGE("Receive state change in unexpect state: %d", state);
            break;
        }
        break;
    }
    case HF_STACK_EVENT_AUDIO_STATE_CHANGED: {
        hfp_audio_state_t state = data->valueint1;

        switch (state) {
        case HFP_AUDIO_STATE_DISCONNECTED:
            hsm_transition_to(sm, &connected_state);
            break;
        case HFP_AUDIO_STATE_CONNECTED:
            break;
        default:
            break;
        }
        break;
    }
    default:
        return default_process_event(sm, event, data);
    }

    return true;
}

hf_state_machine_t *hf_state_machine_new(bt_address_t *addr, void *context)
{
    hf_state_machine_t *hfsm;

    hfsm = (hf_state_machine_t *)malloc(sizeof(hf_state_machine_t));
    if (!hfsm)
        return NULL;

    memset(hfsm, 0, sizeof(hf_state_machine_t));
    hfsm->recognition_active = false;
    hfsm->service = context;
    memcpy(&hfsm->addr, addr, sizeof(bt_address_t));
    hfsm->update_calls = bt_list_new(NULL);
    hfsm->current_calls = bt_list_new(hf_call_delete);
    list_initialize(&hfsm->pending_actions);
    hsm_ctor(&hfsm->sm, (state_t *)&disconnected_state);

    return hfsm;
}

void hf_state_machine_destory(hf_state_machine_t *hfsm)
{
    if (!hfsm)
        return;

    if (hfsm->connect_timer)
        service_loop_cancel_timer(hfsm->connect_timer);
    bt_list_free(hfsm->update_calls);
    bt_list_free(hfsm->current_calls);
    hsm_dtor(&hfsm->sm);
    free((void *)hfsm);
}

void hf_state_machine_dispatch(hf_state_machine_t *hfsm, hfp_hf_msg_t *msg)
{
    if (!hfsm || !msg)
        return;

    hsm_dispatch_event(&hfsm->sm, msg->event, &msg->data);
}

uint32_t hf_state_machine_get_state(hf_state_machine_t *hfsm)
{
    return hsm_get_current_state_value(&hfsm->sm);
}

bt_list_t *hf_state_machine_get_calls(hf_state_machine_t *hfsm)
{
    return hfsm->current_calls;
}
