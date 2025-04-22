#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>

#include "hidd_code.h"
#include "hid_device_service.h"

static void* hidd_callbacks = NULL;

const static uint8_t s_hid_combo_report_desc[] = {
    0x05, 0x0C,
    0x09, 0x01,
    0xA1, 0x01,
    0x85, 0x01,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x01,
    0x75, 0x01,
    0x09, 0xCD,
    0x81, 0x06,
    0x0A, 0x83, 0x01,
    0x81, 0x06,
    0x09, 0xB5,
    0x81, 0x06,
    0x09, 0xB6,
    0x81, 0x06,
    0x09, 0xEA,
    0x81, 0x06,
    0x09, 0xE9,
    0x81, 0x06,
    0x0A, 0x23, 0x02,
    0x81, 0x06,
    0x0A, 0x24, 0x02,
    0x81, 0x06,
    0xC0,

    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,
    0x09, 0x01,
    0xA1, 0x00,
    0x85, 0x02,
    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x03,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x03,
    0x75, 0x01,
    0x81, 0x02,
    0x95, 0x01,
    0x75, 0x05,
    0x81, 0x01,

    0x05, 0x01,
    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x38,
    0x15, 0x81,
    0x25, 0x7F,
    0x75, 0x08,
    0x95, 0x03,
    0x81, 0x06,
    0xC0,
    0xC0
};

static void hidd_app_state_cb(void* cookie, hid_app_state_t state)
{
    syslog(LOG_INFO, "[app_demo] HIDD state: %s", (state == HID_APP_STATE_REGISTERED) ? "registered" : "not registed");
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_HID_DEVICE_APP_STATE;
    node_data->data.hidd_cb._app_state.state = state;
    bt_msg_list_add_tail(&node_data->node);
}

static void hidd_connection_state_cb(void* cookie, bt_address_t* addr, bool le_hid,
    profile_connection_state_t state)
{
    syslog(LOG_INFO, "%s, transport: %s, state:%d", __func__, le_hid ? "le" : "br", state);
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_HID_DEVICE_CONNECTION_STATE;
    memcpy(&node_data->data.hidd_cb._connection_state.addr, addr, sizeof(bt_address_t));
    node_data->data.hidd_cb._connection_state.le_hid = le_hid;
    node_data->data.hidd_cb._connection_state.state = state;
    bt_msg_list_add_tail(&node_data->node);
}

static const hid_device_callbacks_t hidd_test_cbs = {
    .app_state_cb = hidd_app_state_cb,
    .connection_state_cb = hidd_connection_state_cb,
};

static void hidd_register(bt_instance_t* g_bt_ins) {
    hid_device_sdp_settings_t hidd_setting;
    const uint8_t* desc_list;
    uint16_t desc_len;

    memset(&hidd_setting, 0, sizeof(hid_device_sdp_settings_t));
    hidd_setting.name = "HID_Device_Demo";
    hidd_setting.description = "A demo of HID Device implementation";
    hidd_setting.provider = "Xiaomi Vela";
    hidd_setting.hids_info.attr_mask = HID_ATTR_MASK_VIRTUAL_CABLE | HID_ATTR_MASK_RECONNECT_INITIATE | HID_ATTR_MASK_NORMALLY_CONNECTABLE /* | BTHID_ATTR_MASK_BOOT_DEVICE*/;
    desc_list = s_hid_combo_report_desc;
    desc_len = sizeof(s_hid_combo_report_desc);
    hidd_setting.hids_info.dsc_list = malloc(desc_len + 3);
    hidd_setting.hids_info.vendor_id = 0x038F;
    hidd_setting.hids_info.product_id = 0x1234;
    hidd_setting.hids_info.version = 0x100;
    hidd_setting.hids_info.dsc_list_length = (uint16_t)(desc_len + 3); /* 3 bytes for Descriptor Type and Length */
    hidd_setting.hids_info.dsc_list[0] = HID_SDP_DESCRIPTOR_REPORT;
    hidd_setting.hids_info.dsc_list[1] = (uint8_t)(desc_len & 0xFF);
    hidd_setting.hids_info.dsc_list[2] = (uint8_t)(desc_len >> 8);
    memcpy(hidd_setting.hids_info.dsc_list + 3, desc_list, desc_len);

    bt_status_t ret = bt_hid_device_register_app(g_bt_ins, &hidd_setting, false);
    free(hidd_setting.hids_info.dsc_list);
}

void bt_hid_device_init(bt_instance_t* g_bt_ins)
{
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_HID_DEVICE_REGISTER_CALLBACK;
    bt_msg_list_add_tail(&node_data->node);
}


void app_bt_hidd_handle_message(bt_instance_t* g_bt_ins, node_t* node)
{
    app_demo_message_t* msg = &node->data;
    switch(msg->msg_type) {
        case APP_BT_HID_DEVICE_REGISTER_CALLBACK:
            hidd_callbacks = bt_hid_device_register_callbacks(g_bt_ins, &hidd_test_cbs);
            break;
        case APP_BT_HID_DEVICE_REGISTER_APP:
            hidd_register(g_bt_ins);
            break;
        case APP_BT_HID_DEVICE_CONNECT:
            bt_hid_device_connect(g_bt_ins, &msg->hidd_req._bt_hid_device_connect.addr);
            break;
        default:
            break;
    }
}

void app_bt_hidd_handle_callback(bt_instance_t* g_bt_ins, node_t* node)
{
    node_t* node_data;
    app_demo_message_t* msg = &node->data;
    switch(msg->msg_type) {
        case APP_BT_HID_DEVICE_APP_STATE:
            break;
        case APP_BT_HID_DEVICE_CONNECTION_STATE:
            break;
        default:
            break;
    }
}