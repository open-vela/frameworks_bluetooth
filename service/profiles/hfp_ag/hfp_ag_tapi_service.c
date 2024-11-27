/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#define LOG_TAG "hfp_ag_tapi_service"

#include <stdint.h>

#include "bt_hfp_ag.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "hfp_ag_service.h"
#include "hfp_ag_tapi_service.h"
#include "sal_hfp_ag_interface.h"
#include "utils/log.h"

#define PRIMARY_SLOT CONFIG_BLUETOOTH_HFP_AG_PRIMARY_SLOT
#define TELEIF_EVENT_QUERY_REGISTRATION_INFO_DONE 0x34
#define TELEIF_EVENT_REQUEST_DIAL_DONE 0x71
#define TELEIF_EVENT_REQUEST_CALL_MERGE_DONE 0x74
#define TELEIF_EVENT_REQUEST_CALL_LIST_DONE 0x76

typedef struct {
    tapi_context tele_context;
    bt_list_t* current_calls;
    uint8_t call_state;
    bool is_online;
    tele_registration_info* registration_info;
} tele_info_t;

static tele_info_t g_tele_info;
static char* dbus_name = "vela.bluetooth.hfp.ag";
static int get_nums_of_call_state(uint8_t call_state);

static uint16_t get_call_index(char* call_id)
{
    uint16_t call_index = 0;
    int length = strlen(call_id);
    uint8_t temp[2];

    assert(length >= 2);

    temp[0] = (call_id)[length - 1] - '0';
    temp[1] = (call_id)[length - 2] - '0';
    call_index = (uint16_t)(temp[1] * 10 + temp[0]);

    return call_index;
}

static bool tele_call_cmp_index(void* call, void* call_index)
{
    return ((tele_call_info_t*)call)->index == *((uint8_t*)call_index);
}

static bool tele_call_cmp_state(void* call, void* call_state)
{
    return ((tele_call_info_t*)call)->call_state == *((uint8_t*)call_state);
}

static tele_call_info_t* tele_find_call_by_state(uint8_t call_state)
{
    tele_call_info_t* call;

    call = bt_list_find(g_tele_info.current_calls, tele_call_cmp_state, &call_state);

    return call;
}

static tele_call_info_t* tele_find_call_by_index(uint16_t call_index)
{
    tele_call_info_t* call;

    call = bt_list_find(g_tele_info.current_calls, tele_call_cmp_index, &call_index);

    return call;
}

static hfp_ag_call_state_t call_state_to_hfp_ag_state(int state)
{
    hfp_ag_call_state_t hfp_ag_state;

    switch (state) {
    case CALL_STATUS_ACTIVE:
        hfp_ag_state = HFP_AG_CALL_STATE_ACTIVE;
        break;
    case CALL_STATUS_HELD:
        hfp_ag_state = HFP_AG_CALL_STATE_HELD;
        break;
    case CALL_STATUS_DIALING:
        hfp_ag_state = HFP_AG_CALL_STATE_DIALING;
        break;
    case CALL_STATUS_ALERTING:
        hfp_ag_state = HFP_AG_CALL_STATE_ALERTING;
        break;
    case CALL_STATUS_INCOMING:
        hfp_ag_state = HFP_AG_CALL_STATE_INCOMING;
        break;
    case CALL_STATUS_WAITING:
        hfp_ag_state = HFP_AG_CALL_STATE_WAITING;
        break;
    case CALL_STATUS_DISCONNECTED:
        hfp_ag_state = HFP_AG_CALL_STATE_DISCONNECTED;
        break;
    default:
        BT_LOGD("%s, Unknown State:%d\n", __func__, state);
        hfp_ag_state = HFP_AG_CALL_STATE_DISCONNECTED;
        break;
    }
    return hfp_ag_state;
}

static tele_call_info_t* tele_update_call(tapi_call_info* call_info)
{
    BT_LOGD("%s", __func__);
    tele_call_info_t* call;
    call = tele_find_call_by_index(get_call_index(call_info->call_id));

    if (!call) {
        call = (tele_call_info_t*)zalloc(sizeof(tele_call_info_t));
    }

    call->index = get_call_index(call_info->call_id);
    strlcpy(call->call_id, call_info->call_id, sizeof(call->call_id));
    call->call_state = call_state_to_hfp_ag_state(call_info->state);
    strlcpy(call->line_identification, call_info->lineIdentification, sizeof(call->line_identification));
    call->is_multiparty = call_info->multiparty;

    if (!tele_find_call_by_index(get_call_index(call_info->call_id))) {
        bt_list_add_tail(g_tele_info.current_calls, call);
    }

    return call;
}

static int get_nums_of_call_state(uint8_t call_state)
{
    bt_list_t* list = g_tele_info.current_calls;
    bt_list_node_t* node;
    tele_call_info_t* call;
    int nums = 0;

    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        if (call->call_state == call_state) {
            nums++;
        }
    }

    return nums;
}

static bool all_call_disconnected(void)
{
    bt_list_node_t* node;
    bt_list_t* list = g_tele_info.current_calls;
    tele_call_info_t* call;

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
    g_tele_info.call_state = call_state;

    BT_LOGD("%s,active:%d, held:%d, state: %d", __func__, num_active,
        num_held, call_state);

    hfp_ag_phone_state_change(num_active, num_held, call_state, type, number,
        NULL);
}

static void update_call_state(tele_call_info_t* call)
{
    uint8_t active_call_nums = get_nums_of_call_state(HFP_AG_CALL_STATE_ACTIVE);
    uint8_t held_call_nums = get_nums_of_call_state(HFP_AG_CALL_STATE_HELD);
    char* number = "";
    hfp_ag_call_state_t new_state = call->call_state;
    number = call->line_identification;

    if (!call) {
        return;
    }

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

    if (new_state == HFP_AG_CALL_STATE_ALERTING && g_tele_info.call_state != HFP_AG_CALL_STATE_DIALING) {
        phone_state_change(active_call_nums, held_call_nums,
            HFP_AG_CALL_STATE_DIALING,
            HFP_CALL_ADDRTYPE_UNKNOWN,
            number, NULL);
    } else {
        if (new_state == HFP_AG_CALL_STATE_IDLE || new_state == HFP_AG_CALL_STATE_DISCONNECTED) {
            new_state = HFP_AG_CALL_STATE_DISCONNECTED;
            if (!all_call_disconnected() && new_state == g_tele_info.call_state)
                return;
        }
    }

    phone_state_change(active_call_nums, held_call_nums, new_state,
        HFP_CALL_ADDRTYPE_UNKNOWN, number, NULL);
}

static void call_state_changed_cb(tapi_async_result* result)
{
    tapi_call_info* call_info;
    tele_call_info_t* call;

    if (result->status != OK) {
        BT_LOGD("%s: result failed, status = %d", __func__, result->status);
        return;
    }

    call_info = result->data;
    BT_LOGD("%s: call id = %s, state = %d", __func__, call_info->call_id, call_info->state);

    if (call_info->state != CALL_STATUS_DISCONNECTED) {
        call = tele_update_call(call_info);
        update_call_state(call);
    } else {
        call = tele_find_call_by_index(get_call_index(call_info->call_id));
        if (!call) {
            BT_LOGD("%s: call not found", __func__);
            return;
        }

        call = tele_update_call(call_info);

        update_call_state(call);
        bt_list_remove(g_tele_info.current_calls, call);
    }
}

static void get_all_calls_complete(tapi_async_result* result)
{
    tapi_call_info* call_info = NULL;
    tele_call_info_t* call = NULL;
    uint8_t call_nums = result->arg2;

    if (result->status != OK) {
        BT_LOGD("%s: result failed, status = %d", __func__, result->status);
        return;
    }

    if (call_nums == 0) {
        BT_LOGD("%s: no calls", __func__);
        return;
    }

    call_info = result->data;

    for (int i = 0; i < call_nums; i++) {
        call = tele_update_call(call_info + i);
        update_call_state(call);
    }
}

static void radio_state_changed_cb(tapi_async_result* result)
{
    tapi_radio_state radio_state = RADIO_STATE_UNAVAILABLE;
    int ret = -1;

    if (result->status != OK) {
        BT_LOGD("%s: result failed, status = %d", __func__, result->status);
        return;
    }

    g_tele_info.is_online = false;

    if (g_tele_info.current_calls != NULL) {
        bt_list_clear(g_tele_info.current_calls);
    }

    ret = tapi_get_radio_state(g_tele_info.tele_context, PRIMARY_SLOT, &radio_state);

    if (ret) {
        BT_LOGD("%s: get radio state failed, ret = %d", __func__, ret);
        return;
    }

    if (radio_state != RADIO_STATE_ON) {
        BT_LOGD("%s: radio state is not on", __func__);
        return;
    }

    g_tele_info.is_online = true;
    tapi_call_get_all_calls(g_tele_info.tele_context, PRIMARY_SLOT, TELEIF_EVENT_REQUEST_CALL_LIST_DONE, get_all_calls_complete);
}

bt_status_t tele_service_get_network_info(hfp_network_state_t* network,
    hfp_roaming_state_t* roam,
    uint8_t* signal)
{
    tapi_signal_strength signal_info = {};
    bool roam_state;
    int ret = -1;

    // network_state to be add
    *network = g_tele_info.registration_info->reg_state == NETWORK_REGISTRATION_STATUS_REGISTERED ? HFP_NETWORK_AVAILABLE : HFP_NETWORK_NOT_AVAILABLE;
    ret = tapi_network_get_signalstrength(g_tele_info.tele_context, PRIMARY_SLOT, &signal_info);
    if (ret) {
        BT_LOGD("%s: get signal strength failed, ret = %d", __func__, ret);
        *signal = 0;
        *roam = HFP_ROAM_STATE_NO_ROAMING;
        return BT_STATUS_FAIL;
    }

    *signal = ((signal_info.rssi - 1) / 20) + 1;
    ret = tapi_network_is_voice_roaming(g_tele_info.tele_context, PRIMARY_SLOT, &roam_state);
    if (ret) {
        BT_LOGD("%s: get roaming state failed, ret = %d", __func__, ret);
        *roam = HFP_ROAM_STATE_NO_ROAMING;
        return BT_STATUS_FAIL;
    }

    *roam = roam_state ? HFP_ROAM_STATE_ROAMING : HFP_ROAM_STATE_NO_ROAMING;

    BT_LOGD("%s: network = %d, signal = %d, roam = %d", __func__, *network, *signal, *roam);
    return BT_STATUS_SUCCESS;
}

static void update_device_status(void)
{
    hfp_network_state_t network = HFP_NETWORK_NOT_AVAILABLE;
    hfp_roaming_state_t roam = HFP_ROAM_STATE_NO_ROAMING;
    uint8_t signal = 0;
    tele_service_get_network_info(&network, &roam, &signal);
    hfp_ag_device_status_changed(network, roam, signal, 5);
}

static void strength_changed_cb(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGD("%s: result failed, status = %d", __func__, result->status);
        return;
    }

    update_device_status();
}

static void network_get_registration_info_cb(tapi_async_result* result)
{
    tapi_registration_info* registration_info;

    if (result->status != OK) {
        BT_LOGD("%s: result failed, status = %d", __func__, result->status);
        return;
    }

    registration_info = result->data;

    g_tele_info.registration_info->reg_state = (uint8_t)registration_info->reg_state;
    strlcpy(g_tele_info.registration_info->operator_name, registration_info->operator_name, sizeof(g_tele_info.registration_info->operator_name));
    g_tele_info.registration_info->roaming_type = (uint8_t)registration_info->roaming_type;

    BT_LOGD("%s: reg_state: %d, operator name: %s, roaming type: %d", __func__, g_tele_info.registration_info->reg_state, g_tele_info.registration_info->operator_name, g_tele_info.registration_info->roaming_type);
}

static void network_state_changed_cb(tapi_async_result* result)
{
    if (result->status != OK) {
        BT_LOGD("%s: result failed, status = %d", __func__, result->status);
        return;
    }

    tapi_network_get_registration_info(g_tele_info.tele_context, PRIMARY_SLOT, TELEIF_EVENT_QUERY_REGISTRATION_INFO_DONE, network_get_registration_info_cb);
    update_device_status();
}

void tele_service_get_phone_state(uint8_t* num_active, uint8_t* num_held,
    uint8_t* call_state)
{
    *num_active = get_nums_of_call_state(HFP_AG_CALL_STATE_ACTIVE);
    *num_held = get_nums_of_call_state(HFP_AG_CALL_STATE_HELD);
    *call_state = g_tele_info.call_state;
}

void tele_service_query_current_call(bt_address_t* addr)
{
    bt_list_node_t* node;
    bt_list_t* list = g_tele_info.current_calls;
    tele_call_info_t* call;
    int index = 0;

    BT_LOGD("%s", __func__);
    if (!g_tele_info.is_online) {
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
    if (!g_tele_info.is_online || !g_tele_info.registration_info)
        return "";

    BT_LOGD("%s operator name: %s", __func__, g_tele_info.registration_info->operator_name);
    return g_tele_info.registration_info->operator_name;
}

bt_status_t tele_service_answer_call(void)
{
    tele_call_info_t* call = NULL;
    int ret = -1;

    BT_LOGD("%s", __func__);
    if (!g_tele_info.is_online) {
        return BT_STATUS_NOT_ENABLED;
    }

    call = tele_find_call_by_state(HFP_AG_CALL_STATE_INCOMING);

    if (!call) {
        return BT_STATUS_FAIL;
    }

    ret = tapi_call_answer_by_id(g_tele_info.tele_context, PRIMARY_SLOT, call->call_id);
    if (ret) {
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_reject_call(void)
{
    tele_call_info_t* call = NULL;
    int ret = -1;

    BT_LOGD("%s", __func__);
    if (!g_tele_info.is_online) {
        return BT_STATUS_NOT_ENABLED;
    }

    call = tele_find_call_by_state(HFP_AG_CALL_STATE_INCOMING);

    if (!call) {
        return BT_STATUS_FAIL;
    }

    ret = tapi_call_hangup_by_id(g_tele_info.tele_context, PRIMARY_SLOT, call->call_id);
    if (ret) {
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t hangup_call(tele_call_info_t* call)
{
    int ret = -1;

    if (!call) {
        return BT_STATUS_FAIL;
    }

    ret = tapi_call_hangup_by_id(g_tele_info.tele_context, PRIMARY_SLOT, call->call_id);
    if (ret) {
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t hangup_all_call()
{
    int ret = -1;
    bt_list_t* list = g_tele_info.current_calls;
    bt_list_node_t* node;
    tele_call_info_t* call;

    BT_LOGD("%s", __func__);
    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        ret = tapi_call_hangup_by_id(g_tele_info.tele_context, PRIMARY_SLOT, call->call_id);
        if (ret) {
            BT_LOGD("%s hangup call %s failed", __func__, call->line_identification);
            continue;
        }
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_hangup_call(void)
{
    BT_LOGD("%s", __func__);
    if (!g_tele_info.is_online) {
        return BT_STATUS_NOT_ENABLED;
    }

    return hangup_all_call();
}

bt_status_t tele_service_dial_number(char* number)
{
    int ret = -1;

    BT_LOGD("%s", __func__);
    if (!g_tele_info.is_online)
        return BT_STATUS_NOT_ENABLED;

    if (!number)
        return BT_STATUS_FAIL;

    ret = tapi_call_dial(g_tele_info.tele_context, PRIMARY_SLOT, number, 0, TELEIF_EVENT_REQUEST_DIAL_DONE, NULL);
    if (ret) {
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t tele_service_call_control(uint8_t chld)
{
    int ret = -1;
    tele_call_info_t* waiting_call;
    tele_call_info_t* held_call;
    tele_call_info_t* active_call;

    BT_LOGD("%s, chld = %d", __func__, chld);
    if (!g_tele_info.is_online)
        return BT_STATUS_NOT_ENABLED;

    switch (chld) {
    case HFP_HF_CALL_CONTROL_CHLD_0:
        waiting_call = tele_find_call_by_state(HFP_AG_CALL_STATE_WAITING);
        held_call = tele_find_call_by_state(HFP_AG_CALL_STATE_HELD);

        if (waiting_call != NULL) {
            ret = hangup_call(waiting_call);
        } else if (held_call != NULL) {
            /* hangup held call */
            ret = hangup_call(held_call);
        }
        break;
    case HFP_HF_CALL_CONTROL_CHLD_1:
        ret = tapi_call_release_and_answer(g_tele_info.tele_context, PRIMARY_SLOT);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_2:
        ret = tapi_call_hold_and_answer(g_tele_info.tele_context, PRIMARY_SLOT);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_3:
        active_call = tele_find_call_by_state(HFP_AG_CALL_STATE_ACTIVE);
        held_call = tele_find_call_by_state(HFP_AG_CALL_STATE_HELD);

        if (!active_call || !held_call)
            return BT_STATUS_FAIL;

        ret = tapi_call_merge_call(g_tele_info.tele_context, PRIMARY_SLOT, TELEIF_EVENT_REQUEST_CALL_MERGE_DONE, NULL);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_4:
        BT_LOGD("%s, received HFP_HF_CALL_CONTROL_CHLD_4", __func__);
    default:
        return BT_STATUS_FAIL;
    }

    if (ret) {
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static int tele_listen_call_manager_change()
{
    int watch_id;

    watch_id = tapi_register(g_tele_info.tele_context, PRIMARY_SLOT, MSG_NETWORK_STATE_CHANGE_IND, NULL, network_state_changed_cb);
    if (watch_id < 0)
        return watch_id;

    watch_id = tapi_register(g_tele_info.tele_context, PRIMARY_SLOT, MSG_CALL_STATE_CHANGE_IND, NULL, call_state_changed_cb);
    if (watch_id < 0)
        return watch_id;

    watch_id = tapi_register(g_tele_info.tele_context, PRIMARY_SLOT, MSG_RADIO_STATE_CHANGE_IND, NULL, radio_state_changed_cb);
    if (watch_id < 0)
        return watch_id;

    watch_id = tapi_register(g_tele_info.tele_context, PRIMARY_SLOT, MSG_SIGNAL_STRENGTH_CHANGE_IND, NULL, strength_changed_cb);
    return watch_id;
}

static void teleif_on_tapi_client_ready(const char* client_name, void* user_data)
{
    tapi_radio_state radio_state = RADIO_STATE_UNKNOWN;
    if (!client_name) {
        BT_LOGD("%s: tapi is not ready", __func__);
    }

    BT_LOGD("%s: tapi is ready for %s", __func__, client_name);

    tapi_get_radio_state(g_tele_info.tele_context, PRIMARY_SLOT, &radio_state);

    g_tele_info.is_online = radio_state == RADIO_STATE_ON ? true : false;

    if (g_tele_info.is_online) {
        tapi_call_get_all_calls(g_tele_info.tele_context, PRIMARY_SLOT, TELEIF_EVENT_REQUEST_CALL_LIST_DONE, get_all_calls_complete);
        tapi_network_get_registration_info(g_tele_info.tele_context, PRIMARY_SLOT, TELEIF_EVENT_QUERY_REGISTRATION_INFO_DONE, network_get_registration_info_cb);
    }
}

void tele_service_init(void)
{
    int ret = -1;
    BT_LOGD("%s", __func__);
    g_tele_info.current_calls = bt_list_new(NULL);
    g_tele_info.registration_info = (tele_registration_info*)zalloc(sizeof(tele_registration_info));
    g_tele_info.tele_context = tapi_open(dbus_name, teleif_on_tapi_client_ready, NULL);
    g_tele_info.call_state = HFP_AG_CALL_STATE_DISCONNECTED;
    g_tele_info.is_online = false;
    if (!g_tele_info.tele_context) {
        BT_LOGD("tele client connect failed");
        free(g_tele_info.registration_info);
        g_tele_info.registration_info = NULL;
        return;
    }

    ret = tele_listen_call_manager_change();
    if (ret < 0) {
        BT_LOGD("tele client register failed");
        tele_service_cleanup();
        return;
    }
}
void tele_service_cleanup(void)
{
    BT_LOGD("%s", __func__);
    if (!g_tele_info.tele_context)
        return;

    tapi_unregister(g_tele_info.tele_context, PRIMARY_SLOT);
    tapi_close(g_tele_info.tele_context);

    g_tele_info.tele_context = NULL;
    bt_list_clear(g_tele_info.current_calls);
    g_tele_info.call_state = HFP_AG_CALL_STATE_DISCONNECTED;
    g_tele_info.is_online = false;

    if (g_tele_info.registration_info != NULL) {
        free(g_tele_info.registration_info);
        g_tele_info.registration_info = NULL;
    }

    if (g_tele_info.current_calls != NULL) {
        free(g_tele_info.current_calls);
        g_tele_info.current_calls = NULL;
    }
}
