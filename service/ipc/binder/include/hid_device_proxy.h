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

#ifndef __BT_HID_DEVICE_PROXY_H__
#define __BT_HID_DEVICE_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "bt_hid_device.h"
#include "hid_device_stub.h"

#ifdef __cplusplus
extern "C" {
#endif

BpBtHidd* BpBtHidd_new(const char* instance);
void BpBtHidd_delete(BpBtHidd* bpHidd);
void* BpBtHidd_registerCallback(BpBtHidd* bpBinder, AIBinder* cbksBinder);
bool BpBtHidd_unRegisterCallback(BpBtHidd* bpBinder, void* cookie);
bt_status_t BpBtHidd_registerApp(BpBtHidd* bpBinder, hid_device_sdp_settings_t* sdp, bool le_hid);
bt_status_t BpBtHidd_unregisterApp(BpBtHidd* bpBinder);
bt_status_t BpBtHidd_connect(BpBtHidd* bpBinder, bt_address_t* addr);
bt_status_t BpBtHidd_disconnect(BpBtHidd* bpBinder, bt_address_t* addr);
bt_status_t BpBtHidd_sendReport(BpBtHidd* bpBinder, bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size);
bt_status_t BpBtHidd_responseReport(BpBtHidd* bpBinder, bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size);
bt_status_t BpBtHidd_reportError(BpBtHidd* bpBinder, bt_address_t* addr, hid_status_error_t error);
bt_status_t BpBtHidd_virtualUnplug(BpBtHidd* bpBinder, bt_address_t* addr);

#ifdef __cplusplus
}
#endif
#endif /* __BT_HID_DEVICE_PROXY_H__ */