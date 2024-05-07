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

#ifndef __BT_HFP_HF_PROXY_H__
#define __BT_HFP_HF_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "bt_hfp_hf.h"
#include "hfp_hf_stub.h"

#ifdef __cplusplus
extern "C" {
#endif

BpBtHfpHf* BpBtHfpHf_new(const char* instance);
void BpBtHfpHf_delete(BpBtHfpHf* bpPan);
void* BpBtHfpHf_registerCallback(BpBtHfpHf* bpBinder, AIBinder* cbksBinder);
bool BpBtHfpHf_unRegisterCallback(BpBtHfpHf* bpBinder, void* cookie);
bool BpBtHfpHf_isConnected(BpBtHfpHf* bpBinder, bt_address_t* addr);
bool BpBtHfpHf_isAudioConnected(BpBtHfpHf* bpBinder, bt_address_t* addr);
profile_connection_state_t BpBtHfpHf_getConnectionState(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_connect(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_disconnect(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_connectAudio(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_disconnectAudio(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_startVoiceRecognition(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_stopVoiceRecognition(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_dial(BpBtHfpHf* bpBinder, bt_address_t* addr, const char* number);
bt_status_t BpBtHfpHf_dialMemory(BpBtHfpHf* bpBinder, bt_address_t* addr, uint32_t memory);
bt_status_t BpBtHfpHf_redial(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_acceptCall(BpBtHfpHf* bpBinder, bt_address_t* addr, hfp_call_accept_t flag);
bt_status_t BpBtHfpHf_rejectCall(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_holdCall(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_terminateCall(BpBtHfpHf* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpHf_controlCall(BpBtHfpHf* bpBinder, bt_address_t* addr, hfp_call_control_t chld, uint8_t index);
bt_status_t BpBtHfpHf_queryCurrentCalls(BpBtHfpHf* bpBinder, bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator);
bt_status_t BpBtHfpHf_sendAtCmd(BpBtHfpHf* bpBinder, bt_address_t* addr, const char* cmd);
#ifdef __cplusplus
}
#endif
#endif /* __BT_HFP_HF_PROXY_H__ */