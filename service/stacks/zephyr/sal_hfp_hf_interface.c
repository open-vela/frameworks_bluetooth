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

#include "sal_hfp_hf_interface.h"
#include "bt_debug.h"
#include "bt_list.h"
#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "service_loop.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#undef BT_UUID_DECLARE_16
#undef BT_UUID_DECLARE_32
#undef BT_UUID_DECLARE_128

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/sdp.h>

static struct bt_hfp_hf_cb hf_callbacks = {
    .connected = NULL,
    .disconnected = NULL,
    .sco_connected = NULL,
    .sco_disconnected = NULL,
    .service = NULL,
    .outgoing = NULL,
    .remote_ringing = NULL,
    .incoming = NULL,
    .incoming_held = NULL,
    .accept = NULL,
    .reject = NULL,
    .terminate = NULL,
    .held = NULL,
    .retrieve = NULL,
    .signal = NULL,
    .roam = NULL,
    .battery = NULL,
    .ring_indication = NULL,
    .dialing = NULL,
    .clip = NULL,
    .vgm = NULL,
    .vgs = NULL,
    .inband_ring = NULL,
    .operator = NULL,
    .codec_negotiate = NULL,
    .ecnr_turn_off = NULL,
    .call_waiting = NULL,
    .voice_recognition = NULL,
    .vre_state = NULL,
    .textual_representation = NULL,
    .request_phone_number = NULL,
    .subscriber_number = NULL,
    .query_call = NULL,
};

bt_status_t bt_sal_hfp_hf_init(uint32_t hf_features, uint8_t p_max_connection)
{
    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_hf_cleanup(void)
{
    return;
}

bt_status_t pre_hfp_hf_connect()
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_connect(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_disconnect(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_connect_audio(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_disconnect_audio(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_answer_call(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_reject_call(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_hold_call(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_hangup_call(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_dial_number(bt_address_t* addr, const char* number)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_dial_memory(bt_address_t* addr, uint32_t memory)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_call_control(bt_address_t* addr, hfp_call_control_t chld, uint32_t index)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_get_current_calls(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_start_voice_recognition(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_stop_voice_recognition(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_send_battery_level(bt_address_t* addr, uint8_t value)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_send_at_cmd(bt_address_t* addr, const char* cmd, uint16_t len)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_send_dtmf(bt_address_t* addr, char dtmf)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_get_subscriber_number(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}
