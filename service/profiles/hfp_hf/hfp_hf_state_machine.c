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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "audio_control.h"
#include "bt_addr.h"
#include "bt_hfp_hf.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "bt_vendor.h"
#include "hci_parser.h"
#include "hfp_hf_service.h"
#include "hfp_hf_state_machine.h"
#include "media_system.h"
#include "power_manager.h"
#include "sal_hfp_hf_interface.h"
#include "sal_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#define HFP_HF_RETRY_MAX 1

const static char voip_call_number[][HFP_PHONENUM_DIGITS_MAX] = {
    "10000000",
    "10000001"
};

#define HFP_HF_REPORT_CIEV_AND_CACHE(_hfsm, _ciev)                                                  \
    do {                                                                                            \
        if (_hfsm->call_status.last_reported._ciev##_status != _hfsm->call_status._ciev##_status) { \
            BT_LOGD("%s, %s = %d", __func__, #_ciev, _hfsm->call_status._ciev##_status);            \
            hf_service_notify_##_ciev(&hfsm->addr, _hfsm->call_status._ciev##_status);              \
            hfsm->call_status.last_reported._ciev##_status = hfsm->call_status._ciev##_status;      \
        }                                                                                           \
    } while (0)

typedef struct _hf_state_machine {
    state_machine_t sm;
    bt_address_t addr;
    uint16_t sco_conn_handle;
    uint32_t remote_features;
    service_timer_t* connect_timer;
    service_timer_t* offload_timer;
    service_timer_t* retry_timer;
    bool recognition_active;
    bool offloading;
    connection_policy_t connection_policy;
    uint8_t spk_volume;
    uint8_t mic_volume;
    int media_volume;
    void* volume_listener;
    uint32_t set_volume_cnt;
    uint8_t codec;
    uint8_t retry_cnt;
    pending_state_t pending;
    struct list_node pending_actions;
    bt_list_t* current_calls;
    bt_list_t* update_calls;
    hfp_hf_call_status_t call_status;
    uint8_t need_query;
    void* service;
} hf_state_machine_t;

typedef struct {
    struct list_node node;
    uint32_t cmd_code;
    union {
        char number[HFP_PHONENUM_DIGITS_MAX];
    } param;
} hf_at_cmd_t;

#define HF_STM_DEBUG 1
#define HF_CONNECT_TIMEOUT (10 * 1000)
#define HF_WEBCHAT_VERDICT (300 * 1000)
#define HF_WEBCHAT_BLOCK_PERIOD (500 * 1000)
#define HF_WEBCHAT_WAIVER_PERIOD (10 * 1000 * 1000)
#define HF_OFFLOAD_TIMEOUT 500

#if HF_STM_DEBUG
static void hf_stm_trans_debug(state_machine_t* sm, bt_address_t* addr, const char* action);
static void hf_stm_event_debug(state_machine_t* sm, bt_address_t* addr, uint32_t event);
static const char* stack_event_to_string(hfp_hf_event_t event);

#define HF_DBG_ENTER(__sm, __addr) hf_stm_trans_debug(__sm, __addr, "Enter")
#define HF_DBG_EXIT(__sm, __addr) hf_stm_trans_debug(__sm, __addr, "Exit ")
#define HF_DBG_EVENT(__sm, __addr, __event) hf_stm_event_debug(__sm, __addr, __event);
#else
#define HF_DBG_ENTER(__sm, __addr)
#define HF_DBG_EXIT(__sm, __addr)
#define HF_DBG_EVENT(__sm, __addr, __event)
#endif

extern bt_status_t hfp_hf_send_message(hfp_hf_msg_t* msg);

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

static void bt_hci_event_callback(bt_hci_event_t* hci_event, void* context);

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
static void hf_stm_trans_debug(state_machine_t* sm, bt_address_t* addr, const char* action)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s State=%s, Peer=[%s]", action, hsm_get_current_state_name(sm), addr_str);
}

static void hf_stm_event_debug(state_machine_t* sm, bt_address_t* addr, uint32_t event)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(sm),
        addr_str, stack_event_to_string(event));
}

static const char* stack_event_to_string(hfp_hf_event_t event)
{
    switch (event) {
        CASE_RETURN_STR(HF_CONNECT)
        CASE_RETURN_STR(HF_DISCONNECT)
        CASE_RETURN_STR(HF_CONNECT_AUDIO)
        CASE_RETURN_STR(HF_DISCONNECT_AUDIO)
        CASE_RETURN_STR(HF_VOICE_RECOGNITION_START)
        CASE_RETURN_STR(HF_VOICE_RECOGNITION_STOP)
        CASE_RETURN_STR(HF_SET_VOLUME)
        CASE_RETURN_STR(HF_DIAL_NUMBER)
        CASE_RETURN_STR(HF_DIAL_MEMORY)
        CASE_RETURN_STR(HF_DIAL_LAST)
        CASE_RETURN_STR(HF_ACCEPT_CALL)
        CASE_RETURN_STR(HF_REJECT_CALL)
        CASE_RETURN_STR(HF_HOLD_CALL)
        CASE_RETURN_STR(HF_TERMINATE_CALL)
        CASE_RETURN_STR(HF_CONTROL_CALL)
        CASE_RETURN_STR(HF_QUERY_CURRENT_CALLS)
        CASE_RETURN_STR(HF_SEND_AT_COMMAND)
        CASE_RETURN_STR(HF_UPDATE_BATTERY_LEVEL)
        CASE_RETURN_STR(HF_SEND_DTMF)
        CASE_RETURN_STR(HF_TIMEOUT)
        CASE_RETURN_STR(HF_OFFLOAD_START_REQ)
        CASE_RETURN_STR(HF_OFFLOAD_STOP_REQ)
        CASE_RETURN_STR(HF_OFFLOAD_START_EVT)
        CASE_RETURN_STR(HF_OFFLOAD_STOP_EVT)
        CASE_RETURN_STR(HF_OFFLOAD_TIMEOUT_EVT)
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

static bool flag_isset(hf_state_machine_t* hfsm, pending_state_t flag)
{
    return (bool)(hfsm->pending & flag);
}

static void flag_set(hf_state_machine_t* hfsm, pending_state_t flag)
{
    hfsm->pending |= flag;
}

static void flag_clear(hf_state_machine_t* hfsm, pending_state_t flag)
{
    hfsm->pending &= ~flag;
}

static void pending_action_create(hf_state_machine_t* hfsm, uint32_t cmd_code, void* param)
{
    hf_at_cmd_t* cmd = NULL;

    cmd = zalloc(sizeof(hf_at_cmd_t));

    cmd->cmd_code = cmd_code;
    switch (cmd_code) {
    case HFP_ATCMD_CODE_ATD:
    case HFP_ATCMD_CODE_BLDN:
        if (param) {
            strlcpy(cmd->param.number, (const char*)param, sizeof(cmd->param.number));
        }
        break;
    default:
        break;
    }

    list_add_tail(&hfsm->pending_actions, &cmd->node);
}

static hf_at_cmd_t* pending_action_get(hf_state_machine_t* hfsm)
{
    struct list_node* node;

    node = list_remove_head(&hfsm->pending_actions);

    return (hf_at_cmd_t*)node;
}

static void pending_action_destroy(hf_at_cmd_t* cmd)
{
    if (cmd)
        free(cmd);
}

static void set_current_call_name(hf_state_machine_t* hfsm, char* number, char* name)
{
    bt_list_node_t* cnode;
    bt_list_t* clist = hfsm->current_calls;

    if (number == NULL || name == NULL)
        return;

    for (cnode = bt_list_head(clist); cnode != NULL; cnode = bt_list_next(clist, cnode)) {
        hfp_current_call_t* call = bt_list_node(cnode);
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

static bool call_index_cmp(void* data, void* context)
{
    hfp_current_call_t* call = (hfp_current_call_t*)data;

    return call->index == *((int*)context);
}

static bool call_state_cmp(void* data, void* context)
{
    hfp_current_call_t* call = (hfp_current_call_t*)data;

    return call->state == *((hfp_hf_call_state_t*)context);
}

static hfp_current_call_t* hf_call_new(uint32_t idx,
    hfp_call_direction_t dir,
    hfp_hf_call_state_t state,
    hfp_call_mpty_type_t mpty,
    char* number)
{
    hfp_current_call_t* call = malloc(sizeof(hfp_current_call_t));

    BT_LOGD("Current Call[%" PRIu32 "]: dir:%d, state:%d, mpty:%d, number:%s", idx, dir, state, mpty, number);
    call->index = idx;
    call->dir = dir;
    call->state = state;
    call->mpty = mpty;
    snprintf(call->number, HFP_PHONENUM_DIGITS_MAX, "%s", number);
    snprintf(call->name, HFP_NAME_DIGITS_MAX, "%s", "");

    return call;
}

static void hf_call_delete(void* data)
{
    hfp_current_call_t* call = (hfp_current_call_t*)data;

    free(call);
}

static hfp_current_call_t* get_call_by_state(hf_state_machine_t* hfsm, hfp_hf_call_state_t state)
{
    return bt_list_find(hfsm->current_calls, call_state_cmp, &state);
}

static void update_current_calls(hf_state_machine_t* hfsm, hfp_current_call_t* call)
{
    bt_list_add_tail(hfsm->update_calls, call);
}

static void hf_service_fake_ciev(hf_state_machine_t* hfsm)
{
    HFP_HF_REPORT_CIEV_AND_CACHE(hfsm, call);
    HFP_HF_REPORT_CIEV_AND_CACHE(hfsm, callsetup);
    HFP_HF_REPORT_CIEV_AND_CACHE(hfsm, callheld);
}

static void query_current_calls_final(hf_state_machine_t* hfsm)
{
    BT_LOGD("Query current call final");
    bt_list_node_t *cnode, *unode;
    bt_list_t* clist = hfsm->current_calls;
    bt_list_t* ulist = hfsm->update_calls;

    hf_service_fake_ciev(hfsm);

    for (cnode = bt_list_head(clist); cnode != NULL; cnode = bt_list_next(clist, cnode)) {
        hfp_current_call_t* ccall = bt_list_node(cnode);
        hfp_current_call_t* ucall = bt_list_find(ulist, call_index_cmp, &ccall->index);
        if (!ucall) {
            bt_list_node_t* tmp = bt_list_next(clist, cnode);
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
        hfp_current_call_t* ucall = bt_list_node(unode);
        hfp_current_call_t* ccall = bt_list_find(clist, call_index_cmp, &ucall->index);
        /* update new call to current call list */
        if (!ccall) {
            bt_list_add_tail(clist, ucall);
            hf_service_notify_call_state_changed(&hfsm->addr, ucall);
        }
    }

    bt_list_clear(ulist);
}

static void state_machine_reset_calls(hf_state_machine_t* hfsm)
{
    hf_at_cmd_t* node;

    bt_list_clear(hfsm->current_calls);
    bt_list_clear(hfsm->update_calls);
    while ((node = pending_action_get(hfsm)) != NULL)
        pending_action_destroy(node); /* discard pending actions */
    if (hfsm->connect_timer)
        service_loop_cancel_timer(hfsm->connect_timer);
    hfsm->recognition_active = false;
}

static void update_remote_features(hf_state_machine_t* hfsm, uint32_t remote_features)
{
    BT_LOGD("%s, remote features:0x%" PRIu32, __func__, remote_features);
    hfsm->remote_features = remote_features;
}

static bt_status_t hf_offload_send_cmd(hf_state_machine_t* hfsm, bool is_start)
{
    uint8_t ogf;
    uint16_t ocf;
    size_t size;
    uint8_t* payload;
    hfp_offload_config_t config = { 0 };
    uint8_t offload[CONFIG_VSC_MAX_LEN];

    config.sco_hdl = hfsm->sco_conn_handle;
    config.sco_codec = hfsm->codec;
    if (is_start) {
        if (!hfp_offload_start_builder(&config, offload, &size)) {
            BT_LOGE("HFP HF offload start builder failed");
            assert(0);
        }
    } else {
        if (!hfp_offload_stop_builder(&config, offload, &size)) {
            BT_LOGE("HFP HF offload stop builder failed");
            assert(0);
        }
    }

    payload = offload;
    STREAM_TO_UINT8(ogf, payload);
    STREAM_TO_UINT16(ocf, payload);
    size -= sizeof(ogf) + sizeof(ocf);

    return bt_sal_send_hci_command(PRIMARY_ADAPTER, ogf, ocf, size, payload, bt_hci_event_callback, hfsm);
}

static bool check_hfp_allowed(hf_state_machine_t* hfsm)
{
    return hfsm->connection_policy != CONNECTION_POLICY_FORBIDDEN;
}

static void disconnected_enter(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    HF_DBG_ENTER(sm, &hfsm->addr);
    hfsm->need_query = false;
    if (hsm_get_previous_state(sm)) {
        bt_pm_conn_close(PROFILE_HFP_HF, &hfsm->addr);
        bt_media_remove_listener(hfsm->volume_listener);
        hfsm->spk_volume = 0;
        hfsm->mic_volume = 0;
        hfsm->media_volume = INVALID_MEDIA_VOLUME;
        hfsm->volume_listener = NULL;
        hfsm->set_volume_cnt = 0;
        hf_service_notify_connection_state_changed(&hfsm->addr, PROFILE_STATE_DISCONNECTED);
    }

    /* reset cached info */
    state_machine_reset_calls(hfsm);
}

static void disconnected_exit(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    HF_DBG_EXIT(sm, &hfsm->addr);
}

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hfp_hf_data_t* data = (hfp_hf_data_t*)p_data;

    HF_DBG_EVENT(sm, &hfsm->addr, event);
    switch (event) {
    case HF_CONNECT:
        if (!check_hfp_allowed(hfsm)) {
            BT_ADDR_LOG("HF Connect disallowd for %s", &hfsm->addr);
            break;
        }

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
            update_remote_features(hfsm, data->valueint3);
            break;
        case PROFILE_STATE_CONNECTING:
            if (!check_hfp_allowed(hfsm)) {
                BT_ADDR_LOG("HF Connect disallowd for %s", &hfsm->addr);
                bt_sal_hfp_hf_disconnect(&hfsm->addr);
                break;
            }

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
    case HF_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case HF_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        break;
    case HF_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        BT_LOGE("Disconnected: Unexpected stack event: %s", stack_event_to_string(event));
        break;
    }

    return true;
}

static void connect_timeout(service_timer_t* timer, void* data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)data;

    hfp_hf_send_event(&hfsm->addr, HF_TIMEOUT);
}

static void connecting_enter(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    HF_DBG_ENTER(sm, &hfsm->addr);
    // start connecting timeout timer
    hfsm->connect_timer = service_loop_timer_no_repeating(HF_CONNECT_TIMEOUT, connect_timeout, hfsm);
    hf_service_notify_connection_state_changed(&hfsm->addr, PROFILE_STATE_CONNECTING);

    bt_pm_busy(PROFILE_HFP_HF, &hfsm->addr);
    bt_pm_idle(PROFILE_HFP_HF, &hfsm->addr);
}

static void connecting_exit(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    HF_DBG_EXIT(sm, &hfsm->addr);
    // stop timer
    service_loop_cancel_timer(hfsm->connect_timer);
    hfsm->connect_timer = NULL;
}

#ifdef CONFIG_HFP_HF_WEBCHAT_BLOCKER
static int64_t calc_us_diff(uint64_t prev_us, uint64_t next_us)
{
    if ((next_us >= prev_us) && ((next_us - prev_us) < (1ULL << 63)))
        return (next_us - prev_us);

    return -1;
}
#endif

static bool check_sco_allowed(state_machine_t* sm)
{
#ifdef CONFIG_HFP_HF_WEBCHAT_BLOCKER
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    uint64_t current_timestamp_us = get_os_timestamp_us();
    int64_t us_diff;
    bt_list_node_t* cnode;
    bt_list_t* clist = hfsm->current_calls;
    uint32_t index;

    /* Verdict 1: allow SCO request if the recent call is initiated by HF */
    us_diff = calc_us_diff(hfsm->call_status.dialing_timestamp_us, current_timestamp_us);
    if ((us_diff >= 0) && (us_diff < HF_WEBCHAT_WAIVER_PERIOD)) {
        return true;
    }

    /* Verdict 2: reject SCO request if the recent call is speculated to be a web chat */
    us_diff = calc_us_diff(hfsm->call_status.webchat_flag_timestamp_us, current_timestamp_us);
    if ((us_diff >= 0) && (us_diff < HF_WEBCHAT_BLOCK_PERIOD)) {
        hfsm->call_status.webchat_flag_timestamp_us = current_timestamp_us;
        BT_LOGD("%s failed: the recent call is speculated to be a web chat", __func__);
        return false;
    }

    /* Verdict 3: reject SCO request if there is a phone number specifically used for VoIP */
    for (cnode = bt_list_head(clist); cnode != NULL; cnode = bt_list_next(clist, cnode)) {
        hfp_current_call_t* ccall = bt_list_node(cnode);
        for (index = 0; index < ARRAY_SIZE(voip_call_number); index++) {
            if (0 == strcmp(voip_call_number[index], ccall->number)) {
                BT_LOGD("%s failed: there is a phone number specifically used for VoIP", __func__);
                return false;
            }
        }
    }
#endif
    return true;
}

#ifdef CONFIG_HFP_HF_WEBCHAT_BLOCKER
static void channel_type_verdict(state_machine_t* sm, uint32_t event, uint32_t status,
    uint64_t current_timestamp_us)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    int64_t us_diff;

    switch (event) {
    case HF_STACK_EVENT_CALL:
        if (((hfp_call_t)status == HFP_CALL_CALLS_IN_PROGRESS) && (hfsm->call_status.call_status == HFP_CALL_NO_CALLS_IN_PROGRESS) && ((hfsm->call_status.callsetup_status == HFP_CALLSETUP_OUTGOING) || (hfsm->call_status.callsetup_status == HFP_CALLSETUP_ALERTING))) {
            us_diff = calc_us_diff(hfsm->call_status.callsetup_timestamp_us, current_timestamp_us);
            if ((us_diff >= 0) && (us_diff < HF_WEBCHAT_VERDICT)) {
                BT_LOGD("%s: this might be a video chat from WeChat", __func__);
                hfsm->call_status.webchat_flag_timestamp_us = current_timestamp_us;
                if (hf_state_machine_get_state(hfsm) == HFP_HF_STATE_AUDIO_CONNECTED && !check_sco_allowed(sm)) {
                    if (bt_sal_hfp_hf_disconnect_audio(&hfsm->addr) != BT_STATUS_SUCCESS)
                        BT_ADDR_LOG("Terminate audio failed for :%s", &hfsm->addr);
                }
            }
        }
        break;
    case HF_STACK_EVENT_CALLSETUP:
        break;
    case HF_STACK_EVENT_CALLHELD:
        break;
    default:
        break;
    }
}
#endif

static void update_dialing_time(state_machine_t* sm, uint64_t current_timestamp_us)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    BT_LOGD("%s: timestamp = %" PRIu64, __func__, current_timestamp_us);
    hfsm->call_status.dialing_timestamp_us = current_timestamp_us;
}

static void update_call_status(state_machine_t* sm, uint32_t event, uint32_t status)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    uint64_t current_timestamp_us = get_os_timestamp_us();

#ifdef CONFIG_HFP_HF_WEBCHAT_BLOCKER
    channel_type_verdict(sm, event, status, current_timestamp_us);
#endif

    switch (event) {
    case HF_STACK_EVENT_CALL:
        hfsm->call_status.call_status = (hfp_call_t)status;
        hfsm->call_status.call_timestamp_us = current_timestamp_us;
        BT_LOGD("%s: call:%d, timestamp = %" PRIu64, __func__, hfsm->call_status.call_status,
            hfsm->call_status.call_timestamp_us);
        // hf_service_notify_call(&hfsm->addr, status);
        break;
    case HF_STACK_EVENT_CALLSETUP:
        hfsm->call_status.callsetup_status = (hfp_callsetup_t)status;
        hfsm->call_status.callsetup_timestamp_us = current_timestamp_us;
        BT_LOGD("%s: callsetup:%d, timestamp = %" PRIu64, __func__, hfsm->call_status.callsetup_status,
            hfsm->call_status.callsetup_timestamp_us);
        // hf_service_notify_callsetup(&hfsm->addr, status);
        break;
    case HF_STACK_EVENT_CALLHELD:
        hfsm->call_status.callheld_status = (hfp_callheld_t)status;
        hfsm->call_status.callheld_timestamp_us = current_timestamp_us;
        BT_LOGD("%s: callheld:%d, timestamp = %" PRIu64, __func__, hfsm->call_status.callheld_status,
            hfsm->call_status.callsetup_timestamp_us);
        // hf_service_notify_callheld(&hfsm->addr, status);
        break;
    default:
        break;
    }
}

static void hf_retry_callback(service_timer_t* timer, void* data)
{
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    state_machine_t* sm = (state_machine_t*)data;
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hfp_hf_state_t state;

    assert(hfsm);

    bt_addr_ba2str(&hfsm->addr, _addr_str);
    state = hf_state_machine_get_state(hfsm);
    BT_LOGD("%s: device=[%s], state=%d, retry_cnt=%d", __func__, _addr_str, state, hfsm->retry_cnt);
    if (state == HFP_HF_STATE_DISCONNECTED) {
        if (bt_sal_hfp_hf_connect(&hfsm->addr) == BT_STATUS_SUCCESS) {
            hsm_transition_to(sm, &connecting_state);
        } else {
            BT_LOGI("failed to connect %s", _addr_str);
        }
    }

    hfsm->retry_timer = NULL;
}

static bool connecting_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hfp_hf_data_t* data = (hfp_hf_data_t*)p_data;
    uint32_t random_timeout;

    HF_DBG_EVENT(sm, &hfsm->addr, event);
    switch (event) {
    case HF_STACK_EVENT_CONNECTION_STATE_CHANGED: {
        profile_connection_state_t state = data->valueint1;
        profile_connection_reason_t reason = data->valueint2;

        switch (state) {
        case PROFILE_STATE_DISCONNECTED:
            if (reason == PROFILE_REASON_COLLISION && hfsm->retry_cnt < HFP_HF_RETRY_MAX) {
                /* failed to establish HFP connection, retry for up to HFP_HF_RETRY_MAX times */
                if (hfsm->retry_timer == NULL) {
                    srand(time(NULL)); /* set random seed */
                    random_timeout = 100 + (rand() % 800);
                    BT_LOGD("retry HFP connection with device:[%s], delay=%" PRIu32 "ms",
                        bt_addr_str(&hfsm->addr), random_timeout);
                    hfsm->retry_timer = service_loop_timer(random_timeout, 0, hf_retry_callback, sm);
                    hfsm->retry_cnt++;
                }
            }
            hsm_transition_to(sm, &disconnected_state);
            break;
        case PROFILE_STATE_CONNECTED:
            hsm_transition_to(sm, &connected_state);
            update_remote_features(hfsm, data->valueint3);
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
        hfsm->codec = data->valueint1 == HFP_CODEC_MSBC ? HFP_CODEC_MSBC : HFP_CODEC_CVSD;
        break;
    case HF_STACK_EVENT_CALL:
    case HF_STACK_EVENT_CALLSETUP:
    case HF_STACK_EVENT_CALLHELD:
        update_call_status(sm, event, data->valueint1);
        hfsm->need_query = true;
        break;
    case HF_STACK_EVENT_CLIP:
        hfsm->need_query = true;
        break;
    case HF_TIMEOUT:
        BT_LOGI("Connection timeout");
        // try to disconnect peer device
        bt_sal_hfp_hf_disconnect(&hfsm->addr);
        hsm_transition_to(sm, &disconnected_state);
        break;
    case HF_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case HF_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        break;
    case HF_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        break;
    }
    return true;
}

static void accept_call(hf_state_machine_t* hfsm, uint8_t flag)
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

static void reject_call(hf_state_machine_t* hfsm)
{
    if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_INCOMING) != NULL) {
        BT_LOGI("Reject incoming call");
        if (bt_sal_hfp_hf_reject_call(&hfsm->addr) != BT_STATUS_SUCCESS) {
            BT_LOGE("Reject call failed");
        }
    } else if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_HELD) != NULL || get_call_by_state(hfsm, HFP_HF_CALL_STATE_WAITING) != NULL) {
        BT_LOGI("Reject waiting call");
        if (bt_sal_hfp_hf_call_control(&hfsm->addr, HFP_HF_CALL_CONTROL_CHLD_0, 0) != BT_STATUS_SUCCESS)
            BT_LOGE("Reject waiting call(CHLD0) error, line:%d", __LINE__);
    } else {
        BT_LOGE("No call to reject");
    }
}

static void hangup_call(hf_state_machine_t* hfsm)
{
    if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_ACTIVE) != NULL || get_call_by_state(hfsm, HFP_HF_CALL_STATE_DIALING) != NULL || get_call_by_state(hfsm, HFP_HF_CALL_STATE_ALERTING) != NULL) {
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

static void hold_call(hf_state_machine_t* hfsm)
{
    if (get_call_by_state(hfsm, HFP_HF_CALL_STATE_ACTIVE) != NULL) {
        BT_LOGI("Hold active call");
        if (bt_sal_hfp_hf_call_control(&hfsm->addr, HFP_HF_CALL_CONTROL_CHLD_2, 0) != BT_STATUS_SUCCESS) {
            BT_LOGE("Hold active call(CHLD2) failed");
        }
    } else
        BT_LOGE("No call to hold");
}

static void handle_dailing_fail(state_machine_t* sm, char* number)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hfp_current_call_t call = { 0 };
    BT_LOGD("%s, %s", __func__, bt_addr_str(&hfsm->addr));

    call.dir = HFP_CALL_DIRECTION_OUTGOING;
    call.state = HFP_HF_CALL_STATE_DISCONNECTED;
    if (number) {
        BT_LOGD("number: %s", number);
        strlcpy(call.number, number, sizeof(call.number));
    }

    hf_service_notify_call_state_changed(&hfsm->addr, &call);
}

static void handle_hf_set_voice_call_volume(state_machine_t* sm, hfp_volume_type_t type, uint8_t volume)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    bt_status_t status = BT_STATUS_SUCCESS;
    int new_volume = INVALID_MEDIA_VOLUME;

    if (type == HFP_VOLUME_TYPE_SPK) {
        hfsm->spk_volume = volume;

        new_volume = bt_media_volume_hfp_to_media(volume);
        if (new_volume == hfsm->media_volume) {
            return;
        }

        status = bt_media_set_voice_call_volume(new_volume);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Set media voice call volume failed");
        } else if (hfsm->set_volume_cnt < UINT32_MAX) {
            hfsm->set_volume_cnt++;
        }

    } else if (type == HFP_VOLUME_TYPE_MIC) {
        hfsm->mic_volume = volume;
    }
}

static bool default_process_event(state_machine_t* sm, uint32_t event, hfp_hf_data_t* data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
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
        break;
    }
    case HF_QUERY_CURRENT_CALLS:
        status = bt_sal_hfp_hf_get_current_calls(&hfsm->addr);
        if (status != BT_STATUS_SUCCESS)
            BT_LOGE("Query current call failed");
        break;
    case HF_SEND_AT_COMMAND: {
        status = bt_sal_hfp_hf_send_at_cmd(&hfsm->addr, data->string1, strlen(data->string1));
        if (status != BT_STATUS_SUCCESS)
            BT_LOGE("Send at command failed");
        break;
    }
    case HF_UPDATE_BATTERY_LEVEL:
        status = bt_sal_hfp_hf_send_battery_level(&hfsm->addr, (uint8_t)data->valueint1);
        if (status != BT_STATUS_SUCCESS)
            BT_LOGE("Update battery level failed");
        break;
    case HF_SEND_DTMF:
        status = bt_sal_hfp_hf_send_dtmf(&hfsm->addr, (char)data->valueint1);
        if (status != BT_STATUS_SUCCESS)
            BT_LOGE("Send dtmf failed");
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
        update_call_status(sm, event, data->valueint1);
        bt_sal_hfp_hf_get_current_calls(&hfsm->addr);
        break;
    case HF_STACK_EVENT_CLIP: {
        char* number = data->string1;
        char* name = data->string2;
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
        char* number = data->string1;
        if (index == 0) {
            query_current_calls_final(hfsm);
        } else {
            update_current_calls(hfsm, hf_call_new(index, dir, state, mpty, number));
        }
        break;
    }
    case HF_STACK_EVENT_VOLUME_CHANGED: {
        hfp_volume_type_t type = data->valueint1;
        uint8_t hf_vol = data->valueint2;
        handle_hf_set_voice_call_volume(sm, type, hf_vol);
        BT_LOGD("Volume changed, %s:%" PRIu8, type == HFP_VOLUME_TYPE_MIC ? "Mic" : "Spk", hf_vol);
        hf_service_notify_volume_changed(&hfsm->addr, type, hf_vol);
        break;
    }
    case HF_STACK_EVENT_CMD_RESPONSE: {
        const char* resp = data->string1;

        hf_service_notify_cmd_complete(&hfsm->addr, resp);
        break;
    }
    case HF_STACK_EVENT_CMD_RESULT: {
        uint32_t cmd_code = data->valueint1;
        uint32_t cmd_result = data->valueint2;
        hf_at_cmd_t* pending_cmd;

        pending_cmd = pending_action_get(hfsm);
        if (!pending_cmd)
            break;

        if (pending_cmd->cmd_code == cmd_code) {
            switch (cmd_code) {
            case HFP_ATCMD_CODE_ATD:
                if (cmd_result != HFP_ATCMD_RESULT_OK) {
                    BT_LOGE("ATD failed:%" PRIu32, cmd_result);
                    handle_dailing_fail(sm, pending_cmd->param.number);
                }
                break;
            case HFP_ATCMD_CODE_BLDN:
                if (cmd_result != HFP_ATCMD_RESULT_OK) {
                    BT_LOGE("AT+BLDN failed:%" PRIu32, cmd_result);
                    handle_dailing_fail(sm, NULL);
                }
                break;
            }
        }

        pending_action_destroy(pending_cmd);
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
        hfsm->codec = data->valueint1 == HFP_CODEC_MSBC ? HFP_CODEC_MSBC : HFP_CODEC_CVSD;
        break;
    default:
        BT_LOGW("Unexpected event:%" PRIu32 "", event);
        break;
    }

    return true;
}

static void bt_hci_event_callback(bt_hci_event_t* hci_event, void* context)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)context;
    hfp_hf_msg_t* msg;
    hfp_hf_event_t event;

    BT_LOGD("%s, evt_code:0x%x, len:%d", __func__, hci_event->evt_code,
        hci_event->length);
    BT_DUMPBUFFER("vsc", (uint8_t*)hci_event->params, hci_event->length);

    if (flag_isset(hfsm, PENDING_OFFLOAD_START)) {
        event = HF_OFFLOAD_START_EVT;
        flag_clear(hfsm, PENDING_OFFLOAD_START);
    } else if (flag_isset(hfsm, PENDING_OFFLOAD_STOP)) {
        event = HF_OFFLOAD_STOP_EVT;
        flag_clear(hfsm, PENDING_OFFLOAD_STOP);
    } else {
        return;
    }

    msg = hfp_hf_msg_new_ext(event, &hfsm->addr, hci_event, sizeof(bt_hci_event_t) + hci_event->length);
    hfp_hf_send_message(msg);
}

static void hfp_hf_voice_volume_change_callback(void* cookie, int volume)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)cookie;
    hfp_hf_msg_t* msg;

    hfsm->media_volume = volume;
    if (hfsm->set_volume_cnt) {
        hfsm->set_volume_cnt--;
        return;
    }

    msg = hfp_hf_msg_new(HF_SET_VOLUME, &hfsm->addr);
    if (!msg) {
        BT_LOGE("New hf message alloc failed");
        return;
    }

    msg->data.valueint1 = HFP_VOLUME_TYPE_SPK;
    msg->data.valueint2 = volume;
    hfp_hf_send_message(msg);
}

static void connected_enter(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    HF_DBG_ENTER(sm, &hfsm->addr);

    bt_pm_conn_open(PROFILE_HFP_HF, &hfsm->addr);

    if (hfsm->need_query) {
        bt_sal_hfp_hf_get_current_calls(&hfsm->addr);
        hfsm->need_query = false;
    }
    if (hsm_get_previous_state(sm) != &audio_on_state) {
        if (bt_media_get_voice_call_volume(&hfsm->media_volume) == BT_STATUS_SUCCESS) {
            hfsm->spk_volume = bt_media_volume_media_to_hfp(hfsm->media_volume);
            bt_sal_hfp_hf_set_volume(&hfsm->addr, HFP_VOLUME_TYPE_SPK, hfsm->spk_volume);
        } else {
            BT_LOGE("Get voice call volume failed");
        }
        hfsm->volume_listener = bt_media_listen_voice_call_volume_change(hfp_hf_voice_volume_change_callback, hfsm);
        hfsm->retry_cnt = 0;
        if (!hfsm->volume_listener) {
            BT_LOGE("Start to listen voice call volume failed");
        }
        hf_service_notify_connection_state_changed(&hfsm->addr, PROFILE_STATE_CONNECTED);
    }
}

static void connected_exit(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    HF_DBG_EXIT(sm, &hfsm->addr);
}

static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hfp_hf_data_t* data = (hfp_hf_data_t*)p_data;
    uint64_t current_timestamp_us = get_os_timestamp_us();
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
            handle_dailing_fail(sm, data->string1);
            break;
        }
        update_dialing_time(sm, current_timestamp_us);
        pending_action_create(hfsm, HFP_ATCMD_CODE_ATD, data->string1);
        break;
    case HF_DIAL_MEMORY: {
        int memory = data->valueint1;
        BT_LOGD("Dial memory: %d", memory);
        status = bt_sal_hfp_hf_dial_memory(&hfsm->addr, memory);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Dial memory: %d failed", memory);
            handle_dailing_fail(sm, NULL);
            break;
        }
        update_dialing_time(sm, current_timestamp_us);
        pending_action_create(hfsm, HFP_ATCMD_CODE_ATD, NULL);
        break;
    }
    case HF_DIAL_LAST:
        status = bt_sal_hfp_hf_dial_number(&hfsm->addr, NULL);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Dial Last failed");
            handle_dailing_fail(sm, NULL);
            break;
        }
        update_dialing_time(sm, current_timestamp_us);
        pending_action_create(hfsm, HFP_ATCMD_CODE_BLDN, NULL);
        break;
    case HF_STACK_EVENT_AUDIO_REQ:
        status = bt_sal_sco_connection_reply(PRIMARY_ADAPTER, &hfsm->addr, true);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Accept Sco request failed");
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
    case HF_OFFLOAD_START_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case HF_OFFLOAD_STOP_REQ:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        break;
    case HF_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        return default_process_event(sm, event, data);
    }
    return true;
}

static void hfp_hf_offload_timeout_callback(service_timer_t* timer, void* data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)data;
    hfp_hf_msg_t* msg;

    msg = hfp_hf_msg_new(HF_OFFLOAD_TIMEOUT_EVT, &hfsm->addr);
    hf_state_machine_dispatch(hfsm, msg);
    hfp_hf_msg_destroy(msg);
}

static void audio_on_enter(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;

    HF_DBG_ENTER(sm, &hfsm->addr);

    bt_pm_sco_open(PROFILE_HFP_HF, &hfsm->addr);

    if (check_sco_allowed(sm)) { /* would terminate audio connection when needed */
        /* TODO: get volume */
        /* TODO: set remote volume */
        /* TODO: set samplerate */
        bt_media_set_hfp_samplerate(hfsm->codec == HFP_CODEC_MSBC ? 16000 : 8000);
        /* TODO: request audio focus */
        /* TODO: set sco available */
        bt_media_set_sco_available();
    } else {
        BT_LOGI("SCO is not allowed");
        if (bt_sal_hfp_hf_disconnect_audio(&hfsm->addr) != BT_STATUS_SUCCESS)
            BT_ADDR_LOG("Terminate audio failed for :%s", &hfsm->addr);
    }
    hf_service_notify_audio_state_changed(&hfsm->addr, HFP_AUDIO_STATE_CONNECTED);
}

static void audio_on_exit(state_machine_t* sm)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hfp_hf_msg_t* msg;

    HF_DBG_EXIT(sm, &hfsm->addr);

    bt_pm_sco_close(PROFILE_HFP_HF, &hfsm->addr);

    /* TODO: set sco unavailable */
    bt_media_set_sco_unavailable();
    /* TODO: abandon audio focus */

    if (hfsm->offloading) {
        /* In case that AUDIO_CTRL_CMD_STOP is not received on time */
        msg = hfp_hf_msg_new(HF_OFFLOAD_STOP_REQ, &hfsm->addr);
        if (msg) {
            hf_state_machine_dispatch(hfsm, msg);
            hfp_hf_msg_destroy(msg);
        } else {
            BT_LOGE("message alloc failed");
        }
    }

    hf_service_notify_audio_state_changed(&hfsm->addr, HFP_AUDIO_STATE_DISCONNECTED);
}

static bool audio_on_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    hf_state_machine_t* hfsm = (hf_state_machine_t*)sm;
    hfp_hf_data_t* data = (hfp_hf_data_t*)p_data;
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
    case HF_SET_VOLUME: {
        hfp_volume_type_t type;
        uint8_t hf_vol;
        type = data->valueint1;
        hf_vol = bt_media_volume_media_to_hfp(data->valueint2);
        if ((type == HFP_VOLUME_TYPE_MIC) && (hf_vol != hfsm->mic_volume)) {
            status = bt_sal_hfp_hf_set_volume(&hfsm->addr, type, hf_vol);
            if (status != BT_STATUS_SUCCESS) {
                BT_LOGE("Could not set mic volume");
                break;
            }
            hfsm->mic_volume = hf_vol;
            BT_LOGD("Set Mic Volume :%" PRIu8, hfsm->mic_volume);
        } else if ((type == HFP_VOLUME_TYPE_SPK) && (hf_vol != hfsm->spk_volume)) {
            status = bt_sal_hfp_hf_set_volume(&hfsm->addr, type, hf_vol);
            if (status != BT_STATUS_SUCCESS) {
                BT_LOGE("Could not set speaker volume");
                break;
            }
            hfsm->spk_volume = hf_vol;
            BT_LOGD("Set Speaker Volume :%" PRIu8, hfsm->spk_volume);
        }
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
    case HF_OFFLOAD_START_REQ:
        if (hf_offload_send_cmd(hfsm, true) != BT_STATUS_SUCCESS) {
            BT_LOGE("failed to start offload");
            audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_START_FAIL);
            break;
        }
        flag_set(hfsm, PENDING_OFFLOAD_START);
        hfsm->offload_timer = service_loop_timer(HF_OFFLOAD_TIMEOUT, 0, hfp_hf_offload_timeout_callback, hfsm);
        break;
    case HF_OFFLOAD_START_EVT: {
        bt_hci_event_t* hci_event;
        hci_error_t result;

        if (hfsm->offload_timer) {
            service_loop_cancel_timer(hfsm->offload_timer);
            hfsm->offload_timer = NULL;
        }

        hci_event = data->data;
        result = hci_get_result(hci_event);
        if (result != HCI_SUCCESS) {
            BT_LOGE("HF_OFFLOAD_START fail, status:0x%0x", result);
            audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_START_FAIL);
            if (bt_sal_hfp_hf_disconnect_audio(&hfsm->addr) != BT_STATUS_SUCCESS) {
                BT_ADDR_LOG("Terminate audio failed for :%s", &hfsm->addr);
            }
            break;
        }

        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STARTED);
        break;
    }
    case HF_OFFLOAD_TIMEOUT_EVT: {
        flag_clear(hfsm, PENDING_OFFLOAD_START);
        hfsm->offload_timer = NULL;
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_START_FAIL);
        if (bt_sal_hfp_hf_disconnect_audio(&hfsm->addr) != BT_STATUS_SUCCESS) {
            BT_ADDR_LOG("Terminate audio failed for :%s", &hfsm->addr);
        }
        break;
    }
    case HF_OFFLOAD_STOP_REQ:
        if (hfsm->offload_timer) {
            service_loop_cancel_timer(hfsm->offload_timer);
            hfsm->offload_timer = NULL;
            audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_START_FAIL);
        }
        if (hf_offload_send_cmd(hfsm, false) != BT_STATUS_SUCCESS) {
            BT_LOGE("failed to stop offload");
            audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
            break;
        }
        flag_set(hfsm, PENDING_OFFLOAD_STOP);
        break;
    case HF_OFFLOAD_STOP_EVT:
        audio_ctrl_send_control_event(PROFILE_HFP_HF, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        return default_process_event(sm, event, data);
    }

    return true;
}

hf_state_machine_t* hf_state_machine_new(bt_address_t* addr, void* context)
{
    hf_state_machine_t* hfsm;

    hfsm = (hf_state_machine_t*)malloc(sizeof(hf_state_machine_t));
    if (!hfsm)
        return NULL;

    memset(hfsm, 0, sizeof(hf_state_machine_t));
    hfsm->recognition_active = false;
    hfsm->connection_policy = CONNECTION_POLICY_UNKNOWN;
    hfsm->service = context;
    hfsm->codec = HFP_CODEC_CVSD;
    memcpy(&hfsm->addr, addr, sizeof(bt_address_t));
    hfsm->update_calls = bt_list_new(NULL);
    hfsm->current_calls = bt_list_new(hf_call_delete);
    hfsm->media_volume = INVALID_MEDIA_VOLUME;
    list_initialize(&hfsm->pending_actions);
    hsm_ctor(&hfsm->sm, (state_t*)&disconnected_state);

    return hfsm;
}

void hf_state_machine_destory(hf_state_machine_t* hfsm)
{
    if (!hfsm)
        return;

    if (hfsm->connect_timer)
        service_loop_cancel_timer(hfsm->connect_timer);

    if (hfsm->retry_timer)
        service_loop_cancel_timer(hfsm->retry_timer);

    bt_list_free(hfsm->update_calls);
    bt_list_free(hfsm->current_calls);
    bt_media_remove_listener(hfsm->volume_listener);
    hfsm->volume_listener = NULL;
    hsm_dtor(&hfsm->sm);
    free((void*)hfsm);
}

void hf_state_machine_dispatch(hf_state_machine_t* hfsm, hfp_hf_msg_t* msg)
{
    if (!hfsm || !msg)
        return;

    hsm_dispatch_event(&hfsm->sm, msg->event, &msg->data);
}

uint32_t hf_state_machine_get_state(hf_state_machine_t* hfsm)
{
    return hsm_get_current_state_value(&hfsm->sm);
}

bt_list_t* hf_state_machine_get_calls(hf_state_machine_t* hfsm)
{
    return hfsm->current_calls;
}

uint16_t hf_state_machine_get_sco_handle(hf_state_machine_t* hfsm)
{
    return hfsm->sco_conn_handle;
}

void hf_state_machine_set_sco_handle(hf_state_machine_t* hfsm, uint16_t sco_hdl)
{
    hfsm->sco_conn_handle = sco_hdl;
}

uint8_t hf_state_machine_get_codec(hf_state_machine_t* hfsm)
{
    return hfsm->codec;
}

void hf_state_machine_set_offloading(hf_state_machine_t* hfsm, bool offloading)
{
    hfsm->offloading = offloading;
}

void hf_state_machine_set_policy(hf_state_machine_t* hfsm, connection_policy_t policy)
{
    hfsm->connection_policy = policy;
}