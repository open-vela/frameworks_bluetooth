/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __BT_ASYNC_H__
#define __BT_ASYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"

#ifdef CONFIG_BLUETOOTH_FRAMEWORK_ASYNC
typedef void (*bt_status_cb_t)(bt_instance_t* ins, bt_status_t status, void* userdata);
typedef void (*bt_address_cb_t)(bt_instance_t* ins, bt_status_t status, bt_address_t* addr, void* userdata);
typedef void (*bt_uuids_cb_t)(bt_instance_t* ins, bt_status_t status, bt_uuid_t* uuids, uint16_t size, void* userdata);
typedef void (*bt_device_type_cb_t)(bt_instance_t* ins, bt_status_t status, bt_device_type_t dtype, void* userdata);
typedef void (*bt_bool_cb_t)(bt_instance_t* ins, bt_status_t status, bool bbool, void* userdata);
typedef void (*bt_string_cb_t)(bt_instance_t* ins, bt_status_t status, const char* str, void* userdata);
typedef void (*bt_s8_cb_t)(bt_instance_t* ins, bt_status_t status, int8_t val, void* userdata);
typedef void (*bt_u8_cb_t)(bt_instance_t* ins, bt_status_t status, uint8_t val, void* userdata);
typedef void (*bt_u16_cb_t)(bt_instance_t* ins, bt_status_t status, uint16_t val, void* userdata);
typedef void (*bt_u32_cb_t)(bt_instance_t* ins, bt_status_t status, uint32_t val, void* userdata);
#endif

#ifdef __cplusplus
}
#endif

#endif