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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "lea_tbs_service"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_lea_tbs.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_audio_common.h"
#include "lea_tbs_event.h"
#include "lea_tbs_service.h"
#include "lea_tbs_tele_service.h"
#include "sal_lea_tbs_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "tapi.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
/****************************************************************************
 * Private Data
 ****************************************************************************/

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_tbs_service.started)       \
            return BT_STATUS_NOT_ENABLED; \
    }

#define TBS_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_tbs_callbacks_t, _cback, ##__VA_ARGS__)

static uint32_t lea_tbs_id = ADPT_LEA_GTBS_ID;

typedef struct
{
    bool started;
    callbacks_list_t* callbacks;
    pthread_mutex_t tbs_lock;
} lea_tbs_service_t;

static lea_tbs_service_t g_tbs_service = {
    .started = false,
    .callbacks = NULL,
};

/****************************************************************************
 * LEA TBS Interface
 ****************************************************************************/
bt_status_t lea_tbs_call_control_response(uint8_t call_index, lea_adpt_call_control_result_t result);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void lea_tbs_process_debug(lea_tbs_msg_t* msg)
{
    switch (msg->event) {
    case STACK_EVENT_TBS_STATE_CHANGED: {
        BT_LOGD("%s, event:%d, tbs_id:%d, ccid:%d, added:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8,
            msg->event_data.valuebool);
        break;
    }
    case STACK_EVENT_BEARER_SET_CHANED: {
        BT_LOGD("%s, event:%d, tbs_id:%d, result:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valuebool);
        break;
    }
    case STACK_EVENT_CALL_ADDED: {
        BT_LOGD("%s, event:%d, tbs_id:%d, call_index:%d, result:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8,
            msg->event_data.valuebool);
        break;
    }
    case STACK_EVENT_CALL_REMOVED: {
        BT_LOGD("%s, event:%d, tbs_id:%d, call_index:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8);
        break;
    }
    case STACK_EVENT_ACCEPT_CALL: {
        BT_LOGD("%s, event:%d, tbs_id:%d, call_index:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8);
        break;
    }
    case STACK_EVENT_TERMINATE_CALL: {
        BT_LOGD("%s, event:%d, tbs_id:%d, call_index:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8);
        break;
    }
    case STACK_EVENT_LOCAL_HOLD_CALL: {
        BT_LOGD("%s, event:%d, tbs_id:%d, call_index:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8);
        break;
    }
    case STACK_EVENT_LOCAL_RETRIEVE_CALL: {
        BT_LOGD("%s, event:%d, tbs_id:%d, call_index:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint16);
        break;
    }
    case STACK_EVENT_ORIGINATE_CALL: {
        BT_LOGD("%s, event:%d, tbs_id:%d, uri:%s", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.dataarry);
        break;
    }
    case STACK_EVENT_JOIN_CALL: {
        BT_LOGD("%s, event:%d, tbs_id:%d, index_number:%d, index_list:%s", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint32,
            msg->event_data.dataarry);
        break;
    }
    default: {
        BT_LOGD("Idle: Unexpected stack event");
        break;
    }
    }
}

static void lea_tbs_process_message(void* data)
{
    lea_tbs_msg_t* msg = (lea_tbs_msg_t*)data;
    lea_tbs_call_state_t* call;

    lea_tbs_process_debug(msg);

    switch (msg->event) {
    case STACK_EVENT_TBS_STATE_CHANGED: {
        TBS_CALLBACK_FOREACH(g_tbs_service.callbacks, tbs_test_cb, msg->event_data.valueint8,
            msg->event_data.valuebool);
        break;
    }
    case STACK_EVENT_BEARER_SET_CHANED: {
        TBS_CALLBACK_FOREACH(g_tbs_service.callbacks, tbs_test_cb, 0, msg->event_data.valuebool);
        break;
    }
    case STACK_EVENT_CALL_ADDED: {
        TBS_CALLBACK_FOREACH(g_tbs_service.callbacks, tbs_test_cb, msg->event_data.valueint8,
            msg->event_data.valuebool);
        break;
    }
    case STACK_EVENT_CALL_REMOVED: {
        TBS_CALLBACK_FOREACH(g_tbs_service.callbacks, tbs_test_cb, msg->event_data.valueint8,
            msg->event_data.valuebool);
        break;
    }
    case STACK_EVENT_ACCEPT_CALL: {
        char call_id[MAX_CALL_ID_LENGTH];
        sprintf(call_id, "%s%d", CONFIG_BLUETOOTH_LEAUDIO_TBS_CALL_NAME, msg->event_data.valueint8);

        if (lea_tbs_find_call_by_index(msg->event_data.valueint8) != NULL) {
            tele_service_accept_call(call_id);
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_SUCCESS);
        } else {
            msg->event_data.valueint8 = 0; // TS say Call_Index shall be zero... But TBS spec does not say.
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_INVALID_CALL_INDEX);
        }
        break;
    }
    case STACK_EVENT_TERMINATE_CALL: {
        char call_id[MAX_CALL_ID_LENGTH];
        sprintf(call_id, "%s%d", CONFIG_BLUETOOTH_LEAUDIO_TBS_CALL_NAME, msg->event_data.valueint8);

        if (lea_tbs_find_call_by_index(msg->event_data.valueint8) != NULL) {
            tele_service_terminate_call(call_id);
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_SUCCESS);
        } else {
            msg->event_data.valueint8 = 0;
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_INVALID_CALL_INDEX);
        }

        break;
    }
    case STACK_EVENT_LOCAL_HOLD_CALL: {
        call = lea_tbs_find_call_by_index(msg->event_data.valueint8);

        if (!call) {
            lea_tbs_call_control_response(0, ADPT_LEA_TBS_CALL_CONTROL_INVALID_CALL_INDEX);
            return;
        }

        if (call->state == ADPT_LEA_TBS_CALL_STATE_INCOMING || call->state == ADPT_LEA_TBS_CALL_STATE_ACTIVE || call->state == ADPT_LEA_TBS_CALL_STATE_REMOTELY_HELD) {
            tele_service_hold_call();
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_SUCCESS);
        } else {
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_OPEARTION_NOT_POSSIBLE);
        }

        break;
    }
    case STACK_EVENT_LOCAL_RETRIEVE_CALL: {
        call = lea_tbs_find_call_by_index(msg->event_data.valueint8);

        if (!call) {
            lea_tbs_call_control_response(0, ADPT_LEA_TBS_CALL_CONTROL_INVALID_CALL_INDEX);
            return;
        }

        if (call->state == ADPT_LEA_TBS_CALL_STATE_LOCALLY_HELD) {
            tele_service_unhold_call();
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_SUCCESS);
        } else {
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_OPEARTION_NOT_POSSIBLE);
        }

        break;
    }
    case STACK_EVENT_ORIGINATE_CALL: {
        tele_service_originate_call((char*)msg->event_data.dataarry);

        call = lea_tbs_find_call_by_state(ADPT_LEA_TBS_CALL_STATE_ALERTING);
        if (!call) {
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_SUCCESS);
        } else {
            msg->event_data.valueint8 = 0;
            lea_tbs_call_control_response(msg->event_data.valueint8, ADPT_LEA_TBS_CALL_CONTROL_LACK_OF_RESOURCES);
        }

        break;
    }
    case STACK_EVENT_JOIN_CALL: {

        break;
    }
    default: {

        break;
    }
    }
    lea_tbs_msg_destory(msg);
}

static bt_status_t lea_tbs_send_msg(lea_tbs_msg_t* msg)
{
    assert(msg);

    do_in_service_loop(lea_tbs_process_message, msg);

    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_tbs_on_state_changed(uint32_t tbs_id, uint8_t ccid, bool added)
{
    lea_tbs_msg_t* msg;

    msg = lea_tbs_msg_new(STACK_EVENT_TBS_STATE_CHANGED, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8 = ccid;
    msg->event_data.valuebool = added;

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_bearer_info_set(uint32_t tbs_id, char* bearer_ref, bool result)
{
    lea_tbs_msg_t* msg;

    msg = lea_tbs_msg_new(STACK_EVENT_BEARER_SET_CHANED, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valuebool = result;
    strcpy((char*)msg->event_data.dataarry, bearer_ref);

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_call_added(uint32_t tbs_id, uint8_t call_index, bool result)
{
    lea_tbs_msg_t* msg;

    msg = lea_tbs_msg_new(STACK_EVENT_CALL_ADDED, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8 = call_index;
    msg->event_data.valuebool = result;

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_call_removed(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_msg_t* msg;

    msg = lea_tbs_msg_new(STACK_EVENT_CALL_REMOVED, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8 = call_index;

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_accept_call(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_msg_t* msg;

    msg = lea_tbs_msg_new(STACK_EVENT_ACCEPT_CALL, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8 = call_index;

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_terminate_call(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_msg_t* msg;

    msg = lea_tbs_msg_new(STACK_EVENT_TERMINATE_CALL, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8 = call_index;

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_local_hold_call(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_msg_t* msg;

    msg = lea_tbs_msg_new(STACK_EVENT_LOCAL_HOLD_CALL, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8 = call_index;

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_local_retrieve_call(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_msg_t* msg;

    msg = lea_tbs_msg_new(STACK_EVENT_LOCAL_RETRIEVE_CALL, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8 = call_index;

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_originate_call(uint32_t tbs_id, size_t size, char* uri)
{
    lea_tbs_msg_t* msg;

    if (size < 1) {
        BT_LOGW("%s ,the length of uri is zero!", __func__);
        return;
    }

    msg = lea_tbs_msg_new_ext(STACK_EVENT_ORIGINATE_CALL, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    strcpy((char*)msg->event_data.dataarry, uri);

    lea_tbs_send_msg(msg);
}

void lea_tbs_on_join_call(uint32_t tbs_id, uint8_t index_number, size_t size, char* index_list)
{
    lea_tbs_msg_t* msg;

    if (index_number < 1) {
        BT_LOGW("%s ,the number of index is zero!", __func__);
        return;
    }

    msg = lea_tbs_msg_new_ext(STACK_EVENT_JOIN_CALL, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8 = index_number;
    strcpy((char*)msg->event_data.dataarry, index_list);

    lea_tbs_send_msg(msg);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/
bt_status_t lea_tbs_add()
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_add(lea_tbs_id);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_remove()
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_remove(lea_tbs_id);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_set_telephone_bearer_info(lea_tbs_telephone_bearer_t* bearer)
{
    CHECK_ENABLED();
    bt_status_t ret;
    SERVICE_LEA_TELEPHONE_BEARER_S* tele_bearer;

    tele_bearer = (SERVICE_LEA_TELEPHONE_BEARER_S*)malloc(sizeof(SERVICE_LEA_TELEPHONE_BEARER_S));
    tele_bearer->tbs_id = lea_tbs_id;
    tele_bearer->bearer_ref = bearer->bearer_ref;
    tele_bearer->provider_name = bearer->provider_name;
    tele_bearer->uci = bearer->uci;
    tele_bearer->uri_schemes = bearer->uri_schemes;
    tele_bearer->technology = bearer->technology;
    tele_bearer->signal_strength = bearer->signal_strength;
    tele_bearer->signal_strength_report_interval = bearer->signal_strength_report_interval;
    tele_bearer->status_flags = bearer->status_flags;
    tele_bearer->optional_opcodes_supported = bearer->optional_opcodes_supported;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_set_telephone_bearer_info(tele_bearer);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_add_call(lea_tbs_calls_t* call_s)
{
    CHECK_ENABLED();
    bt_status_t ret;
    SERVICE_LEA_TBS_CALL_S* bts_call_s;

    bts_call_s = (SERVICE_LEA_TBS_CALL_S*)malloc(sizeof(SERVICE_LEA_TBS_CALL_S));
    bts_call_s->tbs_id = lea_tbs_id;
    bts_call_s->index = call_s->index;
    bts_call_s->state = call_s->state;
    bts_call_s->flags = call_s->flags;
    bts_call_s->call_uri = call_s->call_uri;
    bts_call_s->incoming_target_uri = call_s->incoming_target_uri;
    bts_call_s->friendly_name = call_s->friendly_name;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_add_call(bts_call_s);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_remove_call(uint8_t call_index)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_remove_call(lea_tbs_id, call_index);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_provider_name_changed(uint8_t* name)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_provider_name_changed(lea_tbs_id, name);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_bearer_technology_changed(lea_adpt_bearer_technology_t technology)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_bearer_technology_changed(lea_tbs_id, technology);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_uri_schemes_supported_list_changed(uint8_t* uri_schemes)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_uri_schemes_supported_list_changed(lea_tbs_id, uri_schemes);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_rssi_value_changed(uint8_t strength)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_rssi_value_changed(lea_tbs_id, strength);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_rssi_interval_changed(uint8_t interval)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_rssi_interval_changed(lea_tbs_id, interval);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_status_flags_changed(uint8_t status_flags)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_status_flags_changed(lea_tbs_id, status_flags);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_call_state_changed(uint8_t number,
    lea_tbs_call_state_t* state_s)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_call_state_changed(lea_tbs_id, number,
        (SERVICE_LEA_TBS_CALL_STATE_S*)state_s);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_notify_termination_reason(uint8_t call_index,
    lea_adpt_termination_reason_t reason)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_notify_termination_reason(lea_tbs_id, call_index, reason);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_tbs_call_control_response(uint8_t call_index,
    lea_adpt_call_control_result_t result)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    ret = bt_sal_lea_tbs_call_control_response(lea_tbs_id, call_index, result);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_tbs_service.tbs_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);

    return BT_STATUS_SUCCESS;
}

static void* lea_tbs_register_callbacks(void* handle, lea_tbs_callbacks_t* callbacks)
{
    if (!g_tbs_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_tbs_service.callbacks, handle, (void*)callbacks);
}

static bool lea_tbs_unregister_callbacks(void** handle, void* cookie)
{
    if (!g_tbs_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_tbs_service.callbacks, handle, cookie);
}

static const lea_tbs_interface_t leaTbsInterface = {
    .size = sizeof(leaTbsInterface),
    .tbs_add = lea_tbs_add,
    .tbs_remove = lea_tbs_remove,
    .set_telephone_bearer = lea_tbs_set_telephone_bearer_info,
    .add_call = lea_tbs_add_call,
    .remove_call = lea_tbs_remove_call,
    .provider_name_changed = lea_tbs_provider_name_changed,
    .bearer_technology_changed = lea_tbs_bearer_technology_changed,
    .uri_schemes_supported_list_changed = lea_tbs_uri_schemes_supported_list_changed,
    .rssi_value_changed = lea_tbs_rssi_value_changed,
    .rssi_interval_changed = lea_tbs_rssi_interval_changed,
    .status_flags_changed = lea_tbs_status_flags_changed,
    .call_state_changed = lea_tbs_call_state_changed,
    .notify_termination_reason = lea_tbs_notify_termination_reason,
    .call_control_response = lea_tbs_call_control_response,
    .register_callbacks = lea_tbs_register_callbacks,
    .unregister_callbacks = lea_tbs_unregister_callbacks,
};

/****************************************************************************
 * Public function
 ****************************************************************************/
static const void* get_lea_tbs_profile_interface(void)
{
    return &leaTbsInterface;
}

static bt_status_t lea_tbs_init(void)
{
    BT_LOGD("%s", __func__);
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_tbs_startup(profile_on_startup_t cb)
{
    BT_LOGD("%s", __func__);
    bt_status_t status;
    pthread_mutexattr_t attr;
    lea_tbs_service_t* service = &g_tbs_service;
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->callbacks = bt_callbacks_list_new(2);
    if (!service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->tbs_lock, &attr);
    service->started = true;

    lea_tbs_add();

    lea_tbs_tele_service_init();

    return BT_STATUS_SUCCESS;

fail:
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    pthread_mutex_destroy(&service->tbs_lock);
    return status;
}

static bt_status_t lea_tbs_shutdown(profile_on_shutdown_t cb)
{
    if (!g_tbs_service.started)
        return BT_STATUS_SUCCESS;

    pthread_mutex_lock(&g_tbs_service.tbs_lock);
    lea_tbs_remove();
    g_tbs_service.started = false;

    bt_callbacks_list_free(g_tbs_service.callbacks);
    g_tbs_service.callbacks = NULL;
    pthread_mutex_unlock(&g_tbs_service.tbs_lock);
    pthread_mutex_destroy(&g_tbs_service.tbs_lock);
    return BT_STATUS_SUCCESS;
}

static void lea_tbs_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_tbs_dump(void)
{
    printf("impl leaudio tbs dump");
    return 0;
}

static const profile_service_t lea_tbs_service = {
    .auto_start = true,
    .name = PROFILE_TBS_NAME,
    .id = PROFILE_LEAUDIO_TBS,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = lea_tbs_init,
    .startup = lea_tbs_startup,
    .shutdown = lea_tbs_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_tbs_profile_interface,
    .cleanup = lea_tbs_cleanup,
    .dump = lea_tbs_dump,
};

void register_lea_tbs_service(void)
{
    register_service(&lea_tbs_service);
}

#endif