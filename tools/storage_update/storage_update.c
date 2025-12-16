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
#include "service_loop.h"
#include "storage.h"
#include "storage_update.h"
#include "storage_version_4.h"
#include "storage_version_5.h"
#include "uv_ext.h"

#include "syslog.h"

#define BT_STORAGE_FILE_PATH "/data/misc/bt/bt_storage.db"

#define BT_KEY_ADAPTER_INFO "AdapterInfo"
#define BT_KEY_BTBOND "BtBonded"
#define BT_KEY_BLEBOND "BleBonded"
#define BT_KEY_BLEWHITELIST "WhiteList"
#define BT_KEY_BLERESOLVINGLIST "ResolvingList"

static uv_db_t* storage_handle = NULL;

static int bt_storage_update_item_size[BT_STORAGE_VERSION_MAX][BT_STORAGE_UPDATE_ITEM_MAX] = {
/*  { Adapter Info size, BTbonded Info size, BLEbonded Info size, WhiteList Info size }  */
#ifdef BLUETOOTH_STORAGE_VERSION_4
    { sizeof(adapter_storage_v4_0_0_t),
        sizeof(remote_device_properties_v4_0_0_t),
        sizeof(remote_device_le_properties_v4_0_0_t),
        sizeof(remote_device_le_properties_v4_0_0_t) },
#endif
#ifdef BLUETOOTH_STORAGE_VERSION_5
    { sizeof(adapter_storage_v5_0_0_t),
        sizeof(remote_device_properties_v5_0_0_t),
        sizeof(remote_device_le_properties_v5_0_0_t),
        sizeof(remote_device_le_properties_v5_0_0_t) },
#endif
    /*   Reserve for future version   */
};

const static char* unqlite_item_key[BT_STORAGE_UNQLITE_ITEM] = {
    BT_KEY_ADAPTER_INFO,
    BT_KEY_BTBOND,
    BT_KEY_BLEBOND,
    BT_KEY_BLEWHITELIST,
};

const static bt_storage_update_func_t verison_map[] = {
#ifdef BLUETOOTH_STORAGE_VERSION_4
    bt_storage_update_v4_0_0_to_v5_0_0,
#endif
    /*   Reserve for future version   */
};

/****************************************************************************
 * Unqlite storage load function
 ****************************************************************************/
static int bt_storage_load_storage_sync_unqlite(const char* key, void** data, uint16_t* length)
{
    uv_buf_t buf;
    int ret;

    if (!data || !length) {
        syslog(LOG_ERR, "%s invalid data or length\n", __func__);
        return -1;
    }

    buf = uv_buf_init(NULL, 0);
    ret = uv_db_get(storage_handle, key, &buf, NULL, NULL);
    if (ret == 0) {
        *data = buf.base;
        *length = buf.len;
    }

    return ret;
}

int bt_storage_load_adapter_info_unqlite(void** data, uint16_t* length)
{
    return bt_storage_load_storage_sync_unqlite(BT_KEY_ADAPTER_INFO, data, length);
}

int bt_storage_load_bonded_device_unqlite(void** data, uint16_t* length)
{
    return bt_storage_load_storage_sync_unqlite(BT_KEY_BTBOND, data, length);
}

int bt_storage_load_le_bonded_device_unqlite(void** data, uint16_t* length)
{
    return bt_storage_load_storage_sync_unqlite(BT_KEY_BLEBOND, data, length);
}

int bt_storage_load_whitelist_device_unqlite(void** data, uint16_t* length)
{
    return bt_storage_load_storage_sync_unqlite(BT_KEY_BLEWHITELIST, data, length);
}

/****************************************************************************
 * storage properties memory malloc/free
 ****************************************************************************/
void bt_storage_update_properties_free(bt_storage_update_properties_t* properties)
{
    for (int i = 0; i < BT_STORAGE_UPDATE_ITEM_MAX; i++) {
        if (properties->storage_info[i].value)
            free(properties->storage_info[i].value);
    }

    free(properties);
}

bt_storage_update_properties_t* bt_storage_update_properties_malloc(int version, bt_storage_update_items_t* prop_items)
{
    assert(version <= BT_STORAGE_VERISON_CURRENT);

    bt_storage_update_properties_t* properties = NULL;
    int items, value_len;

    properties = zalloc(sizeof(bt_storage_update_properties_t));
    if (!properties) {
        syslog(LOG_ERR, "%s properties malloc failed\n", __func__);
        return NULL;
    }

    for (int i = 0; i < BT_STORAGE_UPDATE_ITEM_MAX; ++i) {
        items = prop_items->items[i];
        if (items == 0)
            continue;

        value_len = items * bt_storage_update_item_size[version][i];
        properties->storage_info[i].items = items;
        properties->storage_info[i].value_length = value_len;
        properties->storage_info[i].value = zalloc(value_len);
        if (!properties->storage_info[i].value) {
            syslog(LOG_ERR, "%s properties[%d] malloc failed\n", __func__, i);
            goto error;
        }
    }

    return properties;

error:
    for (int i = 0; i < BT_STORAGE_UPDATE_ITEM_MAX; i++) {
        if (properties->storage_info[i].value)
            free(properties->storage_info[i].value);
    }

    free(properties);

    return NULL;
}

/****************************************************************************
 * storage update main function
 ****************************************************************************/
#if defined(BLUETOOTH_STORAGE_VERSION_4) || defined(BLUETOOTH_STORAGE_VERSION_5)
static int bt_storage_update_get_version_by_db(void)
{
    key_header_t* tmp_value = NULL;
    uint16_t tmp_value_length;
    int ret;

    /* load bonded device info */
    ret = bt_storage_load_bonded_device_unqlite((void**)&tmp_value, &tmp_value_length);
    if (ret) {
        syslog(LOG_INFO, "%s bt bonded load fail:, ret = %d\n", __func__, ret);
        /* may not bond infomation, judge adapter info */
        goto load_adapter;
    }

    if (tmp_value->key_length == (sizeof(remote_device_properties_v4_0_0_t) * tmp_value->items)) {
        return BT_STORAGE_VERSION_4_0_0;
    } else if (tmp_value->key_length == (sizeof(remote_device_properties_v5_0_0_t) * tmp_value->items)) {
        return BT_STORAGE_VERSION_5_0_0;
    } else {
        syslog(LOG_ERR, "%s unknown version\n", __func__);
        return -1;
    }

load_adapter:
    ret = bt_storage_load_adapter_info_unqlite((void**)&tmp_value, &tmp_value_length);
    if (ret) {
        syslog(LOG_INFO, "%s adapter load fail, ret = %d\n", __func__, ret);
        return -1;
    }

    if (tmp_value->key_length == (sizeof(adapter_storage_v4_0_0_t) * tmp_value->items)) {
        return BT_STORAGE_VERSION_4_0_0;
    } else if (tmp_value->key_length == (sizeof(adapter_storage_v5_0_0_t) * tmp_value->items)) {
        return BT_STORAGE_VERSION_5_0_0;
    }

    syslog(LOG_ERR, "%s unknown version\n", __func__);
    return -1;
}
#endif

int bt_storage_update_get_item_len(int version, int storage_item)
{
    return bt_storage_update_item_size[version][storage_item];
}

int bt_storage_get_version(void)
{
    int ret;
    char version_str[BT_STORAGE_VERSION_STR_LEN + 1] = { 0 };

    ret = property_get_binary(BT_KVDB_VERSION_KEY, version_str, sizeof(version_str));
    if (!ret && access(BT_STORAGE_FILE_PATH, F_OK)) { /* file not exist */
        syslog(LOG_INFO, "storage file not exist\n");
        return -1;
    }
#if defined(BLUETOOTH_STORAGE_VERSION_4) || defined(BLUETOOTH_STORAGE_VERSION_5)
    else if (!access(BT_STORAGE_FILE_PATH, F_OK)) { /* Unqlite storage file exist */
        return bt_storage_update_get_version_by_db();
    }
#endif

    return -1;
}

static bool bt_storage_update_kvdb_check(void)
{
    int ret, i;
    uint16_t cnt = 0;

    for (i = BT_STORAGE_UPDATE_ADAPTER_INFO; i < BT_STORAGE_UPDATE_ITEM_MAX; i++) {
        ret = property_list(callback_cnt_list[i].cb, &cnt);
        if (ret < 0) {
            syslog(LOG_ERR, "property_list %s error!", callback_cnt_list[i].key);
            return false;
        }
    }

    return cnt == 0;
}

int bt_storage_remove(void)
{
    int ret = 0;

    syslog(LOG_INFO, __func__);
    /* delete bt storage properties */
#if defined(BLUETOOTH_STORAGE_VERSION_4) || defined(BLUETOOTH_STORAGE_VERSION_5)
    /* delete db file */
    if (!access(BT_STORAGE_FILE_PATH, F_OK))
        ret = unlink(BT_STORAGE_FILE_PATH);
#endif

    if (ret < 0) {
        syslog(LOG_ERR, "remove storage file failed\n");
        return ret;
    }

    return ret;
}

static bt_storage_update_properties_t* bt_storage_update_handler(void* storage_info, int storage_version, int cur_version)
{
    bt_storage_update_properties_t *old_storage, *new_storage;
    bt_storage_update_func_t func;

    old_storage = (bt_storage_update_properties_t*)storage_info;

    for (int i = storage_version; i < cur_version; i++) {
        func = verison_map[i];
        if (!func)
            continue;

        new_storage = func(old_storage);
        bt_storage_update_properties_free(old_storage);
        if (!new_storage)
            return NULL;

        old_storage = new_storage;
    }

    return new_storage;
}

static bt_storage_update_properties_t* bt_storage_update_load_info(int storage_version)
{
    bt_storage_update_properties_t* storage_info = NULL;

    switch (storage_version) {
#ifdef BLUETOOTH_STORAGE_VERSION_4
    case BT_STORAGE_VERSION_4_0_0:
        storage_info = bt_storage_load_info_v4_0_0();
        break;
#endif
#ifdef BLUETOOTH_STORAGE_VERSION_5
    case BT_STORAGE_VERSION_5_0_0:
        storage_info = bt_storage_load_info_v5_0_0();
        break;
#endif
    default:
        syslog(LOG_ERR, "Unknown storage version.");
        break;
    }

    if (!storage_info) {
        syslog(LOG_ERR, "Load storage info failed.");
        return NULL;
    }

    return storage_info;
}

static int bt_storage_update_save_info(bt_storage_update_properties_t* storage_info)
{
    bt_storage_save_adapter_info(
        (adapter_storage_t*)storage_info->storage_info[BT_STORAGE_UPDATE_ADAPTER_INFO].value);
    bt_storage_save_bonded_device(
        (remote_device_properties_t*)storage_info->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].value,
        storage_info->storage_info[BT_STORAGE_UPDATE_BTBOND_INFO].items);
    bt_storage_save_whitelist(
        (remote_device_le_properties_t*)storage_info->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].value,
        storage_info->storage_info[BT_STORAGE_UPDATE_WHITELIST_INFO].items);
    bt_storage_save_le_bonded_device(
        (remote_device_le_properties_t*)storage_info->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].value,
        storage_info->storage_info[BT_STORAGE_UPDATE_BLEBOND_INFO].items);

    return 0;
}

static int bt_storage_update_process(int storage_version)
{
    bt_storage_update_properties_t *storage_info, *updated_info;

    if (storage_version > BT_STORAGE_VERISON_CURRENT) {
        syslog(LOG_ERR, "Storage version fallback is not supported.");
        goto error;
    }

    /* Step 2-1: load current storage info. */
    storage_info = bt_storage_update_load_info(storage_version);
    if (!storage_info) {
        goto error;
    }

    /* Step 2-2: update storage info (Step-by-step upgrade). */
    updated_info = bt_storage_update_handler(storage_info, storage_version, BT_STORAGE_VERISON_CURRENT);
    if (!updated_info) {
        syslog(LOG_ERR, "Storage update failed.");
        goto error;
    }

    /* Step 2-3: save storage info. */
    bt_storage_update_save_info(updated_info);
    uv_run(get_service_uv_loop(), UV_RUN_DEFAULT); // for properties_commit
    bt_storage_update_properties_free(updated_info);
    return 0;

error:
    bt_storage_remove();
    return -1;
}

int bt_storage_unqlite_init(void)
{
    int ret;

    ret = uv_db_init(get_service_uv_loop(), &storage_handle, BT_STORAGE_FILE_PATH);
    if (ret != 0)
        syslog(LOG_ERR, "%s fail, ret:%d", __func__, ret);

    syslog(LOG_DEBUG, "%s successed", __func__);

    return ret;
}

int bt_storage_unqlite_cleanup(void)
{
    syslog(LOG_DEBUG, "%s", __func__);
    if (storage_handle)
        uv_db_close(storage_handle);

    storage_handle = NULL;
    return 0;
}

static int bt_storage_update_init(void)
{
    int ret = 0;

    syslog(LOG_INFO, __func__);

#if defined(BLUETOOTH_STORAGE_VERSION_4) || defined(BLUETOOTH_STORAGE_VERSION_5)
    if (!access(BT_STORAGE_FILE_PATH, F_OK)) {
        ret = bt_storage_unqlite_init();
    }
#endif

    return ret;
}

static void bt_storage_update_cleanup(void)
{
    syslog(LOG_INFO, __func__);

#if defined(BLUETOOTH_STORAGE_VERSION_4) || defined(BLUETOOTH_STORAGE_VERSION_5)
    if (!access(BT_STORAGE_FILE_PATH, F_OK)) {
        bt_storage_unqlite_cleanup();
        unlink(BT_STORAGE_FILE_PATH);
    }
#endif
}

int main(void)
{
    int storage_version, ret;

    ret = bt_storage_update_init();
    if (ret < 0)
        return -1;

    /* Step 1: Get storage version. Parsing version based on different storage formats*/
    storage_version = bt_storage_get_version();
    if (storage_version < 0) {
        syslog(LOG_INFO, "need not update storage\n");
        /* if version missed but other properties are present, it is considered that same key-values is lost. */
        if (!bt_storage_update_kvdb_check()) {
            syslog(LOG_ERR, "KVDB omission\n");
            bt_storage_remove();
        }
        goto exit;
    }

    if (storage_version == BT_STORAGE_VERISON_CURRENT) {
        syslog(LOG_INFO, "Storage version matches current version\n");
        goto exit;
    }

    /* Step 2: Execute the storage upgrade process. */
    ret = bt_storage_update_process(storage_version);
    if (ret < 0) {
        syslog(LOG_ERR, "Storage update failed\n");
        goto exit;
    }

    /* Step 3: Add storage version info. */
    ret = property_set_binary(BT_KVDB_VERSION_KEY, BT_STORAGE_CURRENT_VERSION, strlen(BT_STORAGE_CURRENT_VERSION) + 1, false);
    if (ret < 0) {
        syslog(LOG_ERR, "key %s set error! ret = %d", BT_STORAGE_CURRENT_VERSION, ret);
        goto exit;
    }

    syslog(LOG_INFO, "Storage update successed\n");

exit:
    bt_storage_update_cleanup();

    return 0;
}
