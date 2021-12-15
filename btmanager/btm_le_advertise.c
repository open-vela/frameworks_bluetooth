/****************************************************************************
 * frameworks/bluetooth/src/btmanager/btm_le_advertise.c
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
#define LOG_TAG "btm_leadv"

#include "btm_le_advertise.h"

#include <stdlib.h>

#include "btm_manager.h"
#include "bts_gatt_service.h"
#include "bts_le_advertise.h"

#include "log.h"

typedef struct {
    void** handle_ptr;
    uint8_t advertiser_id;
    btm_le_advertise_callbacks* cb;
} btm_leadv_hdl_t;

static btm_interface_t* bt_mgr_interface = NULL;
static const bts_le_advertise_interface_t* advertiser_interface = NULL;

static void on_le_advertise_started(void* hdl, uint8_t adv_id)
{
    btm_leadv_hdl_t* handle = (btm_leadv_hdl_t*)hdl;
    CHECK_PTR(handle);
    handle->advertiser_id = adv_id;
    BT_CBACK(handle->cb, le_advertise_started_cb, handle);
}

static void on_le_advertise_stopped(void* hdl, uint8_t adv_id)
{
    btm_leadv_hdl_t* handle = (btm_leadv_hdl_t*)hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_advertise_stopped_cb, handle);
    void** handle_ptr = handle->handle_ptr;
    free(handle);
    *handle_ptr = NULL;
}

static void on_le_advertise_failed(void* hdl, int error)
{
    btm_leadv_hdl_t* handle = (btm_leadv_hdl_t*)hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_advertise_failed_cb, handle, error);
    void** handle_ptr = handle->handle_ptr;
    free(handle);
    *handle_ptr = NULL;
}

static bts_ble_advertiser_callbacks bts_le_advertise_cb = {
    .bts_le_advertise_started_cb = on_le_advertise_started,
    .bts_le_advertise_stopped_cb = on_le_advertise_stopped,
    .bts_le_advertise_failed_cb = on_le_advertise_failed,
};

static BT_RESULT_CODE start_advertising(void** hdl_ptr, advertise_param_t* param,
    btm_le_advertise_callbacks* cb)
{
    CHECK_PTR_RETURN(advertiser_interface, BT_RESULT_STATE_NOT_ON);

    btm_leadv_hdl_t** handle_ptr = (btm_leadv_hdl_t**)(hdl_ptr);
    *handle_ptr = (btm_leadv_hdl_t*)malloc(sizeof(btm_leadv_hdl_t));
    memset(*handle_ptr, 0, sizeof(btm_leadv_hdl_t));
    (*handle_ptr)->cb = cb;
    (*handle_ptr)->handle_ptr = (void**)handle_ptr;

    bts_leadv_hdl_t client = {
        .param = param,
        .callbacks = &bts_le_advertise_cb,
        .btm_handle = *handle_ptr,
    };
    BT_RESULT_CODE ret = advertiser_interface->start_adv(client);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, start_adv err:%d", ret);
        free(*handle_ptr);
        *handle_ptr = NULL;
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static BT_RESULT_CODE stop_advertising(void* hdl)
{
    CHECK_PTR_RETURN(advertiser_interface, BT_RESULT_STATE_NOT_ON);
    btm_leadv_hdl_t* handle = (btm_leadv_hdl_t*)(hdl);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    BT_RESULT_CODE ret = advertiser_interface->stop_adv(handle->advertiser_id);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,stop_adv err:%d", ret);
        void** handle_ptr = handle->handle_ptr;
        free(handle);
        *handle_ptr = NULL;
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static btm_le_advertise_interface_t le_advertise_interface = {
    .size = sizeof(le_advertise_interface),

    .start_advertising = start_advertising,
    .stop_advertising = stop_advertising,
};

btm_le_advertise_interface_t* get_btm_leadv_interface(void* bt_mgr)
{
    if (!bt_mgr) {
        BT_LOGE("fail, bt_mgr NULL");
        return NULL;
    }

    bt_mgr_interface = (btm_interface_t*)(bt_mgr);
    const gatt_interface_t* interface = bt_mgr_interface->get_profile_interface(BT_PROFILE_GATT);
    if (!interface) {
        BT_LOGE("fail, get_profile_interface gatt");
        return NULL;
    }

    advertiser_interface = interface->advertiser;
    return &le_advertise_interface;
}
