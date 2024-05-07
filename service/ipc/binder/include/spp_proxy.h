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

#ifndef __BT_SPP_PROXY_H__
#define __BT_SPP_PROXY_H__

#include "spp_stub.h"
#include <android/binder_manager.h>
#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

BpBtSpp* BpBtSpp_new(const char* instance);
void BpBtSpp_delete(BpBtSpp* bpSpp);
void* BpBtSpp_registerApp(BpBtSpp* bpBinder, AIBinder* cbksBinder);
bt_status_t BpBtSpp_unRegisterApp(BpBtSpp* bpBinder, void* handle);
bt_status_t BpBtSpp_serverStart(BpBtSpp* bpBinder, void* handle, uint16_t scn, bt_uuid_t* uuid, uint8_t maxConnection);
bt_status_t BpBtSpp_serverStop(BpBtSpp* bpBinder, void* handle, uint16_t scn);
bt_status_t BpBtSpp_connect(BpBtSpp* bpBinder, void* handle, bt_address_t* addr, int16_t scn, bt_uuid_t* uuid, uint16_t* port);
bt_status_t BpBtSpp_disconnect(BpBtSpp* bpBinder, void* handle, bt_address_t* addr, uint16_t port);

#ifdef __cplusplus
}
#endif
#endif /* __BT_SPP_PROXY_H__ */