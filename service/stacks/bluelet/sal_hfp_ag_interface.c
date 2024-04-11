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
#include "stack_adapter_hfp_ag.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_hfp_ag_interface.h"

#ifdef CONFIG_BLUETOOTH_HFP_AG
static void connection_state_changed_callback(BD_ADDR remote_addr,
                                              SERVICE_PROFILE_CONNECTION_STATE state,
                                              uint32_t remote_features)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_connection_state_changed(&addr, bluelet_profile_connection_state(state),
                                       remote_features);
}

static void sco_connection_state_changed_callback(BD_ADDR remote_addr,
                                                  SERVICE_HFP_SCO_STATE state,
                                                  uint16_t sco_connection_handle)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_audio_state_changed(&addr, bluelet_hf_audio_state(state), sco_connection_handle);
}

static void codec_changed_callback(BD_ADDR remote_addr, SERVICE_HFP_CONFIG_S *config)
{
    bt_address_t addr;
    hfp_codec_config_t codec = { .codec = config->codec };

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_codec_changed(&addr, &codec);
}

static void volume_changed_callback(BD_ADDR remote_addr, SERVICE_HFP_VOLUME_TYPE type,
                                    uint8_t volume)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);

    hfp_ag_on_volume_changed(&addr, (type == VOLUME_MIC) ? HFP_VOLUME_TYPE_MIC : HFP_VOLUME_TYPE_SPK,
                             volume);
}

static void received_cind_request_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_received_cind_request(&addr);
}

static void received_clcc_request_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_received_clcc_request(&addr);
}

static void received_cops_request_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_received_cops_request(&addr);
}

static void voice_recognition_enabled_changed_callback(BD_ADDR remote_addr,
                                                       bool enabled)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_voice_recognition_state_changed(&addr, enabled);
}

static void received_remote_battery_level_callback(BD_ADDR remote_addr, uint8_t value)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_remote_battery_level_update(&addr, value);
}

static void received_answer_call_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_answer_call(&addr);
}

static void received_reject_call_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_reject_call(&addr);
}

static void received_hangup_call_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_hangup_call(&addr);
}

void received_dial_number_callback(BD_ADDR remote_addr, char *number, uint32_t len)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_dial_number(&addr, number, len);
}

void received_chld_request_callback(BD_ADDR remote_addr,
                                    SERVICE_HFP_CALL_CONTROL_CODE ctrl_code, uint8_t idx)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_call_control(&addr, ctrl_code);
}

static void received_at_cmd_callback(BD_ADDR remote_addr, char *at_string,
                                     uint16_t at_length)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_received_at_cmd(&addr, at_string, at_length);
}

static void received_sco_connection_req_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_audio_connect_request(&addr);
}

void received_dtmf_cmd_callback(BD_ADDR remote_addr, char tone)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_received_dtmf(&addr, tone);
}

void received_manufacture_request_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_received_manufacture_request(&addr);
}

void received_model_request_callback(BD_ADDR remote_addr)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    hfp_ag_on_received_model_id_request(&addr);
}

static HFP_AG_CALLBACKS_S ag_callbacks = {
    .size = sizeof(ag_callbacks),
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

bt_status_t bt_sal_hfp_ag_init(uint32_t features, uint8_t max_connection)
{
    SAL_CHECK_RET(service_adapter_hfp_ag_init(features, max_connection, &ag_callbacks),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_ag_cleanup(void)
{
    service_adapter_hfp_ag_cleanup();
}

bt_status_t bt_sal_hfp_ag_connect(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_ag_connect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_disconnect(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_ag_disconnect(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_connect_audio(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_ag_create_sco(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_disconnect_audio(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_ag_disconnect_sco(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_start_voice_recognition(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_ag_enable_voice_recognition(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_stop_voice_recognition(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_hfp_ag_disable_voice_recognition(addr->addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_phone_state_change(bt_address_t *addr, uint8_t num_active,
                                             uint8_t num_held, hfp_ag_call_state_t call_state,
                                             hfp_call_addrtype_t type, const char *number,
                                             const char *name)
{
    SAL_CHECK_PARAM(addr);
    SERVICE_HFP_AG_PHONE_NUMBER_S *phone_number = NULL;
    if (number) {
        uint8_t num_len = strlen(number);
        phone_number = malloc(sizeof(SERVICE_HFP_AG_PHONE_NUMBER_S) + num_len);

        phone_number->type = type;
        phone_number->number_length = num_len;
        memcpy(phone_number->number, number, num_len);
    }

    SAL_CHECK_RET(service_adapter_hfp_ag_phone_state_change(addr->addr, num_active, num_held,
                                                            call_state, phone_number),
                  SERVICE_BT_STATUS_SUCCESS);
    free(phone_number);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_cind_response(bt_address_t *addr, hfp_ag_cind_resopnse_t *response)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(response);
    SERVICE_HFP_AG_CIND_RESPONSE_S resp;

    resp.device_status.service = response->network;
    resp.device_status.signal = response->signal;
    resp.device_status.roam = response->roam;
    resp.device_status.battery = response->battery;
    resp.call = response->call;
    resp.call_setup = response->call_setup;
    resp.call_held = response->call_held;
    SAL_CHECK_RET(service_adapter_hfp_ag_cind_response(addr->addr, resp), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_clcc_response(bt_address_t *addr, uint32_t index,
                                        hfp_call_direction_t dir, hfp_ag_call_state_t call,
                                        hfp_call_mode_t mode, hfp_call_mpty_type_t mpty,
                                        hfp_call_addrtype_t type, const char *number)
{
    SAL_CHECK_PARAM(addr);
    uint8_t num_len = 0;
    int resp_len = 0;
    SERVICE_HFP_AG_CLCC_RESPONSE_S *resp = NULL;

    if (index > 0) {
        if (number)
            num_len = strlen(number) + 1;

        resp_len = sizeof(SERVICE_HFP_AG_CLCC_RESPONSE_S) + num_len;
        resp = malloc(resp_len);
        if (!resp)
            return BT_STATUS_NOMEM;

        memset(resp, 0, resp_len);
        resp->index = index;
        resp->dir = dir;
        resp->status = call;
        resp->mode = mode;
        resp->mpty = mpty;
        resp->number.type = type;
        resp->number.number_length = num_len;
        if (num_len) {
            memcpy(resp->number.number, number, num_len);
            resp->number.number[num_len - 1] = '\0';
        }
    }

    SAL_CHECK_RET(service_adapter_hfp_ag_clcc_response(addr->addr, resp), SERVICE_BT_STATUS_SUCCESS);
    free(resp);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_dial_response(bt_address_t *addr, hfp_atcmd_result_t result)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_hfp_ag_dial_response(addr->addr, result), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_cops_response(bt_address_t *addr, const char *operator_name, uint16_t length)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(operator_name);

    SAL_CHECK_RET(service_adapter_hfp_ag_cops_response(addr->addr, (char *)operator_name, length),
                  SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_notify_device_status_changed(bt_address_t *addr,
                                                       hfp_network_state_t network,
                                                       hfp_roaming_state_t roam,
                                                       uint8_t signal, uint8_t battery)
{
    SAL_CHECK_PARAM(addr);
    SERVICE_HFP_AG_DEVICE_STATUS_S status;

    status.service = network;
    status.signal = signal;
    status.roam = roam;
    status.battery = battery;
    SAL_CHECK_RET(service_adapter_hfp_ag_notify_device_status_changed(addr->addr, status),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_set_inband_ring_enable(bt_address_t *addr, bool enable)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_hfp_ag_set_inband_ring_enable(addr->addr, enable),
                  SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_set_volume(bt_address_t *addr, hfp_volume_type_t type, uint8_t volume)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(volume >= 0 && volume < 16);
    SERVICE_HFP_VOLUME_TYPE vtype;

    vtype = (type == HFP_VOLUME_TYPE_SPK) ? VOLUME_SPEAKER : VOLUME_MIC;
    SAL_CHECK_RET(service_adapter_hfp_ag_set_volume(addr->addr, vtype, volume),
                  SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_send_at_cmd(bt_address_t *addr, const char *atcmd, uint16_t length)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(atcmd);
    SERVICE_HFP_AG_AT_CMD_S cmd;

    cmd.at_string = (char *)atcmd;
    cmd.at_length = length;
    SAL_CHECK_RET(service_adapter_hfp_ag_send_at_cmd(addr->addr, &cmd), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_manufacture_id_response(bt_address_t *addr,
                                                  const char *manufacturer_id,
                                                  uint16_t length)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(manufacturer_id);

    SAL_CHECK_RET(service_adapter_hfp_ag_manufacture_id_response(addr->addr, (char *)manufacturer_id, length),
                  SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_model_id_response(bt_address_t *addr, const char *model_id, uint16_t length)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(model_id);

    SAL_CHECK_RET(service_adapter_hfp_ag_model_id_response(addr->addr, (char *)model_id, length),
                  SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_error_response(bt_address_t *addr, hfp_atcmd_result_t result)
{
    SAL_CHECK_PARAM(addr);

    SAL_CHECK_RET(service_adapter_hfp_ag_send_error_response(addr->addr, result), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}
#endif
