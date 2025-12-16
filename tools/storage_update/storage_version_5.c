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
#include <inttypes.h>
#include <kvdb.h>

#include "bluetooth_define.h"
#include "storage.h"
#include "storage_update.h"
#include "storage_version_4.h"
#include "storage_version_5.h"
#include "uv_ext.h"

#include "syslog.h"

bt_storage_update_properties_t* bt_storage_load_info_v5_0_0(void)
{
    return bt_storage_load_info_unqlite();
}

bt_storage_update_properties_t* bt_storage_update_v4_0_0_to_v5_0_0(bt_storage_update_properties_t* old_storage)
{
    bt_storage_update_properties_t* new_storage;
    bt_storage_update_items_t prop_items = { 0 };
    int i;
    /* v5_0_0 storage structure */
    adapter_storage_v5_0_0_t* new_adapter;
    remote_device_properties_v5_0_0_t* new_btbond;
    remote_device_le_properties_v5_0_0_t *new_lebond, *new_whitelist;
    /* v4_0_0 storage structure */
    adapter_storage_v4_0_0_t* old_adapter;
    remote_device_properties_v4_0_0_t* old_btbond;
    remote_device_le_properties_v4_0_0_t *old_lebond, *old_whitelist;

    old_adapter = (adapter_storage_v4_0_0_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    old_btbond = (remote_device_properties_v4_0_0_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value);
    old_lebond = (remote_device_le_properties_v4_0_0_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value);
    old_whitelist = (remote_device_le_properties_v4_0_0_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value);
    for (i = 0; i < BT_STORAGE_UPDATE_ITEM_MAX; ++i) {
        prop_items.items[i] = old_storage->storage_info[i].items;
    }

    /* properties init */
    new_storage = bt_storage_update_properties_malloc(BT_STORAGE_VERSION_5_0_0, &prop_items);
    if (!new_storage) {
        return NULL;
    }

    /* transform adapter info */
    new_adapter = (adapter_storage_v5_0_0_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    new_adapter->bondable = old_adapter->bondable;
    new_adapter->class_of_device = old_adapter->class_of_device;
    new_adapter->io_capability = old_adapter->io_capability;
    new_adapter->scan_mode = old_adapter->scan_mode;
    strlcpy(new_adapter->name, old_adapter->name, sizeof(new_adapter->name));

    /* transform btbond info */
    new_btbond = (remote_device_properties_v5_0_0_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value);
    for (i = 0; i < prop_items.items[BT_STORAGE_UPDATE_BTBOND_INFO]; ++i) {
        memcpy(&new_btbond->addr, &old_btbond->addr, sizeof(bt_address_t));
        new_btbond->addr_type = old_btbond->addr_type;
        strlcpy(new_btbond->name, old_btbond->name, sizeof(new_btbond->name));
        strlcpy(new_btbond->alias, old_btbond->alias, sizeof(new_btbond->alias));
        new_btbond->class_of_device = old_btbond->class_of_device;
        memcpy(new_btbond->link_key, old_btbond->link_key, 16);
        new_btbond->link_key_type = old_btbond->link_key_type;
        new_btbond->device_type = old_btbond->device_type;
        new_btbond++;
        old_btbond++;
    }

    /* transform blebond info */
    if (prop_items.items[BT_STORAGE_UPDATE_BLEBOND_INFO] > 0) {
        new_lebond = (remote_device_le_properties_v5_0_0_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value);
        memcpy(new_lebond, old_lebond, old_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value_length);
    }

    /* transform whitelist info */
    if (prop_items.items[BT_STORAGE_UPDATE_WHITELIST_INFO] > 0) {
        new_whitelist = (remote_device_le_properties_v5_0_0_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value);
        memcpy(new_whitelist, old_whitelist, old_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value_length);
    }

    return new_storage;
}
