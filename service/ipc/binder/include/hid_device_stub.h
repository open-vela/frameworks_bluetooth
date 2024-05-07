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

#ifndef __BT_HID_DEVICE_STUB_H__
#define __BT_HID_DEVICE_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    void* usr_data;
} IBtHidd;

typedef struct {
    AIBinder_Class* clazz;
    AIBinder* binder;
} BpBtHidd;

typedef enum {
    IHIDD_REGISTER_CALLBACK = FIRST_CALL_TRANSACTION,
    IHIDD_UNREGISTER_CALLBACK,
    IHIDD_REGISTER_APP,
    IHIDD_UNREGISTER_APP,
    IHIDD_CONNECT,
    IHIDD_DISCONNECT,
    IHIDD_SEND_REPORT,
    IHIDD_RESPONSE_REPORT,
    IHIDD_REPORT_ERROR,
    IHIDD_VIRTUAL_UNPLUG
} IBtHidd_Call;

#define HID_DEVICE_BINDER_INSTANCE "Vela.Bluetooth.Hid.Device"

binder_status_t BtHidd_addService(IBtHidd* hidd, const char* instance);
AIBinder* BtHidd_getService(BpBtHidd** bpHidd, const char* instance);

#ifdef __cplusplus
}
#endif
#endif /* __BT_HID_DEVICE_STUB_H__ */