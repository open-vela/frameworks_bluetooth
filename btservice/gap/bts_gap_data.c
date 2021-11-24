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

#define LOG_TAG "bts_gap_data"

#ifndef MAX_BONDED_DEVICES_SUPPORTED
#define MAX_BONDED_DEVICES_SUPPORTED 5
#endif

#define BT_DEFAULT_DEVICE_CLASS (BT_COD_SERVICE_RENDERING | BT_COD_SERVICE_AUDIO | BT_COD_SERVICE_TELEPHONY | BT_COD_AV_HEADSET)
#define BT_DEFAULT_IO_CAPABILITY (SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT)

#define BT_DEFAULT_NAME_PREFIX "XIAOMI WATCH L61"
#define BT_DEFAULT_FILE_NAME "/data/bt_device.db"
#define BT_USER_FILE_NAME "/data/bt_device2.db"
#define BT_APP_INFO_FILE_NAME "/data/bt_storage.db"

#define BT_DEVICE_NAME_MAX_LEN 248
typedef struct {
    uint32_t device_class;
    SERVICE_BT_IO_CAPABILITY io_capability;
    uint8_t bt_name[BT_DEVICE_NAME_MAX_LEN];
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

static size_t file_write_buffer(const char* filename, void* data, size_t size)
{
    BT_LOGD("file_write_buffer filename:%s", filename);
    FILE* file = fopen(filename, "wb+");
    if (!file) {
        BT_LOGE("fopen(%s) failed: %s", filename, strerror(errno));
        return 0;
    }
    size_t ret = fwrite(data, sizeof(uint8_t), size, file);
    if (ret != size) {
        BT_LOGE("fwrite(%s) failed: %s", filename, strerror(errno));
        return 0;
    }
    fflush(file);
    fclose(file);
    return ret;
}

static size_t file_read_buffer(const char* filename, void* data, uint32_t size)
{
    BT_LOGD("file_read_buffer filename:%s", filename);
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        BT_LOGE("fopen(%s) failed: %s", filename, strerror(errno));
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
        BT_LOGE("file_read_buffer(%s) failed: %s", BT_APP_INFO_FILE_NAME, strerror(errno));
        return;
    }

    if (app_info.size != sizeof(bt_storage_t)) {
        BT_LOGE("invalid size:%d", app_info.size);
        return;
    }
    if (app_info.check_sum != (app_info.size + app_info.bonded_number)) {
        BT_LOGE("invalid check_sum:%d", app_info.check_sum);
        return;
    }
    if (app_info.bonded_number > MAX_BONDED_DEVICES_SUPPORTED) {
        BT_LOGE(" bonded_number:%d overflow", app_info.bonded_number);
        return;
    }

    for (int i = 0; i < app_info.bonded_number; i++) {
        bt_status ret = service_adapter_gap_set_bonded_device(app_info.bonded_devices + i);
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
    file_write_buffer(BT_APP_INFO_FILE_NAME, &app_info, sizeof(bt_storage_t));
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

    int size = file_write_buffer(BT_DEFAULT_FILE_NAME, &default_device_info, sizeof(bt_device_info_t));
    if (size < 0) {
        BT_LOGE("file_write_buffer failed: %d", size);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code gap_update_device_name(uint8_t* bt_name, size_t len_name)
{
    if (len_name > BT_DEVICE_NAME_MAX_LEN) {
        BT_LOGE("bt_name len%d too long", len_name);
        return BT_RESULT_FAILED;
    }
    bt_status ret = service_adapter_gap_set_local_name(bt_name, len_name);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGD("set_local_name failed: %d", ret);
        return BT_RESULT_FAILED;
    }

    bt_device_info_t device_info;
    memset(&device_info, 0, sizeof(bt_device_info_t));
    size_t size = file_read_buffer(BT_DEFAULT_FILE_NAME, &device_info, sizeof(bt_device_info_t));
    if (size != sizeof(bt_device_info_t)) {
        BT_LOGE("file_read_buffer failed: %d", size);
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

bt_result_code gap_read_device_info()
{
    bt_status ret;
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