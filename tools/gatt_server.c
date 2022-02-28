/****************************************************************************
 * frameworks/bluetooth/samples/test_gatts.c
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
#define LOG_TAG "bttool_gatts"

#include <debug.h>
#include <nuttx/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bt_tools.h"
#include "btm_gatt_server.h"
#include "btm_le_advertise.h"
#include "btm_manager.h"
#include "log.h"

#define THROUGHTPUT_HORIZON 2

typedef struct {
    struct list_node node;
    uint16_t gatt_mtu;
    bt_address remote_address;
} gatts_device_t;

typedef struct {
    struct list_node node;
    void* adv_handle;
    uint8_t adv_id;
} advertise_handle_t;

static void* gatts_handle;
static void* adv_handle[10];
static btm_gatt_server_interface_t* gatts_interface = NULL;
static btm_interface_t* manager = NULL;
static volatile uint16_t throughtput_cursor = 1;
static struct list_node gatts_device_list = LIST_INITIAL_VALUE(gatts_device_list);
static struct list_node advertise_handle_list = LIST_INITIAL_VALUE(advertise_handle_list);

enum {
    /* IDs of Private IOT service */
    IOT_SERVICE_ID = 1,
    IOT_SERVICE_TX_CHR_ID,
    IOT_SERVICE_TX_CHR_CCC_ID,
    IOT_SERVICE_RX_CHR_ID,
    IOT_SERVICE_READ_CHR_ID,
};

static gatt_element_t s_iot_service_elements[] = {
    { /* Private IOT Service - 0xFF00 */
        IOT_SERVICE_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00 },
        GATT_PRIMARY_SERVICE,
        0,
        GATT_ATT_PERMISSION_READABLE },
    { /* Private Characteristic for TX - 0xFF01 */
        IOT_SERVICE_TX_CHR_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00 },
        GATT_CHARACTERISTIC,
        GATT_ATT_PROPERTY_NOTIFY | GATT_ATT_PROPERTY_INDICATE,
        0 },
    { /* Client Characteristic Configuration Descriptor - 0x2902 */
        IOT_SERVICE_TX_CHR_CCC_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x02, 0x29, 0x00, 0x00 },
        GATT_DESCRIPTOR,
        0,
        GATT_ATT_PERMISSION_READABLE | GATT_ATT_PERMISSION_WRITABLE | GATT_ATT_PERMISSION_AUTHENTICATION_REQUIRED },
    { /* Private Characteristic for RX - 0xFF02 */
        IOT_SERVICE_RX_CHR_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00 },
        GATT_CHARACTERISTIC,
        GATT_ATT_PROPERTY_WRITE_NO_RESPONSE,
        GATT_ATT_PERMISSION_WRITABLE },
    { /* Private Characteristic for read operation demo - 0xFF05 */
        IOT_SERVICE_READ_CHR_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x05, 0xFF, 0x00, 0x00 },
        GATT_CHARACTERISTIC,
        GATT_ATT_PROPERTY_READ,
        GATT_ATT_PERMISSION_READABLE },
};

static gatts_device_t* find_gatts_device(bt_address remote_address)
{
    gatts_device_t* device;
    list_for_every_entry(&gatts_device_list, device, gatts_device_t, node)
    {
        if (!memcmp(device->remote_address, remote_address, sizeof(bt_address))) {
            return device;
        }
    }
    return NULL;
}

static gatts_device_t* add_gatts_device(bt_address remote_address)
{
    gatts_device_t* device = (gatts_device_t*)malloc(sizeof(gatts_device_t));
    if (!device) {
        BT_LOGE("malloc device fail");
        return NULL;
    }

    memset(device, 0, sizeof(gatts_device_t));
    device->gatt_mtu = 23;
    memcpy(device->remote_address, remote_address, sizeof(bt_address));
    list_add_tail(&gatts_device_list, &device->node);
    return device;
}

static bool remove_gatts_device(gatts_device_t* device)
{
    if (!device) {
        return true;
    }
    list_delete(&device->node);
    free(device);
    return true;
}

static advertise_handle_t* find_advertise_handle(uint8_t adv_id)
{
    advertise_handle_t* handle;
    list_for_every_entry(&advertise_handle_list, handle, advertise_handle_t, node)
    {
        if (handle->adv_id == adv_id) {
            return handle;
        }
    }
    return NULL;
}

static advertise_handle_t* find_advertise_handle2(void* adv_handle)
{
    advertise_handle_t* handle;
    list_for_every_entry(&advertise_handle_list, handle, advertise_handle_t, node)
    {
        if (handle->adv_handle == adv_handle) {
            return handle;
        }
    }
    return NULL;
}

static void add_advertise_handle(void* adv_handle, uint8_t adv_id)
{
    advertise_handle_t* handle = (advertise_handle_t*)malloc(sizeof(advertise_handle_t));
    if (!handle) {
        BT_LOGE("malloc advertise_handle_t fail");
        return;
    }

    handle->adv_handle = adv_handle;
    handle->adv_id = adv_id;
    list_add_tail(&advertise_handle_list, &handle->node);
}

static bool remove_advertise_handle(advertise_handle_t* handle)
{
    if (!handle) {
        return false;
    }
    list_delete(&handle->node);
    free(handle);
    return true;
}

static void clear_advertise_handle(void)
{
    advertise_handle_t* handle;
    advertise_handle_t* handle_next;
    list_for_every_entry_safe(&advertise_handle_list, handle, handle_next, advertise_handle_t, node)
    {
        list_delete(&handle->node);
        free(handle);
    }
}

static void le_adv_started_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void le_adv_stopped_callback(void* handle)
{
    advertise_handle_t* advertise_handle = find_advertise_handle2(handle);
    if (!advertise_handle) {
        return;
    }
    BT_LOGD(" %s adv_id:%d", __func__, advertise_handle->adv_id);
    remove_advertise_handle(advertise_handle);
}

static void le_adv_failed_callback(void* handle, int error)
{
    BT_LOGD(" %s:err:%d", __func__, error);
    clear_advertise_handle();
}

static btm_le_advertise_callbacks le_adv_cb = {
    .le_advertise_started_cb = le_adv_started_callback,
    .le_advertise_stopped_cb = le_adv_stopped_callback,
    .le_advertise_failed_cb = le_adv_failed_callback,
};

static char* profile_state_to_str(profile_connection_state state)
{
    switch (state) {
    case PROFILE_DISCONNECTED: {
        return "profile disconnected";
    }
    case PROFILE_CONNECTING: {
        return "profile connecting";
    }
    case PROFILE_CONNECTED: {
        return "profile connected";
    }
    case PROFILE_DISCONNECTING: {
        return "profile disconnecting";
    }
    default: {
        return "unknown state";
    }
    }
}

static void test_server_connection_state_changed_callback(void* handle, bt_address remote_addr, profile_connection_state state)
{
    BT_LOGD("%s  addr:%s, state:%s", __func__, addr_str(remote_addr), profile_state_to_str(state));
    if (state == PROFILE_DISCONNECTED) {
        gatts_device_t* device = find_gatts_device(remote_addr);
        remove_gatts_device(device);
    } else if (state == PROFILE_CONNECTED) {
        add_gatts_device(remote_addr);
    }
}

static void gatt_display_service(gatt_element_t* elements, uint16_t size)
{
    gatt_element_t* item = elements;
    gatt_element_t* item_end = elements + size;

    while (item < item_end) {
        switch (item->type) {
        case GATT_PRIMARY_SERVICE:
            BT_LOGD(">[%lu][PRI]", item->id);
            break;
        case GATT_SECONDARY_SERVICE:
            BT_LOGD(">[%lu][SND]", item->id);
            break;
        case GATT_INCLUDED_SERVICE:
            BT_LOGD(">  [%lu][INC]", item->id);
            break;
        case GATT_CHARACTERISTIC:
            BT_LOGD(">  [%lu][CHR]", item->id);
            break;
        case GATT_DESCRIPTOR:
            BT_LOGD(">    [%lu][DES]", item->id);
            break;
        }
        BT_LOGD("[PROP:%lu", item->properties);
        if (item->properties) {
            BT_LOGD(",");
            if (item->properties & GATT_ATT_PROPERTY_READ) {
                BT_LOGD("R");
            }
            if (item->properties & GATT_ATT_PROPERTY_WRITE_NO_RESPONSE) {
                BT_LOGD("Wn");
            }
            if (item->properties & GATT_ATT_PROPERTY_WRITE) {
                BT_LOGD("W");
            }
            if (item->properties & GATT_ATT_PROPERTY_NOTIFY) {
                BT_LOGD("N");
            }
            if (item->properties & GATT_ATT_PROPERTY_INDICATE) {
                BT_LOGD("I");
            }
        }
        BT_LOGD("]");

        BT_LOGD("[0x%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x]\r\n",
            item->uuid[15], item->uuid[14], item->uuid[13], item->uuid[12],
            item->uuid[11], item->uuid[10], item->uuid[9], item->uuid[8],
            item->uuid[7], item->uuid[6], item->uuid[5], item->uuid[4],
            item->uuid[3], item->uuid[2], item->uuid[1], item->uuid[0]);
        item++;
    }
    BT_LOGD(">");
}

static void test_server_opened_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void test_server_closed_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void test_server_service_added_callback(void* handle, gatt_status status, gatt_element_t* element,
    size_t size)
{
    BT_LOGD("%s, status: %d, size: %d", __func__, status, size);
    gatt_display_service(element, size);
}

static void test_server_service_removed_callback(void* handle, gatt_status status, gatt_element_t* element,
    size_t size)
{
    BT_LOGD("%s, status: %d, size: %d", __func__, status, size);
    gatt_display_service(element, size);
}

static void test_server_phy_read_callback(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx)
{
    BT_LOGD("%s, addr:%s, tx:%d, rx:%d", __func__, addr_str(remote_addr), tx, rx);
}

static void test_server_phy_update_callback(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx, gatt_status status)
{
    BT_LOGD("%s, addr:%s, status:%d, tx:%d, rx:%d", __func__, addr_str(remote_addr), status, tx, rx);
}

static void test_server_read_request_callback(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element)
{
    BT_LOGD("%s, addr:%s, request_id:%lu", __func__, addr_str(remote_addr), request_id);
    gatt_display_service(element, 1);
    gatt_response_t* rsp;
    if (element->id == IOT_SERVICE_READ_CHR_ID) {
        const char* read_rsp = "XIAOMI VELA BT";
        rsp = malloc(sizeof(gatt_response_t) + strlen(read_rsp) + 1);
        memset(rsp, 0, sizeof(gatt_response_t) + strlen(read_rsp) + 1);
        rsp->length = strlen(read_rsp) + 1;
        memcpy(rsp->value, read_rsp, strlen(read_rsp) + 1);
    } else {
        rsp = malloc(sizeof(gatt_response_t));
        memset(rsp, 0, sizeof(gatt_response_t));
        rsp->length = 0;
    }
    rsp->request_id = request_id;
    rsp->status = GATT_STATUS_SUCCESS;
    if (!gatts_interface) {
        BT_LOGE("fail,   gatt interface null");
        free(rsp);
        return;
    }
    gatts_interface->send_response(handle, remote_addr, rsp);
    free(rsp);
}

static void test_server_write_request_callback(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size)
{
    BT_LOGD("%s, addr:%s, request_id:%lu", __func__, addr_str(remote_addr), request_id);
    gatt_display_service(element, 1);
    gatt_response_t* rsp = malloc(sizeof(gatt_response_t));
    if (!rsp) {
        BT_LOGE("error, rsp malloc failed");
        return;
    }
    memset(rsp, 0, sizeof(gatt_response_t));
    switch (element->id) {
    case IOT_SERVICE_TX_CHR_CCC_ID: {
        uint16_t s_iot_tx_ccc_value = value[0] + (value[1] << 8);
        BT_LOGD(" IOT transmission %s", s_iot_tx_ccc_value ? "enabled" : "disabled");
        break;
    }
    case IOT_SERVICE_RX_CHR_ID: {
        BT_LOGD(" IOT received %d bytes,, offset:%d", size, offset);
        break;
    }
    default: {
        BT_LOGD(" write %d bytes from offset %d of element %lu", size, offset, element->id);
        break;
    }
    }
    BT_HEXDUMP(value, size);

    rsp->length = 0;
    rsp->request_id = request_id;
    rsp->status = GATT_STATUS_SUCCESS;
    if (!gatts_interface) {
        BT_LOGE("fail,   gatt interface null");
        free(rsp);
        return;
    }
    gatts_interface->send_response(handle, remote_addr, rsp);
    free(rsp);
}

static void test_server_mtu_changed_callback(void* handle, bt_address remote_addr, uint32_t mtu)
{
    BT_LOGD("%s, addr:%s, mtu: %lu", __func__, addr_str(remote_addr), mtu);
    gatts_device_t* device = find_gatts_device(remote_addr);
    if (device) {
        BT_LOGD("update device:%s, mtu:%lu", addr_str(remote_addr), mtu);
        device->gatt_mtu = mtu;
    }
}

static void test_server_notify_sent_callback(void* handle, bt_address remote_addr, gatt_status status)
{
    throughtput_cursor--;
    BT_LOGD("%s addr:%s, status:%d, throughtput_cursor:%d", __func__, addr_str(remote_addr), status, throughtput_cursor);
}

static void test_server_throughtout_notify(bt_address remote_addr, gatt_element_t* element, uint32_t times, uint16_t mtu)
{
    BT_LOGD("mtu:%u, times:%lu", mtu, times);
    uint8_t* payload = (uint8_t*)malloc(sizeof(uint8_t) * mtu);
    if (!payload) {
        BT_LOGD("malloc payload fail");
        return;
    }
    throughtput_cursor = 1;
    uint32_t msg_counter = 1;
    for (int i = 0; i < times; i++) {
        while (throughtput_cursor >= THROUGHTPUT_HORIZON) {
            usleep(500);
        }
        memset(payload, 1, mtu);
        payload[0] = (msg_counter >> 24) & 0xFF;
        payload[1] = (msg_counter >> 16) & 0xFF;
        payload[2] = (msg_counter >> 8) & 0xFF;
        payload[3] = msg_counter & 0xFF;
        bt_result_code code = gatts_interface->send_notify(gatts_handle, remote_addr, element, payload, mtu);
        throughtput_cursor++;
        BT_LOGD("send_notify times:%d, throughtput_cursor:%d", i, throughtput_cursor);
        if (code != BT_RESULT_SUCCESS) {
            BT_LOGE("fail, send_notify ret:%d", code);
            return;
        }
        msg_counter++;
    }
    free(payload);
}

static btm_gatt_server_callbacks gatts_cb = {
    .gatts_connection_state_changed_cb = test_server_connection_state_changed_callback,
    .gatts_server_opened_cb = test_server_opened_callback,
    .gatts_server_closed_cb = test_server_closed_callback,
    .gatts_service_added_cb = test_server_service_added_callback,
    .gatts_service_removed_cb = test_server_service_removed_callback,
    .gatts_phy_read_cb = test_server_phy_read_callback,
    .gatts_phy_update_cb = test_server_phy_update_callback,
    .gatts_read_request_cb = test_server_read_request_callback,
    .gatts_write_request_cb = test_server_write_request_callback,
    .gatts_mtu_changed_cb = test_server_mtu_changed_callback,
    .gatts_notify_sent_cb = test_server_notify_sent_callback,
};

static int gatts_open(void* handle, int argc, char** argv)
{
    if (!gatts_interface) {
        return -1;
    }
    BT_LOGD("open gatt server");
    bt_result_code ret = gatts_interface->open(&gatts_handle, &gatts_cb);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, open gatt interface ret: %d", ret);
        return 0;
    }
    return 0;
}

static int gatts_close(void* handle, int argc, char** argv)
{
    if (!gatts_interface) {
        return -1;
    }
    BT_LOGD("close gatt server");
    bt_result_code ret = gatts_interface->close(gatts_handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, close gatt interface ret: %d", ret);
        return 0;
    }
    return 0;
}

static int gatts_connect(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("connect remote_addr:%s", addr_str(remote_address));
    bt_result_code ret = gatts_interface->connect(gatts_handle, remote_address, true);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, connect  ret: %d", ret);
        return 0;
    }
    return 0;
}

static int gatts_disconnect(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("disconnect remote_addr:%s", addr_str(remote_address));
    gatts_device_t* device = find_gatts_device(remote_address);
    if (!device) {
        BT_LOGE("device%s not connected", addr_str(remote_address));
        return 0;
    }
    bt_result_code ret = gatts_interface->disconnect(gatts_handle, remote_address);
    if (ret != BT_RESULT_SUCCESS) {
        remove_gatts_device(device);
        BT_LOGD("fail, disconnect  ret: %d", ret);
    }
    return 0;
}

static int gatts_add(void* handle, int argc, char** argv)
{
    if (!gatts_interface) {
        return -1;
    }
    BT_LOGD("add gatt service");
    bt_result_code ret = gatts_interface->add_service(gatts_handle, s_iot_service_elements, sizeof(s_iot_service_elements) / sizeof(gatt_element_t));
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, add gatt service ret: %d", ret);
        return 0;
    }
    return 0;
}

static int gatts_remove(void* handle, int argc, char** argv)
{
    if (!gatts_interface) {
        return -1;
    }
    BT_LOGD("remove gatt service");
    uint32_t id = IOT_SERVICE_ID;
    bt_result_code ret = gatts_interface->remove_service(gatts_handle, &id, 1);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, remove gatt service ret: %d", ret);
        return 0;
    }
    return 0;
}

static int gatts_read_phy(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("read phy  remote_addr:%s", addr_str(remote_address));
    gatts_device_t* device = find_gatts_device(remote_address);
    if (!device) {
        BT_LOGE("device%s not connected", addr_str(remote_address));
        return 0;
    }
    bt_result_code ret = gatts_interface->read_phy(gatts_handle, remote_address);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read phy  ret: %d", ret);
        return 0;
    }
    return 0;
}

static int gatts_update_phy(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 3) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    int tx = atoi(argv[1]);
    int rx = atoi(argv[2]);
    BT_LOGD("update phy  remote_addr:%s, tx:%d, rx:%d", addr_str(remote_address), tx, rx);
    gatts_device_t* device = find_gatts_device(remote_address);
    if (!device) {
        BT_LOGE("device%s not connected", addr_str(remote_address));
        return 0;
    }
    bt_result_code ret = gatts_interface->update_phy(gatts_handle, remote_address, tx, rx);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, update phy  ret: %d", ret);
        return 0;
    }
    return 0;
}

static int gatts_send_notify(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);

    size_t size = strlen(argv[1]);
    uint8_t* payload = (uint8_t*)malloc(size);
    if (!payload) {
        BT_LOGE("error, failed to allocate payload");
        return 0;
    }
    memcpy(payload, argv[1], size);

    BT_LOGD("send notify   remote_addr:%s, value:", addr_str(remote_address));
    BT_HEXDUMP(payload, size);
    gatts_device_t* device = find_gatts_device(remote_address);
    if (!device) {
        free(payload);
        BT_LOGE("device%s not connected", addr_str(remote_address));
        return 0;
    }
    gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
    bt_result_code ret = gatts_interface->send_notify(gatts_handle, remote_address, element, payload, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read phy  ret: %d", ret);
    }
    free(payload);
    return 0;
}

static int gatts_send_indicate(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);

    size_t size = strlen(argv[1]);
    uint8_t* payload = (uint8_t*)malloc(size);
    if (!payload) {
        BT_LOGE("error, failed to allocate payload");
        return 0;
    }
    memcpy(payload, argv[1], size);

    BT_LOGD("send indicate   remote_addr:%s, value:", addr_str(remote_address));
    BT_HEXDUMP(payload, size);
    gatts_device_t* device = find_gatts_device(remote_address);
    if (!device) {
        free(payload);
        BT_LOGE("device%s not connected", addr_str(remote_address));
        return 0;
    }
    gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
    bt_result_code ret = gatts_interface->send_indicate(gatts_handle, remote_address, element, payload, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, send indicate  ret: %d", ret);
    }
    free(payload);
    return 0;
}

static int gatts_do_throughput(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    uint32_t times = atoi(argv[1]);
    BT_LOGD("throughtout remote_addr:%s, times:%lu", addr_str(remote_address), times);
    gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
    if (!element) {
        BT_LOGD("element null");
        return 0;
    }
    gatts_device_t* device = find_gatts_device(remote_address);
    if (!device) {
        BT_LOGE("device%s not connected", addr_str(remote_address));
        return 0;
    }
    test_server_throughtout_notify(remote_address, element, times, device->gatt_mtu);
    BT_LOGD("throughtout notify ...");
    return 0;
}

static int le_start_advertising(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 5) {
        return -1;
    }
    int adv_type = atoi(argv[0]);
    if (adv_type < BLE_EVENT_ADV_IND || adv_type > BLE_EVENT_SCAN_RSP) {
        BT_LOGE("invalid adv_type:%d", adv_type);
        return 0;
    }

    int interval = atoi(argv[1]);
    int duration = atoi(argv[2]);
    int filter_type = atoi(argv[3]);
    uint8_t adv_id = atoi(argv[4]);
    if (adv_id >= sizeof(adv_handle) / sizeof(adv_handle[0])) {
        BT_LOGD("fail, adv_id :%d overflow", adv_id);
        return 0;
    }
    BT_LOGD("start ble adv type:%d, interval:%d, duration:%d, filter_type:%d, adv_id:%u", adv_type, interval, duration, filter_type, adv_id);
    uint8_t s_adv_data[] = { 0x02, 0x01, 0x08, 0x09, 0x09, 0x42, 0x52, 0x54, 0x2D, 0x49, 0x44, 0x4D, 0x30, 0x03, 0x02, 0x00, 0xFF };
    s_adv_data[12] = 0x30 + adv_id;

    advertise_param_t adv_para;
    memset(&adv_para, 0, sizeof(advertise_param_t));
    adv_para.params.adv_type = adv_type;
    adv_para.params.channel_map = BLE_ADV_CHANNEL_DEFAULT;
    adv_para.params.interval = interval;
    adv_para.params.tx_power = -10;
    adv_para.params.own_addr_type = BLE_ADDR_TYPE_UNKNOWN;
    adv_para.duration = duration;
    adv_para.adv_length = sizeof(s_adv_data);
    adv_para.adv_data = (char*)s_adv_data;
    adv_para.scan_rsp_data = (char*)s_adv_data;
    adv_para.scan_rsp_length = sizeof(s_adv_data);
    adv_para.params.filter_policy = filter_type;
    adv_para.adv_id = adv_id;
    btm_le_advertise_interface_t* adv_interface = get_btm_leadv_interface(manager);
    bt_result_code ret = adv_interface->start_advertising(&adv_handle[adv_id], &adv_para, &le_adv_cb);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("start_advertising  fail, ret: %d", ret);
        return 0;
    }

    add_advertise_handle(adv_handle[adv_id], adv_id);
    return 0;
}

static int le_stop_advertising(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }

    int adv_id = atoi(argv[0]);
    BT_LOGD("stop ble adv, adv_id:%d", adv_id);

    advertise_handle_t* advertise_handle = find_advertise_handle(adv_id);
    if (!advertise_handle) {
        BT_LOGE("fail, could not find adv_id:%d", adv_id);
        return 0;
    }

    btm_le_advertise_interface_t* adv_interface = get_btm_leadv_interface(manager);
    bt_result_code ret = adv_interface->stop_advertising(advertise_handle->adv_handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("stop_advertising  fail, ret: %d", ret);
        return 0;
    }

    return 0;
}

static bt_command_t g_gatts_tables[] = {
    { "open", gatts_open, "\"open gatt server\"" },
    { "close", gatts_close, "\"close gatt server\"" },
    { "connect", gatts_connect, "\"gatt server connect   :<address>\"" },
    { "disconnect", gatts_disconnect, "\"gatt server disconnect  :<address>\"" },
    { "add", gatts_add, "\"add gatt service\"" },
    { "remove", gatts_remove, "\"remove gatt service\"" },
    { "read_phy", gatts_read_phy, "\"read phy:<address>\"" },
    { "update_phy", gatts_update_phy, "\"update phy(0: 1M, 1: 2M, 2: LE_Coded) :<address> <tx> <rx>\"" },
    { "send_notify", gatts_send_notify, "\"send notify:<address> <payload> \"" },
    { "send_indicate", gatts_send_indicate, "\"send indicate:<address> <payload>\"" },
    { "throughput", gatts_do_throughput, "\"throughtout:<address> <times>\"" },
    { "start_adv", le_start_advertising, "\"start le adv: <type (0:ADV_IND, 1:DIRECT_IND, 2:SCAN_IND, 3:NONCONN_IND, 4:SCAN_RSP)> <interval> <duration> <filter_type> <adv_id (0:legacy, 1~K: extend)>\"" },
    { "stop_adv", le_stop_advertising, "\"stop le adv <adv_id>\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_gatts_tables); i++) {
        printf("\t%-8s\t%s\n", g_gatts_tables[i].cmd, g_gatts_tables[i].help);
    }
}

static struct option gatts_options[] = {
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
};

int gatt_server_command(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;

    if (gatts_interface == NULL) {
        manager = get_bt_manager_interface();
        gatts_interface = get_btm_gatts_interface(manager);
    }

    while ((opt = getopt_long(argc, argv, "h", gatts_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_gatts_tables); i++) {
            if (strcmp(g_gatts_tables[i].cmd, argv[1]) == 0) {
                if (g_gatts_tables[i].func) {
                    ret = g_gatts_tables[i].func(handle, argc - 2, &argv[2]);
                }
            }
        }
    }

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[1]);
        usage();
    }

    return 0;
}
