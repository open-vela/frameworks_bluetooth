/****************************************************************************
 * tests/unittest/framework_api/src/api_stubs.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/**
 * Thin stubs replicating the public Bluetooth API functions for testing.
 * Each function calls service_manager_get_profile() and dispatches to
 * the corresponding interface method. This avoids compiling the real
 * API source files which have BTSYMBOLS server_ prefix renaming under
 * CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC and logging dependencies.
 */

#include <stddef.h>
#include <stdint.h>

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_status.h"
#include "bt_profile.h"
#include "bt_l2cap.h"

#include "a2dp_sink_service.h"
#include "a2dp_source_service.h"
#include "hfp_hf_service.h"
#include "hfp_ag_service.h"
#include "avrcp_control_service.h"
#include "avrcp_target_service.h"
#include "spp_service.h"
#include "pan_service.h"
#include "hid_device_service.h"
#include "gattc_service.h"
#include "gatts_service.h"

extern const void *service_manager_get_profile(enum profile_id id);

/* ===== A2DP Sink ===== */

void *bt_a2dp_sink_register_callbacks(bt_instance_t *ins,
    const a2dp_sink_callbacks_t *cbs)
{
    a2dp_sink_interface_t *p =
        (a2dp_sink_interface_t *)service_manager_get_profile(PROFILE_A2DP_SINK);
    return p->register_callbacks(NULL, cbs);
}

bool bt_a2dp_sink_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    a2dp_sink_interface_t *p =
        (a2dp_sink_interface_t *)service_manager_get_profile(PROFILE_A2DP_SINK);
    return p->unregister_callbacks(NULL, cookie);
}

bool bt_a2dp_sink_is_connected(bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_sink_interface_t *p =
        (a2dp_sink_interface_t *)service_manager_get_profile(PROFILE_A2DP_SINK);
    return p->is_connected(addr);
}

bool bt_a2dp_sink_is_playing(bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_sink_interface_t *p =
        (a2dp_sink_interface_t *)service_manager_get_profile(PROFILE_A2DP_SINK);
    return p->is_playing(addr);
}

profile_connection_state_t bt_a2dp_sink_get_connection_state(
    bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_sink_interface_t *p =
        (a2dp_sink_interface_t *)service_manager_get_profile(PROFILE_A2DP_SINK);
    return p->get_connection_state(addr);
}

bt_status_t bt_a2dp_sink_connect(bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_sink_interface_t *p =
        (a2dp_sink_interface_t *)service_manager_get_profile(PROFILE_A2DP_SINK);
    return p->connect(addr);
}

bt_status_t bt_a2dp_sink_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_sink_interface_t *p =
        (a2dp_sink_interface_t *)service_manager_get_profile(PROFILE_A2DP_SINK);
    return p->disconnect(addr);
}

bt_status_t bt_a2dp_sink_set_active_device(bt_instance_t *ins,
    bt_address_t *addr)
{
    a2dp_sink_interface_t *p =
        (a2dp_sink_interface_t *)service_manager_get_profile(PROFILE_A2DP_SINK);
    return p->set_active_device(addr);
}

/* ===== A2DP Source ===== */

void *bt_a2dp_source_register_callbacks(bt_instance_t *ins,
    const a2dp_source_callbacks_t *cbs)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->register_callbacks(NULL, cbs);
}

bool bt_a2dp_source_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->unregister_callbacks(NULL, cookie);
}

bool bt_a2dp_source_is_connected(bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->is_connected(addr);
}

bool bt_a2dp_source_is_playing(bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->is_playing(addr);
}

profile_connection_state_t bt_a2dp_source_get_connection_state(
    bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->get_connection_state(addr);
}

bt_status_t bt_a2dp_source_connect(bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->connect(addr);
}

bt_status_t bt_a2dp_source_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->disconnect(addr);
}

bt_status_t bt_a2dp_source_set_silence_device(bt_instance_t *ins,
    bt_address_t *addr, bool silence)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->set_silence_device(addr, silence);
}

bt_status_t bt_a2dp_source_set_active_device(bt_instance_t *ins,
    bt_address_t *addr)
{
    a2dp_source_interface_t *p =
        (a2dp_source_interface_t *)service_manager_get_profile(PROFILE_A2DP);
    return p->set_active_device(addr);
}

/* ===== HFP HF ===== */

void *bt_hfp_hf_register_callbacks(bt_instance_t *ins,
    const hfp_hf_callbacks_t *cbs)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->register_callbacks(NULL, cbs);
}

bool bt_hfp_hf_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->unregister_callbacks(NULL, cookie);
}

bool bt_hfp_hf_is_connected(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->is_connected(addr);
}

bool bt_hfp_hf_is_audio_connected(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->is_audio_connected(addr);
}

profile_connection_state_t bt_hfp_hf_get_connection_state(
    bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->get_connection_state(addr);
}

bt_status_t bt_hfp_hf_connect(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->connect(addr);
}

bt_status_t bt_hfp_hf_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->disconnect(addr);
}

bt_status_t bt_hfp_hf_set_connection_policy(bt_instance_t *ins,
    bt_address_t *addr, connection_policy_t policy)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->set_connection_policy(addr, policy);
}

bt_status_t bt_hfp_hf_connect_audio(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->connect_audio(addr);
}

bt_status_t bt_hfp_hf_disconnect_audio(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->disconnect_audio(addr);
}

bt_status_t bt_hfp_hf_start_voice_recognition(bt_instance_t *ins,
    bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->start_voice_recognition(addr);
}

bt_status_t bt_hfp_hf_stop_voice_recognition(bt_instance_t *ins,
    bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->stop_voice_recognition(addr);
}

bt_status_t bt_hfp_hf_dial(bt_instance_t *ins, bt_address_t *addr,
    const char *number)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->dial(addr, number);
}

bt_status_t bt_hfp_hf_dial_memory(bt_instance_t *ins, bt_address_t *addr,
    uint32_t memory)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->dial_memory(addr, memory);
}

bt_status_t bt_hfp_hf_redial(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->redial(addr);
}

bt_status_t bt_hfp_hf_accept_call(bt_instance_t *ins, bt_address_t *addr,
    hfp_call_accept_t flag)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->accept_call(addr, flag);
}

bt_status_t bt_hfp_hf_reject_call(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->reject_call(addr);
}

bt_status_t bt_hfp_hf_hold_call(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->hold_call(addr);
}

bt_status_t bt_hfp_hf_terminate_call(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->terminate_call(addr);
}

bt_status_t bt_hfp_hf_send_dtmf(bt_instance_t *ins, bt_address_t *addr,
    char dtmf)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->send_dtmf(addr, dtmf);
}

bt_status_t bt_hfp_hf_volume_control(bt_instance_t *ins, bt_address_t *addr,
    hfp_volume_type_t type, uint8_t volume)
{
    hfp_hf_interface_t *p =
        (hfp_hf_interface_t *)service_manager_get_profile(PROFILE_HFP_HF);
    return p->volume_control(addr, type, volume);
}

/* ===== HFP AG ===== */

void *bt_hfp_ag_register_callbacks(bt_instance_t *ins,
    const hfp_ag_callbacks_t *cbs)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->register_callbacks(NULL, cbs);
}

bool bt_hfp_ag_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->unregister_callbacks(NULL, cookie);
}

bool bt_hfp_ag_is_connected(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->is_connected(addr);
}

bool bt_hfp_ag_is_audio_connected(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->is_audio_connected(addr);
}

profile_connection_state_t bt_hfp_ag_get_connection_state(
    bt_instance_t *ins, bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->get_connection_state(addr);
}

bt_status_t bt_hfp_ag_connect(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->connect(addr);
}

bt_status_t bt_hfp_ag_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->disconnect(addr);
}

bt_status_t bt_hfp_ag_connect_audio(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->connect_audio(addr);
}

bt_status_t bt_hfp_ag_disconnect_audio(bt_instance_t *ins, bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->disconnect_audio(addr);
}

bt_status_t bt_hfp_ag_start_virtual_call(bt_instance_t *ins,
    bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->start_virtual_call(addr);
}

bt_status_t bt_hfp_ag_stop_virtual_call(bt_instance_t *ins,
    bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->stop_virtual_call(addr);
}

bt_status_t bt_hfp_ag_start_voice_recognition(bt_instance_t *ins,
    bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->start_voice_recognition(addr);
}

bt_status_t bt_hfp_ag_stop_voice_recognition(bt_instance_t *ins,
    bt_address_t *addr)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->stop_voice_recognition(addr);
}

bt_status_t bt_hfp_ag_volume_control(bt_instance_t *ins, bt_address_t *addr,
    hfp_volume_type_t type, uint8_t volume)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->volume_control(addr, type, volume);
}

bt_status_t bt_hfp_ag_send_at_command(bt_instance_t *ins, bt_address_t *addr,
    const char *at_command)
{
    hfp_ag_interface_t *p =
        (hfp_ag_interface_t *)service_manager_get_profile(PROFILE_HFP_AG);
    return p->send_at_command(addr, at_command);
}

/* ===== AVRCP Control ===== */

void *bt_avrcp_control_register_callbacks(bt_instance_t *ins,
    const avrcp_control_callbacks_t *cbs)
{
    avrcp_control_interface_t *p =
        (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
    return p->register_callbacks(NULL, cbs);
}

bool bt_avrcp_control_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    avrcp_control_interface_t *p =
        (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
    return p->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_avrcp_control_get_element_attributes(bt_instance_t *ins,
    bt_address_t *addr)
{
    avrcp_control_interface_t *p =
        (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
    return p->avrcp_control_get_element_attributes(addr);
}

bt_status_t bt_avrcp_control_send_passthrough_cmd(bt_instance_t *ins,
    bt_address_t *addr, uint8_t cmd, uint8_t state)
{
    avrcp_control_interface_t *p =
        (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
    return p->avrcp_control_send_passthrough_cmd(addr, cmd, state);
}

bt_status_t bt_avrcp_control_get_unit_info(bt_instance_t *ins,
    bt_address_t *addr)
{
    avrcp_control_interface_t *p =
        (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
    return p->avrcp_control_get_unit_info(addr);
}

bt_status_t bt_avrcp_control_get_subunit_info(bt_instance_t *ins,
    bt_address_t *addr)
{
    avrcp_control_interface_t *p =
        (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
    return p->avrcp_control_get_subunit_info(addr);
}

bt_status_t bt_avrcp_control_get_playback_state(bt_instance_t *ins,
    bt_address_t *addr)
{
    avrcp_control_interface_t *p =
        (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
    return p->avrcp_control_get_playback_state(addr);
}

bt_status_t bt_avrcp_control_register_notification(bt_instance_t *ins,
    bt_address_t *addr, uint8_t event, uint32_t interval)
{
    avrcp_control_interface_t *p =
        (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
    return p->avrcp_control_register_notification(addr, event, interval);
}

/* ===== AVRCP Target ===== */

void *bt_avrcp_target_register_callbacks(bt_instance_t *ins,
    const avrcp_target_callbacks_t *cbs)
{
    avrcp_target_interface_t *p =
        (avrcp_target_interface_t *)service_manager_get_profile(PROFILE_AVRCP_TG);
    return p->register_callbacks(NULL, cbs);
}

bool bt_avrcp_target_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    avrcp_target_interface_t *p =
        (avrcp_target_interface_t *)service_manager_get_profile(PROFILE_AVRCP_TG);
    return p->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_avrcp_target_get_play_status_response(bt_instance_t *ins,
    bt_address_t *addr, avrcp_play_status_t play_status,
    uint32_t song_len, uint32_t song_pos)
{
    avrcp_target_interface_t *p =
        (avrcp_target_interface_t *)service_manager_get_profile(PROFILE_AVRCP_TG);
    return p->get_play_status_rsp(addr, play_status, song_len, song_pos);
}

bt_status_t bt_avrcp_target_play_status_notify(bt_instance_t *ins,
    bt_address_t *addr, avrcp_play_status_t play_status)
{
    avrcp_target_interface_t *p =
        (avrcp_target_interface_t *)service_manager_get_profile(PROFILE_AVRCP_TG);
    return p->play_status_notify(addr, play_status);
}

/* ===== SPP ===== */

void *bt_spp_register_app_with_name(bt_instance_t *ins, const char *name,
    const spp_callbacks_t *cbs)
{
    spp_interface_t *p =
        (spp_interface_t *)service_manager_get_profile(PROFILE_SPP);
    return p->register_app(NULL, name, cbs);
}

void *bt_spp_register_app(bt_instance_t *ins, const spp_callbacks_t *cbs)
{
    return bt_spp_register_app_with_name(ins, NULL, cbs);
}

bt_status_t bt_spp_unregister_app(bt_instance_t *ins, void *handle)
{
    spp_interface_t *p =
        (spp_interface_t *)service_manager_get_profile(PROFILE_SPP);
    return p->unregister_app(NULL, handle);
}

bt_status_t bt_spp_server_start(bt_instance_t *ins, void *handle,
    uint16_t scn, bt_uuid_t *uuid, uint8_t max_connection)
{
    spp_interface_t *p =
        (spp_interface_t *)service_manager_get_profile(PROFILE_SPP);
    return p->server_start(handle, scn, uuid, max_connection);
}

bt_status_t bt_spp_server_stop(bt_instance_t *ins, void *handle, uint16_t scn)
{
    spp_interface_t *p =
        (spp_interface_t *)service_manager_get_profile(PROFILE_SPP);
    return p->server_stop(handle, scn);
}

bt_status_t bt_spp_connect(bt_instance_t *ins, void *handle,
    bt_address_t *addr, int16_t scn, bt_uuid_t *uuid, uint16_t *port)
{
    spp_interface_t *p =
        (spp_interface_t *)service_manager_get_profile(PROFILE_SPP);
    return p->connect(handle, addr, scn, uuid, port, false);
}

bt_status_t bt_spp_insecure_connect(bt_instance_t *ins, void *handle,
    bt_address_t *addr, int16_t scn, bt_uuid_t *uuid, uint16_t *port)
{
    spp_interface_t *p =
        (spp_interface_t *)service_manager_get_profile(PROFILE_SPP);
    return p->connect(handle, addr, scn, uuid, port, true);
}

bt_status_t bt_spp_disconnect(bt_instance_t *ins, void *handle,
    bt_address_t *addr, uint16_t port)
{
    spp_interface_t *p =
        (spp_interface_t *)service_manager_get_profile(PROFILE_SPP);
    return p->disconnect(handle, addr, port);
}

/* ===== PAN ===== */

void *bt_pan_register_callbacks(bt_instance_t *ins,
    const pan_callbacks_t *cbs)
{
    pan_interface_t *p =
        (pan_interface_t *)service_manager_get_profile(PROFILE_PANU);
    return p->register_callbacks(NULL, cbs);
}

bool bt_pan_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    pan_interface_t *p =
        (pan_interface_t *)service_manager_get_profile(PROFILE_PANU);
    return p->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_pan_connect(bt_instance_t *ins, bt_address_t *addr,
    uint8_t dst_role, uint8_t src_role)
{
    pan_interface_t *p =
        (pan_interface_t *)service_manager_get_profile(PROFILE_PANU);
    return p->connect(addr, dst_role, src_role);
}

bt_status_t bt_pan_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    pan_interface_t *p =
        (pan_interface_t *)service_manager_get_profile(PROFILE_PANU);
    return p->disconnect(addr);
}

/* ===== HID Device ===== */

void *bt_hid_device_register_callbacks(bt_instance_t *ins,
    const hid_device_callbacks_t *cbs)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->register_callbacks(NULL, cbs);
}

bool bt_hid_device_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_hid_device_register_app(bt_instance_t *ins,
    hid_device_sdp_settings_t *sdp, bool le_hid)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->register_app(sdp, le_hid);
}

bt_status_t bt_hid_device_unregister_app(bt_instance_t *ins)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->unregister_app();
}

bt_status_t bt_hid_device_connect(bt_instance_t *ins, bt_address_t *addr)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->connect(addr);
}

bt_status_t bt_hid_device_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->disconnect(addr);
}

bt_status_t bt_hid_device_send_report(bt_instance_t *ins, bt_address_t *addr,
    uint8_t rpt_id, uint8_t *rpt_data, int rpt_size)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->send_report(addr, rpt_id, rpt_data, rpt_size);
}

bt_status_t bt_hid_device_response_report(bt_instance_t *ins,
    bt_address_t *addr, uint8_t rpt_type, uint8_t *rpt_data, int rpt_size)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->response_report(addr, rpt_type, rpt_data, rpt_size);
}

bt_status_t bt_hid_device_report_error(bt_instance_t *ins, bt_address_t *addr,
    hid_status_error_t error)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->report_error(addr, error);
}

bt_status_t bt_hid_device_virtual_unplug(bt_instance_t *ins,
    bt_address_t *addr)
{
    hid_device_interface_t *p =
        (hid_device_interface_t *)service_manager_get_profile(PROFILE_HID_DEV);
    return p->virtual_unplug(addr);
}

/* ===== GATT Client ===== */

bt_status_t bt_gattc_create_connect(bt_instance_t *ins,
    gattc_handle_t *phandle, gattc_callbacks_t *cbs)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->create_connect(NULL, phandle, cbs);
}

bt_status_t bt_gattc_delete_connect(gattc_handle_t h)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->delete_connect(h);
}

bt_status_t bt_gattc_connect(gattc_handle_t h, bt_address_t *addr,
    ble_addr_type_t addr_type)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->connect(h, addr, addr_type);
}

bt_status_t bt_gattc_disconnect(gattc_handle_t h)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->disconnect(h);
}

bt_status_t bt_gattc_discover_service(gattc_handle_t h, bt_uuid_t *uuid)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->discover_service(h, uuid);
}

bt_status_t bt_gattc_get_attribute_by_handle(gattc_handle_t h,
    uint16_t attr_handle, gatt_attr_desc_t *desc)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->get_attribute_by_handle(h, attr_handle, desc);
}

bt_status_t bt_gattc_get_attribute_by_uuid(gattc_handle_t h,
    uint16_t start_handle, uint16_t end_handle,
    bt_uuid_t *uuid, gatt_attr_desc_t *desc)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->get_attribute_by_uuid(h, start_handle, end_handle, uuid, desc);
}

bt_status_t bt_gattc_read(gattc_handle_t h, uint16_t attr_handle)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->read(h, attr_handle);
}

bt_status_t bt_gattc_write(gattc_handle_t h, uint16_t attr_handle,
    uint8_t *value, uint16_t length)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->write(h, attr_handle, value, length);
}

bt_status_t bt_gattc_write_without_response(gattc_handle_t h,
    uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->write_without_response(h, attr_handle, value, length);
}

bt_status_t bt_gattc_write_with_signed(gattc_handle_t h,
    uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->write_signed(h, attr_handle, value, length);
}

bt_status_t bt_gattc_subscribe(gattc_handle_t h, uint16_t attr_handle,
    uint16_t ccc_value)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->subscribe(h, attr_handle, ccc_value);
}

bt_status_t bt_gattc_unsubscribe(gattc_handle_t h, uint16_t attr_handle)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->unsubscribe(h, attr_handle);
}

bt_status_t bt_gattc_exchange_mtu(gattc_handle_t h, uint32_t mtu)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->exchange_mtu(h, mtu);
}

bt_status_t bt_gattc_update_connection_parameter(gattc_handle_t h,
    uint32_t min_interval, uint32_t max_interval,
    uint32_t latency, uint32_t timeout, uint32_t min_ce_len,
    uint32_t max_ce_len)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->update_connection_parameter(h, min_interval, max_interval,
        latency, timeout, min_ce_len, max_ce_len);
}

bt_status_t bt_gattc_read_phy(gattc_handle_t h)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->read_phy(h);
}

bt_status_t bt_gattc_update_phy(gattc_handle_t h,
    ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->update_phy(h, tx_phy, rx_phy);
}

bt_status_t bt_gattc_read_rssi(gattc_handle_t h)
{
    gattc_interface_t *p =
        (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
    return p->read_rssi(h);
}

/* ===== GATT Server ===== */

bt_status_t bt_gatts_register_service(bt_instance_t *ins,
    gatts_handle_t *phandle, gatts_callbacks_t *cbs)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->register_service(NULL, phandle, cbs);
}

bt_status_t bt_gatts_unregister_service(gatts_handle_t h)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->unregister_service(h);
}

bt_status_t bt_gatts_connect(gatts_handle_t h, bt_address_t *addr,
    ble_addr_type_t addr_type)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->connect(h, addr, addr_type);
}

bt_status_t bt_gatts_connect_bear(gatts_handle_t h, bt_address_t *addr,
    ble_addr_type_t addr_type, uint8_t bear_type)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->connect_bear(h, addr, addr_type, bear_type);
}

bt_status_t bt_gatts_disconnect(gatts_handle_t h, bt_address_t *addr)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->disconnect(h, addr);
}

bt_status_t bt_gatts_add_attr_table(gatts_handle_t h, gatt_srv_db_t *db)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->add_attr_table(h, db);
}

bt_status_t bt_gatts_remove_attr_table(gatts_handle_t h, uint16_t attr_handle)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->remove_attr_table(h, attr_handle);
}

bt_status_t bt_gatts_set_attr_value(gatts_handle_t h, uint16_t attr_handle,
    uint8_t *value, uint16_t length)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->set_attr_value(h, attr_handle, value, length);
}

bt_status_t bt_gatts_get_attr_value(gatts_handle_t h, uint16_t attr_handle,
    uint8_t *value, uint16_t *length)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->get_attr_value(h, attr_handle, value, length);
}

bt_status_t bt_gatts_response(gatts_handle_t h, bt_address_t *addr,
    uint32_t req_handle, uint8_t *value, uint16_t length)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->response(h, addr, req_handle, value, length);
}

bt_status_t bt_gatts_notify(gatts_handle_t h, bt_address_t *addr,
    uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->notify(h, addr, attr_handle, value, length);
}

bt_status_t bt_gatts_indicate(gatts_handle_t h, bt_address_t *addr,
    uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->indicate(h, addr, attr_handle, value, length);
}

bt_status_t bt_gatts_read_phy(gatts_handle_t h, bt_address_t *addr)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->read_phy(h, addr);
}

bt_status_t bt_gatts_update_phy(gatts_handle_t h, bt_address_t *addr,
    ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    gatts_interface_t *p =
        (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
    return p->update_phy(h, addr, tx_phy, rx_phy);
}

/* ===== L2CAP =====
 * L2CAP does not use service_manager_get_profile. The API calls
 * l2cap_*() functions directly. The test file test_bt_l2cap.c
 * provides mock implementations of these functions.
 */

void *bt_l2cap_register_callbacks(bt_instance_t *ins,
    const l2cap_callbacks_t *cbs)
{
    extern void *l2cap_register_callbacks(void *remote,
        const l2cap_callbacks_t *cbs);
    return l2cap_register_callbacks(ins, cbs);
}

bool bt_l2cap_unregister_callbacks(bt_instance_t *ins, void *handle)
{
    extern bool l2cap_unregister_callbacks(void **remote, void *cookie);
    void *tmp = NULL;
    return l2cap_unregister_callbacks(&tmp, handle);
}

bt_status_t bt_l2cap_listen(bt_instance_t *ins, void *handle,
    l2cap_config_option_t *option)
{
    extern bt_status_t l2cap_listen_channel(void *handle,
        l2cap_config_option_t *option);
    return l2cap_listen_channel(handle, option);
}

bt_status_t bt_l2cap_connect(bt_instance_t *ins, void *handle,
    bt_address_t *addr, l2cap_config_option_t *option)
{
    extern bt_status_t l2cap_connect_channel(void *handle,
        bt_address_t *addr, l2cap_config_option_t *option);
    return l2cap_connect_channel(handle, addr, option);
}

bt_status_t bt_l2cap_disconnect(bt_instance_t *ins, void *handle, uint16_t id)
{
    extern bt_status_t l2cap_disconnect_channel(void *handle, uint16_t id);
    return l2cap_disconnect_channel(handle, id);
}

bt_status_t bt_l2cap_stop_listen(bt_instance_t *ins, void *handle,
    uint16_t psm)
{
    extern bt_status_t l2cap_stop_listen_channel(void *handle,
        bt_transport_t transport, uint16_t psm);
    return l2cap_stop_listen_channel(handle, BT_TRANSPORT_BLE, psm);
}

bt_status_t bt_l2cap_stop_listen_with_transport(bt_instance_t *ins,
    void *handle, bt_transport_t transport, uint16_t psm)
{
    extern bt_status_t l2cap_stop_listen_channel(void *handle,
        bt_transport_t transport, uint16_t psm);
    return l2cap_stop_listen_channel(handle, transport, psm);
}
