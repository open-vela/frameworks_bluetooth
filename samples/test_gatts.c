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
#define LOG_TAG "btsample_gatts"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "btm_gatt_server.h"
#include "btm_le_advertise.h"
#include "btm_manager.h"
#include "log.h"

#define THROUGHTPUT_HORIZON 2

static void* gatts_handle;
static void* adv_handle;
static btm_gatt_server_interface_t* gatts_interface = NULL;
static btm_interface_t* manager = NULL;
static bt_address connected_device_addr;
static uint32_t gatt_mtu = 20;
static volatile uint16_t throughtput_cursor = 1;

enum {
    /* IDs of Private IOT service */
    IOT_SERVICE_ID = 1,
    IOT_SERVICE_TX_CHR_ID,
    IOT_SERVICE_TX_CHR_CCC_ID,
    IOT_SERVICE_RX_CHR_ID,
};

static gatt_element_t s_iot_service_elements[] = {
    { /* Private IOT Service - 0xFF00 */
        IOT_SERVICE_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00 },
        GATT_PRIMARY_SERVICE
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
        GATT_ATT_PERMISSION_READABLE | GATT_ATT_PERMISSION_WRITABLE },
    { /* Private Characteristic for RX - 0xFF02 */
        IOT_SERVICE_RX_CHR_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00 },
        GATT_CHARACTERISTIC,
        GATT_ATT_PROPERTY_WRITE_NO_RESPONSE,
        GATT_ATT_PERMISSION_WRITABLE },
};

static void le_adv_started_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void le_adv_stopped_callback(void* handle)
{
    BT_LOGD(" %s", __func__);
}

static void le_adv_failed_callback(void* handle, int error)
{
    BT_LOGD(" %s:err:%d", __func__, error);
}

static void test_server_connection_state_changed_callback(void* handle, bt_address remote_addr, profile_connection_state state)
{
    BT_LOGD("%s  addr:[%02x:%02x:%02x:%02x:%02x:%02x], state:%d", __func__, remote_addr[0], remote_addr[1],
        remote_addr[2], remote_addr[3], remote_addr[4], remote_addr[5], state);
    if (state == 2) {
        memcpy(connected_device_addr, remote_addr, sizeof(bt_address));
    } else {
        memset(connected_device_addr, 0, sizeof(remote_addr));
    }
}

static btm_le_advertise_callbacks le_adv_cb = {
    .le_advertise_started_cb = le_adv_started_callback,
    .le_advertise_stopped_cb = le_adv_stopped_callback,
    .le_advertise_failed_cb = le_adv_failed_callback,
};

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
    BT_LOGD("%s", __func__);
}

static void test_server_service_removed_callback(void* handle, gatt_status status, gatt_element_t* element,
    size_t size)
{
    BT_LOGD("%s", __func__);
}

static void test_server_phy_read_callback(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx)
{
    BT_LOGD("%s", __func__);
}

static void test_server_phy_update_callback(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx, gatt_status status)
{
    BT_LOGD("%s", __func__);
}

static void test_server_read_request_callback(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element)
{
    BT_LOGD("%s", __func__);
    gatt_response_t* rsp;

    if (element->id == IOT_SERVICE_TX_CHR_CCC_ID) {
        rsp = malloc(sizeof(gatt_response_t) + 1);
        memset(rsp, 0, sizeof(gatt_response_t) + 1);
        uint16_t s_iot_tx_ccc_value = 0x2234;
        rsp->length = 2;
        rsp->value[0] = (uint8_t)(s_iot_tx_ccc_value & 0xFF);
        rsp->value[1] = (uint8_t)(s_iot_tx_ccc_value >> 8);
    } else {
        rsp = malloc(sizeof(gatt_response_t));
        memset(rsp, 0, sizeof(gatt_response_t));
        rsp->length = 0;
    }
    rsp->request_id = request_id;
    rsp->status = GATT_STATUS_SUCCESS;
    if (!gatts_interface) {
        BT_LOGE("fail,   gatt interface null");
        return;
    }
    gatts_interface->send_response(handle, remote_addr, rsp);
    free(rsp);
}

static void test_server_write_request_callback(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size)
{
    BT_LOGD("%s", __func__);
    gatt_response_t* rsp;
    switch (element->id) {
    case IOT_SERVICE_TX_CHR_CCC_ID: {
        uint16_t s_iot_tx_ccc_value = value[0] + (value[1] << 8);
        BT_LOGD(" IOT transmission %s!\r\n>", s_iot_tx_ccc_value ? "enabled" : "disabled");
        break;
    }
    case IOT_SERVICE_RX_CHR_ID: {
        BT_LOGD(" IOT received %d bytes!\r\n>", size);
        break;
    }
    default: {
        BT_LOGD(" write %d bytes from offset %d of element %d!\r\n>", size, offset, element->id);
        break;
    }
    }
    rsp->length = 0;
    rsp->request_id = request_id;
    rsp->status = GATT_STATUS_SUCCESS;
    if (!gatts_interface) {
        BT_LOGE("fail,   gatt interface null");
        return;
    }
    gatts_interface->send_response(handle, remote_addr, rsp);
}

static void test_server_mtu_changed_callback(void* handle, bt_address remote_addr, uint32_t mtu)
{
    BT_LOGD("###%s mtu: %d", __func__, mtu);
    gatt_mtu = mtu;
}

static void test_server_notify_sent_callback(void* handle, bt_address remote_addr, gatt_status status)
{
    throughtput_cursor--;
    BT_LOGD("%s status:%d, throughtput_cursor:%d", __func__, status, throughtput_cursor);
}

static void manager_state_changed_callback(btm_bt_state state)
{
    BT_LOGD("%s, state:%d", __func__, state);
}

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
};

static void test_server_throughtout_notify(uint32_t times, uint16_t mtu)
{
    BT_LOGD("###mtu:%d, times:%d", mtu, times);
    uint8_t* payload = (uint8_t*)malloc(sizeof(uint8_t) * mtu);
    uint32_t msg_counter = 1;
    gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
    if (!payload) {
        BT_LOGD("malloc payload fail");
        return;
    }
    if (!element) {
        BT_LOGD("element null");
        return;
    }

    for (int i = 0; i < times; i++) {
        while (throughtput_cursor >= THROUGHTPUT_HORIZON) {
            usleep(500);
        }
        memset(payload, 1, mtu);
        payload[0] = (msg_counter >> 24) & 0xFF;
        payload[1] = (msg_counter >> 16) & 0xFF;
        payload[2] = (msg_counter >> 8) & 0xFF;
        payload[3] = msg_counter & 0xFF;
        bt_result_code code = gatts_interface->send_notify(gatts_handle, connected_device_addr, element, payload, mtu);
        throughtput_cursor++;
        BT_LOGD("send_notify times:%d, throughtput_cursor:%d", i, throughtput_cursor);
        if (code != BT_RESULT_SUCCESS) {
            BT_LOGE("fail, send_notify ret:%d", code);
            return;
        }
        msg_counter++;
    }
    free(payload);
    BT_LOGD("%s done", __func__);
}

static void usage()
{
    BT_LOGD("usage: \n \
      \t a, open gatt server\n \
      \t b, close gatt server\n  \
      \t c, gatt server connect\n \
      \t d, gatt server disconnect\n \
      \t e, gatt server add\n  \
      \t f, gatt server remove\n \
      \t g, gatt server read phy\n \
      \t h, gatt server update phy\n \
      \t i, gatt server send notify\n \
      \t j, gatt server send indicate\n \
      \t k, gatt server do throught\n \
      \t l, gatt start adv\n \
      \t m, gatt stop adv\n \
      \t q, gatt server exit\n");
}

int main(int argc, FAR char* argv[])
{
    void* manager_handle;
    manager = get_bt_manager_interface();
    manager->init(&manager_handle, &mgt_cb);
    manager->enable(manager_handle);

    btm_gatt_server_callbacks gatts_cb = {
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

    gatts_interface = get_btm_gatts_interface(manager);
    if (!gatts_interface) {
        BT_LOGE("fail, get gatt interface fail");
        return -1;
    }

    bool exit = false;
    while (!exit) {
        char ch = getchar();
        switch (ch) {
        case 'a': {
            BT_LOGD("open gatt server");
            bt_result_code ret = gatts_interface->open(&gatts_handle, &gatts_cb);
            if (ret != BT_RESULT_SUCCESS) {
                BT_LOGE("fail, open gatt interface fail");
            }
            break;
        }
        case 'b': {
            BT_LOGD("close gatt server");
            bt_result_code ret = gatts_interface->close(gatts_handle);
            if (ret != BT_RESULT_SUCCESS) {
                BT_LOGD("close  fail, ret: %d", ret);
            }
            break;
        }
        case 'c': {
            BT_LOGD("gatt server try to connect capable device, please input addr:");
            bt_address remote_address;
            scanf("%x:%x:%x:%x:%x:%x", &remote_address[0], &remote_address[1], &remote_address[2], &remote_address[3], &remote_address[4], &remote_address[5]);
            BT_LOGD("remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
            bt_result_code ret = gatts_interface->connect(&gatts_handle, remote_address, true);
            if (ret != BT_RESULT_SUCCESS) {
                BT_LOGD("connect  fail, ret: %d", ret);
            }
            break;
        }
        case 'd': {
            BT_LOGD("gatt server try to disconnect remote device");
            bt_result_code ret = gatts_interface->disconnect(&gatts_handle, connected_device_addr);
            if (ret != BT_RESULT_SUCCESS) {
                BT_LOGD("disconnect  fail, ret: %d", ret);
            }
            break;
        }
        case 'e': {
            BT_LOGD("gatt server add service");
            gatts_interface->add_service(gatts_handle, s_iot_service_elements, sizeof(s_iot_service_elements) / sizeof(gatt_element_t));
            break;
        }
        case 'f': {
            BT_LOGD("gatt server remove service");
            uint32_t id = IOT_SERVICE_ID;
            gatts_interface->remove_service(gatts_handle, &id, 1);
            break;
        }
        case 'g': {
            BT_LOGD("read remote device phy");
            gatts_interface->read_phy(gatts_handle, connected_device_addr);
            break;
        }
        case 'h': {
            BT_LOGD("update phy tx and rx phy(0: 1M, 1: 2M, 2: LE_Coded)");
            int tx, rx;
            scanf("%d rx", &tx, &rx);
            BT_LOGD("  tx%d and rx%d phy", tx, rx);
            gatts_interface->update_phy(gatts_handle, connected_device_addr, tx, rx);
            break;
        }
        case 'i': {
            BT_LOGD("start send notify, please enable peer CCD fist");
            gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
            uint8_t payload[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            gatts_interface->send_notify(gatts_handle, connected_device_addr, element, payload, sizeof(payload) / sizeof(payload[0]));
            break;
        }
        case 'j': {
            BT_LOGD("start send indicate, please enable peer CCD fist");
            gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
            uint8_t payload[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
            gatts_interface->send_indicate(gatts_handle, connected_device_addr, element, payload, sizeof(payload) / sizeof(payload[0]));
            break;
        }
        case 'k': {
            BT_LOGD("please input times, and let's do throughtout job");
            uint32_t times;
            scanf("%d", &times);
            test_server_throughtout_notify(times, gatt_mtu);
            BT_LOGD("throughtout notify ...");
            break;
        }
        case 'l': {
            BT_LOGD("start ble adv");
            const uint8_t s_adv_data[] = { 0x02, 0x01, 0x08, 0x08, 0x09, 0x42, 0x52, 0x54, 0x2D, 0x49, 0x44, 0x4D, 0x03, 0x02, 0x00, 0xFF };
            advertise_param_t adv_para;
            memset(&adv_para, 0, sizeof(advertise_param_t));
            adv_para.params.adv_type = BLE_EVENT_ADV_IND;
            adv_para.params.channel_map = BLE_ADV_CHANNEL_DEFAULT;
            adv_para.params.interval = 48;
            adv_para.params.tx_power = -10;
            adv_para.adv_length = sizeof(s_adv_data);
            adv_para.adv_data = (char*)s_adv_data;
            adv_para.scan_rsp_data = (char*)s_adv_data;
            adv_para.scan_rsp_length = sizeof(s_adv_data);
            btm_le_advertise_interface_t* adv_interface = get_btm_leadv_interface(manager);
            bt_result_code ret = adv_interface->start_advertising(&adv_handle, &adv_para, &le_adv_cb);
            if (ret != BT_RESULT_SUCCESS) {
                BT_LOGD("start_advertising  fail, ret: %d", ret);
            }
            break;
        }
        case 'm': {
            BT_LOGD("stop ble adv");
            btm_le_advertise_interface_t* adv_interface = get_btm_leadv_interface(manager);
            bt_result_code ret = adv_interface->stop_advertising(adv_handle);
            if (ret != BT_RESULT_SUCCESS) {
                BT_LOGD("start_advertising  fail, ret: %d", ret);
            }
            break;
            exit = true;
            break;
        }
        case 'q': {
            BT_LOGD("exit...");
            exit = true;
            break;
        }
        default: {
            usage();
            break;
        }
        }
    }

    return 0;
}
