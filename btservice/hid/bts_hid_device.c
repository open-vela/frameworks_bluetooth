/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#define LOG_TAG "bts_hidd"
#include "bts_hid_device.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bts_service.h"
#include "log.h"
#include "stack_adapter_hid.h"
#include "utils.h"

#define HIDD_UNINPT_DEV "/dev/kbd"

static int hidd_fd = -1;
static uv_poll_t* hidd_uv_handle = NULL;
static profile_state_t current_state = SERVICE_PROFILE_DISCONNECTED;

typedef struct
{
    enum {
        ON_HIDD_APP_STATE_CHANGED = 0,
        ON_HIDD_CONNECTION_STATE_CHANGED,
    } event;

    bts_hidd_hdl_t* handle;
    size_t size;
    void* data;
} bts_hidd_msg_t;

typedef struct {
    bt_address remote_addr;
    profile_state_t state;
} bts_hidd_conn_s;

static void send_msg(bts_hidd_msg_t* msg);
static void handle_msg_received(bt_profile_id id, void* data, size_t size);

static struct list_node hidd_list = LIST_INITIAL_VALUE(hidd_list);

static int8_t gen_hid_dev_id(void)
{
    uint8_t found = 0;
    bts_hidd_hdl_t* handle;
    for (uint8_t i = 1; i < 256; i++) {
        list_for_every_entry(&hidd_list, handle, bts_hidd_hdl_t, node)
        {
            if (handle->device_id == i) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return i;
        }
    }
    BT_LOGE("handle id overflow");
    return -1;
}

#if 0
static bts_hidd_hdl_t* find_hidd_handle(bt_address addr)
{
    bts_hidd_hdl_t* hidd;
    list_for_every_entry(&hidd_list, hidd, bts_hidd_hdl_t, node)
    {
        if (!memcmp(hidd->remote_addr, addr, sizeof(bt_address))) {
            return hidd;
        }
    }
    return NULL;
}
#endif

static bts_hidd_hdl_t* find_hidd_handle2(uint8_t dev_id)
{
    bts_hidd_hdl_t* hidd;
    list_for_every_entry(&hidd_list, hidd, bts_hidd_hdl_t, node)
    {
        if (hidd->device_id == dev_id) {
            return hidd;
        }
    }
    return NULL;
}

static bool add_hidd_handle(bts_hidd_hdl_t handle)
{
    bts_hidd_hdl_t* hidd = (bts_hidd_hdl_t*)malloc(sizeof(bts_hidd_hdl_t));
    CHECK_PTR_RETURN(hidd, false);

    memset(hidd, 0, sizeof(bts_hidd_hdl_t));
    hidd->device_id = gen_hid_dev_id();
    hidd->callbacks = handle.callbacks;
    hidd->btm_handle = handle.btm_handle;
    memcpy(hidd->remote_addr, handle.remote_addr, sizeof(bt_address));
    list_add_tail(&hidd_list, &hidd->node);
    return true;
}

static bool remove_hid_device(bts_hidd_hdl_t* hidd)
{
    list_delete(&hidd->node);
    free(hidd);
    return true;
}

static bts_hidd_msg_t* create_adp_msg(uint8_t event, bts_hidd_hdl_t* handle, void* data, size_t size)
{
    bts_hidd_msg_t* msg = (bts_hidd_msg_t*)malloc(sizeof(bts_hidd_msg_t));
    CHECK_PTR_RETURN(msg, NULL);

    if (size < 0) {
        BT_LOGE("fail, invlaid size:%d", size);
        return NULL;
    }
    msg->event = event;
    msg->handle = handle;
    msg->size = size;
    if (size == 0) {
        return msg;
    }

    void* value = malloc(size);
    if (!value) {
        BT_LOGE("fail, malloc data");
        free(msg);
        return NULL;
    }
    memcpy(value, data, size);
    msg->data = value;

    return msg;
}

static void on_hidd_register_changed_callback(hid_app_state_t registered)
{
    bts_hidd_hdl_t* hidd;
    list_for_every_entry(&hidd_list, hidd, bts_hidd_hdl_t, node)
    {
        bts_hidd_msg_t* msg = create_adp_msg(ON_HIDD_APP_STATE_CHANGED, hidd, &registered, sizeof(hid_app_state_t));
        CHECK_PTR(msg);
        send_msg(msg);
    }
}

static void on_hidd_connection_changed_callback(bt_address remote_addr, profile_state_t state)
{
    BT_LOGD("%s, remote_addr:[%s] state:%d", __func__, addr_str(remote_addr), state);
    current_state = state;
    bts_hidd_conn_s conn;
    memcpy(conn.remote_addr, remote_addr, sizeof(bt_address));
    conn.state = state;
    bts_hidd_hdl_t* hidd;
    list_for_every_entry(&hidd_list, hidd, bts_hidd_hdl_t, node)
    {
        bts_hidd_msg_t* msg = create_adp_msg(ON_HIDD_CONNECTION_STATE_CHANGED, hidd, &conn, sizeof(bts_hidd_conn_s));
        CHECK_PTR(msg);
        send_msg(msg);
    }
}

static void on_hidd_get_report_callback(uint8_t rpt_type, uint8_t rpt_id, uint16_t buffer_size)
{
    BT_LOGD(" %s, rpt_type:%d, rpt_id:%d, buffer_size:%d", __func__, rpt_type, rpt_id, buffer_size);
    uint8_t rpt_data[] = { 0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    rpt_data[0] = rpt_id;
    service_adapter_hid_device_get_report_response(rpt_type, rpt_data, sizeof(rpt_data));
}

static void on_hidd_set_report_callback(uint8_t rpt_type, uint16_t rpt_size, uint8_t* rpt_data)
{
    BT_LOGD("%s, Report Data [T-%d, L-%d]:", __func__, rpt_type, rpt_size);
    while (rpt_size) {
        BT_LOGD(" %02x", *rpt_data);
        rpt_size--;
        rpt_data++;
    }
    service_adapter_hid_device_report_error(BTHID_OK);
}

static void on_hidd_set_protocol_callback(uint8_t protocol)
{
    BT_LOGD("%s,  protocol: %02x", __func__, protocol);
}

static void on_hidd_intr_data_callback(uint8_t rpt_type, uint16_t rpt_size, uint8_t* rpt_data)
{
    BT_LOGD("%s Report Data [T-%d, L-%d]:", __func__, rpt_type, rpt_size);
    while (rpt_size) {
        BT_LOGD(" %02x", *rpt_data);
        rpt_size--;
        rpt_data++;
    }
}

static void on_hidd_device_virtual_unplug_callback(void)
{
    BT_LOGD("%s", __func__);
}

static HID_DEVICE_CALLBACKS_S hid_device_cb = {
    .size = sizeof(HID_DEVICE_CALLBACKS_S),

    .bthd_app_state_cb = on_hidd_register_changed_callback,
    .bthd_connection_state_changed_cb = on_hidd_connection_changed_callback,
    .bthd_get_report_cb = on_hidd_get_report_callback,
    .bthd_set_report_cb = on_hidd_set_report_callback,
    .bthd_set_protocol_cb = on_hidd_set_protocol_callback,
    .bthd_intr_data_cb = on_hidd_intr_data_callback,
    .bthd_virtual_cable_unplug_cb = on_hidd_device_virtual_unplug_callback,
};

static void hid_uv_poll_callback(uv_poll_t* handle, int status, int events)
{
    if (status < 0) {
        BT_LOGE("fail, hid_uv_poll_callback status:%d", status);
        return;
    }

    if (events & UV_READABLE) {
        char buffer[256];
        ssize_t nbytes = read(hidd_fd, buffer, 256);
        if (nbytes < 0) {
            BT_LOGE("read failed: %d, err:%s", nbytes, strerror(errno));
            return;
        }

        if (current_state != SERVICE_PROFILE_CONNECTED) {
            BT_LOGE("hidd not connected, current_state:%d", current_state);
            return;
        }
        SERVICE_GATT_STATUS ret2 = service_adapter_hid_device_send_intr_report(0, (unsigned char*)buffer, nbytes);
        if (ret2 != GATT_SUCCESS) {
            BT_LOGE("fail, hid_device_send_intr_report, ret:%d", ret2);
        }
    }

    if (events & UV_DISCONNECT) {
        BT_LOGD("disconnect:%d", hidd_fd);
        bts_uv_poll_stop(hidd_uv_handle);
    }
}

static bt_result_code hid_device_init(void)
{
    bts_register_profile_process(BT_PROFILE_HIDDEV_ID, &handle_msg_received);
    SERVICE_GATT_STATUS ret = service_adapter_hid_device_init(&hid_device_cb);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, hid_device_init failed: %d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static void hid_device_cleanup(void)
{
    service_adapter_hid_device_cleanup();
    bts_unregister_profile_process(BT_PROFILE_HIDDEV_ID);
}

static bt_result_code hid_device_register_device(bts_hidd_hdl_t handle, bt_hidd_sdp_settings_t sdp, bt_hidd_qos_settings_t tx_qos, bt_hidd_qos_settings_t rx_qos)
{
    SERVICE_GATT_STATUS ret = service_adapter_hid_device_register_app(&sdp, NULL, NULL);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, hid_device_register_app failed, error: %d", ret);
        return BT_RESULT_FAILED;
    }

    bool ret2 = add_hidd_handle(handle);
    if (!ret2) {
        BT_LOGE("fail, add_hidd_handle, err:%d", ret2);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code hid_device_unregister_device(uint16_t device_id)
{
    bts_hidd_hdl_t* handle = find_hidd_handle2(device_id);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    SERVICE_GATT_STATUS ret = service_adapter_hid_device_unregister_app();
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, unregister_device, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code hid_device_connect(uint16_t device_id, bt_address remote_addr)
{
    bts_hidd_hdl_t* handle = find_hidd_handle2(device_id);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    memcpy(handle->remote_addr, remote_addr, sizeof(bt_address));
    SERVICE_GATT_STATUS ret = service_adapter_hid_device_connect(handle->remote_addr);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, hid_device_connect, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code hid_device_disconnect(uint8_t device_id, bt_address remote_addr)
{
    bts_hidd_hdl_t* handle = find_hidd_handle2(device_id);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    SERVICE_GATT_STATUS ret = service_adapter_hid_device_disconnect();
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, hid_device_disconnect, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code hid_device_send_report(uint8_t device_id, uint8_t report_id, uint8_t* buffer, size_t size)
{
    bts_hidd_hdl_t* handle = find_hidd_handle2(device_id);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    SERVICE_GATT_STATUS ret = service_adapter_hid_device_send_intr_report(report_id, buffer, size);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, hid_device_send_intr_report, ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code hid_device_unplug(uint8_t device_id, bt_address remote_addr)
{
    bts_hidd_hdl_t* handle = find_hidd_handle2(device_id);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    SERVICE_GATT_STATUS ret = service_adapter_hid_device_virtual_unplug();
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, device_virtual_unplug, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static const bts_hidd_interface_t hid_device_instance = {
    .size = sizeof(hid_device_instance),

    .init = hid_device_init,
    .clean_up = hid_device_cleanup,
    .register_device = hid_device_register_device,
    .unregister_device = hid_device_unregister_device,
    .connect = hid_device_connect,
    .disconnect = hid_device_disconnect,
    .send_report_test = hid_device_send_report,
    .unplug = hid_device_unplug,
};

const bts_hidd_interface_t* get_bts_hidd_interface(void)
{
    return &hid_device_instance;
}

static void handle_msg_received(bt_profile_id id, void* data, size_t size)
{
    if (id != BT_PROFILE_HIDDEV_ID) {
        BT_LOGE("error, invalid priofile id:%d", id);
        return;
    }
    BT_LOGD("%s", __func__);
    bts_hidd_msg_t* msg = (bts_hidd_msg_t*)(data);
    if (!msg) {
        BT_LOGE("%s fail, msg null", __func__);
        return;
    }
    bts_hidd_hdl_t* handle = (bts_hidd_hdl_t*)(msg->handle);
    if (!handle) {
        BT_LOGE("%s fail, handle null", __func__);
        return;
    }

    switch (msg->event) {
    case ON_HIDD_APP_STATE_CHANGED: {
        hid_app_state_t* registered = (hid_app_state_t*)(msg->data);
        if (!*registered) {
            BT_LOGD("unregistered, remove_hid_device handle");
            remove_hid_device(handle);
            close(hidd_fd);
            bts_uv_poll_stop(hidd_uv_handle);
            hidd_fd = 0;
            hidd_uv_handle = NULL;
            return;
        }

        hidd_fd = open(HIDD_UNINPT_DEV, O_RDONLY);
        if (hidd_fd < 0) {
            BT_LOGE("open(%s) failed: %s", HIDD_UNINPT_DEV, strerror(errno));
            return;
        }

        hidd_uv_handle = bts_uv_poll_start(hidd_fd, UV_READABLE | UV_DISCONNECT, hid_uv_poll_callback, NULL);
        if (!hidd_uv_handle) {
            BT_LOGE("fail, bts_uv_poll_start");
            return;
        }
        BT_CBACK(handle->callbacks, bts_hidd_app_state_changed_cb, handle->btm_handle, handle->device_id, *registered);
        break;
    }
    case ON_HIDD_CONNECTION_STATE_CHANGED: {
        bts_hidd_conn_s* conn = (bts_hidd_conn_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_hidd_connection_state_changed_cb, handle->btm_handle, conn->remote_addr, conn->state);
        break;
    }
    default: {
        BT_LOGW("invalid event:%d", msg->event);
        break;
    }
    }
    if (msg->size > 0)
        free(msg->data);
    free(msg);
}

static void send_msg(bts_hidd_msg_t* msg)
{
    bts_send_uv_msg(BT_PROFILE_HIDDEV_ID, msg, sizeof(bts_hidd_msg_t));
}
