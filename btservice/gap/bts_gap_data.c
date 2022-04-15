/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_gap_data.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "bts_gap_data"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bts_service.h"
#include "log.h"
#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"
#include "utils.h"
#include "uv_ext.h"

#define BT_DEFAULT_DEVICE_CLASS (BT_COD_SERVICE_CAPTURING | BT_COD_SERVICE_AUDIO | BT_COD_WERABLE_WATCH)
#define BT_DEFAULT_IO_CAPABILITY (SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT)
#define GAP_CONFIG_INIT_TIMEOUT 1500
#ifndef BT_DEVICE_NAME_MAX_LEN
#define BT_DEVICE_NAME_MAX_LEN 248
#endif

#define BT_CONFIG_FILE_PATH "/data/misc/bt/bt_config.db"
#define BT_KEY_DEVICE_INFO "key_deviceinfo"
#define BT_KEY_IOCAP "key_iocap"
#define BT_KEY_BTNAME "key_btname"
#define BT_KEY_SCANMODE "key_scanmode"
#define BT_KEY_BONDABLE "key_bondable"
#define BT_KEY_COD "key_cod"
#define BT_KEY_BTBOND "key_btbond"
#define BT_KEY_BLEBOND "key_blebond"
#define BT_KEY_BLEWHITELIST "key_blewhitelist"

typedef struct {
    uint32_t cod;
    SERVICE_BT_IO_CAPABILITY io_capability;
    char bt_name[BT_DEVICE_NAME_MAX_LEN];
    SERVICE_BT_SCAN_MODE scan_mode;
    bool bondable;
} bt_device_info_t;

typedef enum {
    STORAGE_TYPE_BT,
    STORAGE_TYPE_BLE,
} storage_type;

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

static uv_db_t* handle = NULL;
static bt_device_info_t device_info;
static bts_service_adapter_state_changed_callback adapter_state_changed_cb = NULL;
static uv_timer_t* config_init_timer = NULL;
static bool state_on = false;

static void gap_init_timeout(char* data)
{
    if (!adapter_state_changed_cb) {
        BT_LOGW("%s, cb null", __func__);
        return;
    }
    if (state_on) {
        return;
    }
    state_on = true;
    adapter_state_changed_cb(BTM_STATE_ON);
}

static void update_device_info_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    if (status != 0) {
        BT_LOGW("%s, status:%d, key:%s, cookie:%s", __func__, status, key, (char*)cookie);
        return;
    }
    uv_db_commit(handle);
}

static bt_result_code gap_uv_db_update_deviceinfo(char* val)
{
    uv_buf_t buf = uv_buf_init((char*)&device_info, sizeof(bt_device_info_t));
    int res = uv_db_set(handle, BT_KEY_DEVICE_INFO, &buf, update_device_info_callback, val);
    if (res != 0) {
        BT_LOGE("fail, uv_db_set(%s) ret:%d", val, res);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code gap_bt_update_name(char* name, uint8_t size)
{
    if (size > BT_DEVICE_NAME_MAX_LEN) {
        BT_LOGE("bt_name len%d overflow", size);
        return BT_RESULT_FAILED;
    }

    SERVICE_BT_STATUS ret = service_adapter_gap_set_local_name(name, size);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGD("set_local_name failed: %" PRIu32, ret);
        return BT_RESULT_FAILED;
    }

    memcpy(device_info.bt_name, name, size);
    return gap_uv_db_update_deviceinfo(BT_KEY_BTNAME);
}

bt_result_code gap_bt_update_scan_mode(bt_scan_mode scan_mode, bool bondable)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_set_scan_mode(scan_mode, bondable);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }

    device_info.scan_mode = scan_mode;
    device_info.bondable = bondable;

    return gap_uv_db_update_deviceinfo(BT_KEY_SCANMODE);
}

bt_result_code gap_bt_update_io_capability(bt_io_capability io_capability)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_set_local_io_capability(io_capability);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }

    device_info.io_capability = io_capability;
    return gap_uv_db_update_deviceinfo(BT_KEY_IOCAP);
}

bt_result_code gap_bt_update_device_class(uint32_t class_of_device)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_set_local_device_class(class_of_device);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }

    device_info.cod = class_of_device;
    return gap_uv_db_update_deviceinfo(BT_KEY_COD);
}

static void gap_update_device(bt_device_info_t* info)
{
    if (!info) {
        BT_LOGE("%s info null", __func__);
        return;
    }

    BT_LOGD("bt name:%s", info->bt_name);
    BT_LOGD("io_capability:%d", info->io_capability);
    BT_LOGD("scan_mode:%d", info->scan_mode);
    BT_LOGD("device_class:0x%" PRIu32, info->cod);
    BT_LOGD("bondable:%d", info->bondable);

    device_info.cod = info->cod;
    device_info.io_capability = info->io_capability;
    device_info.bondable = info->bondable;
    device_info.scan_mode = info->scan_mode;
    memcpy(device_info.bt_name, info->bt_name, strlen(info->bt_name) + 1);

    SERVICE_BT_STATUS ret = service_adapter_gap_set_local_device_class(info->cod);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set_local_device_class failed: %" PRIu32, ret);
    }

    ret = service_adapter_gap_set_local_io_capability(info->io_capability);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set_local_io_capability failed: %" PRIu32, ret);
    }

    ret = service_adapter_gap_set_local_name(info->bt_name, strlen(info->bt_name) + 1);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGD("set_local_name failed: %" PRIu32, ret);
    }

    ret = service_adapter_gap_set_scan_mode(info->scan_mode, info->bondable);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGD("set_scan_mode failed: %" PRIu32, ret);
    }
}

static void gap_config_load_default_device(void)
{
    if (state_on) {
        BT_LOGW("discard default, and load app config");
        return;
    }
    bt_device_info_t info;
    memset(&info, 0, sizeof(bt_device_info_t));
    info.cod = BT_DEFAULT_DEVICE_CLASS;
    info.io_capability = BT_DEFAULT_IO_CAPABILITY;
    info.bondable = true;
    info.scan_mode = BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE;

    srand(time(NULL));
    int r = rand() % 999;
    sprintf(info.bt_name, "%s-%03X", "XIAOMI VELA", r);

    gap_update_device(&info);
}

static void load_device_info_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    if (status != 0) {
        BT_LOGW("%s load default config", __func__);
        gap_config_load_default_device();
        goto end;
    }

    if (value.len != sizeof(bt_device_info_t)) {
        BT_LOGW("%s, size check fail", __func__);
        gap_config_load_default_device();
        goto end;
    }
    gap_update_device((bt_device_info_t*)(value.base));

end:
    if (adapter_state_changed_cb && !state_on) {
        state_on = true;
        adapter_state_changed_cb(BTM_STATE_ON);
    }
}

static bt_result_code gap_config_load_device_info(void)
{
    int res = uv_db_get(handle, BT_KEY_DEVICE_INFO, NULL, load_device_info_callback, NULL);
    if (res != 0) {
        gap_config_load_default_device();
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_storage_t* gap_bt_get_bonded_devices(void)
{
    int bonded_number = service_adapter_gap_get_bonded_devices(NULL, 0);
    uint32_t size = sizeof(bt_storage_t) - sizeof(void*) + sizeof(SERVICE_REMOTE_DEVICE_S) * bonded_number;
    bt_storage_t* bt_storage = malloc(size);
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage malloc ");
        return NULL;
    }
    memset(bt_storage, 0, size);
    bt_storage->type = STORAGE_TYPE_BT;
    bt_storage->bonded_number = bonded_number;
    SERVICE_REMOTE_DEVICE_S* device = (void*)((uint8_t*)bt_storage + sizeof(bt_storage_t) - sizeof(void*));
    service_adapter_gap_get_bonded_devices(device, bonded_number);
    bt_storage->size = size;
    bt_storage->check_sum = bt_storage->size + bt_storage->bonded_number;
    return bt_storage;
}

static void bt_bond_store_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    bt_storage_t* bt_storage = (bt_storage_t*)cookie;
    free(bt_storage);
    uv_db_commit(handle);
}

void gap_bt_bond_store(void)
{
    bt_storage_t* bt_storage = gap_bt_get_bonded_devices();
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage  nullptr");
        return;
    }

    uv_buf_t buf = uv_buf_init((char*)bt_storage, bt_storage->size);
    int res = uv_db_set(handle, BT_KEY_BTBOND, &buf, bt_bond_store_callback, bt_storage);
    if (res != 0) {
        free(bt_storage);
        BT_LOGE("fail, uv_db_set res:%d", res);
        return;
    }
}

static void load_btbond_devices_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    if (status != 0) {
        BT_LOGW("%s status:%d", __func__, status);
        return;
    }

    bt_storage_t* bt_storage = (bt_storage_t*)(value.base);
    if (bt_storage->check_sum != (bt_storage->size + bt_storage->bonded_number)) {
        BT_LOGE("invalid check_sum:%" PRIu32, bt_storage->check_sum);
        return;
    }

    SERVICE_REMOTE_DEVICE_S* device = (SERVICE_REMOTE_DEVICE_S*)((uint8_t*)bt_storage + sizeof(bt_storage_t) - sizeof(void*));
    for (int i = 0; i < bt_storage->bonded_number; i++) {
        SERVICE_BT_STATUS ret = service_adapter_gap_set_bonded_device(device + i);
        if (ret != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE(" service_adapter_gap_set_bonded_device failed: %" PRIu32, ret);
        }
    }
}

static void gap_config_load_btbond_devices(void)
{
    int res = uv_db_get(handle, BT_KEY_BTBOND, NULL, load_btbond_devices_callback, NULL);
    if (res != 0) {
        return;
    }
}

static bt_storage_t* gap_ble_get_bonded_devices(ble_keys_t* key, uint8_t count)
{
    uint32_t size = sizeof(bt_storage_t) - sizeof(void*) + sizeof(ble_keys_t) * count;
    bt_storage_t* bt_storage = malloc(size);
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage nullptr");
        return NULL;
    }
    memset(bt_storage, 0, size);
    bt_storage->type = STORAGE_TYPE_BLE;
    bt_storage->bonded_number = count;

    ble_keys_t* keys = (void*)((uint8_t*)bt_storage + sizeof(bt_storage_t) - sizeof(void*));
    for (int i = 0; i < count; i++) {
        memcpy(keys + i, key + i, sizeof(ble_keys_t));
    }
    bt_storage->size = size;
    bt_storage->check_sum = bt_storage->size + bt_storage->bonded_number;

    return bt_storage;
}

static void ble_bond_store_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    bt_storage_t* bt_storage = (bt_storage_t*)cookie;
    free(bt_storage);
    uv_db_commit(handle);
}

void gap_ble_bond_store(ble_keys_t* key, uint8_t count)
{
    bt_storage_t* bt_storage = gap_ble_get_bonded_devices(key, count);
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage  nullptr");
        return;
    }

    uv_buf_t buf = uv_buf_init((char*)bt_storage, bt_storage->size);
    int res = uv_db_set(handle, BT_KEY_BLEBOND, &buf, ble_bond_store_callback, bt_storage);
    if (res != 0) {
        free(bt_storage);
        BT_LOGE("fail, uv_db_set res:%d", res);
        return;
    }
}

static void load_blebond_devices_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    if (status != 0) {
        BT_LOGW("%s status:%d", __func__, status);
        return;
    }

    bt_storage_t* bt_storage = (bt_storage_t*)(value.base);
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage is NULL");
        return;
    }

    if (bt_storage->check_sum != (bt_storage->size + bt_storage->bonded_number)) {
        BT_LOGE("invalid check_sum:%" PRIu32, bt_storage->check_sum);
        return;
    }
    if (bt_storage->bonded_number == 0) {
        BT_LOGW(" bonded_number %d ", bt_storage->bonded_number);
        return;
    }

    ble_keys_t* keys = (ble_keys_t*)((uint8_t*)bt_storage + sizeof(bt_storage_t) - sizeof(void*));
    SERVICE_BT_STATUS ret = service_adapter_gap_ble_set_bonded_devices((SERVICE_BLE_KEYS_S*)keys, bt_storage->bonded_number);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE(" service_adapter_gap_set_le_bonded_device failed: %" PRIu32, ret);
        return;
    }
}

static void gap_config_load_blebond_devices(void)
{
    int res = uv_db_get(handle, BT_KEY_BLEBOND, NULL, load_blebond_devices_callback, NULL);
    if (res != 0) {
        return;
    }
}

static gap_ble_whitelist_data* gap_ble_getwhitelist(void)
{
    int whitelist_number = service_adapter_gap_ble_get_white_list_devices(NULL, 0);

    size_t size = sizeof(gap_ble_whitelist_data) - sizeof(SERVICE_REMOTE_BLE_DEVICE_S*) + sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * whitelist_number;
    gap_ble_whitelist_data* ble_whitelist_storage = (gap_ble_whitelist_data*)malloc(size);
    if (!ble_whitelist_storage) {
        BT_LOGE("error, malloc failed");
        return NULL;
    }
    memset(ble_whitelist_storage, 0, size);
    SERVICE_REMOTE_BLE_DEVICE_S* devices = (SERVICE_REMOTE_BLE_DEVICE_S*)(&(ble_whitelist_storage->devices));
    whitelist_number = service_adapter_gap_ble_get_white_list_devices(devices, whitelist_number);
    ble_whitelist_storage->num = whitelist_number;
    ble_whitelist_storage->size = size;
    return ble_whitelist_storage;
}

static void ble_whitelist_store_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    gap_ble_whitelist_data* ble_whitelist = (gap_ble_whitelist_data*)cookie;
    free(ble_whitelist);
    uv_db_commit(handle);
}

bt_result_code gap_ble_whitelist_store_update(bool added, bt_address addr)
{
    gap_ble_whitelist_data* ble_whitelist = gap_ble_getwhitelist();
    if (!ble_whitelist) {
        BT_LOGE("fail, ble_whitelist  nullptr");
        return BT_RESULT_FAILED;
    }

    uv_buf_t buf = uv_buf_init((char*)ble_whitelist, ble_whitelist->size);
    int res = uv_db_set(handle, "key_blewhitelist", &buf, ble_whitelist_store_callback, ble_whitelist);
    if (res != 0) {
        free(ble_whitelist);
        BT_LOGE("fail, uv_db_set res:%d", res);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static void load_whitelist_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    if (status != 0) {
        BT_LOGW("%s status:%d", __func__, status);
        return;
    }

    gap_ble_whitelist_data* ble_whitelist = (gap_ble_whitelist_data*)(value.base);
    if (!ble_whitelist) {
        BT_LOGE("fail, ble_whitelist nullptr");
        return;
    }

    size_t size = sizeof(gap_ble_whitelist_data) - sizeof(SERVICE_REMOTE_BLE_DEVICE_S*) + sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * (ble_whitelist->num);
    if (ble_whitelist->size != size) {
        BT_LOGE("error, ble whitelist size check fail");
        return;
    }

    SERVICE_REMOTE_BLE_DEVICE_S* devices = (SERVICE_REMOTE_BLE_DEVICE_S*)(&(ble_whitelist->devices));
    for (int i = 0; i < ble_whitelist->num; i++) {
        SERVICE_BT_STATUS ret = service_adapter_gap_ble_add_white_list(devices[i].bd_addr);
        if (ret != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("add whitelist %s fail, ret:%" PRIu32, addr_str(devices[i].bd_addr), ret);
        }
    }
}

static void gap_config_load_ble_whitelist(void)
{
    int res = uv_db_get(handle, "key_blewhitelist", NULL, load_whitelist_callback, NULL);
    if (res != 0) {
        return;
    }
}

bt_result_code gap_bt_config_init(bts_service_adapter_state_changed_callback cb)
{
    adapter_state_changed_cb = cb;
    state_on = false;
    config_init_timer = start_timer(GAP_CONFIG_INIT_TIMEOUT, 0, gap_init_timeout, NULL);

    int res = uv_db_init(get_service_loop(), &handle, BT_CONFIG_FILE_PATH);
    if (res != 0) {
        BT_LOGE("fail, uv_db_init res:%d", res);
        return BT_RESULT_FAILED;
    }

    gap_config_load_device_info();
    gap_config_load_btbond_devices();
    gap_config_load_blebond_devices();
    gap_config_load_ble_whitelist();
    return BT_RESULT_SUCCESS;
}

void gap_bt_config_cleanup(void)
{
    if (handle)
        uv_db_close(handle);
}