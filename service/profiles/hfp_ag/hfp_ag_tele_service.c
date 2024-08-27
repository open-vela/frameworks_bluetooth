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
#define LOG_TAG "tele_service"

#include <stdint.h>

#include "bt_list.h"
#include "hfp_ag_service.h"
#include "hfp_ag_tele_service.h"
#include "sal_hfp_ag_interface.h"
#include "telephony_interface.h"

#include "utils/log.h"

#define PRIMARY_SLOT CONFIG_BLUETOOTH_HFP_AG_PRIMARY_SLOT

static void on_connection_state_changed(tele_client_t* tele, bool connected);
static void radio_state_changed(tele_client_t* tele, int radio_state);
static void on_call_added(tele_client_t* tele, tele_call_t* call);
static void on_call_removed(tele_client_t* tele, tele_call_t* call);
static void on_call_state_changed(tele_client_t* tele, tele_call_t* call,
    int state);
static void call_disconnect_reason(tele_client_t* tele, tele_call_t* call, int reason);
static void on_operator_status_changed(tele_client_t* tele, int status);
static void on_network_reg_state_changed(tele_client_t* tele, int status);
static void on_signal_strength_changed(tele_client_t* tele, int strength);
static void radio_state_changed(tele_client_t* tele, int radio_state);

static void update_call_state(hfp_ag_call_state_t new_state);
static void get_current_calls(tele_client_t* tele, tele_call_t** calls, uint8_t nums);

static tele_callbacks_t tele_cbs = {
    .connection_state_cb = on_connection_state_changed,
    .radio_state_change_cb = radio_state_changed,
    .call_added_cb = on_call_added,
    .call_removed_cb = on_call_removed,
    .operator_status_changed_cb = on_operator_status_changed,
    .operator_name_changed_cb = NULL,
    .network_reg_state_changed_cb = on_network_reg_state_changed,
    .signal_strength_changed_cb = on_signal_strength_changed,
};

tele_call_callbacks_t tele_call_cbs = {
    .call_state_changed_cb = on_call_state_changed,
    .call_disconnect_reason_cb = call_disconnect_reason,
};

static tele_client_t* tele_context = NULL;
static bt_list_t* g_current_calls = NULL;
static uint8_t g_num_active = 0;
static uint8_t g_num_held = 0;
static uint8_t g_call_state = CALL_STATUS_DISCONNECTED;
static bool is_online = false;
static bool is_connected = false;

static void on_connection_state_changed(tele_client_t* tele, bool connected)
{
    is_connected = connected;

    if (connected) {
        is_online = teleif_modem_is_radio_on(tele, PRIMARY_SLOT);
        if (is_online)
            teleif_get_all_calls(tele, PRIMARY_SLOT, get_current_calls);
    }

    BT_LOGD("%s, connected:%d, is_online:%d", __func__, connected, is_online);
}

static void radio_state_changed(tele_client_t* tele, int radio_state)
{
    BT_LOGD("%s, radio_state:%d", __func__, radio_state);

    is_online = false;
    bt_list_clear(g_current_calls);

    if (radio_state == RADIO_STATUS_ON) {
        is_online = true;
        teleif_get_all_calls(tele, PRIMARY_SLOT, get_current_calls);
    } else {
        /* TODO: disconnect hfp ag connection? */
    }
}

static void dump_call(tele_call_t* call)
{
    BT_LOGD("Call:\n"
            "\tState:%d\n"
            "\tStartTime:%s\n"
            "\tLineIdentification:%s\n"
            "\tIncomingLine:%s\n"
            "\tName:%s\n"
            "\tRemoteHeld:%d, Emergency:%d\n"
            "\tMultiparty:%d, RemoteMultiparty:%d",
        call->call_state, call->start_time, call->line_identification,
        call->incoming_line, call->name, call->is_remote_held, call->is_emergency,
        call->is_multiparty, call->is_remote_multiparty);
}

static bool call_is_found(void* data, void* context)
{
    return data == context;
}

static void get_current_calls(tele_client_t* tele, tele_call_t** calls, uint8_t nums)
{
    tele_call_t* call;

    if (!calls || !nums)
        return;

    for (int i = 0; i < nums; i++) {
        call = calls[i];
        if (!bt_list_find(g_current_calls, call_is_found, call)) {
            dump_call(call);
            bt_list_add_tail(g_current_calls, call);
            teleif_call_register_callbacks(tele, call, &tele_call_cbs);
        }
    }

    update_call_state(call->call_state);
}

static void on_call_added(tele_client_t* tele, tele_call_t* call)
{
    if (bt_list_find(g_current_calls, call_is_found, call))
        return;

    dump_call(call);
    bt_list_add_tail(g_current_calls, call);
    teleif_call_register_callbacks(tele, call, &tele_call_cbs);
    update_call_state(call->call_state);
}

static void on_call_removed(tele_client_t* tele, tele_call_t* call)
{
    BT_LOGD("%s", __func__);
    if (call->call_state != HFP_AG_CALL_STATE_IDLE && call->call_state != HFP_AG_CALL_STATE_DISCONNECTED) {
        /* An active, setup, or held call is terminated */
        update_call_state(call->call_state);
    }
    teleif_call_unregister_callbacks(tele, call, &tele_call_cbs);
    bt_list_remove(g_current_calls, call);
}

static void update_device_status(void)
{
    uint8_t signal = 0;
    hfp_network_state_t network_state;
    hfp_roaming_state_t roam_state;

    tele_service_get_network_info(&network_state, &roam_state, &signal);
    hfp_ag_device_status_changed(NULL, network_state, roam_state, signal, 5);
}

static void on_operator_status_changed(tele_client_t* tele, int status)
{
    update_device_status();
}

static void on_network_reg_state_changed(tele_client_t* tele, int status)
{
    update_device_status();
}

static void on_signal_strength_changed(tele_client_t* tele, int strength)
{
    teleif_modem_get_radio_power(tele, PRIMARY_SLOT);
    update_device_status();
}

static void on_call_state_changed(tele_client_t* tele, tele_call_t* call,
    int state)
{
    BT_LOGD("%s, callstate:%d", __func__, state);
    update_call_state(state);
}

static void call_disconnect_reason(tele_client_t* tele, tele_call_t* call, int reason)
{
    BT_LOGD("%s disconnect_reason:%d", __func__, reason);
}

static void dial_number_callback(tele_client_t* tele, bool succeeded)
{
    uint8_t result = succeeded ? HFP_ATCMD_RESULT_OK : HFP_ATCMD_RESULT_ERROR;

    BT_LOGD("Dial result:%d", succeeded);
    hfp_ag_dial_result(result);
}

static tele_call_t* get_call_by_state(uint8_t call_state)
{
    bt_list_t* list = g_current_calls;
    bt_list_node_t* node;
    tele_call_t* call;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        if (call->call_state == call_state)
            return call;
    }

    return NULL;
}

static int get_nums_of_call_state(uint8_t call_state)
{
    bt_list_t* list = g_current_calls;
    bt_list_node_t* node;
    tele_call_t* call;
    int nums = 0;

    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        if (call->call_state == call_state)
            nums++;
    }

    return nums;
}

static bool all_call_disconnected(void)
{
    bt_list_node_t* node;
    bt_list_t* list = g_current_calls;
    tele_call_t* call;

    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        if (call->call_state != HFP_AG_CALL_STATE_IDLE && call->call_state != HFP_AG_CALL_STATE_DISCONNECTED)
            return false;
    }

    return true;
}

static void phone_state_change(uint8_t num_active, uint8_t num_held,
    hfp_ag_call_state_t call_state,
    hfp_call_addrtype_t type, const char* number,
    const char* name)
{
    g_num_active = num_active;
    g_num_held = num_held;
    g_call_state = call_state;
    BT_LOGD("%s,active:%d, held:%d, state: %d, number:%s", __func__, num_active,
        num_held, call_state, number);
    hfp_ag_phone_state_change(NULL, num_active, num_held, call_state, type, number, NULL);
}

static void update_call_state(hfp_ag_call_state_t new_state)
{
    uint8_t active_call_nums = get_nums_of_call_state(HFP_AG_CALL_STATE_ACTIVE);
    uint8_t held_call_nums = get_nums_of_call_state(HFP_AG_CALL_STATE_HELD);
    char* number = "";
    tele_call_t* call;

    BT_LOGD("%s,state: %d", __func__, new_state);
    call = get_call_by_state(new_state);
    if (!call)
        return;

    number = call->line_identification;

    switch (new_state) {
    case HFP_AG_CALL_STATE_INCOMING:
    case HFP_AG_CALL_STATE_WAITING:
        call->is_incoming = true;
        break;
    case HFP_AG_CALL_STATE_DIALING:
    case HFP_AG_CALL_STATE_ALERTING:
        call->is_incoming = false;
        break;
    case HFP_AG_CALL_STATE_IDLE:
    case HFP_AG_CALL_STATE_DISCONNECTED:
        call->is_incoming = false; // reset.
        break;
    default:
        /* nothing to do at this stage */
        break;
    }

    if (new_state == HFP_AG_CALL_STATE_ALERTING && g_call_state != HFP_AG_CALL_STATE_DIALING) {
        phone_state_change(active_call_nums, held_call_nums,
            HFP_AG_CALL_STATE_DIALING,
            HFP_CALL_ADDRTYPE_UNKNOWN,
            number, NULL);
    } else if (new_state == HFP_AG_CALL_STATE_IDLE || new_state == HFP_AG_CALL_STATE_DISCONNECTED) {
        new_state = HFP_AG_CALL_STATE_DISCONNECTED;
        /* if all disconnected, send disconnected notification */
        if (!all_call_disconnected() && new_state == g_call_state)
            return;
    }

    phone_state_change(active_call_nums, held_call_nums, new_state,
        HFP_CALL_ADDRTYPE_UNKNOWN, number, NULL);
}

void tele_service_init(void)
{
    g_current_calls = bt_list_new(NULL);
    tele_context = teleif_client_connect("HFP-AG");
    if (!tele_context) {
        BT_LOGD("tele client connect failed");
        return;
    }

    teleif_register_callbacks(tele_context, PRIMARY_SLOT, &tele_cbs);
    BT_LOGD("%s end", __func__);
}

void tele_service_cleanup(void)
{
    if (!tele_context)
        return;
    teleif_unregister_callbacks(tele_context, PRIMARY_SLOT, &tele_cbs);
    teleif_client_disconnect(tele_context);
    bt_list_free(g_current_calls);
    g_current_calls = NULL;
}

bt_status_t tele_service_dial_number(char* number)
{
    if (!is_connected || !is_online)
        return BT_STATUS_NOT_ENABLED;

    if (!number)
        return BT_STATUS_FAIL;

    if (teleif_call_dial_number(tele_context, PRIMARY_SLOT, number, dial_number_callback) != 0) {
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_answer_call(void)
{
    if (!is_connected || !is_online)
        return BT_STATUS_NOT_ENABLED;

    tele_call_t* call = get_call_by_state(CALL_STATUS_INCOMING);
    if (!call)
        return BT_STATUS_FAIL;

    if (teleif_call_answer_call(tele_context, call) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_reject_call(void)
{
    if (!is_connected || !is_online)
        return BT_STATUS_NOT_ENABLED;

    tele_call_t* call = get_call_by_state(CALL_STATUS_INCOMING);
    if (!call)
        return BT_STATUS_FAIL;

    if (teleif_call_reject_call(tele_context, call) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_hangup_call(void)
{
    if (!is_connected || !is_online)
        return BT_STATUS_NOT_ENABLED;

    if (teleif_call_hangup_all_call(tele_context, PRIMARY_SLOT) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_call_control(uint8_t chld)
{
    if (!is_connected || !is_online)
        return BT_STATUS_NOT_ENABLED;

    switch (chld) {
    case HFP_HF_CALL_CONTROL_CHLD_0: {
        tele_call_t* waiting_call = get_call_by_state(CALL_STATUS_WAITING);
        tele_call_t* held_call = get_call_by_state(CALL_STATUS_HELD);

        if (waiting_call != NULL) {
            teleif_call_hangup_call(tele_context, waiting_call);
        } else if (held_call != NULL) {
            /* hangup held call */
            teleif_call_hangup_call(tele_context, held_call);
        }
    } break;
    case HFP_HF_CALL_CONTROL_CHLD_1:
        teleif_call_release_and_answer(tele_context, PRIMARY_SLOT);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_2:
        teleif_call_hold_and_answer(tele_context, PRIMARY_SLOT);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_3: {
        tele_call_t* active_call = get_call_by_state(CALL_STATUS_ACTIVE);
        tele_call_t* held_call = get_call_by_state(CALL_STATUS_HELD);

        if (!active_call || !held_call)
            return BT_STATUS_FAIL;

        teleif_call_merge_call(tele_context, PRIMARY_SLOT);
    } break;
    case HFP_HF_CALL_CONTROL_CHLD_4:
    default:
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

void tele_service_get_phone_state(uint8_t* num_active, uint8_t* num_held,
    uint8_t* call_state)
{
    *num_active = g_num_active;
    *num_held = g_num_held;
    *call_state = g_call_state;
}

void tele_service_query_current_call(bt_address_t* addr)
{
    bt_list_node_t* node;
    bt_list_t* list = g_current_calls;
    tele_call_t* call;
    int index = 0;

    if (!is_connected || !is_online) {
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
            call->call_state, HFP_CALL_MODE_VOICE,
            call->is_multiparty, HFP_CALL_ADDRTYPE_UNKNOWN,
            call->line_identification);
    }

    /* Send "OK\r\n" */
    bt_sal_hfp_ag_clcc_response(addr, 0, 0, 0, 0, 0, 0, NULL);
}

char* tele_service_get_operator(void)
{
    char* name = NULL;
    int status;

    if (!is_connected || !is_online)
        return "";

    teleif_network_get_operator(tele_context, PRIMARY_SLOT, &name, &status);

    return name;
}

bt_status_t tele_service_get_network_info(hfp_network_state_t* network,
    hfp_roaming_state_t* roam,
    uint8_t* signal)
{
    char* name;
    int status;
    int strength = -1;
    bool is_roaming = false;

    if (!is_connected || !is_online) {
        *network = HFP_NETWORK_NOT_AVAILABLE;
        *roam = HFP_ROAM_STATE_NO_ROAMING;
        *signal = 0;
        return BT_STATUS_NOT_ENABLED;
    }

    teleif_network_get_operator(tele_context, PRIMARY_SLOT, &name, &status);
    teleif_network_get_signal_strength(tele_context, PRIMARY_SLOT, &strength);
    is_roaming = teleif_network_is_roaming(tele_context, PRIMARY_SLOT);
    if (status == OPERATOR_STATUS_AVAILABLE || status == OPERATOR_STATUS_CURRENT)
        *network = HFP_NETWORK_AVAILABLE;
    else
        *network = HFP_NETWORK_NOT_AVAILABLE;
    *roam = is_roaming ? HFP_ROAM_STATE_ROAMING : HFP_ROAM_STATE_NO_ROAMING;
    *signal = ((strength - 1) / 20) + 1;

    BT_LOGD("Network:%d, roaming:%d, strength:%d", *network, *roam, *signal);

    return BT_STATUS_SUCCESS;
}