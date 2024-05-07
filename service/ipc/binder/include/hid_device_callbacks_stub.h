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

#ifndef __HID_DEVICE_CALLBACKS_STUB_H__
#define __HID_DEVICE_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_hid_device.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    const hid_device_callbacks_t* callbacks;
    void* cookie;
} IBtHiddCallbacks;

typedef enum {
    ICBKS_HIDD_APP_STATE = FIRST_CALL_TRANSACTION,
    ICBKS_HIDD_CONNECTION_STATE,
    ICBKS_GET_REPORT,
    ICBKS_SET_REPORT,
    ICBKS_RECEIVE_REPORT,
    ICBKS_VIRTUAL_UNPLUG
} IBtHiddCallbacks_Call;

AIBinder* BtHiddCallbacks_getBinder(IBtHiddCallbacks* adapter);
binder_status_t BtHiddCallbacks_associateClass(AIBinder* binder);
IBtHiddCallbacks* BtHiddCallbacks_new(const hid_device_callbacks_t* callbacks);
void BtHiddCallbacks_delete(IBtHiddCallbacks* cbks);

#ifdef __cplusplus
}
#endif
#endif /* __HID_DEVICE_CALLBACKS_STUB_H__ */