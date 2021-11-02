
#include <stdio.h>
#include <stdlib.h>

#include "btm_gatt_server.h"
#include "btm_le_advertise.h"
#include "btm_manager.h"
#include "log.h"

#define LOG_TAG "btsample_gatts"

static gatt_server_t* gatts_handle;
static gatt_advertiser_t* adv_handle;
static btm_le_gatts_interface_t* gatts_interface = NULL;
static btm_interface_t* manager = NULL;
static bd_addr_t connected_device_addr;
static uint16_t gatt_mtu = 20;

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
    adv_handle = handle;
}

static void le_adv_stopped_callback(void* handle)
{
    BT_LOGD(" %s", __func__);
}

static void le_adv_failed_callback(void* handle, int error)
{
    BT_LOGD(" %s:err:%d", __func__, error);
}

static void test_server_connection_state_changed_callback(gatt_server_t* handle, bd_addr_t remote_addr, bt_state_t state)
{
    BT_LOGD("%s  addr:[%02x:%02x:%02x:%02x:%02x:%02x], state:%d", __func__, remote_addr[0], remote_addr[1],
        remote_addr[2], remote_addr[3], remote_addr[4], state);
    if (state == 2) {
        memcpy(connected_device_addr, remote_addr, sizeof(bd_addr_t));
    } else {
        memset(connected_device_addr, 0, sizeof(remote_addr));
    }
}

static gatt_advertise_callbacks cb2 = {
    .le_advertise_started_cb = le_adv_started_callback,
    .le_advertise_stopped_cb = le_adv_stopped_callback,
    .le_advertise_failed_cb = le_adv_failed_callback,
};

static const uint8_t s_adv_data[]
    = { 0x02, 0x01, 0x08, 0x08, 0x09, 0x42, 0x52, 0x54, 0x20, 0x59, 0x61, 0x6F, 0x03, 0x02, 0x00, 0xFF };
static advertise_param_t adv_para;

static void test_server_opened_callback(gatt_server_t* handle)
{
    BT_LOGD("%s", __func__);
    if (!gatts_interface) {
        BT_LOGE("fail,  gatt interface null");
        return;
    }
    gatts_interface->add_service(gatts_handle, s_iot_service_elements, sizeof(s_iot_service_elements) / sizeof(SERVICE_GATT_ELEMENT_S));
    BT_LOGD("add gatt server done");
    memset(&adv_para, 0, sizeof(SERVICE_SCAN_ADV_PARAMS_S));
    adv_para.params.adv_type = BLE_ADV_IND;
    adv_para.params.channel_map = ADV_CHANNEL_DEFAULT;
    adv_para.params.interval = 48;
    adv_para.params.tx_power = -10;
    adv_para.adv_length = sizeof(s_adv_data);
    adv_para.adv_data = (char*)s_adv_data;
    adv_para.scan_rsp_data = (char*)s_adv_data;
    adv_para.scan_rsp_length = sizeof(s_adv_data);
    btm_gatt_advertise_interface_t* adv_interface = get_le_advertise_interface(manager);
    bt_result_code ret = adv_interface->start_advertising(&adv_handle, &adv_para, &cb2);
}

static void test_server_closed_callback(gatt_server_t* handle)
{
    BT_LOGD("%s", __func__);
}

static void test_server_service_added_callback(gatt_server_t* handle, gatt_service_status_t status, gatt_element_t* element,
    size_t size)
{
    BT_LOGD("%s", __func__);
}

static void test_server_service_removed_callback(gatt_server_t* handle, gatt_service_status_t status, gatt_element_t* element,
    size_t size)
{
    BT_LOGD("%s", __func__);
}

static void test_server_phy_read_callback(gatt_server_t* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    BT_LOGD("%s", __func__);
}

static void test_server_phy_update_callback(gatt_server_t* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx, gatt_service_status_t status)
{
    BT_LOGD("%s", __func__);
}

static void test_server_read_request_callback(gatt_server_t* handle, bd_addr_t remote_addr, uint32_t request_id,
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

static void test_server_write_request_callback(gatt_server_t* handle, bd_addr_t remote_addr, uint32_t request_id,
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

static void test_server_mtu_changed_callback(gatt_server_t* handle, bd_addr_t remote_addr, uint32_t mtu)
{
    BT_LOGD("%s mtu: %d", __func__, mtu);
    gatt_mtu = mtu;
}

static void test_server_notify_sent_callback(gatt_server_t* handle, bd_addr_t remote_addr, gatt_service_status_t status)
{
    BT_LOGD("%s status:%d", __func__, status);
}

static uint8_t payload[] = {
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
};

static uint8_t payload2[] = {
    0x01,
    0x02,
    0x01,
    0x02,
    0x01,
};

int main(int argc, FAR char* argv[])
{
    manager = get_bt_manager_interface();
    manager->init(NULL);
    manager->enable();

    gatt_server_callbacks cb = {
        .le_server_connection_state_changed_cb = test_server_connection_state_changed_callback,
        .le_server_opened_cb = test_server_opened_callback,
        .le_server_closed_cb = test_server_closed_callback,
        .le_server_service_added_cb = test_server_service_added_callback,
        .le_server_service_removed_cb = test_server_service_removed_callback,
        .le_server_phy_read_cb = test_server_phy_read_callback,
        .le_server_phy_update_cb = test_server_phy_update_callback,
        .le_server_read_request_cb = test_server_read_request_callback,
        .le_server_write_request_cb = test_server_write_request_callback,
        .le_server_mtu_changed_cb = test_server_mtu_changed_callback,
        .le_server_notify_sent_cb = test_server_notify_sent_callback,
    };

    gatts_interface = get_le_gatts_interface(manager);
    if (!gatts_interface) {
        BT_LOGE("fail, get gatt interface fail");
        return -1;
    }
    BT_LOGD("open gatt server ...");
    gatts_interface->open(&gatts_handle, &cb);
    BT_LOGD("add gatt server ...");

    while (true) {
        char ch = getchar();
        switch (ch) {
        case 'q': {
            BT_LOGD("exit ...");
            exit(0);
        }
        case 'n': {
            BT_LOGD("send notify");
            gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
            gatts_interface->send_notify(gatts_handle, connected_device_addr, element, payload, sizeof(payload) / sizeof(payload[0]));
            break;
        }
        case 'i': {
            BT_LOGD("send indicate");
            gatt_element_t* element = (gatt_element_t*)(s_iot_service_elements + IOT_SERVICE_TX_CHR_ID - 1);
            gatts_interface->send_indicate(gatts_handle, connected_device_addr, element, payload2, sizeof(payload2) / sizeof(payload2[0]));
            break;
        }

        default: {
            BT_LOGD("please input:\t q 'exit' \t n 'send notify' \t i 'send indicate'");
            break;
        }
        }
    }

    return 0;
}