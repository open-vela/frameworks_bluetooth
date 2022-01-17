/****************************************************************************
 * frameworks/bluetooth/src/btmanager/btm_le_scan.c
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
#define LOG_TAG "btm_lescan"

#include "btm_le_scan.h"

#include <stdlib.h>
#include <string.h>

#include "btm_manager.h"
#include "bts_gatt_service.h"
#include "bts_le_scan.h"
#include "log.h"

typedef struct {
    void** handle_ptr;
    uint8_t scanner_id;
    btm_le_scan_callbacks* cb;
} btm_lescan_hdl_t;

static btm_interface_t* bt_mgr_interface = NULL;
static const bts_le_scan_interface_t* scanner_interface = NULL;

static void on_le_scan_result(void* hdl, const scan_result_t* result)
{
    btm_lescan_hdl_t* handle = (btm_lescan_hdl_t*)hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_scan_result_cb, handle, result);
}

static void on_le_scan_failed(void* hdl, int error)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BTM-SCAN-FAILED");
    btm_lescan_hdl_t* handle = (btm_lescan_hdl_t*)hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_scan_failed_cb, handle, error);
    void** handle_ptr = handle->handle_ptr;
    free(handle);
    *handle_ptr = NULL;
}

static void on_le_scan_started(void* hdl, uint8_t scanner_id)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BTM-SCAN-STARTED");
    btm_lescan_hdl_t* handle = (btm_lescan_hdl_t*)hdl;
    CHECK_PTR(handle);
    handle->scanner_id = scanner_id;
    BT_CBACK(handle->cb, le_scan_started_cb, handle);
}

static void on_le_scan_stopped(void* hdl)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BTM-SCAN-STOPPED");
    btm_lescan_hdl_t* handle = (btm_lescan_hdl_t*)hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_scan_stopped_cb, handle);
    void** handle_ptr = handle->handle_ptr;
    free(handle);
    *handle_ptr = NULL;
}

static bts_ble_scanner_callbacks bts_le_scan_cb = {
    .bts_le_scan_result_cb = on_le_scan_result,
    .bts_ble_scan_failed_cb = on_le_scan_failed,
    .bts_ble_scan_started_cb = on_le_scan_started,
    .bts_ble_scan_stopped_cb = on_le_scan_stopped,
};

static bt_result_code start_scan(void** hdl_ptr, ble_scan_filter_t* filter, scan_params_t* setttings,
    btm_le_scan_callbacks* cb)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BTM-SCAN-START");
    CHECK_PTR_RETURN(scanner_interface, BT_RESULT_STATE_NOT_ON);

    btm_lescan_hdl_t** handle_ptr = (btm_lescan_hdl_t**)(hdl_ptr);
    *handle_ptr = (btm_lescan_hdl_t*)malloc(sizeof(btm_lescan_hdl_t));
    memset(*handle_ptr, 0, sizeof(btm_lescan_hdl_t));
    (*handle_ptr)->cb = cb;
    (*handle_ptr)->handle_ptr = (void**)handle_ptr;

    bts_lescan_hdl_t client = {
        .filter = filter,
        .settings = setttings,
        .callbacks = &bts_le_scan_cb,
        .btm_handle = *handle_ptr,
    };

    bt_result_code ret = scanner_interface->start_scan(client);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,start_scan err:%d", ret);
        free(*handle_ptr);
        *handle_ptr = NULL;
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code stop_scan(void* hdl)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BTM-SCAN-STOP");
    CHECK_PTR_RETURN(scanner_interface, BT_RESULT_STATE_NOT_ON);
    btm_lescan_hdl_t* handle = (btm_lescan_hdl_t*)(hdl);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    bt_result_code ret = scanner_interface->stop_scan(handle->scanner_id);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,stop_scan err:%d", ret);
        void** handle_ptr = handle->handle_ptr;
        free(handle);
        *handle_ptr = NULL;
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static btm_le_scan_interface_t le_scan_interface = {
    .size = sizeof(le_scan_interface),

    .start_scan = start_scan,
    .stop_scan = stop_scan,
};

btm_le_scan_interface_t* get_btm_lescan_interface(void* bt_mgr)
{
    if (!bt_mgr) {
        BT_LOGE("fail, bt_mgr NULL");
        return NULL;
    }
    bt_mgr_interface = (btm_interface_t*)(bt_mgr);
    const gatt_interface_t* interface = (gatt_interface_t*)bt_mgr_interface->get_profile_interface(BT_PROFILE_GATT);
    if (!interface) {
        BT_LOGE("fail, get_profile_interface gatt");
        return NULL;
    }

    scanner_interface = interface->scanner;
    return &le_scan_interface;
}