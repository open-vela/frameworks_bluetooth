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

#define BT_DEFAULT_DEVICE_CLASS (BT_COD_SERVICE_CAPTURING | BT_COD_SERVICE_AUDIO | BT_COD_WERABLE_WATCH)
#define BT_DEFAULT_IO_CAPABILITY (SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT)

#define BT_DEFAULT_NAME_PREFIX "XIAOMI VELA"
#define BT_DEFAULT_FILE_NAME "/data/misc/bt/bt_default.db"
#define BT_USER_FILE_NAME "/data/misc/bt/bt_user.db"
#define BT_APP_INFO_FILE_NAME "/data/misc/bt/bt_app.db"
#define LE_APP_INFO_FILE_NAME "/data/misc/bt/le_app.db"

#ifndef BT_DEVICE_NAME_MAX_LEN
#define BT_DEVICE_NAME_MAX_LEN 248
#endif
#define GAP_DATA_INIT_TIMEOUT 1000

typedef struct {
    uint32_t device_class;
    SERVICE_BT_IO_CAPABILITY io_capability;
    char bt_name[BT_DEVICE_NAME_MAX_LEN];
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

typedef void (*gap_on_open)(uv_fs_t* req);
typedef void (*gap_on_read)(uv_fs_t* req);
typedef void (*gap_on_write)(uv_fs_t* req);
typedef void (*gap_on_close)(uv_fs_t* req);
typedef void (*gap_on_clean)(uv_fs_t* req);
typedef void (*gap_on_except)(uv_fs_t* req);

typedef struct gap_bt_uv_ops {
    gap_on_open on_open;
    gap_on_write on_write;
    gap_on_read on_read;
    gap_on_close on_close;
    gap_on_clean on_clean;
    gap_on_except on_except;

    uv_fs_t open_req;
    uv_fs_t read_req;
    uv_fs_t write_req;
    uv_fs_t close_req;
    uv_fs_t clean_req;
    uv_fs_t except_req;

    uv_buf_t iov;
    uv_fs_t data;
} gap_bt_uv_ops;

static void gap_bt_device_load(const char* file);

static bt_device_info_t current_bt_device_info;
static bts_service_adapter_state_changed_callback adapter_state_changed_cb = NULL;
static uv_timer_t* gap_data_init_timer = NULL;
static bool enable_report_btm_state_on = false;

static void gap_bt_uv_op_on_open(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    if (req->result < 0) {
        if (ops->on_except) {
            ops->except_req.data = ops;
            ops->on_except(&ops->except_req);
        } else {
            BT_LOGW(" open file: %s", uv_strerror((int)req->result));
            ops->clean_req.data = ops;
            ops->on_clean(&ops->clean_req);
        }
        return;
    }

    ops->open_req.data = ops;
    ops->on_open(&ops->open_req);
}

static void gap_bt_uv_op_on_read(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    if (req->result < 0) {
        BT_LOGE("Read error: %s", uv_strerror(req->result));
        ops->clean_req.data = ops;
        ops->on_clean(&ops->clean_req);
        return;
    }

    if (req->result == 0) {
        BT_LOGD("read eof");
        ops->clean_req.data = ops;
        ops->on_clean(&ops->clean_req);
        return;
    }

    ops->read_req.data = ops;
    ops->on_read(&ops->read_req);
}

static void gap_bt_uv_op_on_write(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    if (req->result < 0) {
        BT_LOGE("Write error: %s", uv_strerror((int)req->result));
        ops->clean_req.data = ops;
        ops->on_clean(&ops->clean_req);
        return;
    }

    ops->write_req.data = ops;
    ops->on_write(&ops->write_req);
}

static void gap_bt_uv_op_on_close(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }
    ops->close_req.data = ops;
    ops->on_close(&ops->close_req);
}

static void gap_bt_uv_op_on_clean(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }
    ops->clean_req.data = ops;
    ops->on_clean(&ops->clean_req);
}

static void gap_bt_name_update_on_clean(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }
    uv_fs_req_cleanup(&ops->open_req);
    uv_fs_req_cleanup(&ops->write_req);
    uv_fs_req_cleanup(&ops->close_req);
    free(ops);
}

static void gap_bt_name_update_on_close(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->clean_req.data = ops;
    gap_bt_uv_op_on_clean(&ops->clean_req);
}

static void gap_bt_name_update_on_write(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->close_req.data = ops;
    uv_fs_close(get_service_loop(), &ops->close_req, ops->open_req.result, gap_bt_uv_op_on_close);
}

static void gap_bt_name_update_on_open(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->iov = uv_buf_init((char*)&current_bt_device_info, sizeof(bt_device_info_t));
    ops->write_req.data = ops;
    uv_fs_write(get_service_loop(), &ops->write_req, ops->open_req.result, &ops->iov, 1, 0, gap_bt_uv_op_on_write);
}

bt_result_code gap_bt_update_name(char* name, uint8_t size)
{
    BT_LOGD("%s", __func__);
    if (size > BT_DEVICE_NAME_MAX_LEN) {
        BT_LOGE("bt_name len%d too long", size);
        return BT_RESULT_FAILED;
    }

    SERVICE_BT_STATUS ret = service_adapter_gap_set_local_name(name, size);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGD("set_local_name failed: %d", ret);
        return BT_RESULT_FAILED;
    }

    gap_bt_uv_ops* ops = malloc(sizeof(gap_bt_uv_ops));
    if (!ops) {
        BT_LOGE("malloc");
        return BT_RESULT_FAILED;
    }
    memset(ops, 0, sizeof(gap_bt_uv_ops));
    ops->on_open = gap_bt_name_update_on_open;
    ops->on_write = gap_bt_name_update_on_write;
    ops->on_close = gap_bt_name_update_on_close;
    ops->on_clean = gap_bt_name_update_on_clean;
    ops->open_req.data = ops;
    memcpy(current_bt_device_info.bt_name, name, size);

    int rc = uv_fs_open(get_service_loop(), &ops->open_req, BT_USER_FILE_NAME, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR, gap_bt_uv_op_on_open);
    if (rc < 0) {
        ops->clean_req.data = ops;
        gap_bt_uv_op_on_clean(&ops->clean_req);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static void gap_bt_device_load_on_clean(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }
    uv_fs_req_cleanup(&ops->open_req);
    uv_fs_req_cleanup(&ops->read_req);
    uv_fs_req_cleanup(&ops->close_req);
    free(ops);
}

static void gap_bt_device_load_on_close(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->clean_req.data = ops;
    gap_bt_uv_op_on_clean(&ops->clean_req);
}

static void gap_bt_device_load_on_except(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }
    ops->clean_req.data = ops;
    ops->on_clean(&ops->clean_req);
    gap_bt_device_load(BT_DEFAULT_FILE_NAME);
}

static void gap_bt_device_load_on_read(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    BT_LOGD("bt name:%s", current_bt_device_info.bt_name);
    BT_LOGD("io_capability:%d", current_bt_device_info.io_capability);

    SERVICE_BT_STATUS ret = service_adapter_gap_set_local_device_class(current_bt_device_info.device_class);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set_local_device_class failed: %d", ret);
    }

    ret = service_adapter_gap_set_local_io_capability(current_bt_device_info.io_capability);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set_local_io_capability failed: %d", ret);
    }

    ret = service_adapter_gap_set_local_name(current_bt_device_info.bt_name, strlen(current_bt_device_info.bt_name) + 1);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGD("set_local_name failed: %d", ret);
    }

    if (gap_data_init_timer) {
        stop_timer(gap_data_init_timer);
    }
    if (adapter_state_changed_cb && enable_report_btm_state_on) {
        enable_report_btm_state_on = false;
        adapter_state_changed_cb(BTM_STATE_ON);
    }
    ops->close_req.data = ops;
    uv_fs_close(get_service_loop(), &ops->close_req, ops->open_req.result, gap_bt_uv_op_on_close);
}

static void gap_bt_device_load_on_open(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->iov = uv_buf_init((char*)&current_bt_device_info, sizeof(bt_device_info_t));
    ops->read_req.data = ops;
    uv_fs_read(get_service_loop(), &ops->read_req, req->result, &ops->iov, 1, -1, gap_bt_uv_op_on_read);
}

static void gap_bt_device_load(const char* file)
{
    gap_bt_uv_ops* ops = malloc(sizeof(gap_bt_uv_ops));
    if (!ops) {
        BT_LOGE("malloc");
        return;
    }
    memset(ops, 0, sizeof(gap_bt_uv_ops));
    ops->on_open = gap_bt_device_load_on_open;
    ops->on_read = gap_bt_device_load_on_read;
    ops->on_close = gap_bt_device_load_on_close;
    ops->on_clean = gap_bt_device_load_on_clean;
    ops->on_except = gap_bt_device_load_on_except;
    ops->open_req.data = ops;

    int rc = uv_fs_open(get_service_loop(), &ops->open_req, file, O_RDONLY, S_IWUSR | S_IRUSR, gap_bt_uv_op_on_open);
    if (rc < 0) {
        BT_LOGE("fail, open file %s failed", file);
        ops->clean_req.data = ops;
        gap_bt_uv_op_on_clean(&ops->clean_req);
        return;
    }
}

static void gap_bt_init_device_info(void)
{
    BT_LOGD("%s", __func__);
    gap_bt_device_load(BT_USER_FILE_NAME);
}

static void gap_bt_factory_update_on_clean(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }
    uv_fs_req_cleanup(&ops->open_req);
    uv_fs_req_cleanup(&ops->write_req);
    uv_fs_req_cleanup(&ops->close_req);
    free(ops->data.data);
    free(ops);
}

static void gap_bt_factory_update_on_close(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->clean_req.data = ops;
    gap_bt_uv_op_on_clean(&ops->clean_req);
    gap_bt_init_device_info();
}

static void gap_bt_factory_update_on_write(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->close_req.data = ops;
    uv_fs_close(get_service_loop(), &ops->close_req, ops->open_req.result, gap_bt_uv_op_on_close);
}

static void gap_bt_factory_update_on_open(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    bt_device_info_t* default_device_info = (bt_device_info_t*)malloc(sizeof(bt_device_info_t));
    if(!default_device_info) {
        BT_LOGE("fail, malloc default_device_info failed");
        return;
    }
    memset(default_device_info, 0, sizeof(default_device_info));
    default_device_info->device_class = BT_DEFAULT_DEVICE_CLASS;
    default_device_info->io_capability = BT_DEFAULT_IO_CAPABILITY;

    srand(time(NULL));
    int r = rand() % 999;
    sprintf(default_device_info->bt_name, "%s-%03X", BT_DEFAULT_NAME_PREFIX, r);
    ops->data.data = default_device_info;

    ops->iov = uv_buf_init((char*)default_device_info, sizeof(bt_device_info_t));
    ops->write_req.data = ops;

    memcpy(&current_bt_device_info, default_device_info, sizeof(bt_device_info_t));
    uv_fs_write(get_service_loop(), &ops->write_req, ops->open_req.result, &ops->iov, 1, -1, gap_bt_uv_op_on_write);
}

static void gap_data_timeout(char* data)
{
    BT_LOGD("%s", __func__);
    if (adapter_state_changed_cb && enable_report_btm_state_on) {
        enable_report_btm_state_on = false;
        adapter_state_changed_cb(BTM_STATE_ON);
    }
}

bt_result_code gap_bluetooth_device_init(bts_service_adapter_state_changed_callback cb)
{
    BT_LOGD("%s", __func__);
    adapter_state_changed_cb = cb;
    enable_report_btm_state_on = true;
    gap_data_init_timer = start_timer(GAP_DATA_INIT_TIMEOUT, 0, gap_data_timeout, NULL);
    uv_fs_t req;
    int rc = uv_fs_access(get_service_loop(), &req, BT_DEFAULT_FILE_NAME, F_OK, NULL);
    if (!rc) {
        uv_fs_req_cleanup(&req);
        BT_LOGD("bt default  device exist");
        gap_bt_init_device_info();
        return BT_RESULT_SUCCESS;
    }
    uv_fs_req_cleanup(&req);

    gap_bt_uv_ops* ops = malloc(sizeof(gap_bt_uv_ops));
    if (!ops) {
        BT_LOGE("malloc");
        return BT_RESULT_FAILED;
    }
    memset(ops, 0, sizeof(gap_bt_uv_ops));
    ops->on_open = gap_bt_factory_update_on_open;
    ops->on_write = gap_bt_factory_update_on_write;
    ops->on_close = gap_bt_factory_update_on_close;
    ops->on_clean = gap_bt_factory_update_on_clean;
    ops->open_req.data = ops;

    rc = uv_fs_open(get_service_loop(), &ops->open_req, BT_DEFAULT_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR, gap_bt_uv_op_on_open);
    if (rc < 0) {
        ops->clean_req.data = ops;
        gap_bt_uv_op_on_clean(&ops->clean_req);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static void gap_bt_bond_store_on_clean(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }
    uv_fs_req_cleanup(&ops->open_req);
    uv_fs_req_cleanup(&ops->write_req);
    uv_fs_req_cleanup(&ops->close_req);
    free(ops->data.data);
    free(ops);
}

static void gap_bt_bond_store_on_open(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    bt_storage_t* storage = (bt_storage_t*)ops->data.data;
    ops->iov = uv_buf_init((char*)storage, storage->size);
    ops->write_req.data = ops;
    uv_fs_write(get_service_loop(), &ops->write_req, ops->open_req.result, &ops->iov, 1, -1, gap_bt_uv_op_on_write);
}

static void gap_bt_bond_store_on_write(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->close_req.data = ops;
    uv_fs_close(get_service_loop(), &ops->close_req, ops->open_req.result, gap_bt_uv_op_on_close);
}

static void gap_bt_bond_store_on_close(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->clean_req.data = ops;
    gap_bt_uv_op_on_clean(&ops->clean_req);
}

static bt_storage_t* gap_bt_get_bonded_devices(void)
{
    int bonded_number = service_adapter_gap_get_bonded_devices(NULL, 0);
    uint32_t size = sizeof(bt_storage_t) - sizeof(void*) + sizeof(SERVICE_REMOTE_DEVICE_S) * bonded_number;
    bt_storage_t* bt_storage = malloc(size);
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage nullptr");
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

static void gap_bluetooth_bond_store(bt_storage_t* bt_storage, const char* file)
{
    gap_bt_uv_ops* ops = malloc(sizeof(gap_bt_uv_ops));
    if (!ops) {
        BT_LOGE("malloc");
        return;
    }
    memset(ops, 0, sizeof(gap_bt_uv_ops));
    ops->on_open = gap_bt_bond_store_on_open;
    ops->on_write = gap_bt_bond_store_on_write;
    ops->on_close = gap_bt_bond_store_on_close;
    ops->on_clean = gap_bt_bond_store_on_clean;
    ops->open_req.data = ops;
    ops->data.data = bt_storage;

    int rc = uv_fs_open(get_service_loop(), &ops->open_req, file, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR, gap_bt_uv_op_on_open);
    if (rc < 0) {
        BT_LOGE("failed to open:%s", file);
        ops->clean_req.data = ops;
        gap_bt_uv_op_on_clean(&ops->clean_req);
        return;
    }
}

void gap_bt_bond_store(void)
{
    BT_LOGD("%s", __func__);
    bt_storage_t* bt_storage = gap_bt_get_bonded_devices();
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage  nullptr");
        return;
    }

    gap_bluetooth_bond_store(bt_storage, BT_APP_INFO_FILE_NAME);
}

void gap_ble_bond_store(ble_keys_t* key, uint8_t count)
{
    BT_LOGD("%s", __func__);
    uint32_t size = sizeof(bt_storage_t) - sizeof(void*) + sizeof(ble_keys_t) * count;
    bt_storage_t* bt_storage = malloc(size);
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage nullptr");
        return;
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

    gap_bluetooth_bond_store(bt_storage, LE_APP_INFO_FILE_NAME);
}

static void gap_bt_bond_load_on_clean(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }
    uv_fs_req_cleanup(&ops->open_req);
    uv_fs_req_cleanup(&ops->read_req);
    uv_fs_req_cleanup(&ops->close_req);
    free(ops->data.data);
    free(ops);
}

static void gap_bt_bond_load_on_open(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->read_req.data = ops;
    uint32_t size;
    ops->iov = uv_buf_init((char*)&size, sizeof(uint32_t));
    uv_fs_read(get_service_loop(), &ops->read_req, req->result, &ops->iov, 1, -1, NULL);

    BT_LOGD("%s  size:%" PRIu32, __func__, size);

    bt_storage_t* bt_info = (bt_storage_t*)malloc(size);
    memset(bt_info, 0, size);
    ops->data.data = bt_info;
    ops->iov = uv_buf_init((char*)bt_info, size);
    uv_fs_read(get_service_loop(), &ops->read_req, req->result, &ops->iov, 1, 0, gap_bt_uv_op_on_read);
}

static void gap_btstack_set_bt_bond_device(bt_storage_t* bt_storage)
{
    if (!bt_storage) {
        BT_LOGE("fail, bt_storage is NULL");
        return;
    }

    if (bt_storage->check_sum != (bt_storage->size + bt_storage->bonded_number)) {
        BT_LOGE("invalid check_sum:%" PRIu32, bt_storage->check_sum);
        return;
    }

    SERVICE_REMOTE_DEVICE_S* device = (SERVICE_REMOTE_DEVICE_S*)((uint8_t*)bt_storage + sizeof(bt_storage_t) - sizeof(void*));
    for (int i = 0; i < bt_storage->bonded_number; i++) {
        SERVICE_BT_STATUS ret = service_adapter_gap_set_bonded_device(device + i);
        if (ret != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE(" service_adapter_gap_set_bonded_device failed: %d", ret);
        }
    }
}

static void gap_btstack_set_ble_bond_device(bt_storage_t* bt_storage)
{
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
        BT_LOGE(" service_adapter_gap_set_le_bonded_device failed: %d", ret);
        return;
    }
}

static void gap_bt_bond_load_on_read(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    bt_storage_t* bt_info = (bt_storage_t*)(ops->data.data);
    if (bt_info->type == STORAGE_TYPE_BT) {
        gap_btstack_set_bt_bond_device(bt_info);
    } else if (bt_info->type == STORAGE_TYPE_BLE) {
        gap_btstack_set_ble_bond_device(bt_info);
    } else {
        BT_LOGE("fail, unknown storage type")
    }

    ops->close_req.data = ops;
    uv_fs_close(get_service_loop(), &ops->close_req, ops->open_req.result, gap_bt_uv_op_on_close);
}

static void gap_bt_bond_load_on_close(uv_fs_t* req)
{
    gap_bt_uv_ops* ops = (gap_bt_uv_ops*)req->data;
    if (!ops) {
        BT_LOGE("fail,  ops nullptr");
        return;
    }

    ops->clean_req.data = ops;
    gap_bt_uv_op_on_clean(&ops->clean_req);
}

static void gap_bt_bond_load(const char* file)
{
    gap_bt_uv_ops* ops = malloc(sizeof(gap_bt_uv_ops));
    if (!ops) {
        BT_LOGE("malloc");
        return;
    }
    memset(ops, 0, sizeof(gap_bt_uv_ops));
    ops->on_open = gap_bt_bond_load_on_open;
    ops->on_read = gap_bt_bond_load_on_read;
    ops->on_close = gap_bt_bond_load_on_close;
    ops->on_clean = gap_bt_bond_load_on_clean;
    ops->open_req.data = ops;

    int rc = uv_fs_open(get_service_loop(), &ops->open_req, file, O_RDONLY, S_IWUSR | S_IRUSR, gap_bt_uv_op_on_open);
    if (rc < 0) {
        BT_LOGE(" failed to open file:%s", file)
        ops->clean_req.data = ops;
        gap_bt_uv_op_on_clean(&ops->clean_req);
        return;
    }
}

void gap_bluetooth_bond_init(void)
{
    BT_LOGD("%s", __func__);
    gap_bt_bond_load(BT_APP_INFO_FILE_NAME);
    gap_bt_bond_load(LE_APP_INFO_FILE_NAME);
}
