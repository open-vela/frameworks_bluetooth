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
int bt_storage_save_gatt_cache_device(remote_device_gatt_properties_t* remote, uint16_t size);
int bt_storage_load_bonded_device(load_storage_callback_t cb);
int bt_storage_load_whitelist_device(load_storage_callback_t cb);
int bt_storage_load_le_bonded_device(load_storage_callback_t cb);
int bt_storage_load_gatt_cache_device(load_storage_callback_t cb);

#ifdef CONFIG_BLUETOOTH_STORAGE_PROPERTY_SUPPORT
#define GEN_PROP_KEY(buf, key, address, len) snprintf((buf), (len), "%s%02X:%02X:%02X:%02X:%02X:%02X", \
    (key),                                                                                             \
    (address)->addr[5], (address)->addr[4], (address)->addr[3],                                        \
    (address)->addr[2], (address)->addr[1], (address)->addr[0])

#define PARSE_PROP_KEY(addr_str, name, name_prefix_len, addr_str_len, addr_ptr) \
    do {                                                                        \
        strlcpy((addr_str), (name) + (name_prefix_len), (addr_str_len));        \
        bt_addr_str2ba((addr_str), (addr_ptr));                                 \
    } while (0)

#define ERROR_ADAPTERINFO_VALUE -1

#define BT_KVDB_ADAPTERINFO_NAME "persist.bluetooth.adapterInfo.name"
#define BT_KVDB_ADAPTERINFO_COD "persist.bluetooth.adapterInfo.class_of_device"
#define BT_KVDB_ADAPTERINFO_IOCAP "persist.bluetooth.adapterInfo.io_capability"
#define BT_KVDB_ADAPTERINFO_SCAN "persist.bluetooth.adapterInfo.scan_mode"
#define BT_KVDB_ADAPTERINFO_BOND "persist.bluetooth.adapterInfo.bondable"
#define BT_KVDB_ADAPTERINFO_IRK "persist.bluetooth.adapterInfo.irk"

#define BT_KVDB_ADAPTERINFO "persist.bluetooth.adapterInfo."
#define BT_KVDB_BTBOND "persist.bluetooth.btbonded."
#define BT_KVDB_BLEBOND "persist.bluetooth.blebonded."
#define BT_KVDB_BLEWHITELIST "persist.bluetooth.whitelist."
#define BT_KVDB_BLEGATTDBHASH "persist.bluetooth.blegattDBhash."

int bt_storage_properties_destory(void);
void bt_storage_delete(char* key, uint16_t items, char* prop_name);
#endif

#endif /* _BT_STORAGE_H__ */