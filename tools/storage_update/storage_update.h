/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef __STORAGE_UPDATE_H__
#define __STORAGE_UPDATE_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "uv_ext.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define BLUETOOTH_STORAGE_VERSION_4 1
#define BLUETOOTH_STORAGE_VERSION_5 1
#define BT_STORAGE_UNQLITE_ITEM 4

/****************************************************************************
 * Public Types
 ****************************************************************************/
enum {
    BT_STORAGE_UPDATE_ADAPTER_INFO = 0,
    BT_STORAGE_UPDATE_BTBOND_INFO,
    BT_STORAGE_UPDATE_BLEBOND_INFO,
    BT_STORAGE_UPDATE_WHITELIST_INFO,
    BT_STORAGE_UPDATE_GATT_HASH_INFO,
    BT_STORAGE_UPDATE_ITEM_MAX,
};

typedef struct {
    uint16_t items;
    uint16_t value_length;
    void* value;
} bt_storage_update_value_t;

typedef struct {
    int items[BT_STORAGE_UPDATE_ITEM_MAX];
} bt_storage_update_items_t;

typedef struct {
    bt_storage_update_value_t storage_info[BT_STORAGE_UPDATE_ITEM_MAX];
} bt_storage_update_properties_t;

enum {
    BT_STORAGE_VERSION_4_0_0 = 0, // name_str:64 Bytes
    BT_STORAGE_VERSION_5_0_0, // name_str:65 Bytes
    BT_STORAGE_VERSION_5_0_1, // add 80 Bytes UUIDs
    BT_STORAGE_VERSION_5_0_2, // version for dev-bluetooth/dev/openvela
    BT_STORAGE_VERSION_5_0_3, // version for zblue
    BT_STORAGE_VERSION_MAX,
};

#define BT_STORAGE_VERISON_CURRENT BT_STORAGE_VERSION_5_0_3 /* need to change per version */

typedef bt_storage_update_properties_t* (*bt_storage_update_func_t)(bt_storage_update_properties_t* old_storage);

/****************************************************************************
 * Public Functions
 ****************************************************************************/
bt_storage_update_properties_t* bt_storage_update_properties_malloc(int version, bt_storage_update_items_t* prop_items);
void bt_storage_update_properties_free(bt_storage_update_properties_t* props);
int bt_storage_get_version(void);
int bt_storage_update_get_item_len(int version, int storage_item);
int bt_storage_remove(void);

/* Unqlite */
int bt_storage_unqlite_init(void);
int bt_storage_unqlite_cleanup(void);

int bt_storage_save_item_unqlite(void* data, int items, int version, int storage_item);

int bt_storage_load_whitelist_device_unqlite(void** data, uint16_t* length);
int bt_storage_load_bonded_device_unqlite(void** data, uint16_t* length);
int bt_storage_load_adapter_info_unqlite(void** data, uint16_t* length);
int bt_storage_load_le_bonded_device_unqlite(void** data, uint16_t* length);

/* KVDB */
bt_storage_update_properties_t* bt_storage_load_info_kvdb(int version);

int bt_storage_save_item_kvdb(void* data, int items, int version, int storage_item);

#endif /* __STORAGE_UPDATE_H__ */