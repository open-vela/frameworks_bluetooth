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

#ifndef __BT_HFP_AG_PROXY_H__
#define __BT_HFP_AG_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "hfp_ag_stub.h"

#ifdef __cplusplus
extern "C" {
#endif

BpBtHfpAg* BpBtHfpAg_new(const char* instance);
void BpBtHfpAg_delete(BpBtHfpAg* bpPan);
void* BpBtHfpAg_registerCallback(BpBtHfpAg* bpBinder, AIBinder* cbksBinder);
bool BpBtHfpAg_unRegisterCallback(BpBtHfpAg* bpBinder, void* cookie);
bool BpBtHfpAg_isConnected(BpBtHfpAg* bpBinder, bt_address_t* addr);
bool BpBtHfpAg_isAudioConnected(BpBtHfpAg* bpBinder, bt_address_t* addr);
profile_connection_state_t BpBtHfpAg_getConnectionState(BpBtHfpAg* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpAg_connect(BpBtHfpAg* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpAg_disconnect(BpBtHfpAg* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpAg_connectAudio(BpBtHfpAg* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpAg_disconnectAudio(BpBtHfpAg* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpAg_startVoiceRecognition(BpBtHfpAg* bpBinder, bt_address_t* addr);
bt_status_t BpBtHfpAg_stopVoiceRecognition(BpBtHfpAg* bpBinder, bt_address_t* addr);
#ifdef __cplusplus
}
#endif
#endif /* __BT_HFP_AG_PROXY_H__ */