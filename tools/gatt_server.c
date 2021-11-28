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

static void* gatts_handle;
static void* adv_handle;
static btm_gatt_server_interface_t* gatts_interface = NULL;
static btm_interface_t* manager = NULL;
static uint16_t gatt_mtu = 20;
static volatile uint16_t throughtput_cursor = 1;

enum {
    /* IDs of Private IOT service */
    IOT_SERVICE_ID = 1,
    IOT_SERVICE_TX_CHR_ID,
    IOT_SERVICE_TX_CHR_CCC_ID,
    IOT_SERVICE_RX_CHR_ID,
};

static const gatt_element_t s_iot_service_elements[] = {
    { /* Private IOT Service - 0xFF00 */
        IOT_SERVICE_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00 },
        PRIMARY_SERVICE,
        0,
        GATT_PERMISSION_READABLE },
    { /* Private Characteristic for TX - 0xFF01 */
        IOT_SERVICE_TX_CHR_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00 },
        CHARACTERISTIC,
        GATT_PROPERTY_NOTIFY | GATT_PROPERTY_INDICATE,
        0 },
    { /* Client Characteristic Configuration Descriptor - 0x2902 */
        IOT_SERVICE_TX_CHR_CCC_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x02, 0x29, 0x00, 0x00 },
        DESCRIPTOR,
        0,
        GATT_PERMISSION_READABLE | GATT_PERMISSION_WRITABLE },
    { /* Private Characteristic for RX - 0xFF02 */
        IOT_SERVICE_RX_CHR_ID,
        { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00 },
        CHARACTERISTIC,
        GATT_PROPERTY_WRITE_NO_RESPONSE,
        GATT_PERMISSION_WRITABLE },
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

static btm_le_advertise_callbacks le_adv_cb = {
    .le_advertise_started_cb = le_adv_started_callback,
    .le_advertise_stopped_cb = le_adv_stopped_callback,
    .le_advertise_failed_cb = le_adv_failed_callback,
};

static void test_server_connection_state_changed_callback(void* handle, bd_addr_t remote_addr, profile_state_t state)
{
    BT_LOGD("%s  addr:[%02x:%02x:%02x:%02x:%02x:%02x], state:%d", __func__, remote_addr[0], remote_addr[1],
        remote_addr[2], remote_addr[3], remote_addr[4], remote_addr[5], state);
}

static void test_server_opened_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void test_server_closed_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void test_server_service_added_callback(void* handle, gatt_status_t status, gatt_element_t* element,
    size_t size)
{
    BT_LOGD("%s", __func__);
}

static void test_server_service_removed_callback(void* handle, gatt_status_t status, gatt_element_t* element,
    size_t size)
{
    BT_LOGD("%s", __func__);
}

static void test_server_phy_read_callback(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    BT_LOGD("%s", __func__);
}

static void test_server_phy_update_callback(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx, gatt_status_t status)
{
    BT_LOGD("%s", __func__);
}

static void test_server_read_request_callback(void* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element)
{
    BT_LOGD("%s", __func__);
    gatt_response_t* rsp;
    if (element->id == IOT_SERVICE_TX_CHR_CCC_ID) {
        rsp = malloc(sizeof(gatt_response_t) + 1);
        uint16_t s_iot_tx_ccc_value = 0x2234;
        rsp->length = 2;
        rsp->value[0] = (uint8_t)(s_iot_tx_ccc_value & 0xFF);
        rsp->value[1] = (uint8_t)(s_iot_tx_ccc_value >> 8);
    } else {
        rsp = malloc(sizeof(gatt_response_t));
        rsp->length = 0;
    }
    rsp->request_id = request_id;
    rsp->status = GATT_SUCCESS;
    if (!gatts_interface) {
        BT_LOGE("fail,   gatt interface null");
        return;
    }
    gatts_interface->send_response(handle, remote_addr, rsp);
    free(rsp);
}

static void test_server_write_request_callback(void* handle, bd_addr_t remote_addr, uint32_t request_id,
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
    rsp->status = GATT_SUCCESS;
    if (!gatts_interface) {
        BT_LOGE("fail,   gatt interface null");
        return;
    }
    gatts_interface->send_response(handle, remote_addr, rsp);
}

static void test_server_mtu_changed_callback(void* handle, bd_addr_t remote_addr, uint32_t mtu)
{
    BT_LOGD("%s mtu: %d", __func__, mtu);
    gatt_mtu = mtu;
}

static void test_server_notify_sent_callback(void* handle, bd_addr_t remote_addr, gatt_status_t status)
{
    throughtput_cursor--;
    BT_LOGD("%s status:%d, throughtput_cursor:%d", __func__, status, throughtput_cursor);
}

static void manager_init_status_changed_callback(bt_result_code status)
{
    BT_LOGD("%s, state:%d", __func__, status);
}

static void manager_state_changed_callback(bt_manager_bt_state state)
{
    BT_LOGD("%s, state:%d", __func__, state);
}

static void test_server_throughtout_notify(bd_addr_t remote_addr, gatt_element_t* element, uint32_t times, uint16_t mtu)
{
    BT_LOGD("mtu:%d, times:%d", mtu, times);
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
        return -1;
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
        return -1;
    }
    return 0;
}

static int gatts_connect(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bd_addr_t remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("connect remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    bt_result_code ret = gatts_interface->connect(gatts_handle, remote_address, true);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, connect  ret: %d", ret);
    }
    return 0;
}

static int gatts_disconnect(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bd_addr_t remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("disconnect remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    bt_result_code ret = gatts_interface->disconnect(gatts_handle, remote_address);
    if (ret != BT_RESULT_SUCCESS) {
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
    bt_result_code ret = gatts_interface->add_service(gatts_handle, s_iot_service_elements, sizeof(s_iot_service_elements) / sizeof(SERVICE_GATT_ELEMENT_S));
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, add gatt service ret: %d", ret);
        return -1;
    }
    return 0;
}

static int gatts_remove(void* handle, int argc, char** argv)
{
    if (!gatts_interface) {
        return -1;
    }
    BT_LOGD("remove gatt service");
    bt_result_code ret = gatts_interface->remove_service(gatts_handle, s_iot_service_elements);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, remove gatt service ret: %d", ret);
        return -1;
    }
    return 0;
}

static int gatts_read_phy(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bd_addr_t remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("read phy remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    bt_result_code ret = gatts_interface->read_phy(gatts_handle, remote_address);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read phy  ret: %d", ret);
    }
    return 0;
}

static int gatts_update_phy(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 2) {
        return -1;
    }
    bd_addr_t remote_address;
    str2ba(argv[0], remote_address);
    int tx = atoi(argv[1]);
    int rx = atoi(argv[2]);
    BT_LOGD("update phy(0: 1M, 1: 2M, 2: LE_Coded) remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x], tx:%d, rx:%d", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5], tx, rx);
    bt_result_code ret = gatts_interface->update_phy(gatts_handle, remote_address, tx, rx);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, update phy  ret: %d", ret);
    }
    return 0;
}

static int gatts_send_notify(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bd_addr_t remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("send notify remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
    uint8_t payload[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    bt_result_code ret = gatts_interface->send_notify(gatts_handle, remote_address, element, payload, sizeof(payload) / sizeof(payload[0]));
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read phy  ret: %d", ret);
    }
    return 0;
}

static int gatts_send_indicate(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bd_addr_t remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("send indicate remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
    uint8_t payload[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    bt_result_code ret = gatts_interface->send_indicate(gatts_handle, remote_address, element, payload, sizeof(payload) / sizeof(payload[0]));
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, send indicate  ret: %d", ret);
    }
    return 0;
}

static int gatts_do_throughput(void* handle, int argc, char** argv)
{
    if (!gatts_interface || argc < 1) {
        return -1;
    }
    bd_addr_t remote_address;
    str2ba(argv[0], remote_address);
    uint32_t times = atoi(argv[1]);
    BT_LOGD("throughtout remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x], times:%d", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5], times);
    gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
    if (!element) {
        BT_LOGD("element null");
        return;
    }
    test_server_throughtout_notify(remote_address, element, times, gatt_mtu);
    BT_LOGD("throughtout notify ...");
    return 0;
}

static int le_start_advertising(void* handle, int argc, char** argv)
{
    if (!gatts_interface) {
        return -1;
    }
    BT_LOGD("start ble adv");
    const uint8_t s_adv_data[] = { 0x02, 0x01, 0x08, 0x08, 0x09, 0x42, 0x52, 0x54, 0x20, 0x59, 0x61, 0x6F, 0x03, 0x02, 0x00, 0xFF };
    advertise_param_t adv_para;
    memset(&adv_para, 0, sizeof(SERVICE_SCAN_ADV_PARAMS_S));
    adv_para.params.adv_type = BLE_ADV_IND;
    adv_para.params.channel_map = ADV_CHANNEL_DEFAULT;
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
        return -1;
    }
    return 0;
}

static int le_stop_advertising(void* handle, int argc, char** argv)
{
    if (!gatts_interface) {
        return -1;
    }
    BT_LOGD("stop ble adv");
    btm_le_advertise_interface_t* adv_interface = get_btm_leadv_interface(manager);
    bt_result_code ret = adv_interface->stop_advertising(adv_handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("stop_advertising  fail, ret: %d", ret);
        return -1;
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
    { "send_notify", gatts_send_notify, "\"send notify:<address> \"" },
    { "send_indicate", gatts_send_indicate, "\"send indicate remote_addr :<address>\"" },
    { "throughput", gatts_do_throughput, "\"throughtout:<address> <times>\"" },
    { "start_adv", le_start_advertising, "\"start le adv\"" },
    { "stop_adv", le_stop_advertising, "\"stop le adv\"" },
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
