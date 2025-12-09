/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#define LOG_TAG "tele_service"

#include <stdint.h>

#include "bt_list.h"
#include "hfp_ag_service.h"
#include "hfp_ag_tele_service.h"
#include "sal_hfp_ag_interface.h"
#include "tapi.h"
#include "tapi_phone.h"
#include "utils/log.h"

static bt_list_t* g_current_calls = NULL;
static uint8_t g_call_state = CALL_STATUS_DISCONNECTED;
static uint8_t g_num_active = 0;
static uint8_t g_num_held = 0;
static bool is_online = false;
static tapi_network_operator_status g_operator_status = OPERATOR_STATUS_UNKNOWN;
static char g_operator_name[MAX_OPERATOR_NAME_LENGTH + 1];
static tapi_registration_state g_network_status = NETWORK_REGISTRATION_STATUS_NOT_REGISTERED;
static int g_strength = 0;

static void dump_call(tapi_call_info* call)
{
    BT_LOGD("Call:\n"
            "\tCall-id:%s\n"
            "\tState:%d\n"
            "\tStartTime:%s\n"
            "\tLineIdentification:%s\n"
            "\tIncomingLine:%s\n"
            "\tName:%s\n"
            "\tRemoteHeld:%d, Emergency:%d\n"
            "\tMultiparty:%d, RemoteMultiparty:%d\n"
            "\tCallinfo:%s, DisconnectReason:%d",
        call->call_id, call->state, call->start_time, call->lineIdentification,
        call->incoming_line, call->name, call->remote_held, call->is_emergency_number,
        call->multiparty, call->remote_multiparty, call->info, call->disconnect_reason);
}

static bool call_is_found(void* data, void* context)
{
    tapi_call_info* a = (tapi_call_info*)data;
    tapi_call_info* b = (tapi_call_info*)context;

    if (strcmp(a->call_id, b->call_id) == 0) {
        return true;
    }
    return false;
}

static void call_info_delete(void* data)
{
    tapi_call_info* call = (tapi_call_info*)data;
    if (call == NULL) {
        return;
    }
    free(call);
}

static int get_nums_of_call_state(uint8_t call_state)
{
    bt_list_t* list = g_current_calls;
    bt_list_node_t* node;
    tapi_call_info* call;
    int nums = 0;

    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        if (call->state == call_state)
            nums++;
    }

    return nums;
}

static void phone_state_change(uint8_t num_active, uint8_t num_held,
    tapi_call_status call_state, const char* name,
    hfp_call_addrtype_t type, const char* number)
{
    tapi_call_status out_call_state = call_state;

    g_num_active = num_active;
    g_num_held = num_held;
    if (call_state == CALL_STATUS_ALERTING && g_call_state != CALL_STATUS_DIALING) {
        out_call_state = CALL_STATUS_DIALING;
    }
    g_call_state = call_state;
    BT_LOGD("%s,active:%d, held:%d, state: %d, number:%s", __func__, num_active,
        num_held, call_state, number);
    hfp_ag_phone_state_change(NULL, num_active, num_held, out_call_state, type, number, NULL);
}

static void update_call_state(tapi_call_info* call)
{
    uint8_t active_call_nums = get_nums_of_call_state(CALL_STATUS_ACTIVE);
    uint8_t held_call_nums = get_nums_of_call_state(CALL_STATUS_HELD);
    hfp_ag_call_state_t state;

    if (!call) {
        BT_LOGD("%s:call is NULL", __func__);
        return;
    }
    state = call->state;
    if (call->state == CALL_STATUS_DISCONNECTED) {
        state = HFP_AG_CALL_STATE_DISCONNECTED;
    }
    BT_LOGD("%s,state: %d", __func__, state);

    switch (state) {
    case HFP_AG_CALL_STATE_INCOMING:
    case HFP_AG_CALL_STATE_WAITING:
        call->is_incoming = true;
        break;
    case HFP_AG_CALL_STATE_DIALING:
    case HFP_AG_CALL_STATE_ALERTING:
    case HFP_AG_CALL_STATE_IDLE:
    case HFP_AG_CALL_STATE_DISCONNECTED:
        call->is_incoming = false;
        break;
    default:
        break;
    }

    phone_state_change(active_call_nums, held_call_nums, state,
        NULL, HFP_CALL_ADDRTYPE_UNKNOWN, call->lineIdentification);
}

static void radio_state_changed(int radio_state)
{
    BT_LOGD("%s, radio_state:%d", __func__, radio_state);

    if (radio_state == RADIO_STATE_ON) {
        is_online = true;
    } else {
        is_online = false;
        g_strength = 0;
        bt_list_clear(g_current_calls);
    }
}

bt_status_t tele_service_get_network_info(hfp_network_state_t* network,
    hfp_roaming_state_t* roam,
    uint8_t* signal)
{
    if (!is_online) {
        *network = HFP_NETWORK_NOT_AVAILABLE;
        *roam = HFP_ROAM_STATE_NO_ROAMING;
        *signal = 0;
        return BT_STATUS_NOT_ENABLED;
    }

    if (g_operator_status == OPERATOR_STATUS_AVAILABLE || g_operator_status == OPERATOR_STATUS_CURRENT) {
        *network = HFP_NETWORK_AVAILABLE;
    } else {
        *network = HFP_NETWORK_NOT_AVAILABLE;
    }

    if (g_network_status == NETWORK_REGISTRATION_STATUS_ROAMING) {
        *roam = HFP_ROAM_STATE_ROAMING;
    } else {
        *roam = HFP_ROAM_STATE_NO_ROAMING;
    }

    *signal = g_strength;

    BT_LOGD("%s, Network:%d, roaming:%d, strength:%d", __func__, *network, *roam, *signal);

    return BT_STATUS_SUCCESS;
}

static void update_device_status(void)
{
    uint8_t signal = 0;
    hfp_network_state_t network_state;
    hfp_roaming_state_t roam_state;

    tele_service_get_network_info(&network_state, &roam_state, &signal);
    hfp_ag_device_status_changed(NULL, network_state, roam_state, signal, 5);
}

static void operator_status_changed(int status)
{
    BT_LOGD("%s,%d\n", __func__, status);
    g_operator_status = status;
    update_device_status();
}

static void operator_name_changed(const char* name)
{
    BT_LOGD("%s,%s\n", __func__, name);
    strlcpy(g_operator_name, name, sizeof(g_operator_name));
}

static void network_reg_state_changed(int status)
{
    BT_LOGD("%s,%d\n", __func__, status);
    g_network_status = status;
    update_device_status();
}

static void network_strength_changed(int strength)
{
    BT_LOGD("%s,%d\n", __func__, strength);
    g_strength = strength;

    update_device_status();
}

static void modem_status_changed(int status)
{
    BT_LOGD("%s,%d\n", __func__, status);
}

static void radio_power_changed(bool state)
{
    BT_LOGD("%s,%d\n", __func__, state);
}

static void call_state_changed(tapi_call_info* call_info)
{
    tapi_call_info* call = NULL;
    tapi_call_info* exist_call = bt_list_find(g_current_calls, call_is_found, call_info);

    BT_LOGD("%s\n", __func__);

    dump_call(call_info);

    if (exist_call != NULL)
        bt_list_remove(g_current_calls, exist_call);

    if (bt_list_is_empty(g_current_calls) && call_info->state == CALL_STATUS_DISCONNECTED) {
        update_call_state(call_info);
    }

    if (call_info->state != CALL_STATUS_DISCONNECTED) {
        call = (tapi_call_info*)calloc(1, sizeof(tapi_call_info));
        memcpy(call, call_info, sizeof(tapi_call_info));
        bt_list_add_tail(g_current_calls, call);
        update_call_state(call);
    }
}

static tapi_call_info* get_call_by_state(uint8_t call_state)
{
    bt_list_t* list = g_current_calls;
    bt_list_node_t* node;
    tapi_call_info* call;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        if (call->state == call_state)
            return call;
    }

    return NULL;
}

static void dial_number_callback(tapi_async_result* ar)
{
    uint8_t result = ar->status == 0 ? HFP_ATCMD_RESULT_OK : HFP_ATCMD_RESULT_ERROR;

    BT_LOGD("Dial result:%d", ar->status);
    hfp_ag_dial_result(result);
}

bt_status_t tele_service_dial_number(char* number)
{
    tapi_call_data_t call_info;

    BT_LOGD("%s\n", __func__);
    if (!is_online)
        return BT_STATUS_NOT_ENABLED;

    if (!number)
        return BT_STATUS_FAIL;

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    if (call_info.phone_info == NULL) {
        BT_LOGE("%s:calloc fail", __func__);
        return BT_STATUS_FAIL;
    }
    call_info.phone_info->slot = 0;
    call_info.phone_info->phone_number = number;
    call_info.phone_info->hide_callerid = 1;
    call_info.phone_info->call_id = NULL;
    call_info.wtp_info = NULL;

    if (tapi_dial_call(call_info, dial_number_callback, NULL) != 0) {
        BT_LOGD("%s:dial fail", __func__);
        free(call_info.phone_info);
        return BT_STATUS_FAIL;
    }

    free(call_info.phone_info);
    return BT_STATUS_SUCCESS;
}

static void answer_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGD("%s:answer call fail,status=%d", __func__, result->status);
    } else {
        BT_LOGD("%s:answer call success", __func__);
    }
}

bt_status_t tele_service_answer_call(void)
{
    tapi_call_data_t call_info;
    tapi_call_info* call;

    BT_LOGD("%s\n", __func__);
    if (!is_online)
        return BT_STATUS_NOT_ENABLED;

    call = get_call_by_state(CALL_STATUS_INCOMING);
    if (!call) {
        BT_LOGE("%s:no call to answer", __func__);
        return BT_STATUS_FAIL;
    }

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    if (call_info.phone_info == NULL) {
        BT_LOGE("%s:calloc fail", __func__);
        return BT_STATUS_FAIL;
    }

    call_info.phone_info->slot = 0;
    call_info.phone_info->call_id = call->call_id;
    call_info.wtp_info = NULL;

    if (tapi_answer_call(call_info, answer_call_done, NULL) != 0) {
        BT_LOGE("%s:answer call fail", __func__);
        free(call_info.phone_info);
        return BT_STATUS_FAIL;
    }

    free(call_info.phone_info);
    return BT_STATUS_SUCCESS;
}

static void reject_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGE("%s:reject call fail,status=%d", __func__, result->status);
    } else {
        BT_LOGD("%s:reject call success", __func__);
    }
}

bt_status_t tele_service_reject_call(void)
{
    tapi_call_data_t call_info;
    tapi_call_info* call;

    BT_LOGD("%s\n", __func__);
    if (!is_online)
        return BT_STATUS_NOT_ENABLED;

    call = get_call_by_state(CALL_STATUS_INCOMING);
    if (!call) {
        BT_LOGE("%s:no call to reject", __func__);
        return BT_STATUS_FAIL;
    }

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    if (call_info.phone_info == NULL) {
        BT_LOGE("%s:calloc fail", __func__);
        return BT_STATUS_FAIL;
    }

    call_info.phone_info->slot = 0;
    call_info.phone_info->call_id = call->call_id;
    call_info.wtp_info = NULL;

    if (tapi_reject_call(call_info, reject_call_done, NULL) != 0) {
        BT_LOGE("%s:reject call fail", __func__);
        free(call_info.phone_info);
        return BT_STATUS_FAIL;
    }

    free(call_info.phone_info);
    return BT_STATUS_SUCCESS;
}

static void hangup_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGE("%s:hangup call fail,status=%d", __func__, result->status);
    } else {
        BT_LOGD("%s:hangup call success", __func__);
    }
}

bt_status_t tele_service_hangup_call(void)
{
    tapi_call_data_t call_info;

    BT_LOGD("%s\n", __func__);
    if (!is_online)
        return BT_STATUS_NOT_ENABLED;

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    if (call_info.phone_info == NULL) {
        BT_LOGE("%s:calloc fail", __func__);
        return BT_STATUS_FAIL;
    }

    call_info.phone_info->slot = 0;
    call_info.phone_info->call_id = NULL;
    call_info.wtp_info = NULL;

    if (tapi_hangup_call(call_info, hangup_call_done, NULL) != 0) {
        BT_LOGE("%s:hangup call fail", __func__);
        free(call_info.phone_info);
        return BT_STATUS_FAIL;
    }

    free(call_info.phone_info);
    return BT_STATUS_SUCCESS;
}

static void hangup_special_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGE("%s:hangup special call fail,status=%d", __func__, result->status);
    } else {
        BT_LOGD("%s:hangup special call success", __func__);
    }
}

bt_status_t tele_service_hangup_special_call(tapi_call_info* call)
{
    tapi_call_data_t call_info;

    BT_LOGD("%s\n", __func__);
    if (!is_online)
        return BT_STATUS_NOT_ENABLED;

    call_info.phone_info = (tapi_cell_call_data_t*)calloc(1, sizeof(tapi_cell_call_data_t));
    if (call_info.phone_info == NULL) {
        BT_LOGE("%s:calloc fail", __func__);
        return BT_STATUS_FAIL;
    }

    call_info.phone_info->slot = 0;
    call_info.phone_info->call_id = call->call_id;
    call_info.wtp_info = NULL;

    if (tapi_hangup_call(call_info, hangup_special_call_done, NULL) != 0) {
        BT_LOGE("%s:hangup call fail", __func__);
        free(call_info.phone_info);
        return BT_STATUS_FAIL;
    }

    free(call_info.phone_info);
    return BT_STATUS_SUCCESS;
}

static void release_and_answer_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGE("%s:release and answer call fail,status=%d", __func__, result->status);
    } else {
        BT_LOGD("%s:release and answer call success", __func__);
    }
}

static void hold_and_answer_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGE("%s:hold and answer call fail,status=%d", __func__, result->status);
    } else {
        BT_LOGD("%s:hold and answer call success", __func__);
    }
}

static void merge_call_done(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGE("%s:merge call fail,status=%d", __func__, result->status);
    } else {
        BT_LOGD("%s:merge call success", __func__);
    }
}

bt_status_t tele_service_call_control(uint8_t chld)
{
    BT_LOGD("%s\n", __func__);
    if (!is_online)
        return BT_STATUS_NOT_ENABLED;

    switch (chld) {
    case HFP_HF_CALL_CONTROL_CHLD_0: {
        tapi_call_info* waiting_call = get_call_by_state(CALL_STATUS_WAITING);
        tapi_call_info* held_call = get_call_by_state(CALL_STATUS_HELD);

        if (waiting_call != NULL) {
            tele_service_hangup_special_call(waiting_call);
        } else if (held_call != NULL) {
            /* hangup held call */
            tele_service_hangup_special_call(held_call);
        }
    } break;
    case HFP_HF_CALL_CONTROL_CHLD_1:
        tapi_release_and_answer_call(ESIM_TYPE, release_and_answer_call_done, NULL);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_2:
        tapi_hold_and_answer_call(ESIM_TYPE, hold_and_answer_call_done, NULL);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_3: {
        tapi_call_info* active_call = get_call_by_state(CALL_STATUS_ACTIVE);
        tapi_call_info* held_call = get_call_by_state(CALL_STATUS_HELD);

        if (!active_call || !held_call)
            return BT_STATUS_FAIL;

        tapi_merge_call(ESIM_TYPE, merge_call_done, NULL);
    } break;
    case HFP_HF_CALL_CONTROL_CHLD_4:
    default:
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

static tele_callbacks_t esim_cb_info = {
    .radio_state_change_cb = radio_state_changed,
    .operator_status_changed_cb = operator_status_changed,
    .operator_name_changed_cb = operator_name_changed,
    .network_reg_state_changed_cb = network_reg_state_changed,
    .strength_changed_cb = network_strength_changed,
    .modem_status_changed_cb = modem_status_changed,
    .radio_power_changed_cb = radio_power_changed,
    .call_state_changed_cb = call_state_changed,
};

static void register_esim_callback_done(tapi_async_result* result)
{
    BT_LOGD("%s,%d\n", __func__, result->status);
    if (result->status != 0) {
        BT_LOGE("%s: register esim callback fail", __func__);
    }
}

void tele_service_init(void)
{
    bool remote;
    int ret;
    g_current_calls = bt_list_new(call_info_delete);

    BT_LOGD("%s\n", __func__);
    if (CONFIG_BLUETOOTH_SERVER) {
        // cp
        remote = true;
    } else {
        // ap
        remote = false;
    }

    if (tapi_start_phone_service_client(uv_default_loop(), NULL, remote) < 0) {
        BT_LOGE("error:phone service client init fail\n");
        return;
    }

    ret = tapi_client_register_callbacks(esim_cb_info, register_esim_callback_done, NULL);
    if (ret < 0) {
        BT_LOGE("%s: register esim callback fail", __func__);
    }

    BT_LOGD("%s end", __func__);
}

static void unregister_esim_callback_done(tapi_async_result* result)
{
    BT_LOGD("%s,%d\n", __func__, result->status);
}

void tele_service_cleanup(void)
{
    int ret = 0;

    BT_LOGD("%s\n", __func__);
    ret = tapi_client_unregister_callbacks(unregister_esim_callback_done, NULL);
    if (ret < 0) {
        BT_LOGE("%s:unregister esim callback fail,ret=%d", __func__, ret);
    }

    tapi_stop_phone_service_client();
    bt_list_free(g_current_calls);
    g_current_calls = NULL;
}

void tele_service_get_phone_state(uint8_t* num_active, uint8_t* num_held,
    uint8_t* call_state)
{
    *num_active = g_num_active;
    *num_held = g_num_held;
    *call_state = g_call_state;
    BT_LOGD("%s\n", __func__);
}

void tele_service_query_current_call(bt_address_t* addr)
{
    bt_list_node_t* node;
    bt_list_t* list = g_current_calls;
    tapi_call_info* call;
    int index = 0;

    BT_LOGD("%s\n", __func__);
    if (!is_online) {
        /* Send "OK\r\n" */
        bt_sal_hfp_ag_clcc_response(addr, 0, 0, 0, 0, 0, 0, NULL);
        return;
    }

    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        index++;
        call = bt_list_node(node);

        /* Send "+CLCC" result code. */
        bt_sal_hfp_ag_clcc_response(addr, index, call->is_incoming,
            call->state, HFP_CALL_MODE_VOICE,
            call->multiparty, HFP_CALL_ADDRTYPE_UNKNOWN,
            call->lineIdentification);
    }

    /* Send "OK\r\n" */
    bt_sal_hfp_ag_clcc_response(addr, 0, 0, 0, 0, 0, 0, NULL);
}

char* tele_service_get_operator(void)
{
    BT_LOGD("%s\n", __func__);
    if (!is_online)
        return "";

    return g_operator_name;
}
