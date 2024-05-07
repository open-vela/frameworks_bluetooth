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
#define LOG_THF "hfp_hf_api"

#include "bt_hfp_hf.h"
#include "bt_profile.h"

#include "hfp_hf_callbacks_stub.h"
#include "hfp_hf_proxy.h"
#include "hfp_hf_stub.h"

#include "utils/log.h"
#include <stdint.h>

void* bt_hfp_hf_register_callbacks(bt_instance_t* ins, const hfp_hf_callbacks_t* callbacks)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    IBtHfpHfCallbacks* cbks = BtHfpHfCallbacks_new(callbacks);
    AIBinder* binder = BtHfpHfCallbacks_getBinder(cbks);
    if (!binder) {
        BtHfpHfCallbacks_delete(cbks);
        return NULL;
    }

    void* remote_cbks = BpBtHfpHf_registerCallback(hf, binder);
    if (!remote_cbks) {
        BtHfpHfCallbacks_delete(cbks);
        return NULL;
    }
    cbks->cookie = remote_cbks;

    return cbks;
}

bool bt_hfp_hf_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    IBtHfpHfCallbacks* cbks = cookie;
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    bool ret = BpBtHfpHf_unRegisterCallback(hf, cbks->cookie);
    if (ret)
        BtHfpHfCallbacks_delete(cbks);

    return ret;
}

bool bt_hfp_hf_is_connected(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_isConnected(hf, addr);
}

bool bt_hfp_hf_is_audio_connected(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_isAudioConnected(hf, addr);
}

profile_connection_state_t bt_hfp_hf_get_connection_state(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_getConnectionState(hf, addr);
}

bt_status_t bt_hfp_hf_connect(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_connect(hf, addr);
}

bt_status_t bt_hfp_hf_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_disconnect(hf, addr);
}

bt_status_t bt_hfp_hf_connect_audio(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_connectAudio(hf, addr);
}

bt_status_t bt_hfp_hf_disconnect_audio(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_disconnectAudio(hf, addr);
}

bt_status_t bt_hfp_hf_start_voice_recognition(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_startVoiceRecognition(hf, addr);
}

bt_status_t bt_hfp_hf_stop_voice_recognition(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_stopVoiceRecognition(hf, addr);
}

bt_status_t bt_hfp_hf_dial(bt_instance_t* ins, bt_address_t* addr, const char* number)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_dial(hf, addr, number);
}

bt_status_t bt_hfp_hf_dial_memory(bt_instance_t* ins, bt_address_t* addr, uint32_t memory)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_dialMemory(hf, addr, memory);
}

bt_status_t bt_hfp_hf_redial(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_redial(hf, addr);
}

bt_status_t bt_hfp_hf_accept_call(bt_instance_t* ins, bt_address_t* addr, hfp_call_accept_t flag)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_acceptCall(hf, addr, flag);
}

bt_status_t bt_hfp_hf_reject_call(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_rejectCall(hf, addr);
}

bt_status_t bt_hfp_hf_hold_call(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_holdCall(hf, addr);
}

bt_status_t bt_hfp_hf_terminate_call(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_terminateCall(hf, addr);
}

bt_status_t bt_hfp_hf_control_call(bt_instance_t* ins, bt_address_t* addr, hfp_call_control_t chld, uint8_t index)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_controlCall(hf, addr, chld, index);
}

bt_status_t bt_hfp_hf_query_current_calls(bt_instance_t* ins, bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_queryCurrentCalls(hf, addr, calls, num, allocator);
}

bt_status_t bt_hfp_hf_send_at_cmd(bt_instance_t* ins, bt_address_t* addr, const char* cmd)
{
    BpBtHfpHf* hf = (BpBtHfpHf*)bluetooth_get_proxy(ins, PROFILE_HFP_HF);

    return BpBtHfpHf_sendAtCmd(hf, addr, cmd);
}
