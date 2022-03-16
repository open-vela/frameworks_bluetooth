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
#define LOG_TAG "hfp_hf"
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <connectivity/bt.h>
#include <uORB/uORB.h>
#include "stack_adapter_hfp.h"
#include "stack_adapter_service_base.h"

#include "btm_manager.h"
#include "bts_service.h"
#include "btm_hfp_hf.h"
#include "bts_hf_client.h"
#include "bts_hf_client_event.h"
#include "bts_hf_client_state_machine.h"

#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct
{
    struct list_node node;
    void* handle;
    bt_address bd_addr;
    hf_state_machine_t* sm;
} hf_client_device_t;

typedef struct {
    hf_state_machine_t* hfsm;
    hf_client_msg_t* msg;
} hf_client_inter_msg_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static hf_state_machine_t* get_state_machine(bt_address bd_addr);
static void hf_client_send_message(hf_state_machine_t* sm, hf_client_msg_t* msg);

/****************************************************************************
 * Private Data
 ****************************************************************************/
hf_client_service_t g_hfp_service = {
    .started = false,
    .orb_fd = -1,
    .device_list = LIST_INITIAL_VALUE(g_hfp_service.device_list)
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static hf_client_device_t* find_hf_device_by_addr(bt_address bd_addr)
{
    hf_client_device_t* device;
    struct list_node* node;

    list_for_every(&g_hfp_service.device_list, node)
    {
        device = (hf_client_device_t*)node;
        if (memcmp(device->bd_addr, bd_addr, sizeof(bt_address)) == 0)
            return device;
    }

    return NULL;
}

static hf_client_device_t* hf_client_device_new(hf_state_machine_t* sm, bt_address bd_addr)
{
    hf_client_device_t* device;

    device = (hf_client_device_t*)malloc(sizeof(hf_client_device_t));
    if (!device)
        return NULL;

    memcpy(device->bd_addr, bd_addr, sizeof(bt_address));
    device->sm = sm;
    list_add_tail(&g_hfp_service.device_list, &device->node);

    return device;
}

static void hf_client_device_delete(hf_client_device_t* device)
{
    hf_client_msg_t* msg;
    if (!device)
        return;

    msg = HF_MSG_NEW(DISCONNECT, NULL);
    if (msg == NULL)
        return;

    hf_client_state_machine_handle_msg(device->sm, msg);
    hf_client_msg_destory(msg);
    hf_client_state_machine_destory(device->sm);
    list_delete(&device->node);
    free((void*)device);
}

static hf_state_machine_t* get_state_machine(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_device_t* device;

    if (!g_hfp_service.started)
        return NULL;

    device = find_hf_device_by_addr(bd_addr);
    if (device)
        return device->sm;

    sm = hf_client_state_machine_new(&g_hfp_service, bd_addr);
    if (!sm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    device = hf_client_device_new(sm, bd_addr);
    if (!device) {
        BT_LOGE("New device alloc failed");
        hf_client_state_machine_destory(sm);
        return NULL;
    }

    return sm;
}

static hf_client_connection_state_t trans_adp_connection_state(profile_connection_state state)
{
    switch (state) {
    case PROFILE_DISCONNECTED:
        return HF_CLIENT_CONNECTION_STATE_DISCONNECTED;
    case PROFILE_CONNECTING:
        return HF_CLIENT_CONNECTION_STATE_CONNECTING;
    case PROFILE_CONNECTED:
        BT_LOGD("PERFORMANCE-HF-BLUELET-CONNECTED");
        return HF_CLIENT_CONNECTION_STATE_CONNECTED;
    case PROFILE_DISCONNECTING:
        return HF_CLIENT_CONNECTION_STATE_DISCONNECTING;
    default:
        BT_LOGE("Unknow connection state: %d", state);
        break;
    }

    return HF_CLIENT_CONNECTION_STATE_DISCONNECTED;
}

static hf_client_audio_state_t trans_adp_audio_state(SERVICE_HFP_SCO_STATE state)
{
    switch (state) {
    case SERVICE_HFP_SCO_CONNECTED:
        return HF_CLIENT_AUDIO_STATE_CONNECTED;
    case SERVICE_HFP_SCO_DISCONNECTED:
        return HF_CLIENT_AUDIO_STATE_DISCONNECTED;
    case SERVICE_HFP_SCO_UNKNOWN:
        return HF_CLIENT_AUDIO_STATE_DISCONNECTED;
    default:
        BT_LOGE("Unknow audio state: %d", state);
        break;
    }

    return HF_CLIENT_AUDIO_STATE_DISCONNECTED;
}

static hf_client_callsetup_t trans_adp_callsetup_state(SERVICE_HFP_CALL_SETUP_STATE state)
{
    switch (state) {
    case HFP_CALL_SETUP_STATE_NONE:
        return HF_CLIENT_CALLSETUP_NONE;
    case HFP_CALL_SETUP_STATE_IN:
        return HF_CLIENT_CALLSETUP_INCOMING;
    case HFP_CALL_SETUP_STATE_OUT:
        return HF_CLIENT_CALLSETUP_OUTGOING;
    case HFP_CALL_SETUP_STATE_ALERT:
        return HF_CLIENT_CALLSETUP_ALERTING;
    default:
        BT_LOGE("Unknow call setup state: %d", state);
        break;
    }

    return HF_CLIENT_CALLSETUP_NONE;
}

static void adp_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CONNECTION_STATE_CHANGED, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = trans_adp_connection_state(state);

    hf_client_send_message(sm, msg);
}

static void adp_sco_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_SCO_STATE state, uint16_t sco_connection_handle)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_AUDIO_STATE_CHANGED, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = trans_adp_audio_state(state);
    msg->event_data.valueint2 = sco_connection_handle;

    hf_client_send_message(sm, msg);
}

static void adp_codec_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_CONFIG_S* config)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    BT_LOGD(" HF codec config [codec:%d][sample rate:%lu][bit width:%d]", config->codec,
        config->sample_rate, config->bit_width);

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CODEC_CHANGED, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = config->codec;

    hf_client_send_message(sm, msg);
}

static void adp_call_setup_state_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_CALL_SETUP_STATE state)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CALLSETUP, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = trans_adp_callsetup_state(state);

    hf_client_send_message(sm, msg);
}

static void adp_call_active_state_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_CALL_ACTIVE_STATE state)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CALL, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = (state == HFP_CALL_ACTIVE_STATE_NONE) ? HF_CLIENT_CALL_NO_CALLS_IN_PROGRESS : HF_CLIENT_CALL_CALLS_IN_PROGRESS;

    hf_client_send_message(sm, msg);
}

static void adp_call_held_state_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_CALL_HELD_STATE state)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CALLHELD, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = (state == HFP_CALL_HELD_STATE_NONE) ? HF_CLIENT_CALLHELD_NONE : HF_CLIENT_CALLHELD_HELD;

    hf_client_send_message(sm, msg);
}

static void adp_volume_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_VOLUME_TYPE type, uint8_t volume)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_VOLUME_CHANGED, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = (type == VOLUME_MIC) ? HF_CLIENT_VOLUME_TYPE_MIC : HF_CLIENT_VOLUME_TYPE_SPK;
    msg->event_data.valueint2 = volume;

    hf_client_send_message(sm, msg);
}

static void adp_ring_active_state_changed_cb(BD_ADDR remote_addr, bool active, bool inband_ring)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_RING_INDICATION, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = active ? 1 : 0;
    msg->event_data.valueint2 = inband_ring ? HF_CLIENT_IN_BAND_RINGTONE_PROVIDED : HF_CLIENT_IN_BAND_RINGTONE_NOT_PROVIDED;

    hf_client_send_message(sm, msg);
}

static void adp_voice_recognition_enabled_changed_cb(BD_ADDR remote_addr, bool enabled)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_VR_STATE_CHANGED, remote_addr);
    if (!msg)
        return;
    msg->event_data.valueint1 = enabled ? HF_CLIENT_VR_STATE_STARTED : HF_CLIENT_VR_STATE_STOPPED;

    hf_client_send_message(sm, msg);
}

static void adp_received_at_cmd_resp_cb(BD_ADDR remote_addr, char* response, uint16_t response_length)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CMD_RESPONSE, remote_addr);
    if (!msg)
        return;
    HF_MSG_ADD_STR(msg, 1, response, response_length);

    hf_client_send_message(sm, msg);
}

static void adp_received_sco_connection_req_cb(BD_ADDR remote_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_AUDIO_REQ, remote_addr);
    if (!msg)
        return;

    hf_client_send_message(sm, msg);
}

static void adp_clip_cb(BD_ADDR remote_addr, const char* number, const char* name)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CLIP, remote_addr);
    if (!msg)
        return;

    HF_MSG_ADD_STR(msg, 1, number, strlen(number));
    HF_MSG_ADD_STR(msg, 2, name, strlen(name));

    hf_client_send_message(sm, msg);
}

static void adp_current_call_callback(BD_ADDR remote_addr, uint32_t idx,
    SERVICE_HFP_CURRENT_CALL_DIR dir,
    SERVICE_HFP_CURRENT_CALL_STATUS status, SERVICE_HFP_CURRENT_CALL_MODE mode,
    SERVICE_HFP_CURRENT_CALL_MPTY mpty, const char* number, uint32_t type)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CURRENT_CALLS, remote_addr);
    if (!msg)
        return;

    msg->event_data.valueint1 = idx;
    msg->event_data.valueint2 = (hf_client_call_direction_t)dir;
    msg->event_data.valueint3 = (hf_client_call_state_t)status;
    msg->event_data.valueint4 = (hf_client_call_mpty_type_t)mpty;
    HF_MSG_ADD_STR(msg, 1, number, strlen(number));

    hf_client_send_message(sm, msg);
}

static void adp_at_command_result_callback(BD_ADDR remote_addr, uint32_t at_cmd_code, uint32_t result)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    switch (at_cmd_code) {
    case HFP_ATCC_ATD:
        break;
    default:
        return;
    }
    sm = get_state_machine(remote_addr);
    if (!sm)
        return;
    msg = HF_MSG_NEW(STACK_EVENT_CMD_RESULT, remote_addr);
    if (!msg)
        return;

    msg->event_data.valueint1 = at_cmd_code;
    msg->event_data.valueint2 = result;
    hf_client_send_message(sm, msg);
}

HFP_CALLBACKS_S hfp_adp_callbacks = {
    sizeof(HFP_CALLBACKS_S),
    adp_connection_state_changed_cb,
    adp_sco_connection_state_changed_cb,
    adp_codec_changed_cb,
    adp_call_setup_state_changed_cb,
    adp_call_active_state_changed_cb,
    adp_call_held_state_changed_cb,
    adp_volume_changed_cb,
    adp_ring_active_state_changed_cb,
    adp_voice_recognition_enabled_changed_cb,
    adp_received_at_cmd_resp_cb,
    adp_received_sco_connection_req_cb,
    adp_clip_cb,
    adp_current_call_callback,
    adp_at_command_result_callback
};

static void hf_client_send_message(hf_state_machine_t* sm, hf_client_msg_t* msg)
{
    hf_client_inter_msg_t* imsg = (hf_client_inter_msg_t*)malloc(sizeof(hf_client_inter_msg_t));

    imsg->hfsm = sm;
    imsg->msg = msg;
    bts_send_uv_msg(BT_PROFILE_HANDSFREE_HF_ID, imsg, sizeof(hf_client_inter_msg_t));
}

static void hf_client_cleanup(void)
{
    hf_client_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_hfp_service.device_list, node, tmp)
    {
        device = (hf_client_device_t*)node;
        hf_client_device_delete(device);
    }
    if (g_hfp_service.orb_fd > 0)
        orb_unadvertise(g_hfp_service.orb_fd);
    g_hfp_service.orb_fd = -1;
    service_adapter_hfp_cleanup();
    g_hfp_service.started = false;
}

static void hf_client_service_event_process(void* data, size_t size)
{
    hf_client_inter_msg_t* imsg = (hf_client_inter_msg_t*)data;
    hf_client_msg_t* msg = imsg->msg;

    switch (msg->event) {
    case CLEANUP:
        hf_client_cleanup();
        break;
    default:
        hf_client_state_machine_handle_msg(imsg->hfsm, msg);
        break;
    }

    hf_client_msg_destory(msg);
    free(imsg);
}

static void bts_hf_client_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    hf_client_service_event_process(data, size);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
bt_result_code bts_hf_client_init(const hf_client_service_callbacks_t* callbacks)
{
    SERVICE_BT_STATUS status;
    uint32_t features = BT_HFP_BRSF_HF_HFINDICATORS | BT_HFP_BRSF_HF_RMTVOLCTRL |
                        BT_HFP_BRSF_HF_ENHANCED_CALLSTATUS | BT_HFP_BRSF_HF_NREC |
                        BT_HFP_BRSF_HF_3WAYCALL | BT_HFP_BRSF_HF_CLIP |
                        BT_HFP_BRSF_HF_CODEC_NEGOTIATION | BT_HFP_BRSF_HF_BVRA;
                        //BT_HFP_BRSF_HF_BVRA;

    if (g_hfp_service.started)
        return BT_RESULT_SUCCESS;

    g_hfp_service.callbacks = (hf_client_callbacks_t*)callbacks;
    list_initialize(&g_hfp_service.device_list);
    bts_register_profile_process(BT_PROFILE_HANDSFREE_HF_ID,
        bts_hf_client_handle_service_msg);
    status = service_adapter_hfp_init(features, 1, &hfp_adp_callbacks);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        return BT_RESULT_FAILED;
    }
    g_hfp_service.orb_fd = orb_advertise(ORB_ID(hfp_state), NULL);
    if (g_hfp_service.orb_fd < 0) {
        BT_LOGE("g_hfp_service.orb_fd advertise failed");
        return BT_RESULT_FAILED;
    }

    g_hfp_service.started = true;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_connect(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(CONNECT, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_disconnect(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(DISCONNECT, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_connect_audio(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(CONNECT_AUDIO, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_disconnect_audio(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(DISCONNECT_AUDIO, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_start_voice_recognition(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(VOICE_RECOGNITION_START, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_stop_voice_recognition(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(VOICE_RECOGNITION_STOP, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_volume_control(bt_address bd_addr, hf_client_volume_type_t type, int volume)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    hf_client_event_t event = (type == HF_CLIENT_VOLUME_TYPE_MIC) ? SET_MIC_VOLUME : SET_SPEAKER_VOLUME;
    msg = HF_MSG_NEW(event, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    msg->event_data.valueint1 = volume;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_dial(bt_address bd_addr, const char* number)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(DIAL_NUMBER, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    HF_MSG_ADD_STR(msg, 1, number, strlen(number));
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_dial_memory(bt_address bd_addr, uint32_t memory)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(DIAL_MEMORY, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    msg->event_data.valueint1 = memory;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_redial(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(DIAL_LAST, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_accept_call(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(ACCEPT_CALL, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_reject_call(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(REJECT_CALL, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_hold_call(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(HOLD_CALL, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_terminate_call(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(TERMINATE_CALL, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_query_current_calls(bt_address bd_addr)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(QUERY_CURRENT_CALLS, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_send_at_cmd(bt_address bd_addr, const char* cmd)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(SEND_AT_COMMAND, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    HF_MSG_ADD_STR(msg, 1, cmd, strlen(cmd));
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_hf_client_update_battery_level(bt_address bd_addr, uint8_t battery)
{
    hf_state_machine_t* sm;
    hf_client_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = HF_MSG_NEW(UPDATE_BATTERY_LEVEL, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    msg->event_data.valueint1 = (uint32_t)battery;
    hf_client_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

void bts_hf_client_cleanup(void)
{
    hf_client_msg_t* msg;

    msg = HF_MSG_NEW(CLEANUP, NULL);
    if (!msg)
        return;

    hf_client_send_message(NULL, msg);
}
