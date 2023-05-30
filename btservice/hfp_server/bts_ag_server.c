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
#define LOG_TAG "hfp_ag"
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <sys/types.h>
#ifdef CONFIG_UORB
#include <connectivity/bt.h>
#include <uORB/uORB.h>
#endif
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "stack_adapter_hfp_ag.h"
#include "stack_adapter_service_base.h"

#include "btm_hfp_ag.h"
#include "btm_manager.h"
#include "bts_ag_server.h"
#include "bts_ag_server_event.h"
#include "bts_ag_server_state_machine.h"
#include "bts_service.h"

#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifndef CONFIG_AG_SERVER_MAX_CONNECTIONS
#define CONFIG_AG_SERVER_MAX_CONNECTIONS 1
#endif

/* * HFP HF supported features - bit mask */
#define HFP_BRSF_HF_NREC 0x00000001 /* * 0, EC and/or NR function */
#define HFP_BRSF_HF_3WAYCALL 0x00000002 /* * 1, Call waiting and 3-way calling */
#define HFP_BRSF_HF_CLIP 0x00000004 /* * 2, CLI presentation capability */
#define HFP_BRSF_HF_BVRA 0x00000008 /* * 3, Voice recognition activation */
#define HFP_BRSF_HF_RMTVOLCTRL 0x00000010 /* * 4, Remote volume control */
#define HFP_BRSF_HF_ENHANCED_CALLSTATUS 0x00000020 /* * 5, Enhanced call status */
#define HFP_BRSF_HF_ENHANCED_CALLCONTROL 0x00000040 /* * 6, Enhanced call control */
#define HFP_BRSF_HF_CODEC_NEGOTIATION 0x00000080 /* * 7, Codec negotiation */
#define HFP_BRSF_HF_HFINDICATORS 0x00000100 /* * 8, HF Indicators */
#define HFP_BRSF_HF_ESCO_S4T2_SETTING 0x00000200 /* * 9, eSCO S4 (and T2) settings supported */

/* * HFP AG supported features - bit mask */
#define HFP_BRSF_AG_3WAYCALL 0x00000001 /* * 0, Three-way calling */
#define HFP_BRSF_AG_NREC 0x00000002 /* * 1, EC and/or NR function */
#define HFP_BRSF_AG_BVRA 0x00000004 /* * 2, Voice recognition function */
#define HFP_BRSF_AG_INBANDRING 0x00000008 /* * 3, In-band ring tone capability */
#define HFP_BRSF_AG_BINP 0x00000010 /* * 4, Attach a number to a voice tag */
#define HFP_BRSF_AG_REJECT_CALL 0x00000020 /* * 5, Ability to reject a call */
#define HFP_BRSF_AG_ENHANCED_CALLSTATUS 0x00000040 /* * 6, Enhanced call status */
#define HFP_BRSF_AG_ENHANCED_CALLCONTROL 0x00000080 /* * 7, Enhanced call control */
#define HFP_BRSF_AG_EXTENDED_ERRORRESULT 0x00000100 /* * 8, Extended Error Result Codes */
#define HFP_BRSF_AG_CODEC_NEGOTIATION 0x00000200 /* * 9, Codec negotiation */
#define HFP_BRSF_AG_HFINDICATORS 0x00000400 /* * 10, HF Indicators */
#define HFP_BRSF_AG_eSCO_S4T2_SETTING 0x00000800 /* * 11, eSCO S4 (and T2) settings supported */

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct
{
    struct list_node node;
    void* handle;
    bt_address bd_addr;
    ag_state_machine_t* sm;
} ag_server_device_t;

typedef struct {
    ag_state_machine_t* hfsm;
    ag_server_msg_t* msg;
} ag_server_inter_msg_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/
ag_server_service_t g_hfp_ag_service = {
    .started = false,
    .orb_fd = -1,
    .device_list = LIST_INITIAL_VALUE(g_hfp_ag_service.device_list)
};

static uint32_t ag_support_features = HFP_BRSF_AG_HFINDICATORS | HFP_BRSF_AG_REJECT_CALL | HFP_BRSF_AG_ENHANCED_CALLSTATUS | HFP_BRSF_AG_ENHANCED_CALLCONTROL | HFP_BRSF_AG_EXTENDED_ERRORRESULT | HFP_BRSF_AG_CODEC_NEGOTIATION | HFP_BRSF_AG_eSCO_S4T2_SETTING;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static profile_connection_state_t bluelet_profile_connection_state(SERVICE_PROFILE_CONNECTION_STATE state)
{
    switch (state) {
    case SERVICE_PROFILE_DISCONNECTED:
        return PROFILE_STATE_DISCONNECTED;
    case SERVICE_PROFILE_CONNECTING:
        return PROFILE_STATE_CONNECTING;
    case SERVICE_PROFILE_CONNECTED:
        return PROFILE_STATE_CONNECTED;
    case SERVICE_PROFILE_DISCONNECTING:
        return PROFILE_STATE_DISCONNECTING;
    default:
        BT_LOGE("Unknow connection state: %d", state);
        return PROFILE_STATE_DISCONNECTED;
    }
}

static hfp_audio_state_t bluelet_hf_audio_state(SERVICE_HFP_SCO_STATE state)
{
    switch (state) {
    case SERVICE_HFP_SCO_CONNECTED:
        return HFP_AUDIO_STATE_CONNECTED;
    case SERVICE_HFP_SCO_DISCONNECTED:
        return HFP_AUDIO_STATE_DISCONNECTED;
    case SERVICE_HFP_SCO_UNKNOWN:
    default:
        BT_LOGE("Unknow audio state: %d", state);
        return HFP_AUDIO_STATE_DISCONNECTED;
    }
}

static ag_server_device_t* find_ag_device_by_addr(bt_address bd_addr)
{
    ag_server_device_t* device;
    struct list_node* node;

    list_for_every(&g_hfp_ag_service.device_list, node)
    {
        device = (ag_server_device_t*)node;
        if (memcmp(device->bd_addr, bd_addr, sizeof(bt_address)) == 0)
            return device;
    }

    return NULL;
}

static ag_server_device_t* ag_server_device_new(ag_state_machine_t* sm, bt_address bd_addr)
{
    ag_server_device_t* device;

    device = (ag_server_device_t*)malloc(sizeof(ag_server_device_t));
    if (!device)
        return NULL;

    memcpy(device->bd_addr, bd_addr, sizeof(bt_address));
    device->sm = sm;
    list_add_tail(&g_hfp_ag_service.device_list, &device->node);

    return device;
}

static void ag_server_device_delete(ag_server_device_t* device)
{
    ag_server_msg_t* msg;
    if (!device)
        return;

    msg = AG_MSG_NEW(DISCONNECT, NULL);
    if (msg == NULL)
        return;

    ag_server_state_machine_handle_msg(device->sm, msg);
    ag_server_msg_destory(msg);
    ag_server_state_machine_destory(device->sm);
    list_delete(&device->node);
    free((void*)device);
}

void ag_server_send_message(ag_state_machine_t* sm, ag_server_msg_t* msg)
{
    ag_server_inter_msg_t* imsg = (ag_server_inter_msg_t*)malloc(sizeof(ag_server_inter_msg_t));

    imsg->hfsm = sm;
    imsg->msg = msg;
    bts_send_uv_msg(BT_PROFILE_HANDSFREE_AG_ID, imsg, sizeof(ag_server_inter_msg_t));
}

static ag_state_machine_t* get_state_machine(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_device_t* device;

    if (!g_hfp_ag_service.started) {
        BT_LOGE("get_state_machine failed");
        return NULL;
    }
    device = find_ag_device_by_addr(bd_addr);
    if (device)
        return device->sm;

    sm = ag_server_state_machine_new(&g_hfp_ag_service, bd_addr);
    if (!sm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    device = ag_server_device_new(sm, bd_addr);
    if (!device) {
        BT_LOGE("New device alloc failed");
        ag_server_state_machine_destory(sm);
        return NULL;
    }

    return sm;
}

static void ag_server_service_event_process(void* data, size_t size)
{
    ag_server_inter_msg_t* imsg = (ag_server_inter_msg_t*)data;
    ag_server_msg_t* msg = imsg->msg;

    switch (msg->event) {
    /*//active all device
    case DEVICE_STATUS_CHANGED:
    case PHONE_STATE_CHANGE:
    case SET_VOLUME:
    case SET_INBAND_RING_ENABLE:
    case DIALING_RESULT:
        bt_list_foreach(g_hfp_ag_service.ag_devices, ag_dispatch_msg_foreach, msg);
        break;*/
    case CLEANUP:
        ag_server_cleanup();
        break;
    default:
        ag_server_state_machine_handle_msg(imsg->hfsm, msg);
        break;
    }

    ag_server_msg_destory(msg);
    free(imsg);
}

/****************************************************************************
 * Adp Functions
 ****************************************************************************/

void ag_server_connection_state_changed(bt_address bd_addr, profile_connection_state_t state)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_CONNECTION_STATE_CHANGED, bd_addr);
    if (!msg)
        return;
    msg->data.valueint1 = state;
    ag_server_send_message(sm, msg);
}

void ag_server_audio_state_changed(bt_address bd_addr, hfp_audio_state_t state)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_AUDIO_STATE_CHANGED, bd_addr);
    if (!msg)
        return;
    msg->data.valueint1 = state;
    ag_server_send_message(sm, msg);
}

void ag_server_codec_changed(bt_address bd_addr, hfp_codec_config_t* config)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_CODEC_CHANGED, bd_addr);
    if (!msg)
        return;
    msg->data.valueint1 = config->codec;
    ag_server_send_message(sm, msg);
}

void ag_server_volume_changed(bt_address bd_addr,
    ag_server_volume_type_t type,
    uint8_t volume)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_VOLUME_CHANGED, bd_addr);
    if (!msg)
        return;
    msg->data.valueint1 = type;
    msg->data.valueint2 = volume;

    ag_server_send_message(sm, msg);
}

void ag_server_received_cind_request(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_AT_CIND_REQUEST, bd_addr);
    if (!msg)
        return;

    ag_server_send_message(sm, msg);
}

void ag_server_received_clcc_request(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_AT_CLCC_REQUEST, bd_addr);
    if (!msg)
        return;

    ag_server_send_message(sm, msg);
}

void ag_server_received_cops_request(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_AT_COPS_REQUEST, bd_addr);
    if (!msg)
        return;

    ag_server_send_message(sm, msg);
}

void ag_server_voice_recognition_state_changed(bt_address bd_addr, bool started)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_VR_STATE_CHANGED, bd_addr);
    if (!msg)
        return;
    msg->data.valueint1 = started;
    ag_server_send_message(sm, msg);
}

void ag_server_remote_battery_level_update(bt_address bd_addr, uint8_t value)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_BATTERY_UPDATE, bd_addr);
    if (!msg)
        return;
    msg->data.valueint1 = value;
    ag_server_send_message(sm, msg);
}

void ag_server_answer_call(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_ANSWER_CALL, bd_addr);
    if (!msg)
        return;

    ag_server_send_message(sm, msg);
}

void ag_server_reject_call(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_REJECT_CALL, bd_addr);
    if (!msg)
        return;

    ag_server_send_message(sm, msg);
}

void ag_server_hangup_call(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_HANGUP_CALL, bd_addr);
    if (!msg)
        return;

    ag_server_send_message(sm, msg);
}

void ag_server_received_at_cmd(bt_address bd_addr, char* at_string, uint16_t at_length)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_AT_COMMAND, bd_addr);
    if (!msg)
        return;
    AG_MSG_ADD_STR(msg, 1, at_string, at_length);

    ag_server_send_message(sm, msg);
}

void ag_server_audio_connect_request(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_AUDIO_REQ, bd_addr);
    if (!msg)
        return;

    ag_server_send_message(sm, msg);
}

void ag_server_dial_number(bt_address bd_addr, char* number, uint32_t length)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_DIAL_NUMBER, bd_addr);
    if (!msg)
        return;
    AG_MSG_ADD_STR(msg, 1, number, length);

    ag_server_send_message(sm, msg);
}

void ag_server_dial_memory(bt_address bd_addr, uint32_t location)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_DIAL_MEMORY, bd_addr);
    if (!msg)
        return;
    msg->data.valueint1 = location;

    ag_server_send_message(sm, msg);
}

void ag_server_call_control(bt_address bd_addr, ag_server_call_control_t control)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return;
    msg = AG_MSG_NEW(STACK_EVENT_CALL_CONTROL, bd_addr);
    if (!msg)
        return;
    msg->data.valueint1 = control;

    ag_server_send_message(sm, msg);
}

void ag_server_received_dtmf(bt_address bd_addr, char tone)
{
}

void ag_server_received_manufacture_request(bt_address bd_addr)
{
    service_adapter_hfp_ag_manufacture_id_response(bd_addr, "xiaomi", strlen("xiaomi"));
}

void ag_server_received_model_id_request(bt_address bd_addr)
{
    service_adapter_hfp_ag_model_id_response(bd_addr, "2109119BC", strlen("2109119BC"));
}

/****************************************************************************
 * Adp Functions callback
 ****************************************************************************/

static void connection_state_changed_callback(BD_ADDR remote_addr,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    ag_server_connection_state_changed(remote_addr, bluelet_profile_connection_state(state));
}

static void sco_connection_state_changed_callback(BD_ADDR remote_addr,
    SERVICE_HFP_SCO_STATE state)
{
    ag_server_audio_state_changed(remote_addr, bluelet_hf_audio_state(state));
}

static void codec_changed_callback(BD_ADDR remote_addr, SERVICE_HFP_CONFIG_S* config)
{
    hfp_codec_config_t codec = { .codec = config->codec };

    ag_server_codec_changed(remote_addr, &codec);
}

static void volume_changed_callback(BD_ADDR remote_addr,
    SERVICE_HFP_VOLUME_TYPE type,
    uint8_t volume)
{
    ag_server_volume_changed(remote_addr,
        (type == VOLUME_MIC) ? AG_SERVER_VOLUME_TYPE_MIC : AG_SERVER_VOLUME_TYPE_SPK,
        volume);
}

static void received_cind_request_callback(BD_ADDR remote_addr)
{
    ag_server_received_cind_request(remote_addr);
}

static void received_clcc_request_callback(BD_ADDR remote_addr)
{
    ag_server_received_clcc_request(remote_addr);
}

static void received_cops_request_callback(BD_ADDR remote_addr)
{
    ag_server_received_cops_request(remote_addr);
}

static void voice_recognition_enabled_changed_callback(BD_ADDR remote_addr,
    bool enabled)
{
    ag_server_voice_recognition_state_changed(remote_addr, enabled);
}

static void received_remote_battery_level_callback(BD_ADDR remote_addr, uint8_t value)
{
    ag_server_remote_battery_level_update(remote_addr, value);
}

static void received_answer_call_callback(BD_ADDR remote_addr)
{
    ag_server_answer_call(remote_addr);
}

static void received_reject_call_callback(BD_ADDR remote_addr)
{
    ag_server_reject_call(remote_addr);
}

static void received_hangup_call_callback(BD_ADDR remote_addr)
{
    ag_server_hangup_call(remote_addr);
}

void received_dial_number_callback(BD_ADDR remote_addr, char* number, uint32_t len)
{
    ag_server_dial_number(remote_addr, number, len);
}

void received_chld_request_callback(BD_ADDR remote_addr,
    SERVICE_HFP_CALL_CONTROL_CODE ctrl_code, uint8_t idx)
{
    ag_server_call_control(remote_addr, ctrl_code);
}

static void received_at_cmd_callback(BD_ADDR remote_addr, char* at_string,
    uint16_t at_length)
{
    BT_LOGD("received_at_cmd_callback, %s", at_string);
    ag_server_received_at_cmd(remote_addr, at_string, at_length);
}

static void received_sco_connection_req_callback(BD_ADDR remote_addr)
{
    ag_server_audio_connect_request(remote_addr);
}

void received_dtmf_cmd_callback(BD_ADDR remote_addr, char tone)
{
    ag_server_received_dtmf(remote_addr, tone);
}

void received_manufacture_request_callback(BD_ADDR remote_addr)
{
    ag_server_received_manufacture_request(remote_addr);
}

void received_model_request_callback(BD_ADDR remote_addr)
{
    ag_server_received_model_id_request(remote_addr);
}

static HFP_AG_CALLBACKS_S hfp_ag_callbacks = {
    .size = sizeof(hfp_ag_callbacks),
    .hfp_ag_connection_state_changed_cb = connection_state_changed_callback,
    .hfp_ag_sco_connection_state_changed_cb = sco_connection_state_changed_callback,
    .hfp_ag_codec_changed_cb = codec_changed_callback,
    .hfp_ag_volume_changed_cb = volume_changed_callback,
    .hfp_ag_received_cind_request_cb = received_cind_request_callback,
    .hfp_ag_received_clcc_request_cb = received_clcc_request_callback,
    .hfp_ag_received_cops_request_cb = received_cops_request_callback,
    .hfp_ag_voice_recognition_enabled_changed_cb = voice_recognition_enabled_changed_callback,
    .hfp_ag_received_remote_battery_level_cb = received_remote_battery_level_callback,
    .hfp_ag_received_answer_call_cb = received_answer_call_callback,
    .hfp_ag_received_reject_call_cb = received_reject_call_callback,
    .hfp_ag_received_hangup_call_cb = received_hangup_call_callback,
    .hfp_ag_received_dial_number_cb = received_dial_number_callback,
    .hfp_ag_received_chld_request_cb = received_chld_request_callback,
    .hfp_ag_received_at_cmd_cb = received_at_cmd_callback,
    .hfp_ag_received_sco_connection_req_cb = received_sco_connection_req_callback,
    .hfp_ag_received_dtmf_cmd_cb = received_dtmf_cmd_callback,
    .hfp_ag_received_manufacture_request_cb = received_manufacture_request_callback,
    .hfp_ag_received_model_request_cb = received_model_request_callback,
    /* bind callback */
    /* biev callback */
};

void ag_server_cleanup(void)
{
    ag_server_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_hfp_ag_service.device_list, node, tmp)
    {
        device = (ag_server_device_t*)node;
        ag_server_device_delete(device);
    }
#ifdef CONFIG_UORB
    if (g_hfp_ag_service.orb_fd > 0)
        orb_unadvertise(g_hfp_ag_service.orb_fd);
#endif
    g_hfp_ag_service.orb_fd = -1;
    service_adapter_hfp_ag_cleanup();
    g_hfp_ag_service.started = false;
}

static uint32_t get_ag_features(void)
{
#ifdef CONFIG_KVDB
    return property_get_int32("persist.bluetooth.hfp.ag_features", ag_support_features);
#else
    return ag_support_features;
#endif
}

void bts_ag_server_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    ag_server_service_event_process(data, size);
}
/****************************************************************************
 * Public Functions
 ****************************************************************************/

bt_result_code bts_ag_server_init(const ag_server_service_callbacks_t* callbacks)
{
    SERVICE_BT_STATUS status;

    if (g_hfp_ag_service.started)
        return BT_RESULT_SUCCESS;

    g_hfp_ag_service.callbacks = (ag_server_callbacks_t*)callbacks;
    list_initialize(&g_hfp_ag_service.device_list);
    bts_register_profile_process(BT_PROFILE_HANDSFREE_AG_ID,
        bts_ag_server_handle_service_msg);
    status = service_adapter_hfp_ag_init(get_ag_features(), 1, &hfp_ag_callbacks);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        return BT_RESULT_FAILED;
    }
#ifndef CONFIG_ARCH_SIM
#ifdef CONFIG_UORB
    g_hfp_ag_service.orb_fd = orb_advertise_queue(ORB_ID(hfp_state),
        NULL, CONFIG_BLUETOOTH_ORB_QUEUE_SIZE);
    if (g_hfp_ag_service.orb_fd < 0) {
        BT_LOGE("g_hfp_ag_service.orb_fd advertise failed");
        return BT_RESULT_FAILED;
    }
#endif
#endif

    g_hfp_ag_service.started = true;

    return BT_RESULT_SUCCESS;
}

bool bts_ag_server_is_connected(bt_address bd_addr)
{
    ag_server_device_t* device = find_ag_device_by_addr(bd_addr);

    if (!device) {
        return false;
    }

    bool connected = ag_server_state_machine_get_state(device->sm) >= AG_SERVER_STATE_CONNECTED;

    return connected;
}

bool bts_ag_server_is_audio_connected(bt_address bd_addr)
{
    ag_server_device_t* device = find_ag_device_by_addr(bd_addr);

    if (!device) {
        return false;
    }

    bool connected = ag_server_state_machine_get_state(device->sm) == AG_SERVER_STATE_AUDIO_CONNECTED;
    return connected;
}

ag_server_state_t bts_ag_server_get_connection_state(bt_address bd_addr)
{
    ag_server_device_t* device = find_ag_device_by_addr(bd_addr);
    ag_server_state_t conn_state;
    uint32_t state;

    if (!device)
        return AG_SERVER_STATE_DISCONNECTING;

    state = ag_server_state_machine_get_state(device->sm);
    if (state == AG_SERVER_STATE_DISCONNECTED)
        conn_state = PROFILE_STATE_DISCONNECTED;
    else if (state == AG_SERVER_STATE_CONNECTING)
        conn_state = PROFILE_STATE_CONNECTING;
    else if (state == AG_SERVER_STATE_DISCONNECTING)
        conn_state = PROFILE_STATE_DISCONNECTING;
    else
        conn_state = PROFILE_STATE_CONNECTED;

    return conn_state;
}

bt_result_code bts_ag_server_connect(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(CONNECT, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    ag_server_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_disconnect(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(DISCONNECT, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    ag_server_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_connect_audio(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(CONNECT_AUDIO, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    ag_server_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_disconnect_audio(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(DISCONNECT_AUDIO, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    ag_server_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_start_voice_recognition(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(VOICE_RECOGNITION_START, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    ag_server_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_stop_voice_recognition(bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(VOICE_RECOGNITION_STOP, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    ag_server_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_phone_state_change(bt_address bd_addr,
    uint8_t num_active,
    uint8_t num_held,
    ag_server_call_state_t call_state,
    ag_server_call_addrtype_t type, const char* number,
    const char* name)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(PHONE_STATE_CHANGE, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    msg->data.valueint1 = num_active;
    msg->data.valueint2 = num_held;
    msg->data.valueint3 = call_state;
    msg->data.valueint4 = type;
    AG_MSG_ADD_STR(msg, 1, number, strlen(number));
    AG_MSG_ADD_STR(msg, 2, name, strlen(name));
    ag_server_send_message(sm, msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_device_status_changed(
    bt_address bd_addr,
    hfp_network_state_t network,
    hfp_roaming_state_t roam,
    uint8_t signal, uint8_t battery)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(DEVICE_STATUS_CHANGED, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    msg->data.valueint1 = network;
    msg->data.valueint2 = roam;
    msg->data.valueint3 = signal;
    msg->data.valueint4 = battery;

    ag_server_send_message(sm, msg);
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_set_inband_ring_enable(
    bt_address bd_addr)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(SET_INBAND_RING_ENABLE, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    ag_server_send_message(sm, msg);
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_send_at_command(
    bt_address bd_addr,
    char* at_command)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(SEND_AT_COMMAND, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    AG_MSG_ADD_STR(msg, 1, at_command, strlen(at_command));

    ag_server_send_message(sm, msg);
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_dial_result(bt_address bd_addr, uint8_t result)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(DIALING_RESULT, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    msg->data.valueint1 = result;

    ag_server_send_message(sm, msg);
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_cind_response(bt_address bd_addr,
    hfp_network_state_t service,
    uint8_t signal,
    hfp_roaming_state_t roam,
    uint8_t battery,
    hfp_call_t call,
    hfp_callsetup_t call_setup,
    hfp_callheld_t call_held)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(CIND_RESP, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    msg->data.valueint1 = service;
    msg->data.valueint2 = signal;
    msg->data.valueint3 = roam;
    msg->data.valueint4 = battery;
    msg->data.valueint5 = call;
    msg->data.valueint6 = call_setup;
    msg->data.valueint7 = call_held;

    ag_server_send_message(sm, msg);
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_clcc_response(bt_address bd_addr,
    uint32_t index,
    uint8_t dir,
    ag_server_call_state_t status,
    uint8_t mode,
    uint8_t mpty,
    const char* number)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(CLCC_RESP, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    msg->data.valueint1 = index;
    msg->data.valueint2 = dir;
    msg->data.valueint3 = status;
    msg->data.valueint4 = mode;
    msg->data.valueint5 = mpty;

    AG_MSG_ADD_STR(msg, 1, number, strlen(number));

    ag_server_send_message(sm, msg);
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_ag_server_cops_response(bt_address bd_addr,
    char* operator_name, uint16_t length)
{
    ag_state_machine_t* sm;
    ag_server_msg_t* msg;

    sm = get_state_machine(bd_addr);
    if (!sm)
        return BT_RESULT_FAILED;

    msg = AG_MSG_NEW(COPS_RESP, bd_addr);
    if (!msg)
        return BT_RESULT_ALLOC_BUFFER_FAILED;

    AG_MSG_ADD_STR(msg, 1, operator_name, length);

    ag_server_send_message(sm, msg);
    return BT_RESULT_SUCCESS;
}

void bts_ag_server_cleanup(void)
{
    ag_server_msg_t* msg;

    msg = AG_MSG_NEW(CLEANUP, NULL);
    if (!msg)
        return;

    ag_server_send_message(NULL, msg);
}
