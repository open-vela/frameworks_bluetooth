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

#ifndef __BLUETOOTH_PROXY_H__
#define __BLUETOOTH_PROXY_H__

#include "bluetooth_stub.h"
#include <android/binder_manager.h>
#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

BpBtManager* BpBtManager_new(const char* instance);
void BpBtManager_delete(BpBtManager* bpManager);
bt_status_t BpBtManager_createInstance(AIBinder* binder, uint32_t handle,
    uint32_t type, const char* hostName,
    uint32_t* appId);
bt_status_t BpBtManager_getInstance(AIBinder* binder, const char* name, uint32_t* handle);
bt_status_t BpBtManager_deleteInstance(BpBtManager* bpBinder, uint32_t appId);
bt_status_t BpBtManager_startService(BpBtManager* bpBinder, uint32_t appId, uint32_t profileId);
bt_status_t BpBtManager_stopService(BpBtManager* bpBinder, uint32_t appId, uint32_t profileId);

#ifdef __cplusplus
}
#endif
#endif /* __BLUETOOTH_PROXY_H__ */