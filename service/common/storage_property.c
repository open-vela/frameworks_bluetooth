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

#define LOG_TAG "storage_property"
#include <nuttx/crc16.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "bluetooth_define.h"
#include "service_loop.h"
#include "storage.h"
#include "utils/log.h"
#include "uv_ext.h"

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

#define BT_KVDB_ADAPTERINFO "persist.bluetooth.adapterInfo."
#define BT_KVDB_BTBOND "persist.bluetooth.btbonded."
#define BT_KVDB_BLEBOND "persist.bluetooth.blebonded."
#define BT_KVDB_BLEWHITELIST "persist.bluetooth.whitelist."

typedef struct {
    void* key;
    uint16_t items;
    uint16_t offset;
    uint32_t value_length;
    uint8_t value[0];
} bt_property_value_t;

static void storage_commit(service_work_t* work, void* userdata)
{
    property_commit();
}

static int storage_set_key(const char* key, void* data, size_t length)
{
    int ret;

    ret = property_set_binary(key, data, length, true);
    if (ret < 0) {
        BT_LOGE("key %s set error!", key);
        return ret;
    }
    service_loop_work(NULL, storage_commit, NULL);
    return ret;
}

static void callback_bt_load_addr(const char* name, const char* value, void* cookie)
{
    char addr_str[BT_ADDR_STR_LENGTH];
    bt_property_value_t* prop_value;
    remote_device_properties_t* remote;

    prop_value = (bt_property_value_t*)cookie;
    if (strncmp(name, (char*)prop_value->key, strlen(prop_value->key)))
        return;

    assert(prop_value->offset < prop_value->items);
    remote = (remote_device_properties_t*)prop_value->value + prop_value->offset;

    PARSE_PROP_KEY(addr_str, name, strlen((char*)prop_value->key), BT_ADDR_STR_LENGTH, &remote->addr);
    prop_value->offset++;
}

static void callback_le_load_addr(const char* name, const char* value, void* cookie)
{
    char addr_str[BT_ADDR_STR_LENGTH];
    bt_property_value_t* prop_value;
    remote_device_le_properties_t* remote;

    prop_value = (bt_property_value_t*)cookie;
    if (strncmp(name, (char*)prop_value->key, strlen(prop_value->key)))
        return;

    assert(prop_value->offset < prop_value->items);
    remote = (remote_device_le_properties_t*)prop_value->value + prop_value->offset;

    PARSE_PROP_KEY(addr_str, name, strlen((char*)prop_value->key), BT_ADDR_STR_LENGTH, &remote->addr);
    prop_value->offset++;
}

static void storage_get_key(const char* key, void* data, uint16_t value_len, void* cookie)
{
    bt_property_value_t* prop_value;
    remote_device_properties_t* bt_remote;
    remote_device_le_properties_t* le_remote;
    bt_address_t* addr;
    size_t prop_size;
    char* prop_name;
    int i;

    if (!key || !data)
        return;

    if (!strncmp(key, BT_KVDB_ADAPTERINFO, strlen(BT_KVDB_ADAPTERINFO))) {
        adapter_storage_t* adapter = (adapter_storage_t*)data;
        adapter->class_of_device = property_get_int32(BT_KVDB_ADAPTERINFO_COD, ERROR_ADAPTERINFO_VALUE);
        adapter->io_capability = property_get_int32(BT_KVDB_ADAPTERINFO_IOCAP, ERROR_ADAPTERINFO_VALUE);
        adapter->scan_mode = property_get_int32(BT_KVDB_ADAPTERINFO_SCAN, ERROR_ADAPTERINFO_VALUE);
        adapter->bondable = property_get_int32(BT_KVDB_ADAPTERINFO_BOND, ERROR_ADAPTERINFO_VALUE);
        return;
    }

    prop_value = (bt_property_value_t*)data;
    if (prop_value->items == 0) {
        ((load_storage_callback_t)cookie)(NULL, 0, 0);
        return;
    }

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        BT_LOGE("property_name malloc failed!");
        return;
    }

    if (!strncmp(key, BT_KVDB_BTBOND, strlen(BT_KVDB_BTBOND))) {
        property_list(callback_bt_load_addr, data); // get addr to generate property name
        for (i = 0; i < prop_value->items; i++) {
            bt_remote = (remote_device_properties_t*)prop_value->value + i;
            addr = &bt_remote->addr;
            GEN_PROP_KEY(prop_name, key, addr, PROP_NAME_MAX);
            /**
             * Note: It should be ensured that "addr" is the first member of the struct remote_device_properties_t
             * and "addr_type" is the second member.
             * */
            prop_size = sizeof(remote_device_properties_t) - offsetof(remote_device_properties_t, addr_type);
            property_get_binary(prop_name, &bt_remote->addr_type, prop_size);
        }
    } else { /*!BT_KVDB_BTBOND*/
        property_list(callback_le_load_addr, data); // get addr to generate property name
        for (i = 0; i < prop_value->items; i++) {
            le_remote = (remote_device_le_properties_t*)prop_value->value + i;
            addr = &le_remote->addr;
            GEN_PROP_KEY(prop_name, key, addr, PROP_NAME_MAX);
            /**
             * Note: It should be ensured that "addr" is the first member of the struct remote_device_le_properties_t
             * and "addr_type" is the second member.
             * */
            prop_size = sizeof(remote_device_le_properties_t) - offsetof(remote_device_le_properties_t, addr_type);
            property_get_binary(prop_name, &le_remote->addr_type, prop_size);
        }
    }
    ((load_storage_callback_t)cookie)(prop_value->value, value_len, prop_value->items);
    free(prop_name);
}

static void adapter_properties_default(adapter_storage_t* prop)
{
    srand(time(NULL));
    memset(prop->name, 0, sizeof(prop->name));
    snprintf(prop->name, sizeof(prop->name), "%s-%03X", "XIAOMI VELA", rand() % 999);
    prop->class_of_device = DEFAULT_DEVICE_OF_CLASS;
    prop->io_capability = DEFAULT_IO_CAPABILITY;
    prop->scan_mode = DEFAULT_SCAN_MODE;
    prop->bondable = DEFAULT_BONDABLE_MODE;
}

int bt_storage_save_adapter_info(adapter_storage_t* adapter)
{
    property_set_binary(BT_KVDB_ADAPTERINFO_NAME, adapter->name, sizeof(adapter->name), true);
    property_set_int32_oneway(BT_KVDB_ADAPTERINFO_COD, adapter->class_of_device);
    property_set_int32_oneway(BT_KVDB_ADAPTERINFO_IOCAP, adapter->io_capability);
    property_set_int32_oneway(BT_KVDB_ADAPTERINFO_SCAN, adapter->scan_mode);
    property_set_int32_oneway(BT_KVDB_ADAPTERINFO_BOND, adapter->bondable);
    service_loop_work(NULL, storage_commit, NULL);
    return 0;
}

int bt_storage_load_adapter_info(adapter_storage_t* adapter)
{
    if (property_get_binary(BT_KVDB_ADAPTERINFO_NAME, adapter->name, sizeof(adapter->name)) > 0) {
        storage_get_key(BT_KVDB_ADAPTERINFO, (void*)adapter, sizeof(adapter_storage_t), NULL);
        if (adapter->class_of_device != ERROR_ADAPTERINFO_VALUE && adapter->io_capability != ERROR_ADAPTERINFO_VALUE
            && adapter->scan_mode != ERROR_ADAPTERINFO_VALUE && adapter->bondable != ERROR_ADAPTERINFO_VALUE)
            return 0;
    }
    BT_LOGE("load default adapter info!");
    adapter_properties_default(adapter);
    bt_storage_save_adapter_info(adapter);

    return 0;
}

static int bt_storage_save_remote_device(const char* key, void* value, uint16_t value_size, uint16_t items)
{
    size_t prop_vlen;
    char* prop_name;
    remote_device_properties_t* data;
    bt_address_t* addr;
    int i;
    int ret;

    if (!key || !value)
        return 0;

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        BT_LOGE("property_name malloc failed!");
        return -ENOMEM;
    }
    data = (remote_device_properties_t*)value;
    prop_vlen = value_size - offsetof(remote_device_properties_t, addr_type);
    for (i = 0; i < items; i++) {
        addr = &data->addr;
        GEN_PROP_KEY(prop_name, key, addr, PROP_NAME_MAX);
        /**
         * Note: It should be ensured that "addr" is the first member of the struct remote_device_properties_t
         * and "addr_type" is the second member.
         * */
        ret = storage_set_key(prop_name, &data->addr_type, prop_vlen);
        if (ret < 0) {
            free(prop_name);
            return ret;
        }
        data++;
    }
    free(prop_name);
    return 0;
}

/*BR_KVDB_BLEBOND or BT_KVDB_BLEWHITELIST*/
static int bt_storage_save_le_remote_device(const char* key, void* value, uint16_t value_size, uint16_t items)
{
    size_t prop_vlen;
    char* prop_name;
    remote_device_le_properties_t* data;
    bt_address_t* addr;
    int i;
    int ret;

    if (!key || !value)
        return 0;

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        BT_LOGE("property_name malloc failed!");
        return -ENOMEM;
    }
    data = (remote_device_le_properties_t*)value;
    prop_vlen = value_size - offsetof(remote_device_le_properties_t, addr_type);
    for (i = 0; i < items; i++) {
        addr = &data->addr;
        GEN_PROP_KEY(prop_name, key, addr, PROP_NAME_MAX);
        /**
         * Note: It should be ensured that "addr" is the first member of the struct remote_device_le_properties_t
         * and "addr_type" is the second member.
         * */
        ret = storage_set_key(prop_name, &data->addr_type, prop_vlen);
        if (ret < 0) {
            free(prop_name);
            return ret;
        }
        data++;
    }
    free(prop_name);
    return 0;
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

static void callback_load_key(const char* name, const char* value, void* cookie)
{
    char addr_str[BT_ADDR_STR_LENGTH];
    bt_property_value_t* prop_value;
    bt_address_t* addr;

    prop_value = (bt_property_value_t*)cookie;

    if (strncmp(name, (char*)prop_value->key, strlen(prop_value->key)))
        return;

    assert(prop_value->offset < prop_value->items);
    addr = (bt_address_t*)prop_value->value + prop_value->offset;
    PARSE_PROP_KEY(addr_str, name, strlen((char*)prop_value->key), BT_ADDR_STR_LENGTH, addr);
    prop_value->offset++;
}

static void bt_storage_delete(char* key, uint16_t items, char* prop_name)
{
    bt_property_value_t* prop_value;
    uint32_t total_length;
    bt_address_t* addr;
    int i;

    if (!key || !prop_name)
        return;

    total_length = items * sizeof(bt_address_t);
    prop_value = malloc(sizeof(bt_property_value_t) + total_length);
    if (!prop_value) {
        BT_LOGE("property malloc failed!");
        return;
    }

    prop_value->key = key;
    prop_value->items = items;
    prop_value->offset = 0;
    prop_value->value_length = total_length;

    property_list(callback_load_key, (void*)prop_value);
    for (i = 0; i < items; i++) {
        addr = (bt_address_t*)prop_value->value + i;
        GEN_PROP_KEY(prop_name, key, addr, PROP_NAME_MAX);
        property_delete(prop_name);
    }

    free(prop_value);
    service_loop_work(NULL, storage_commit, NULL);
}

int bt_storage_save_bonded_device(remote_device_properties_t* remote, uint16_t size)
{
    uint16_t items = 0;
    char* prop_name;
    int ret;

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        BT_LOGE("property_name malloc failed!");
        return -ENOMEM;
    }

    /* remove all BREDR bond device property before save new property*/
    property_list(callback_bt_count, &items);
    bt_storage_delete(BT_KVDB_BTBOND, items, prop_name);

    ret = bt_storage_save_remote_device(BT_KVDB_BTBOND, remote, sizeof(remote_device_properties_t), size);
    if (ret < 0) {
        BT_LOGE("save bonded device failed!");
        items = 0;
        property_list(callback_bt_count, &items);
        bt_storage_delete(BT_KVDB_BTBOND, items, prop_name);
    }

    free(prop_name);
    return ret;
}

int bt_storage_save_whitelist(remote_device_le_properties_t* remote, uint16_t size)
{
    uint16_t items = 0;
    char* prop_name;
    int ret;

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        BT_LOGE("property_name malloc failed!");
        return -ENOMEM;
    }

    /* remove all whitelist device property before save new property*/
    property_list(callback_whitelist_count, &items);
    bt_storage_delete(BT_KVDB_BLEWHITELIST, items, prop_name);

    ret = bt_storage_save_le_remote_device(BT_KVDB_BLEWHITELIST, remote, sizeof(remote_device_le_properties_t), size);
    if (ret < 0) {
        BT_LOGE("save whitelist device failed!");
        items = 0;
        property_list(callback_whitelist_count, &items);
        bt_storage_delete(BT_KVDB_BLEWHITELIST, items, prop_name);
    }

    free(prop_name);
    return ret;
}

int bt_storage_save_le_bonded_device(remote_device_le_properties_t* remote, uint16_t size)
{
    uint16_t items = 0;
    char* prop_name;
    int ret;

    prop_name = (char*)malloc(PROP_NAME_MAX);
    if (!prop_name) {
        BT_LOGE("property_name malloc failed!");
        return -ENOMEM;
    }

    /* remove all BLE bond device property before save new property*/
    property_list(callback_le_count, &items);
    bt_storage_delete(BT_KVDB_BLEBOND, items, prop_name);

    ret = bt_storage_save_le_remote_device(BT_KVDB_BLEBOND, remote, sizeof(remote_device_le_properties_t), size);
    if (ret < 0) {
        BT_LOGE("save LE bonded device failed!");
        items = 0;
        property_list(callback_le_count, &items);
        bt_storage_delete(BT_KVDB_BLEBOND, items, prop_name);
    }

    free(prop_name);
    return ret;
}

int bt_storage_load_bonded_device(load_storage_callback_t cb)
{
    uint16_t items;
    bt_property_value_t* prop_value;
    uint32_t total_length;
    int ret;

    items = 0;
    ret = property_list(callback_bt_count, &items);
    if (ret < 0) {
        BT_LOGE("property_list failed!");
        return ret;
    }

    total_length = items * sizeof(remote_device_properties_t);
    prop_value = malloc(sizeof(bt_property_value_t) + total_length);
    if (!prop_value) {
        BT_LOGE("property malloc failed!");
        return -ENOMEM;
    }

    prop_value->key = BT_KVDB_BTBOND;
    prop_value->items = items;
    prop_value->offset = 0;
    prop_value->value_length = total_length;

    storage_get_key(BT_KVDB_BTBOND, (void*)prop_value, sizeof(remote_device_properties_t), (void*)cb);
    free(prop_value);

    return 0;
}

int bt_storage_load_whitelist_device(load_storage_callback_t cb)
{
    uint16_t items;
    bt_property_value_t* prop_value;
    uint32_t total_length;
    int ret;

    items = 0;
    ret = property_list(callback_whitelist_count, &items);
    if (ret < 0) {
        BT_LOGE("property_list failed!");
        return ret;
    }

    total_length = items * sizeof(remote_device_le_properties_t);
    prop_value = malloc(sizeof(bt_property_value_t) + total_length);
    if (!prop_value) {
        BT_LOGE("property malloc failed!");
        return -ENOMEM;
    }

    prop_value->key = BT_KVDB_BLEWHITELIST;
    prop_value->items = items;
    prop_value->offset = 0;
    prop_value->value_length = total_length;

    storage_get_key(BT_KVDB_BLEWHITELIST, (void*)prop_value, sizeof(remote_device_le_properties_t), (void*)cb);
    free(prop_value);

    return 0;
}

int bt_storage_load_le_bonded_device(load_storage_callback_t cb)
{
    uint16_t items;
    bt_property_value_t* prop_value;
    uint32_t total_length;
    int ret;

    items = 0;
    ret = property_list(callback_le_count, &items);
    if (ret < 0) {
        BT_LOGE("property_list failed!");
        return ret;
    }

    total_length = items * sizeof(remote_device_le_properties_t);
    prop_value = malloc(sizeof(bt_property_value_t) + total_length);
    if (!prop_value) {
        BT_LOGE("property malloc failed!");
        return -ENOMEM;
    }

    prop_value->key = BT_KVDB_BLEBOND;
    prop_value->items = items;
    prop_value->offset = 0;
    prop_value->value_length = total_length;

    storage_get_key(BT_KVDB_BLEBOND, (void*)prop_value, sizeof(remote_device_le_properties_t), (void*)cb);
    free(prop_value);

    return 0;
}

int bt_storage_init(void)
{
    return 0;
}

int bt_storage_cleanup(void)
{
    return 0;
}
