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
#include "service_loop.h"
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

typedef void (*kvdb_callback_t)(const char* name, const char* value, void* cookie);

typedef struct {
    char* key;
    kvdb_callback_t cb;
} bt_storage_update_kvdb_callback_t;

typedef struct {
    const void* key;
    uint16_t items;
    uint16_t offset;
    uint32_t value_length;
    void* value;
} bt_property_value_t;

static void callback_adapter_count(const char* name, const char* value, void* count_u16);
static void callback_bt_count(const char* name, const char* value, void* count_u16);
static void callback_le_count(const char* name, const char* value, void* count_u16);
static void callback_whitelist_count(const char* name, const char* value, void* count_u16);
static void callback_gatt_dbhash_count(const char* name, const char* value, void* count_u16);

static uv_db_t* storage_handle = NULL;

static int bt_storage_update_item_size[BT_STORAGE_VERSION_MAX][BT_STORAGE_UPDATE_ITEM_MAX] = {
/*  { Adapter Info size, BTbonded Info size, BLEbonded Info size, WhiteList Info size }  */
#ifdef BLUETOOTH_STORAGE_VERSION_4
    { sizeof(adapter_storage_v4_0_0_t),
        sizeof(remote_device_properties_v4_0_0_t),
        sizeof(remote_device_le_properties_v4_0_0_t),
        sizeof(remote_device_le_properties_v4_0_0_t),
        0 },
#endif
#ifdef BLUETOOTH_STORAGE_VERSION_5
    { sizeof(adapter_storage_v5_0_0_t),
        sizeof(remote_device_properties_v5_0_0_t),
        sizeof(remote_device_le_properties_v5_0_0_t),
        sizeof(remote_device_le_properties_v5_0_0_t),
        0 },
    /* version 5_0_1 */
    { sizeof(adapter_storage_v5_0_1_t),
        sizeof(remote_device_properties_v5_0_1_t),
        sizeof(remote_device_le_properties_v5_0_1_t),
        sizeof(remote_device_le_properties_v5_0_1_t),
        0 },
    /* version 5_0_2 */
    { sizeof(adapter_storage_v5_0_2_t),
        sizeof(remote_device_properties_v5_0_2_t),
        sizeof(remote_device_le_properties_v5_0_2_t),
        sizeof(remote_device_le_properties_v5_0_2_t),
        0 },
    /* version 5_0_3 */
    {
        sizeof(adapter_storage_v5_0_3_t),
        sizeof(remote_device_properties_v5_0_3_t),
        sizeof(remote_device_le_properties_v5_0_3_t),
        sizeof(remote_device_le_properties_v5_0_3_t),
        sizeof(remote_device_gatt_properties_v5_0_3_t) },
#endif
    /*   Reserve for future version   */
};

const static bt_storage_update_kvdb_callback_t callback_cnt_list[BT_STORAGE_UPDATE_ITEM_MAX] = {
    { BT_KVDB_ADAPTERINFO, callback_adapter_count },
    { BT_KVDB_BTBOND, callback_bt_count },
    { BT_KVDB_BLEBOND, callback_le_count },
    { BT_KVDB_BLEWHITELIST, callback_whitelist_count },
    { BT_KVDB_BLEGATTDBHASH, callback_gatt_dbhash_count },
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
#ifdef BLUETOOTH_STORAGE_VERSION_5
    bt_storage_update_v5_0_0_to_v5_0_1,
    bt_storage_update_v5_0_1_to_v5_0_2,
    bt_storage_update_v5_0_2_to_v5_0_3,
#endif
    /*   Reserve for future version   */
};

/****************************************************************************
 * Unqlite storage save function
 ****************************************************************************/
static int bt_storage_save_storage_sync_unqlite(const char* key, void* data, int length)
{
    uv_buf_t buf;
    int ret;

    buf = uv_buf_init((char*)data, length);
    ret = uv_db_set(storage_handle, key, &buf, NULL, NULL);
    if (ret != 0) {
        syslog(LOG_ERR, "key %s set error:%d", key, ret);
        return ret;
    }

    syslog(LOG_DEBUG, "key %s set success:%d", key, ret);
    uv_db_commit(storage_handle);
    return ret;
}

int bt_storage_save_item_unqlite(void* data, int items, int version, int storage_item)
{
    key_header_t* header;
    int total_len, ret;

    total_len = items * bt_storage_update_item_size[version][storage_item];
    header = zalloc(sizeof(key_header_t) + total_len);
    if (!header) {
        syslog(LOG_ERR, "%s key malloc failed\n", __func__);
        return -1;
    }

    header->items = items;
    header->key_length = total_len;
    if (data && items)
        memcpy(header->key_value, data, total_len);

    ret = bt_storage_save_storage_sync_unqlite(unqlite_item_key[storage_item], header, sizeof(key_header_t) + total_len);
    free(header);

    return ret;
}

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
 * KVDB storage save function
 ****************************************************************************/
static int bt_storage_save_storage_kvdb(const char* key, void* data, int item_len, int num)
{
    char *prop_name, *tmp_data;
    bt_address_t addr;
    int i, ret = 0;
    size_t prop_vlen;

    if (!key || !data)
        return 0;

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        syslog(LOG_ERR, "property_name malloc failed!");
        return -ENOMEM;
    }

    tmp_data = (char*)data;
    prop_vlen = item_len - BT_ADDR_LENGTH;
    for (i = 0; i < num; i++) {
        memcpy(addr.addr, tmp_data, BT_ADDR_LENGTH);
        GEN_PROP_KEY(prop_name, key, &addr, PROP_NAME_MAX);
        /**
         * Note: It should be ensured that "addr" is the first member of the struct remote_device_le_properties_t
         * and "addr_type" is the second member.
         * */
        ret = property_set_binary(prop_name, tmp_data + BT_ADDR_LENGTH, prop_vlen, false);
        if (ret < 0) {
            syslog(LOG_ERR, "key %s set error!", prop_name);
            free(prop_name);
            return ret;
        }

        tmp_data += item_len;
    }

    property_commit();
    free(prop_name);
    return ret;
}

int bt_storage_save_item_kvdb(void* data, int items, int version, int storage_item)
{
    int ret, load_num = 0;
    char* prop_name;

    if (storage_item == BT_STORAGE_UPDATE_ADAPTER_INFO) {
        syslog(LOG_INFO, "adapter_info not use this function");
        return -1;
    }

    ret = property_list(callback_cnt_list[storage_item].cb, &load_num);
    syslog(LOG_DEBUG, "bt_storage_save_item_kvdb [%s] load_num = %d", callback_cnt_list[storage_item].key, load_num);
    if (ret < 0) {
        syslog(LOG_ERR, "property_list [%d] failed!, ret = %d", storage_item, ret);
        return ret;
    }

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        syslog(LOG_ERR, "property_name malloc failed!");
        return -ENOMEM;
    }

    bt_storage_delete(callback_cnt_list[storage_item].key, load_num, prop_name);
    free(prop_name);

    ret = bt_storage_save_storage_kvdb(callback_cnt_list[storage_item].key, data, bt_storage_update_item_size[version][storage_item], items);
    if (ret < 0) {
        syslog(LOG_ERR, "bt_storage_save_storage_kvdb [%d] failed!", storage_item);
        return ret;
    }

    syslog(LOG_DEBUG, "bt_storage_save_item_kvdb [%s] success!", callback_cnt_list[storage_item].key);
    return ret;
}

/****************************************************************************
 * KVDB storage load function
 ****************************************************************************/
static void callback_adapter_count(const char* name, const char* value, void* count_u16)
{
    if (!strncmp(name, BT_KVDB_ADAPTERINFO, strlen(BT_KVDB_ADAPTERINFO))) {
        (*(uint16_t*)count_u16)++;
    }
}

static void callback_bt_count(const char* name, const char* value, void* count_u16)
{
    if (!strncmp(name, BT_KVDB_BTBOND, strlen(BT_KVDB_BTBOND))) {
        (*(uint16_t*)count_u16)++;
    }
}

static void callback_le_count(const char* name, const char* value, void* count_u16)
{
    if (!strncmp(name, BT_KVDB_BLEBOND, strlen(BT_KVDB_BLEBOND))) {
        (*(uint16_t*)count_u16)++;
    }
}

static void callback_whitelist_count(const char* name, const char* value, void* count_u16)
{
    if (!strncmp(name, BT_KVDB_BLEWHITELIST, strlen(BT_KVDB_BLEWHITELIST))) {
        (*(uint16_t*)count_u16)++;
    }
}

static void callback_gatt_dbhash_count(const char* name, const char* value, void* count_u16)
{
    if (!strncmp(name, BT_KVDB_BLEGATTDBHASH, strlen(BT_KVDB_BLEGATTDBHASH))) {
        (*(uint16_t*)count_u16)++;
    }
}

static void callback_load_addr(const char* name, const char* value, void* cookie)
{
    bt_property_value_t* prop_value = (bt_property_value_t*)cookie;
    char addr_str[BT_ADDR_STR_LENGTH];
    bt_address_t* addr;

    if (strncmp(name, prop_value->key, strlen(prop_value->key)))
        return;

    assert(prop_value->offset < prop_value->items);
    addr = (bt_address_t*)prop_value->value + prop_value->offset * prop_value->value_length;
    PARSE_PROP_KEY(addr_str, name, strlen((char*)prop_value->key), BT_ADDR_STR_LENGTH, addr);
    prop_value->offset++;
}

static int bt_storage_load_storage_kvdb(const char* key, bt_storage_update_value_t* prop_value, int item_len)
{
    bt_property_value_t* value;
    int i, prop_size;
    char* prop_name;
    bt_address_t* addr;
    char* storage_value;

    if (!prop_value)
        return -1;

    value = (bt_property_value_t*)zalloc(sizeof(bt_property_value_t));
    if (!value) {
        syslog(LOG_ERR, "%s value malloc failed\n", __func__);
        return -1;
    }

    value->items = prop_value->items;
    value->offset = 0;
    value->key = key;
    value->value_length = item_len;
    value->value = prop_value->value;

    property_list(callback_load_addr, value); // get addr to generate property name
    free(value);

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        syslog(LOG_ERR, "property_name malloc failed!");
        return -ENOMEM;
    }

    for (i = 0; i < prop_value->items; i++) {
        addr = (bt_address_t*)((char*)prop_value->value + i * item_len); // first 6 Bytes is address.
        storage_value = (char*)addr + sizeof(bt_address_t);
        GEN_PROP_KEY(prop_name, key, addr, PROP_NAME_MAX);
        /**
         * Note: It should be ensured that "addr" is the first member of the struct remote_device_properties_t
         * and "addr_type" is the second member.
         * */
        prop_size = property_get_binary(prop_name, storage_value, PROP_VALUE_MAX);
        if (prop_size < 0) {
            syslog(LOG_ERR, "property_get_binary failed!");
            free(prop_name);
            return -1;
        }
    }

    free(prop_name);
    return 0;
}

bt_storage_update_properties_t* bt_storage_load_info_kvdb(int version)
{
    bt_storage_update_properties_t* properties;
    int ret, i, item_len;
    bt_storage_update_items_t prop_items = { 0 };

    prop_items.items[BT_STORAGE_UPDATE_ADAPTER_INFO] = 1;
    for (i = BT_STORAGE_UPDATE_BTBOND_INFO; i < BT_STORAGE_UPDATE_ITEM_MAX; i++) {
        if (!callback_cnt_list[i].cb)
            continue;

        ret = property_list(callback_cnt_list[i].cb, &prop_items.items[i]);
        if (ret < 0) {
            syslog(LOG_ERR, "property_list [%d] failed, ret = %d", i, ret);
            return NULL;
        }
    }

    properties = bt_storage_update_properties_malloc(version, &prop_items);
    if (!properties) {
        return NULL;
    }

    for (i = BT_STORAGE_UPDATE_BTBOND_INFO; i < BT_STORAGE_UPDATE_ITEM_MAX; i++) {
        if (prop_items.items[i] > 0) {
            item_len = bt_storage_update_item_size[version][i];
            ret = bt_storage_load_storage_kvdb(callback_cnt_list[i].key, &properties->storage_info[i], item_len);
            if (ret < 0)
                goto error;
        }
    }

    return properties;

error:
    bt_storage_update_properties_free(properties);
    return NULL;
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
    int cur_version;
    int ret;

    /* load bonded device info */
    ret = bt_storage_load_bonded_device_unqlite((void**)&tmp_value, &tmp_value_length);
    if (ret) {
        syslog(LOG_INFO, "%s bt bonded load fail:, ret = %d\n", __func__, ret);
        /* may not bond infomation, judge adapter info */
        goto load_adapter;
    }

    if (tmp_value->key_length == (sizeof(remote_device_properties_v4_0_0_t) * tmp_value->items)) {
        cur_version = BT_STORAGE_VERSION_4_0_0;
    } else if (tmp_value->key_length == (sizeof(remote_device_properties_v5_0_0_t) * tmp_value->items)) {
        cur_version = BT_STORAGE_VERSION_5_0_0;
    } else if (tmp_value->key_length == (sizeof(remote_device_properties_v5_0_1_t) * tmp_value->items)) {
        cur_version = BT_STORAGE_VERSION_5_0_1;
    } else {
        syslog(LOG_ERR, "%s unknown version\n", __func__);
        cur_version = -1;
    }

    free(tmp_value);
    return cur_version;

load_adapter:
    ret = bt_storage_load_adapter_info_unqlite((void**)&tmp_value, &tmp_value_length);
    if (ret) {
        syslog(LOG_INFO, "%s adapter load fail, ret = %d\n", __func__, ret);
        return -1;
    }

    if (tmp_value->key_length == (sizeof(adapter_storage_v4_0_0_t) * tmp_value->items)) {
        cur_version = BT_STORAGE_VERSION_4_0_0;
    } else if (tmp_value->key_length == (sizeof(adapter_storage_v5_0_1_t) * tmp_value->items)) {
        /* version 5_0_0 equal version 5_0_1, goto the latest version*/
        cur_version = BT_STORAGE_VERSION_5_0_1;
    } else {
        syslog(LOG_ERR, "%s unknown version\n", __func__);
        cur_version = -1;
    }

    free(tmp_value);
    return cur_version;
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

#ifdef BLUETOOTH_STORAGE_VERSION_5
    if (!strncasecmp(version_str, "v5_0_2", strlen(version_str))) {
        return BT_STORAGE_VERSION_5_0_2;
    } else if (!strncasecmp(version_str, "v5_0_3", strlen(version_str))) {
        return BT_STORAGE_VERSION_5_0_3;
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
    if (!bt_storage_update_kvdb_check())
        ret = bt_storage_properties_destory();
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
    bt_storage_update_properties_t *old_storage, *new_storage = NULL;
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
    case BT_STORAGE_VERSION_5_0_1:
        storage_info = bt_storage_load_info_v5_0_1();
        break;
    case BT_STORAGE_VERSION_5_0_2:
        storage_info = bt_storage_load_info_v5_0_2();
        break;
    case BT_STORAGE_VERSION_5_0_3:
        storage_info = bt_storage_load_info_v5_0_3();
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
    bt_storage_save_gatt_cache_device(
        (remote_device_gatt_properties_t*)storage_info->storage_info[BT_STORAGE_UPDATE_GATT_HASH_INFO].value,
        storage_info->storage_info[BT_STORAGE_UPDATE_GATT_HASH_INFO].items);

    return 0;
}

static int bt_storage_update_process(int storage_version)
{
    bt_storage_update_properties_t *storage_info, *updated_info = NULL;

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
    syslog(LOG_DEBUG, "%s, handle: %p", __func__, storage_handle);
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
    ret = bt_storage_unqlite_init();
#endif

    return ret;
}

static void bt_storage_update_cleanup(void)
{
    syslog(LOG_INFO, __func__);

#if defined(BLUETOOTH_STORAGE_VERSION_4) || defined(BLUETOOTH_STORAGE_VERSION_5)
    bt_storage_unqlite_cleanup();
    if (!access(BT_STORAGE_FILE_PATH, F_OK)) {
        unlink(BT_STORAGE_FILE_PATH);
    }

    syslog(LOG_INFO, "uv_loop_close\n");
    uv_loop_close(get_service_uv_loop());
    syslog(LOG_INFO, "uv_library_shutdown\n");
    uv_library_shutdown();
#endif
}

int main(int argc, char** argv)
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
