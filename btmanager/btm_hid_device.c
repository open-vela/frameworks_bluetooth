/****************************************************************************
 * frameworks/bluetooth/src/btmanager/btm_hid_device.c
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
#define LOG_TAG "btm_hidd"

#include "btm_hid_device.h"

#include <stdlib.h>
#include <string.h>

#include "btm_manager.h"
#include "bts_hid_service.h"
#include "log.h"

typedef struct
{
    void** handle_ptr;
    bt_address remote_addr;
    const bt_hid_device_callbacks* callbacks;
    uint8_t device_id;
} btm_hidd_hdl_t;

static btm_interface_t* bt_mgr_interface = NULL;
static const bts_hidd_interface_t* hidd_interface = NULL;

static void hidd_app_state_changed(void* hdl, uint8_t device_id, hid_app_state registered)
{
    BT_LOGD("PERFORMANCE-HID-DEV-PROFILE-BTM-APP-STATE:%d", registered);
    btm_hidd_hdl_t* handle = (btm_hidd_hdl_t*)hdl;
    CHECK_PTR(handle);
    handle->device_id = device_id;
    BT_CBACK(handle->callbacks, hidd_app_state_changed_cb, handle, registered);
    if (!registered) {
        void** handle_ptr = handle->handle_ptr;
        free(handle);
        *handle_ptr = NULL;
    }
}

static void on_bts_hidd_connection_state_changed(void* hdl, bt_address remote_addr, profile_connection_state state)
{
    BT_LOGD("PERFORMANCE-HID-DEV-PROFILE-BTM-CONNECTION-STATE:%d", state);
    btm_hidd_hdl_t* handle = (btm_hidd_hdl_t*)hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, hidd_connection_state_changed_cb, handle, remote_addr, state);
}

static bts_hid_device_callbacks bts_hidd_cb = {
    .bts_hidd_app_state_changed_cb = hidd_app_state_changed,
    .bts_hidd_connection_state_changed_cb = on_bts_hidd_connection_state_changed,
};

static bt_result_code hidd_register_device(void** ptr, bt_hidd_sdp_settings_t sdp, bt_hidd_qos_settings_t tx_qos, bt_hidd_qos_settings_t rx_qos, bt_hid_device_callbacks* cb)
{
    btm_hidd_hdl_t** handle_ptr = (btm_hidd_hdl_t**)ptr;
    CHECK_PTR_RETURN(hidd_interface, BT_RESULT_STATE_NOT_ON);
    *handle_ptr = (btm_hidd_hdl_t*)malloc(sizeof(btm_hidd_hdl_t));
    memset(*handle_ptr, 0, sizeof(btm_hidd_hdl_t));
    (*handle_ptr)->callbacks = cb;
    (*handle_ptr)->handle_ptr = (void**)handle_ptr;

    bts_hidd_hdl_t hidd = {
        .callbacks = &bts_hidd_cb,
        .btm_handle = *handle_ptr,
    };

    bt_result_code ret = hidd_interface->register_device(hidd, sdp, tx_qos, rx_qos);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,register_device err:%d", ret);
        free(*handle_ptr);
        *handle_ptr = NULL;
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code hidd_unregister_device(void* hdl)
{
    btm_hidd_hdl_t* handle = (btm_hidd_hdl_t*)hdl;
    CHECK_PTR_RETURN(hidd_interface, BT_RESULT_STATE_NOT_ON);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    bt_result_code ret = hidd_interface->unregister_device(handle->device_id);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,unregister_device err:%d", ret);
        void** handle_ptr = handle->handle_ptr;
        free(handle);
        *handle_ptr = NULL;
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code hidd_connect(void* hdl, bt_address remote_addr)
{
    BT_LOGD("PERFORMANCE-HID-DEV-PROFILE-BTM-CONNECT-START");
    btm_hidd_hdl_t* handle = (btm_hidd_hdl_t*)hdl;
    CHECK_PTR_RETURN(hidd_interface, BT_RESULT_STATE_NOT_ON);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    bt_result_code ret = hidd_interface->connect(handle->device_id, remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,connect err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code hidd_disconnect(void* hdl, bt_address remote_addr)
{
    BT_LOGD("PERFORMANCE-HID-DEV-PROFILE-BTM-DISCONNECT-START");
    btm_hidd_hdl_t* handle = (btm_hidd_hdl_t*)hdl;
    CHECK_PTR_RETURN(hidd_interface, BT_RESULT_STATE_NOT_ON);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    bt_result_code ret = hidd_interface->disconnect(handle->device_id, remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,disconnect err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code hidd_unplug(void* hdl, bt_address remote_addr)
{
    btm_hidd_hdl_t* handle = (btm_hidd_hdl_t*)hdl;
    CHECK_PTR_RETURN(hidd_interface, BT_RESULT_STATE_NOT_ON);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    bt_result_code ret = hidd_interface->unplug(handle->device_id, remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,unplug err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code hidd_send_report(void* hdl, uint8_t report_id, uint8_t* buffer, size_t size)
{
    btm_hidd_hdl_t* handle = (btm_hidd_hdl_t*)hdl;
    CHECK_PTR_RETURN(hidd_interface, BT_RESULT_STATE_NOT_ON);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    bt_result_code ret = hidd_interface->send_report(handle->device_id, report_id, buffer, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail,send_report err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static btm_hid_device_interface_t bt_hidd_interface = {
    .size = sizeof(bt_hidd_interface),

    .register_device = hidd_register_device,
    .unregister_device = hidd_unregister_device,
    .connect = hidd_connect,
    .disconnect = hidd_disconnect,
    .send_report = hidd_send_report,
    .unplug = hidd_unplug,
};

btm_hid_device_interface_t* get_btm_hid_device_interface(void* bt_mgr)
{
    if (!bt_mgr) {
        BT_LOGE("fail, bt_mgr NULL");
        return NULL;
    }
    bt_mgr_interface = (btm_interface_t*)bt_mgr;
    hid_interface_t* interface = (hid_interface_t*)bt_mgr_interface->get_profile_interface(BT_PROFILE_HIDDEV);
    if (!interface) {
        BT_LOGE("fail, get_profile_interface hid");
        return NULL;
    }
    hidd_interface = interface->hidd;
    return &bt_hidd_interface;
}