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
#include "bt_storage.h"
#include "storage_update.h"
#include "storage_version_4.h"
#include "storage_version_5.h"
#include "uv_ext.h"

#include "syslog.h"

bt_storage_update_properties_t* bt_storage_load_info_v5_0_0(void)
{
    return bt_storage_load_info_unqlite();
}

bt_storage_update_properties_t* bt_storage_load_info_v5_0_1(void)
{
    return bt_storage_load_info_unqlite();
}

bt_storage_update_properties_t* bt_storage_load_info_v5_0_2(void)
{
    bt_storage_update_properties_t* properties;
    adapter_storage_v5_0_2_t* adapter_info;
    int ret;

    /* load device information */
    properties = bt_storage_load_info_kvdb(BT_STORAGE_VERSION_5_0_2);
    if (!properties) {
        return NULL;
    }

    /* load adapter information */
    adapter_info = (adapter_storage_v5_0_2_t*)(properties->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    ret = property_get_binary(BT_KVDB_ADAPTERINFO_NAME, adapter_info->name, sizeof(adapter_info->name));
    adapter_info->class_of_device = property_get_int32(BT_KVDB_ADAPTERINFO_COD, ERROR_ADAPTERINFO_VALUE);
    adapter_info->io_capability = property_get_int32(BT_KVDB_ADAPTERINFO_IOCAP, ERROR_ADAPTERINFO_VALUE);
    adapter_info->scan_mode = property_get_int32(BT_KVDB_ADAPTERINFO_SCAN, ERROR_ADAPTERINFO_VALUE);
    adapter_info->bondable = property_get_int32(BT_KVDB_ADAPTERINFO_BOND, ERROR_ADAPTERINFO_VALUE);
    if (ret < 0 || adapter_info->class_of_device == ERROR_ADAPTERINFO_VALUE
        || adapter_info->io_capability == ERROR_ADAPTERINFO_VALUE
        || adapter_info->scan_mode == ERROR_ADAPTERINFO_VALUE
        || adapter_info->bondable == ERROR_ADAPTERINFO_VALUE) {
        syslog(LOG_ERR, "adapter info load failed");
        bt_storage_update_properties_free(properties);
        return NULL;
    }

    return properties;
}

bt_storage_update_properties_t* bt_storage_load_info_v5_0_3(void)
{
    bt_storage_update_properties_t* properties;
    adapter_storage_v5_0_3_t* adapter_info;
    int ret, ret1;

    /* load device information */
    properties = bt_storage_load_info_kvdb(BT_STORAGE_VERSION_5_0_3);
    if (!properties) {
        return NULL;
    }

    /* load adapter information */
    adapter_info = (adapter_storage_v5_0_3_t*)(properties->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    ret = property_get_binary(BT_KVDB_ADAPTERINFO_NAME, adapter_info->name, sizeof(adapter_info->name));
    ret1 = property_get_binary(BT_KVDB_ADAPTERINFO_IRK, adapter_info->irk, sizeof(adapter_info->irk));
    adapter_info->class_of_device = property_get_int32(BT_KVDB_ADAPTERINFO_COD, ERROR_ADAPTERINFO_VALUE);
    adapter_info->io_capability = property_get_int32(BT_KVDB_ADAPTERINFO_IOCAP, ERROR_ADAPTERINFO_VALUE);
    adapter_info->scan_mode = property_get_int32(BT_KVDB_ADAPTERINFO_SCAN, ERROR_ADAPTERINFO_VALUE);
    adapter_info->bondable = property_get_int32(BT_KVDB_ADAPTERINFO_BOND, ERROR_ADAPTERINFO_VALUE);
    if (ret < 0 || ret1 < 0 || adapter_info->class_of_device == ERROR_ADAPTERINFO_VALUE
        || adapter_info->io_capability == ERROR_ADAPTERINFO_VALUE
        || adapter_info->scan_mode == ERROR_ADAPTERINFO_VALUE
        || adapter_info->bondable == ERROR_ADAPTERINFO_VALUE) {
        syslog(LOG_ERR, "adapter info load failed");
        bt_storage_update_properties_free(properties);
        return NULL;
    }

    return properties;
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

bt_storage_update_properties_t* bt_storage_update_v5_0_0_to_v5_0_1(bt_storage_update_properties_t* old_storage)
{
    bt_storage_update_properties_t* new_storage;
    bt_storage_update_items_t prop_items = { 0 };
    int i;
    /* v5_0_1 storage structure */
    adapter_storage_v5_0_1_t* new_adapter;
    remote_device_properties_v5_0_1_t* new_btbond;
    remote_device_le_properties_v5_0_1_t *new_lebond, *new_whitelist;
    /* v5_0_0 storage structure */
    adapter_storage_v5_0_0_t* old_adapter;
    remote_device_properties_v5_0_0_t* old_btbond;
    remote_device_le_properties_v5_0_0_t *old_lebond, *old_whitelist;

    old_adapter = (adapter_storage_v5_0_0_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    old_btbond = (remote_device_properties_v5_0_0_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value);
    old_lebond = (remote_device_le_properties_v5_0_0_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value);
    old_whitelist = (remote_device_le_properties_v5_0_0_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value);
    for (i = 0; i < BT_STORAGE_UPDATE_ITEM_MAX; ++i) {
        prop_items.items[i] = old_storage->storage_info[i].items;
    }

    /* properties init */
    new_storage = bt_storage_update_properties_malloc(BT_STORAGE_VERSION_5_0_1, &prop_items);
    if (!new_storage) {
        return NULL;
    }

    /* transform adapter info */
    new_adapter = (adapter_storage_v5_0_1_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    memcpy(new_adapter, old_adapter, sizeof(adapter_storage_v5_0_1_t));

    /* transform btbond info */
    new_btbond = (remote_device_properties_v5_0_1_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value);
    for (i = 0; i < prop_items.items[BT_STORAGE_UPDATE_BTBOND_INFO]; ++i) {
        memcpy(new_btbond, old_btbond, sizeof(remote_device_properties_v5_0_0_t));
        memset(new_btbond->uuids, 0, sizeof(new_btbond->uuids));
        new_btbond++;
        old_btbond++;
    }

    /* transform blebond info */
    if (prop_items.items[BT_STORAGE_UPDATE_BLEBOND_INFO] > 0) {
        new_lebond = (remote_device_le_properties_v5_0_1_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value);
        memcpy(new_lebond, old_lebond, old_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value_length);
    }

    /* transform whitelist info */
    if (prop_items.items[BT_STORAGE_UPDATE_WHITELIST_INFO] > 0) {
        new_whitelist = (remote_device_le_properties_v5_0_1_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value);
        memcpy(new_whitelist, old_whitelist, old_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value_length);
    }

    return new_storage;
}

bt_storage_update_properties_t* bt_storage_update_v5_0_1_to_v5_0_2(bt_storage_update_properties_t* old_storage)
{
    bt_storage_update_properties_t* new_storage;
    bt_storage_update_items_t prop_items = { 0 };
    int i;
    /* v5_0_1 storage structure */
    adapter_storage_v5_0_2_t* new_adapter;
    remote_device_properties_v5_0_2_t* new_btbond;
    remote_device_le_properties_v5_0_2_t *new_lebond, *new_whitelist;
    /* v5_0_0 storage structure */
    adapter_storage_v5_0_1_t* old_adapter;
    remote_device_properties_v5_0_1_t* old_btbond;
    remote_device_le_properties_v5_0_1_t *old_lebond, *old_whitelist;

    old_adapter = (adapter_storage_v5_0_1_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    old_btbond = (remote_device_properties_v5_0_1_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value);
    old_lebond = (remote_device_le_properties_v5_0_1_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value);
    old_whitelist = (remote_device_le_properties_v5_0_1_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value);
    for (i = 0; i < BT_STORAGE_UPDATE_ITEM_MAX; ++i) {
        prop_items.items[i] = old_storage->storage_info[i].items;
    }

    /* properties init */
    new_storage = bt_storage_update_properties_malloc(BT_STORAGE_VERSION_5_0_2, &prop_items);
    if (!new_storage) {
        return NULL;
    }

    /* transform adapter info */
    new_adapter = (adapter_storage_v5_0_2_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    new_adapter->bondable = old_adapter->bondable;
    new_adapter->class_of_device = old_adapter->class_of_device;
    new_adapter->io_capability = old_adapter->io_capability;
    new_adapter->scan_mode = old_adapter->scan_mode;
    strlcpy(new_adapter->name, old_adapter->name, sizeof(new_adapter->name));

    /* transform btbond info */
    new_btbond = (remote_device_properties_v5_0_2_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value);
    for (i = 0; i < prop_items.items[BT_STORAGE_UPDATE_BTBOND_INFO]; ++i) {
        memcpy(&new_btbond->addr, &old_btbond->addr, sizeof(bt_address_t));
        new_btbond->addr_type = old_btbond->addr_type;
        strlcpy(new_btbond->name, old_btbond->name, sizeof(new_btbond->name));
        strlcpy(new_btbond->alias, old_btbond->alias, sizeof(new_btbond->alias));
        new_btbond->class_of_device = old_btbond->class_of_device;
        memcpy(new_btbond->link_key, old_btbond->link_key, 16);
        new_btbond->link_key_type = old_btbond->link_key_type;
        new_btbond->device_type = old_btbond->device_type;
        memcpy(new_btbond->uuids, old_btbond->uuids, sizeof(old_btbond->uuids));
        new_btbond++;
        old_btbond++;
    }

    /* transform blebond info */
    new_lebond = (remote_device_le_properties_v5_0_2_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value);
    for (i = 0; i < prop_items.items[BT_STORAGE_UPDATE_BLEBOND_INFO]; ++i) {
        memcpy(&new_lebond->addr, &old_lebond->addr, sizeof(bt_address_t));
        new_lebond->addr_type = old_lebond->addr_type;
        new_lebond->device_type = old_lebond->device_type;
        memcpy(new_lebond->smp_key, old_lebond->smp_key, 80);
        new_lebond++;
        old_lebond++;
    }

    /* transform whitelist info */
    new_whitelist = (remote_device_le_properties_v5_0_2_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value);
    for (i = 0; i < prop_items.items[BT_STORAGE_UPDATE_WHITELIST_INFO]; ++i) {
        memcpy(&new_whitelist->addr, &old_whitelist->addr, sizeof(bt_address_t));
        new_whitelist->addr_type = old_whitelist->addr_type;
        new_whitelist->device_type = old_whitelist->device_type;
        memcpy(new_whitelist->smp_key, old_whitelist->smp_key, 80);
        new_whitelist++;
        old_whitelist++;
    }

    return new_storage;
}

bt_storage_update_properties_t* bt_storage_update_v5_0_2_to_v5_0_3(bt_storage_update_properties_t* old_storage)
{
    bt_storage_update_properties_t* new_storage;
    bt_storage_update_items_t prop_items = { 0 };
    int i;
    /* v5_0_3 storage structure */
    adapter_storage_v5_0_3_t* new_adapter;
    remote_device_properties_v5_0_3_t* new_btbond;
    remote_device_le_properties_v5_0_3_t *new_lebond, *new_whitelist;
    /* v5_0_2 storage structure */
    adapter_storage_v5_0_2_t* old_adapter;
    remote_device_properties_v5_0_2_t* old_btbond;
    remote_device_le_properties_v5_0_2_t *old_lebond, *old_whitelist;

    old_adapter = (adapter_storage_v5_0_2_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    old_btbond = (remote_device_properties_v5_0_2_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value);
    old_lebond = (remote_device_le_properties_v5_0_2_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value);
    old_whitelist = (remote_device_le_properties_v5_0_2_t*)(old_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value);
    for (i = 0; i < BT_STORAGE_UPDATE_ITEM_MAX; ++i) {
        prop_items.items[i] = old_storage->storage_info[i].items;
    }

    /* properties init */
    new_storage = bt_storage_update_properties_malloc(BT_STORAGE_VERSION_5_0_3, &prop_items);
    if (!new_storage) {
        return NULL;
    }

    /* transform adapter info */
    new_adapter = (adapter_storage_v5_0_3_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    memcpy(new_adapter, old_adapter, sizeof(adapter_storage_v5_0_2_t));
    /* TODO: transform local_irk */

    /* transform btbond info */
    if (prop_items.items[BT_STORAGE_UPDATE_BTBOND_INFO] > 0) {
        new_btbond = (remote_device_properties_v5_0_3_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value);
        memcpy(new_btbond, old_btbond, old_storage->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value_length);
    }

    /* transform blebond info */
    new_lebond = (remote_device_le_properties_v5_0_3_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value);
    for (i = 0; i < prop_items.items[BT_STORAGE_UPDATE_BLEBOND_INFO]; ++i) {
        memcpy(new_lebond, old_lebond, sizeof(remote_device_le_properties_v5_0_2_t));
        /* TODO: transform local_csrk */
        new_lebond++;
        old_lebond++;
    }

    /* transform whitelist info */
    new_whitelist = (remote_device_le_properties_v5_0_3_t*)(new_storage->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value);
    for (i = 0; i < prop_items.items[BT_STORAGE_UPDATE_WHITELIST_INFO]; ++i) {
        memcpy(new_whitelist, old_whitelist, sizeof(remote_device_le_properties_v5_0_2_t));
        /* TODO: transform local_csrk */
        new_whitelist++;
        old_whitelist++;
    }

    /* No raw material conversion GATT HASH */

    return new_storage;
}
