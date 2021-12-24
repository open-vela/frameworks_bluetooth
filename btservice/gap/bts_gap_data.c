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

#ifndef MAX_BONDED_DEVICES_SUPPORTED
#define MAX_BONDED_DEVICES_SUPPORTED 5
#endif

#define BT_DEFAULT_DEVICE_CLASS (BT_COD_SERVICE_RENDERING | BT_COD_SERVICE_AUDIO | BT_COD_SERVICE_TELEPHONY | BT_COD_AV_HEADSET)
#define BT_DEFAULT_IO_CAPABILITY (SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT)

#define BT_DEFAULT_NAME_PREFIX "XIAOMI WATCH L61"
#define BT_DEFAULT_FILE_NAME "/data/bt_device.db"
#define BT_USER_FILE_NAME "/data/bt_device2.db"
#define BT_APP_INFO_FILE_NAME "/data/bt_storage.db"
#define LE_APP_INFO_FILE_NAME "/data/le_storage.db"

#ifndef BT_DEVICE_NAME_MAX_LEN
#define BT_DEVICE_NAME_MAX_LEN 248
#endif
typedef struct {
    uint32_t device_class;
    SERVICE_BT_IO_CAPABILITY io_capability;
    char bt_name[BT_DEVICE_NAME_MAX_LEN];
} bt_device_info_t;

typedef struct {
    /* Basic information to save */
    uint32_t size;
    uint8_t spk_volume;
    uint8_t mic_volume;
    uint16_t bonded_number;
    SERVICE_REMOTE_DEVICE_S bonded_devices[MAX_BONDED_DEVICES_SUPPORTED];
    uint32_t check_sum;
    /* More items to be added from here for future extension */
} bt_storage_t;

typedef struct {
    /* Basic information to save */
    uint32_t size;
    uint16_t bonded_number;
    ble_keys_t bonded_devices[MAX_BONDED_DEVICES_SUPPORTED];
    uint32_t check_sum;
    /* More items to be added from here for future extension */
} le_storage_t;

static size_t file_write_buffer(const char* filename, void* data, size_t size)
{
    FILE* file = fopen(filename, "wb+");
    if (!file) {
        return 0;
    }
    size_t ret = fwrite(data, sizeof(uint8_t), size, file);
    if (ret != size) {
        return 0;
    }
    fflush(file);
    fclose(file);
    return ret;
}

static size_t file_read_buffer(const char* filename, void* data, uint32_t size)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return 0;
    }

    size_t size_read = fread(data, sizeof(uint8_t), size, file);
    fclose(file);
    return size_read;
}

void gap_read_data_storage(void)
{
    bt_storage_t app_info;
    memset(&app_info, 0, sizeof(bt_storage_t));
    size_t size = file_read_buffer(BT_APP_INFO_FILE_NAME, &app_info, sizeof(bt_storage_t));
    if (size != sizeof(bt_storage_t)) {
        return;
    }

    if (app_info.size != sizeof(bt_storage_t)) {
        BT_LOGE("invalid size:%ld", app_info.size);
        return;
    }
    if (app_info.check_sum != (app_info.size + app_info.bonded_number)) {
        BT_LOGE("invalid check_sum:%ld", app_info.check_sum);
        return;
    }
    if (app_info.bonded_number > MAX_BONDED_DEVICES_SUPPORTED) {
        BT_LOGE(" bonded_number:%d overflow", app_info.bonded_number);
        return;
    }

    for (int i = 0; i < app_info.bonded_number; i++) {
        SERVICE_BT_STATUS ret = service_adapter_gap_set_bonded_device(app_info.bonded_devices + i);
        if (ret != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE(" service_adapter_gap_set_bonded_device failed: %d", ret);
        }
    }
}

void gap_update_data_storage(void)
{
    bt_storage_t app_info;
    memset(&app_info, 0, sizeof(bt_storage_t));
    app_info.size = sizeof(bt_storage_t);
    app_info.bonded_number = (uint16_t)service_adapter_gap_get_bonded_devices(app_info.bonded_devices, MAX_BONDED_DEVICES_SUPPORTED);
    app_info.check_sum = app_info.size + app_info.bonded_number;
    size_t size = file_write_buffer(BT_APP_INFO_FILE_NAME, &app_info, sizeof(bt_storage_t));
    if (size != sizeof(bt_storage_t)) {
        BT_LOGE("fail,  %s err: %s", __func__, strerror(errno));
        return;
    }
}

void gap_read_le_storage(void)
{
    le_storage_t app_info;
    memset(&app_info, 0, sizeof(le_storage_t));
    size_t size = file_read_buffer(LE_APP_INFO_FILE_NAME, &app_info, sizeof(le_storage_t));
    if (size != sizeof(le_storage_t)) {
        return;
    }

    if (app_info.size != sizeof(le_storage_t)) {
        BT_LOGE("invalid size:%ld", app_info.size);
        return;
    }
    if (app_info.check_sum != (app_info.size + app_info.bonded_number)) {
        BT_LOGE("invalid check_sum:%ld", app_info.check_sum);
        return;
    }
    if (app_info.bonded_number > MAX_BONDED_DEVICES_SUPPORTED) {
        BT_LOGE(" bonded_number:%d overflow", app_info.bonded_number);
        return;
    }

    BT_LOGD("%s", __func__);
    SERVICE_BT_STATUS ret = service_adapter_gap_ble_set_bonded_devices((SERVICE_BLE_KEYS_S*)(app_info.bonded_devices), app_info.bonded_number);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE(" service_adapter_gap_set_le_bonded_device failed: %d", ret);
        return;
    }
}

void gap_update_le_storage(ble_keys_t* key, uint8_t count)
{
    le_storage_t app_info;
    memset(&app_info, 0, sizeof(le_storage_t));
    app_info.size = sizeof(le_storage_t);
    app_info.bonded_number = count;

    for (int i = 0; i < count && i < MAX_BONDED_DEVICES_SUPPORTED; i++) {
        memcpy(app_info.bonded_devices + i, key + i, sizeof(ble_keys_t));
    }
    app_info.check_sum = app_info.size + app_info.bonded_number;

    size_t size = file_write_buffer(LE_APP_INFO_FILE_NAME, &app_info, sizeof(le_storage_t));
    if (size != sizeof(le_storage_t)) {
        BT_LOGE("fail,  %s err: %s", __func__, strerror(errno));
        return;
    }
}

bt_result_code gap_create_factory_info(bool force)
{
    BT_LOGD("%s force:%d", __func__, force);
    if (!access(BT_DEFAULT_FILE_NAME, F_OK) && !force) {
        BT_LOGD("factory exist");
        return BT_RESULT_SUCCESS;
    }

    bt_device_info_t default_device_info = {
        .device_class = BT_DEFAULT_DEVICE_CLASS,
        .io_capability = BT_DEFAULT_IO_CAPABILITY,
    };
    memset(default_device_info.bt_name, 0, BT_DEVICE_NAME_MAX_LEN);
    srand(time(NULL));
    int r = rand() % 999;
    sprintf(default_device_info.bt_name, "%s-%03X", BT_DEFAULT_NAME_PREFIX, r);

    size_t size = file_write_buffer(BT_DEFAULT_FILE_NAME, &default_device_info, sizeof(bt_device_info_t));
    if (size != sizeof(bt_device_info_t)) {
        BT_LOGE("fail,  %s err: %s", __func__, strerror(errno));
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code gap_update_device_name(char* bt_name, uint8_t len_name)
{
    if (len_name > BT_DEVICE_NAME_MAX_LEN) {
        BT_LOGE("bt_name len%d too long", len_name);
        return BT_RESULT_FAILED;
    }
    SERVICE_BT_STATUS ret = service_adapter_gap_set_local_name(bt_name, len_name);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGD("set_local_name failed: %d", ret);
        return BT_RESULT_FAILED;
    }

    bt_device_info_t device_info;
    memset(&device_info, 0, sizeof(bt_device_info_t));
    size_t size = file_read_buffer(BT_DEFAULT_FILE_NAME, &device_info, sizeof(bt_device_info_t));
    if (size != sizeof(bt_device_info_t)) {
        BT_LOGE("fail,%s failed: %s", __func__, strerror(errno));
        return BT_RESULT_FAILED;
    }
    memset(device_info.bt_name, 0, BT_DEVICE_NAME_MAX_LEN);
    memcpy(device_info.bt_name, bt_name, len_name);

    size = file_write_buffer(BT_USER_FILE_NAME, &device_info, sizeof(bt_device_info_t));
    if (size != sizeof(bt_device_info_t)) {
        BT_LOGE("file_write_buffer failed: %d", size);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code gap_read_device_info(void)
{
    SERVICE_BT_STATUS ret;
    bt_device_info_t device_info;
    memset(&device_info, 0, sizeof(bt_device_info_t));
    size_t size = file_read_buffer(BT_USER_FILE_NAME, &device_info, sizeof(bt_device_info_t));
    if (size == sizeof(bt_device_info_t)) {
        BT_LOGD("read user device info");
        goto done;
    }

    BT_LOGW("user bt info(%s) invalid, read default bt info", BT_USER_FILE_NAME);
    size = file_read_buffer(BT_DEFAULT_FILE_NAME, &device_info, sizeof(bt_device_info_t));
    if (size == sizeof(bt_device_info_t)) {
        BT_LOGD("read factory device info");
        goto done;
    }
    return BT_RESULT_FAILED;

done:
    ret = service_adapter_gap_set_local_device_class(device_info.device_class);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set_local_device_class failed: %d", ret);
        return BT_RESULT_FAILED;
    }

    ret = service_adapter_gap_set_local_io_capability(device_info.io_capability);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set_local_io_capability failed: %d", ret);
        return BT_RESULT_FAILED;
    }

    ret = service_adapter_gap_set_local_name(device_info.bt_name, strlen(device_info.bt_name) + 1);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGD("set_local_name failed: %d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}