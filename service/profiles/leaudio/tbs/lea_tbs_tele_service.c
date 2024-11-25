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
#define LOG_TAG "lea_tbs_tele_service"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_lea_tbs.h"
#include "bt_list.h"
#include "lea_tbs_service.h"
#include "lea_tbs_tele_service.h"
#include "sal_lea_tbs_interface.h"
#include "tapi.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS

#define TBS_EVENT_REQUEST_DIAL_DONE 0x71
#define TBS_EVENT_REQUEST_CALL_LIST_DONE 0x76
#define PRIMARY_SLOT CONFIG_BLUETOOTH_LEAUDIO_TBS_PRIMARY_SLOT

#define BTS_DEFAULT_BEARER_REF "1"
#define BTS_DEFAULT_TECH LEA_TBS_BEARER_5G
#define BTS_DEFAULT_SIGNAL_STRENGTH 100
#define BTS_DEFAULT_SIGNAL_STRENGTH_REPORT_INTERVAL 0
#define BTS_DEFAULT_STATUS_FLAGS LEA_TBS_STATUS_INBAND_RINGTONE_ENABLED | LEA_TBS_STATUS_SERVER_IN_SILENT_MODE
#define BTS_DEFAULT_OPTIONAL_OPCODE_SUPPORTED LEA_TBS_SUPPORTED_CCP_OP_LOCAL_HOLD | LEA_TBS_SUPPORTED_CCP_OP_JOIN

static const char* BTS_DEFAULT_NAME = "unknown";
static const char* BTS_DEFAULT_UCI = "GTBS";
static const char* BTS_DEFAULT_URI_SCHEMES = "tel";

static tapi_context context;
static bt_list_t* g_current_calls = NULL;
static bool isRemoteControl = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint8_t get_call_index(char* call_id)
{
    int length = strlen(call_id);
    char temp = (call_id)[length - 1];
    uint8_t call_index = temp - '0';
    return call_index;
}

static lea_adpt_call_state_t call_state_to_tbs_state(int state)
{
    lea_adpt_call_state_t tbs_state;

    if (state == CALL_STATUS_ACTIVE) {
        tbs_state = ADPT_LEA_TBS_CALL_STATE_ACTIVE;
    } else if (state == CALL_STATUS_HELD) {
        tbs_state = ADPT_LEA_TBS_CALL_STATE_LOCALLY_HELD;
    } else if (state == CALL_STATUS_DIALING) {
        tbs_state = ADPT_LEA_TBS_CALL_STATE_DIALING;
    } else if (state == CALL_STATUS_ALERTING) {
        tbs_state = ADPT_LEA_TBS_CALL_STATE_ALERTING;
    } else if (state == CALL_STATUS_INCOMING) {
        tbs_state = ADPT_LEA_TBS_CALL_STATE_INCOMING;
    } else if (state == CALL_STATUS_WAITING) {
        tbs_state = ADPT_LEA_TBS_CALL_STATE_INCOMING;
    } else {
        BT_LOGW("%s, Others State:%d\n", __func__, state);
    }
    return tbs_state;
}

static tapi_pref_net_mode net_mode_to_tbs_tech(int netMode)
{
    lea_adpt_bearer_technology_t tbs_tech;

    if (netMode == NETWORK_PREF_NET_TYPE_UMTS) {
        tbs_tech = ADPT_LEA_TBS_BEARER_3G;
    } else if (netMode == NETWORK_PREF_NET_TYPE_GSM_ONLY) {
        tbs_tech = ADPT_LEA_TBS_BEARER_GSM;
    } else if (netMode == NETWORK_PREF_NET_TYPE_WCDMA_ONLY) {
        tbs_tech = ADPT_LEA_TBS_BEARER_WCDMA;
    } else if (netMode == NETWORK_PREF_NET_TYPE_LTE_ONLY) {
        tbs_tech = ADPT_LEA_TBS_BEARER_LTE;
    } else {
        BT_LOGW("%s, Others Net Mode:%d\n", __func__, netMode);
    }
    return tbs_tech;
}

static lea_adpt_termination_reason_t call_term_reason_to_tbs_reason(int reason)
{
    lea_adpt_termination_reason_t tbs_reason;

    if (reason == CALL_DISCONNECT_REASON_LOCAL_HANGUP) {
        if (isRemoteControl) {
            tbs_reason = ADPT_LEA_TBS_REASON_ENDED_BY_CLIENT;
            isRemoteControl = false;
        } else {
            tbs_reason = ADPT_LEA_TBS_REASON_ENDED_FROM_SERVER;
        }
    } else if (reason == CALL_DISCONNECT_REASON_REMOTE_HANGUP) {
        tbs_reason = ADPT_LEA_TBS_REASON_ENDED_BY_REMOTE;
    } else if (reason == CALL_DISCONNECT_REASON_NETWORK_HANGUP) {
        tbs_reason = ADPT_LEA_TBS_REASON_NETWORK_CONGESTION;
    } else {
        BT_LOGW("%s, Others Terminate Reason: %d\n", __func__, reason);
    }
    return tbs_reason;
}

static uint8_t lea_tbs_get_call_flags(uint8_t state)
{
    uint8_t call_flags = 0;
    switch (state) {
    case ADPT_LEA_TBS_CALL_STATE_INVAILD: {
        call_flags = 0;
        break;
    }

    case LEA_TBS_CALL_STATE_INCOMING: {
        call_flags &= ~ADPT_LEA_CALL_FLAGS_OUTGOING_CALL;
        call_flags &= ~ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_NETWORK;
        call_flags &= ~ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_SERVER;
        break;
    }

    case ADPT_LEA_TBS_CALL_STATE_DIALING:
    case ADPT_LEA_TBS_CALL_STATE_ALERTING: {
        call_flags |= ADPT_LEA_CALL_FLAGS_OUTGOING_CALL;
        call_flags &= ~ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_NETWORK;
        call_flags &= ~ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_SERVER;
        break;
    }

    case ADPT_LEA_TBS_CALL_STATE_ACTIVE: {
        call_flags &= ~ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_NETWORK;
        call_flags &= ~ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_SERVER;
        break;
    }

    case ADPT_LEA_TBS_CALL_STATE_LOCALLY_HELD: {
        call_flags &= ~ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_NETWORK;
        call_flags |= ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_SERVER;
        break;
    }

    case ADPT_LEA_TBS_CALL_STATE_REMOTELY_HELD: {
        call_flags |= ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_NETWORK;
        call_flags &= ~ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_SERVER;
        break;
    }

    case ADPT_LEA_TBS_CALL_STATE_BOTH_HELD: {
        call_flags |= ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_NETWORK;
        call_flags |= ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_SERVER;
        break;
    }
    }
    return call_flags;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool lea_tbs_call_cmp_index(void* call, void* call_index)
{
    return ((lea_tbs_call_state_t*)call)->index == *((uint8_t*)call_index);
}

static bool lea_tbs_call_cmp_state(void* call, void* call_state)
{
    return ((lea_tbs_call_state_t*)call)->state == *((uint8_t*)call_state);
}

lea_tbs_call_state_t* lea_tbs_find_call_by_index(uint8_t call_index)
{
    lea_tbs_call_state_t* call;

    call = bt_list_find(g_current_calls, lea_tbs_call_cmp_index, &call_index);

    return call;
}

lea_tbs_call_state_t* lea_tbs_find_call_by_state(uint8_t call_state)
{
    lea_tbs_call_state_t* call;

    call = bt_list_find(g_current_calls, lea_tbs_call_cmp_state, &call_state);

    return call;
}

lea_tbs_calls_t* lea_tbs_tele_add_call(tapi_call_info* call_info)
{
    BT_LOGD("%s", __func__);
    if (lea_tbs_find_call_by_index(get_call_index(call_info->call_id)) != NULL)
        return NULL;

    lea_tbs_calls_t* call_s;

    call_s = (lea_tbs_calls_t*)malloc(sizeof(lea_tbs_calls_t));
    if (!call_s) {
        BT_LOGE("error, malloc %s", __func__);
        return NULL;
    }

    call_s->index = get_call_index(call_info->call_id);
    call_s->state = call_state_to_tbs_state(call_info->state);
    call_s->flags = lea_tbs_get_call_flags(call_info->state);
    strcpy((char*)call_s->call_uri, call_info->lineIdentification);
    strcpy((char*)call_s->incoming_target_uri, call_info->lineIdentification);
    strcpy((char*)call_s->friendly_name, call_info->name);

    lea_tbs_add_call(call_s);

    return call_s;
}

static void tbs_on_tapi_client_ready(const char* client_name, void* user_data)
{
    char* name = (char*)BTS_DEFAULT_NAME;
    tapi_signal_strength ss = { 0 };
    tapi_pref_net_mode value = NETWORK_PREF_NET_TYPE_ANY;
    lea_tbs_telephone_bearer_t* bearer;

    bearer = (lea_tbs_telephone_bearer_t*)malloc(sizeof(lea_tbs_telephone_bearer_t));
    if (client_name != NULL)
        BT_LOGD("%s :tapi is ready for %s\n", __func__, client_name);

    tapi_network_get_display_name(context, PRIMARY_SLOT, &name);
    tapi_get_pref_net_mode(context, PRIMARY_SLOT, &value);
    tapi_network_get_signalstrength(context, PRIMARY_SLOT, &ss);

    bearer->bearer_ref = (void*)BTS_DEFAULT_BEARER_REF;
    strcpy((char*)bearer->provider_name, name);
    strcpy((char*)bearer->uci, BTS_DEFAULT_UCI);
    strcpy((char*)bearer->uri_schemes, BTS_DEFAULT_URI_SCHEMES);
    bearer->technology = net_mode_to_tbs_tech(value);
    bearer->signal_strength = ss.rssi;
    bearer->signal_strength_report_interval = BTS_DEFAULT_SIGNAL_STRENGTH_REPORT_INTERVAL;
    bearer->status_flags = BTS_DEFAULT_STATUS_FLAGS;
    bearer->optional_opcodes_supported = BTS_DEFAULT_OPTIONAL_OPCODE_SUPPORTED;

    lea_tbs_set_telephone_bearer_info(bearer);
    free(bearer);
    bearer = NULL;
}

static void tbs_call_list_query_complete(tapi_async_result* result)
{
    tapi_call_info* call_info;
    lea_tbs_call_state_t* state_s = malloc(sizeof(lea_tbs_call_state_t) * result->arg2);
    lea_tbs_call_state_t* sub_call;

    if (result->status != OK)
        return;

    if (result->arg2 == 0)
        return;

    call_info = result->data;

    for (int i = 0; i < result->arg2; i++) {
        (state_s + i)->index = get_call_index(call_info[i].call_id);
        (state_s + i)->state = call_state_to_tbs_state(call_info[i].state);
        (state_s + i)->flags = lea_tbs_get_call_flags(call_state_to_tbs_state(call_info[i].state));
        BT_LOGD("%s, state:%d", __func__, (state_s + i)->state);

        sub_call = lea_tbs_find_call_by_index((state_s + i)->index);
        if (!sub_call) {
            bt_list_add_tail(g_current_calls, state_s + i);
        } else {
            memcpy(sub_call, state_s + i, sizeof(lea_tbs_call_state_t));
        }
    }
    lea_tbs_call_state_changed(result->arg2, state_s);
}

static void tbs_call_manager_call_async_fun(tapi_async_result* result)
{
    uint8_t call_index;
    lea_adpt_termination_reason_t reason;
    tapi_call_info* call_info;
    tapi_cell_identity** cell_list;
    tapi_cell_identity* cell;
    int param = result->arg2;

    call_info = (tapi_call_info*)result->data;

    if (result->msg_id == MSG_CELLINFO_CHANGE_IND) {
        cell_list = result->data;

        if (cell_list != NULL) {
            while (--param >= 0) {
                cell = *cell_list++;
            }
        }

        lea_tbs_provider_name_changed((uint8_t*)cell->alpha_long);
        lea_tbs_bearer_technology_changed(cell->type);
        lea_tbs_rssi_value_changed(cell->signal_strength.rsrp);
    }

    if (call_info->state != CALL_STATUS_DISCONNECTED) {
        lea_tbs_tele_add_call(call_info);
        tapi_call_get_all_calls(context, PRIMARY_SLOT, TBS_EVENT_REQUEST_CALL_LIST_DONE,
            tbs_call_list_query_complete);
    } else {
        call_index = get_call_index(call_info->call_id);
        reason = call_term_reason_to_tbs_reason(call_info->disconnect_reason);

        lea_tbs_notify_termination_reason(call_index, reason);
        bt_list_remove(g_current_calls, lea_tbs_find_call_by_index(call_index));
        lea_tbs_remove_call(get_call_index(call_info->call_id));
    }
}

static int tbs_listen_call_manager_change()
{
    int watch_id;

    watch_id = tapi_call_register_call_state_change(context, PRIMARY_SLOT, NULL,
        tbs_call_manager_call_async_fun);
    if (watch_id < 0)
        return watch_id;

    watch_id = tapi_network_register(context, PRIMARY_SLOT, MSG_CELLINFO_CHANGE_IND,
        NULL, tbs_call_manager_call_async_fun);
    if (watch_id < 0)
        return watch_id;

    return watch_id;
}

bt_status_t tele_service_accept_call(char* call_id)
{
    tapi_call_answer_by_id(context, PRIMARY_SLOT, call_id);

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_terminate_call(char* call_id)
{
    isRemoteControl = true;

    tapi_call_hangup_by_id(context, PRIMARY_SLOT, call_id);

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_hold_call()
{
    tapi_call_hold_call(context, PRIMARY_SLOT);

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_unhold_call()
{
    tapi_call_unhold_call(context, PRIMARY_SLOT);

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_originate_call(char* uri)
{
    tapi_call_dial(context, PRIMARY_SLOT, uri, 0, TBS_EVENT_REQUEST_DIAL_DONE,
        NULL);

    return BT_STATUS_SUCCESS;
}

void lea_tbs_tele_service_init(void)
{
    char* dbus_name = "vela.bluetooth.tool";
    context = tapi_open(dbus_name, tbs_on_tapi_client_ready, NULL);
    if (context == NULL) {
        return;
    }
    g_current_calls = bt_list_new(NULL);
    tbs_listen_call_manager_change();
}

#endif