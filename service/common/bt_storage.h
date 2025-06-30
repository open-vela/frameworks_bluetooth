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
#ifndef _BT_STORAGE_H__
#define _BT_STORAGE_H__

#include "bluetooth_define.h"

typedef void (*load_storage_callback_t)(void* data, uint16_t length, uint16_t items);

int bt_storage_init(void);
int bt_storage_cleanup(void);

int bt_storage_save_adapter_info(adapter_storage_t* adapter);
int bt_storage_load_adapter_info(adapter_storage_t* adapter);
int bt_storage_save_bonded_device(remote_device_properties_t* remote, uint16_t size);
int bt_storage_save_whitelist(remote_device_le_properties_t* remote, uint16_t size);
int bt_storage_save_le_bonded_device(remote_device_le_properties_t* remote, uint16_t size);
int bt_storage_load_bonded_device(load_storage_callback_t cb);
int bt_storage_load_whitelist_device(load_storage_callback_t cb);
int bt_storage_load_le_bonded_device(load_storage_callback_t cb);

#endif /* _BT_STORAGE_H__ */