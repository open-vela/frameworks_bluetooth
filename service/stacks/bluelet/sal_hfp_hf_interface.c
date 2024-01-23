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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stack_adapter_common.h"
#include "stack_adapter_hfp.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_hfp_hf_interface.h"

#ifdef CONFIG_BLUETOOTH_HFP_HF
static uint32_t hf_at_command_code_map(uint32_t atcc)
{
    switch (atcc) {
    case HFP_ATCC_ATD:
        return HFP_ATCMD_CODE_ATD;
    default:
        return HFP_ATCMD_CODE_UNKNOWN;
    }
}
static void connection_state_changed_callback(BD_ADDR remote_addr,
                                              SERVICE_PROFILE_CONNECTION_STATE state,
                                              uint32_t remote_features)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_connection_state_changed(&addr, bluelet_profile_connection_state(state),
                                       remote_features);
}

static void sco_connection_state_changed_callback(BD_ADDR remote_addr,
                                                  SERVICE_HFP_SCO_STATE state,
                                                  uint16_t sco_connection_handle)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_audio_connection_state_changed(&addr, bluelet_hf_audio_state(state), sco_connection_handle);
}

static void codec_changed_callback(BD_ADDR remote_addr, SERVICE_HFP_CONFIG_S *config)
{
    bt_address_t addr = { 0 };
    hfp_codec_config_t codec = { .codec = config->codec };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_codec_changed(&addr, &codec);
}

static void call_setup_state_changed_callback(BD_ADDR remote_addr,
                                              SERVICE_HFP_CALL_SETUP_STATE state)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_call_setup_state_changed(&addr, state);
}

static void call_active_state_changed_callback(BD_ADDR remote_addr,
                                               SERVICE_HFP_CALL_ACTIVE_STATE state)
{
    bt_address_t addr = { 0 };
    hfp_call_t call;

    memcpy(addr.addr, remote_addr, 6);
    call = (state == HFP_CALL_ACTIVE_STATE_NONE) ? HFP_CALL_NO_CALLS_IN_PROGRESS : HFP_CALL_CALLS_IN_PROGRESS;
    hfp_hf_on_call_active_state_changed(&addr, call);
}

static void call_held_state_changed_callback(BD_ADDR remote_addr,
                                             SERVICE_HFP_CALL_HELD_STATE state)
{
    bt_address_t addr = { 0 };
    hfp_callheld_t held;

    memcpy(addr.addr, remote_addr, 6);
    held = (state == HFP_CALL_HELD_STATE_NONE) ? HFP_CALLHELD_NONE : HFP_CALLHELD_HELD;
    hfp_hf_on_call_held_state_changed(&addr, held);
}

static void volume_changed_callback(BD_ADDR remote_addr, SERVICE_HFP_VOLUME_TYPE type,
                                    uint8_t volume)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_volume_changed(&addr, (type == VOLUME_MIC) ? HFP_VOLUME_TYPE_MIC : HFP_VOLUME_TYPE_SPK,
                             volume);
}

static void ring_active_state_changed_callback(BD_ADDR remote_addr, bool active,
                                               bool inband_ring)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_ring_active_state_changed(&addr, active, inband_ring ? HFP_IN_BAND_RINGTONE_NOT_PROVIDED : HFP_IN_BAND_RINGTONE_PROVIDED);
}

static void voice_recognition_enabled_changed_callback(BD_ADDR remote_addr, bool enabled)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_voice_recognition_state_changed(&addr, enabled);
}

static void received_at_cmd_resp_callback(BD_ADDR remote_addr, char *response,
                                          uint16_t response_length)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_received_at_cmd_resp(&addr, response, response_length);
}

static void received_sco_connection_req_callback(BD_ADDR remote_addr)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_received_sco_connection_req(&addr);
}

static void clip_callback(BD_ADDR remote_addr, const char *number, const char *name)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_clip(&addr, number, name);
}

static void current_call_callback(BD_ADDR remote_addr, uint32_t idx,
                                  SERVICE_HFP_CURRENT_CALL_DIR dir,
                                  SERVICE_HFP_CURRENT_CALL_STATUS status, SERVICE_HFP_CURRENT_CALL_MODE mode,
                                  SERVICE_HFP_CURRENT_CALL_MPTY mpty, const char *number, uint32_t type)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_current_call_response(&addr, idx, dir, status, mpty, number, type);
}

static void at_command_result_callback(BD_ADDR remote_addr, uint32_t at_cmd_code, uint32_t result)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, remote_addr, 6);
    hfp_hf_on_at_command_result_response(&addr, hf_at_command_code_map(at_cmd_code), result);
}

static const HFP_CALLBACKS_S hf_callbacks = {
    .size = sizeof(hf_callbacks),
    .hfp_connection_state_changed_cb = connection_state_changed_callback,
    .hfp_sco_connection_state_changed_cb = sco_connection_state_changed_callback,
    .hfp_codec_changed_cb = codec_changed_callback,
    .hfp_call_setup_state_changed_cb = call_setup_state_changed_callback,
    .hfp_call_active_state_changed_cb = call_active_state_changed_callback,
    .hfp_call_held_state_changed_cb = call_held_state_changed_callback,
    .hfp_volume_changed_cb = volume_changed_callback,
    .hfp_ring_active_state_changed_cb = ring_active_state_changed_callback,
    .hfp_voice_recognition_enabled_changed_cb = voice_recognition_enabled_changed_callback,
    .hfp_received_at_cmd_cb = received_at_cmd_resp_callback,
    .hfp_received_sco_connection_req_cb = received_sco_connection_req_callback,
    .hfp_clip_cb = clip_callback,
    .hfp_current_call_cb = current_call_callback,
    .hfp_at_command_result_cb = at_command_result_callback,
};

bt_status_t bt_sal_hfp_hf_init(uint32_t hf_features, uint8_t max_connection)
{
    SAL_CHECK_RET(service_adapter_hfp_init(hf_features, max_connection,
                                           (HFP_CALLBACKS_S *)&hf_callbacks),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_hf_cleanup(void)
{
    service_adapter_hfp_cleanup();
}

bt_status_t bt_sal_hfp_hf_connect(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_connect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_disconnect(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_disconnect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_connect_audio(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_create_sco(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_disconnect_audio(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_disconnect_sco(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_answer_call(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_answer_call(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_reject_call(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_reject_call(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_hold_call(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_hold_call(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_hangup_call(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_hangup_call(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_number(bt_address_t *addr, const char *number)
{
    SAL_CHECK_PARAM(addr);

    /*dial last number when number is null */
    SAL_CHECK_RET(service_adapter_hfp_dial_number(addr->addr, number), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_memory(bt_address_t *addr, uint32_t memory)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_dial_memory(addr->addr, memory), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_call_control(bt_address_t *addr, hfp_call_control_t chld, uint32_t index)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_call_control(addr->addr, chld, index), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_get_current_calls(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_get_current_calls(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_set_volume(bt_address_t *addr, hfp_volume_type_t type, uint8_t volume)
{
    SAL_CHECK_PARAM(addr);
    SERVICE_HFP_VOLUME_TYPE vtype;

    vtype = (type == HFP_VOLUME_TYPE_SPK) ? VOLUME_SPEAKER : VOLUME_MIC;
    SAL_CHECK_RET(service_adapter_hfp_set_volume(addr->addr, vtype, volume), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_start_voice_recognition(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_enable_voice_recognition(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_stop_voice_recognition(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_disable_voice_recognition(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_battery_level(bt_address_t *addr, uint8_t value)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_send_battery_value(addr->addr, value), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

static void at_cmd_resp_callback(BD_ADDR remote_addr, char *response, uint16_t response_length)
{
    received_at_cmd_resp_callback(remote_addr, response, response_length);
}

bt_status_t bt_sal_hfp_hf_send_at_cmd(bt_address_t *addr, const char *cmd, uint16_t len)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(cmd);
    SERVICE_HFP_AT_CMD_S at_cmd = { (char *)cmd, len };

    SAL_CHECK_RET(service_adapter_hfp_send_at_cmd(addr->addr, &at_cmd, at_cmd_resp_callback),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_dtmf(bt_address_t *addr, char dtmf)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_tx_dtmf(addr->addr, dtmf), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif
