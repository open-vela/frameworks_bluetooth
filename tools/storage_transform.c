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
#include <inttypes.h>
#include <kvdb.h>

#include "bluetooth_define.h"
#include "storage.h"
#include "syslog.h"
#include "uv_ext.h"

#define BLUETOOTH_V35_UPGRDE_TAG "persist.bluetooth.v35.upgraded"
#define BT_STORAGE_FILE_PATH "/data/misc/bt/bt_storage.db"

// Defined macro in rel-3.5
#define BT_DEVICE_NAME_MAX_LEN 63 // 63 used in rel-3.5
#define SMP_KEYS_MAX_SIZE 80
#define UUID_SIZE 16
#define BD_ADDR_SIZE 6

#define BT_CONFIG_FILE_PATH "/data/misc/bt/bt_config.db"
#define BT_KEY_DEVICE_INFO "key_deviceinfo"
#define BT_KEY_BTBOND "key_btbond"
#define BT_KEY_BLEBOND "key_blebond"
#define BT_KEY_BLEWHITELIST "key_blewhitelist"

// Type dependency for rel-3.5
typedef uint8_t BT_UUID_T[UUID_SIZE];
typedef uint8_t BD_ADDR[BD_ADDR_SIZE];

typedef enum {
    STORAGE_TYPE_BT,
    STORAGE_TYPE_BLE,
} storage_type;

typedef struct {
    BD_ADDR bd_addr;
    uint8_t addr_type;
} SERVICE_REMOTE_BLE_DEVICE_S;

typedef struct
{
    uint8_t bd[6];
    uint8_t atype;
    uint8_t cap;
    uint8_t ltk_len;
    uint8_t div[2];
    uint8_t ltk[16];
    uint8_t ediv[2];
    uint8_t rand[8];
    uint8_t irk[16];
    uint8_t csrk[16];
} smp_keys_parsing_t;

// Storage format in rel-3.5

typedef struct {
    uint32_t size;
    uint32_t check_sum;
    uint8_t spk_volume;
    uint8_t mic_volume;
    uint16_t bonded_number;
    storage_type type;
    void* bonded_devices;
} bt_storage_t;

typedef struct gap_ble_whitelist_data {
    uint32_t size;
    uint32_t num;
    SERVICE_REMOTE_BLE_DEVICE_S* devices;
} gap_ble_whitelist_data;

typedef struct {
    uint32_t cod;
    bt_io_capability_t io_capability; // undefined BT_IO_CAPABILITY_UNKNOW in rel-3.5
    char bt_name[BT_DEVICE_NAME_MAX_LEN];
    bt_scan_mode_t scan_mode;
    bool bondable;
} bt_device_info_t;

typedef struct {
    BD_ADDR bd_addr;
    uint8_t addr_type; // 1 Byte
    char bt_name[BT_DEVICE_NAME_MAX_LEN + 1];
    BT_UUID_T uuids[10];
    uint8_t link_key[16];
    uint8_t link_key_type; // 1 Byte
    uint16_t device_volume; // 2 Bytes
    uint32_t device_profile; // 4 Bytes
    uint32_t cod;
    uint8_t device_type;
    uint8_t rssi;
} SERVICE_REMOTE_DEVICE_S;

typedef struct {
    uint8_t smp_keys[SMP_KEYS_MAX_SIZE];
} ble_keys_t;

typedef struct {
    uv_loop_t* loop;
    uv_db_t* read_db;
} bt_storage_transformer_t;

static void transform_deviceinfo_to_adapterinfo(bt_device_info_t* device_info, adapter_storage_t* adapter_info)
{
    syslog(LOG_INFO, __func__);
    strlcpy(adapter_info->name, device_info->bt_name, sizeof(adapter_info->name)); // shorter name
    adapter_info->class_of_device = device_info->cod;
    adapter_info->io_capability = device_info->io_capability;
    adapter_info->scan_mode = device_info->scan_mode;
    adapter_info->bondable = device_info->bondable;
}

static void transform_btbond_to_deviceproperty(SERVICE_REMOTE_DEVICE_S* remote_device, remote_device_properties_t* device_property)
{
    syslog(LOG_INFO, __func__);
    bt_addr_set(&device_property->addr, remote_device->bd_addr);
    device_property->addr_type = remote_device->addr_type;
    strlcpy(device_property->name, remote_device->bt_name, sizeof(device_property->name));
    memset(&device_property->alias, 0, sizeof(device_property->alias));
    device_property->class_of_device = remote_device->cod;
    memcpy(device_property->link_key, remote_device->link_key, 16);
    device_property->link_key_type = remote_device->link_key_type;
    device_property->device_type = remote_device->device_type + 1; // +1 for adaption
}

static void transform_blebond_to_deviceleproperty(ble_keys_t* remote_device, remote_device_le_properties_t* device_property)
{
    smp_keys_parsing_t* smp_keys = (smp_keys_parsing_t*)remote_device->smp_keys;

    syslog(LOG_INFO, __func__);
    bt_addr_set(&device_property->addr, smp_keys->bd);
    device_property->addr_type = smp_keys->atype; // The correspondence of this field has not been confirmed.
    memcpy(device_property->smp_key, smp_keys, sizeof(device_property->smp_key));
    device_property->device_type = 0; // unknown
}

static void transform_blewhitelist_to_deviceleproperty(SERVICE_REMOTE_BLE_DEVICE_S* remote_device, remote_device_le_properties_t* device_property)
{
    syslog(LOG_INFO, __func__);
    bt_addr_set(&device_property->addr, remote_device->bd_addr);
    device_property->addr_type = remote_device->addr_type;
}

static void load_device_info_cb(int status, const char* key, uv_buf_t value, void* cookie)
{
    adapter_storage_t adapter_info;

    syslog(LOG_INFO, __func__);
    if (status != 0) {
        syslog(LOG_WARNING, "adapter info load failed, status is %d\n", status); // There is no adapter info in rel-3.5 in default
        return;
    }

    if (value.len != sizeof(bt_device_info_t)) {
        syslog(LOG_ERR, "adapter info load error\n");
        return;
    }

    transform_deviceinfo_to_adapterinfo((bt_device_info_t*)value.base, &adapter_info);
    bt_storage_save_adapter_info(&adapter_info);
}

static void load_btbond_cb(int status, const char* key, uv_buf_t value, void* cookie)
{
    uint16_t i;
    bt_storage_t* bt_storage;
    SERVICE_REMOTE_DEVICE_S* device;
    remote_device_properties_t* device_properties;

    syslog(LOG_INFO, __func__);
    if (status != 0) {
        syslog(LOG_WARNING, "bt bonded device load failed, status is %d\n", status);
        return;
    }

    bt_storage = (bt_storage_t*)(value.base);
    if (bt_storage->check_sum != (bt_storage->size + bt_storage->bonded_number)) {
        syslog(LOG_ERR, "loaded bt bonded device info erro\n");
        return;
    }

    syslog(LOG_DEBUG, "loaded %" PRIu32 "bytes info", bt_storage->size);
    syslog(LOG_DEBUG, "device type size is %zu", sizeof(SERVICE_REMOTE_DEVICE_S));

    device = (SERVICE_REMOTE_DEVICE_S*)(&bt_storage->bonded_devices);
    device_properties = (remote_device_properties_t*)malloc(sizeof(remote_device_properties_t) * bt_storage->bonded_number);
    if (!device_properties) {
        syslog(LOG_ERR, "malloc failed");
        return;
    }

    for (i = 0; i < bt_storage->bonded_number; i++)
        transform_btbond_to_deviceproperty(device + i, device_properties + i);

    bt_storage_save_bonded_device(device_properties, bt_storage->bonded_number);
    free(device_properties);
}

static void load_blebond_cb(int status, const char* key, uv_buf_t value, void* cookie)
{
    uint16_t i;
    remote_device_le_properties_t* device_properties;
    bt_storage_t* bt_storage;
    ble_keys_t* keys;

    syslog(LOG_INFO, __func__);
    if (status != 0) {
        syslog(LOG_WARNING, "ble bonded device load failed, status is %d", status);
        return;
    }

    bt_storage = (bt_storage_t*)(value.base);
    if (bt_storage->check_sum != (bt_storage->size + bt_storage->bonded_number)) {
        syslog(LOG_ERR, "loaded ble bonded device info erro");
        return;
    }

    keys = (ble_keys_t*)(&bt_storage->bonded_devices);
    device_properties = (remote_device_le_properties_t*)malloc(sizeof(remote_device_le_properties_t) * bt_storage->bonded_number);
    if (!device_properties) {
        syslog(LOG_ERR, "malloc failed\n");
        return;
    }

    for (i = 0; i < bt_storage->bonded_number; i++)
        transform_blebond_to_deviceleproperty(keys + i, device_properties + i);

    bt_storage_save_le_bonded_device(device_properties, bt_storage->bonded_number);
    free(device_properties);
}

static void load_blewhitelist_cb(int status, const char* key, uv_buf_t value, void* cookie)
{
    uint16_t i;
    remote_device_le_properties_t* device_properties;
    gap_ble_whitelist_data* ble_whitelist;
    SERVICE_REMOTE_BLE_DEVICE_S* whitelist_device;

    syslog(LOG_INFO, __func__);
    if (status != 0) {
        syslog(LOG_WARNING, "ble whitelist device load failed, status is %d\n", status);
        return;
    }

    ble_whitelist = (gap_ble_whitelist_data*)(value.base);
    if (ble_whitelist->size != value.len) {
        syslog(LOG_ERR, "loaded ble whitelist device info erro\n");
        return;
    }

    syslog(LOG_DEBUG, "loaded %zu bytes info", value.len);
    syslog(LOG_DEBUG, "device type size is %zu", sizeof(SERVICE_REMOTE_BLE_DEVICE_S));

    whitelist_device = (SERVICE_REMOTE_BLE_DEVICE_S*)(&ble_whitelist->devices); // obtain the address of the first device
    device_properties = (remote_device_le_properties_t*)malloc(sizeof(remote_device_le_properties_t) * ble_whitelist->num);
    if (!device_properties) {
        syslog(LOG_ERR, "malloc failed\n");
        return;
    }

    for (i = 0; i < ble_whitelist->num; i++)
        transform_blewhitelist_to_deviceleproperty(whitelist_device + i, device_properties + i);

    bt_storage_save_whitelist(device_properties, ble_whitelist->num);
    free(device_properties);
}

static void load_from_db_with_key(uv_db_t* db, const char* key, void* cb, void* cookie)
{
    uv_buf_t buf;
    int res;

    syslog(LOG_INFO, "load from db with key %s\n", key);
    res = uv_db_get(db, key, &buf, cb, cookie);
    assert(res == 0);
}

static void bt_storage_transformer_init(bt_storage_transformer_t* transformer)
{
    syslog(LOG_INFO, __func__);
    transformer->loop = uv_default_loop();
    uv_db_init(transformer->loop, &transformer->read_db, BT_CONFIG_FILE_PATH);
    bt_storage_init();
}

static void bt_storage_transformer_deinit(bt_storage_transformer_t* transformer)
{
    syslog(LOG_INFO, __func__);

    if (transformer->read_db)
        uv_db_close(transformer->read_db);
    bt_storage_cleanup();
    uv_loop_close(transformer->loop);
}

static void bt_storage_transformer_work(bt_storage_transformer_t* transformer)
{
    syslog(LOG_INFO, __func__);

    load_from_db_with_key(transformer->read_db, BT_KEY_DEVICE_INFO, load_device_info_cb, NULL);
    load_from_db_with_key(transformer->read_db, BT_KEY_BTBOND, load_btbond_cb, NULL);
    load_from_db_with_key(transformer->read_db, BT_KEY_BLEBOND, load_blebond_cb, NULL);
    load_from_db_with_key(transformer->read_db, BT_KEY_BLEWHITELIST, load_blewhitelist_cb, NULL);

    uv_run(transformer->loop, UV_RUN_DEFAULT);
}

int main(void)
{
    bt_storage_transformer_t transformer = { 0 };

    if (!access(BT_STORAGE_FILE_PATH, F_OK)) {
        syslog(LOG_INFO, "bt_storage.db exits\n");
        return 0;
    }

    if (access(BT_CONFIG_FILE_PATH, F_OK)) {
        syslog(LOG_INFO, "bt_config.db not exits\n");
        return 0;
    }

    bt_storage_transformer_init(&transformer);
    bt_storage_transformer_work(&transformer);
    bt_storage_transformer_deinit(&transformer);
    if (property_set_bool(BLUETOOTH_V35_UPGRDE_TAG, true))
        syslog(LOG_ERR, "set %s failed\n", BLUETOOTH_V35_UPGRDE_TAG);

    return 0;
}
