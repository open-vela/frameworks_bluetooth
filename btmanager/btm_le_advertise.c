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
    uint8_t advertiser_id;
    btm_le_advertise_callbacks* cb;
} btm_leadv_hdl_t;

static btm_interface_t* bt_mgr_interface = NULL;
static bts_le_advertise_interface_t* advertiser_interface = NULL;

static void on_le_advertise_started(btm_leadv_hdl_t* handle, uint8_t adv_id)
{
    CHECK_PTR(handle);
    handle->advertiser_id = adv_id;
    BT_CBACK(handle->cb, le_advertise_started_cb, handle);
}

static void on_le_advertise_stopped(btm_leadv_hdl_t* handle, uint8_t adv_id)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_advertise_stopped_cb, handle);
    free(handle);
}

static void on_le_advertise_failed(btm_leadv_hdl_t* handle, int error)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->cb, le_advertise_failed_cb, handle, error);
}

static bts_ble_advertiser_callbacks bts_le_advertise_cb = {
    .bts_le_advertise_started_cb = on_le_advertise_started,
    .bts_le_advertise_stopped_cb = on_le_advertise_stopped,
    .bts_le_advertise_failed_cb = on_le_advertise_failed,
};

static bt_result_code start_advertising(btm_leadv_hdl_t** handle_ptr, advertise_param_t* param,
    btm_le_advertise_callbacks* cb)
{
    CHECK_PTR_RETURN(advertiser_interface, BT_RESULT_STATE_NOT_ON);

    *handle_ptr = (btm_leadv_hdl_t*)malloc(sizeof(btm_leadv_hdl_t));
    memset(*handle_ptr, 0, sizeof(btm_leadv_hdl_t));
    (*handle_ptr)->cb = cb;

    bts_leadv_hdl_t client = {
        .param = param,
        .callbacks = &bts_le_advertise_cb,
        .btm_handle = *handle_ptr,
    };
    bt_result_code ret = advertiser_interface->start_adv(client);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, start_adv err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code stop_advertising(btm_leadv_hdl_t* handle)
{
    CHECK_PTR_RETURN(advertiser_interface, BT_RESULT_STATE_NOT_ON);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    bt_result_code ret = advertiser_interface->stop_adv(handle->advertiser_id);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,stop_adv err:%d", ret);
        free(handle);
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
    gatt_interface_t* interface = bt_mgr_interface->get_profile_interface(BT_PROFILE_GATT);
    if (!interface) {
        BT_LOGE("fail, get_profile_interface gatt");
        return NULL;
    }

    advertiser_interface = interface->advertiser;
    return &le_advertise_interface;
}
