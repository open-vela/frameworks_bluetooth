#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>

#include "hidd_code.h"

void app_bt_get_local_name(bt_instance_t* g_bt_ins)
{
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_GAP_GET_NAME;
    bt_msg_list_add_tail(&node_data->node);
}

void app_bt_set_scan_mode(bt_instance_t* g_bt_ins, bt_scan_mode_t mode, bool bondable)
{
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_GAP_SET_SCANMODE;
    node_data->data.gap_req._bt_adapter_set_scan_mode.mode = mode;
    node_data->data.gap_req._bt_adapter_set_scan_mode.bondable = bondable;
    bt_msg_list_add_tail(&node_data->node);
}

void app_bt_set_io_capability(bt_instance_t* g_bt_ins, bt_io_capability_t capability)
{
    node_t* node_data = (node_t*)malloc(sizeof(node_t));
    node_data->data.msg_type = APP_BT_GAP_SET_IO_CAPABILITY;
    node_data->data.gap_req._bt_adapter_set_io_capability.cap = capability;
    bt_msg_list_add_tail(&node_data->node);
}

void bt_gap_init(bt_instance_t* g_bt_ins)
{
    // Get local device name
    app_bt_get_local_name(g_bt_ins);

    //  Set the scanning mode to make the device locally connectable and discoverable.
    app_bt_set_scan_mode(g_bt_ins, BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);

    // Set io capability to NOINPUTNOOUTPUT. 
    app_bt_set_io_capability(g_bt_ins, BT_IO_CAPABILITY_NOINPUTNOOUTPUT);

#ifdef CONFIG_BLUETOOTH_HID_DEVICE
    bt_hid_device_init(g_bt_ins);
#endif
}

void app_bt_gap_handle_message(bt_instance_t* g_bt_ins, node_t* node)
{
    app_demo_message_t* msg = &node->data;
    switch(msg->msg_type) {
        case APP_BT_GAP_DISABLE:
            bt_adapter_disable(g_bt_ins);
            break;
        case APP_BT_GAP_GET_NAME:
            bt_adapter_get_name(g_bt_ins, msg->gap_req._bt_adapter_get_name.name, BT_NAME_LENGTH);
            syslog(LOG_INFO, "[app_demo] Adapter Name: %s\n", msg->gap_req._bt_adapter_get_name.name);
            break;
        case APP_BT_GAP_SET_SCANMODE:
            bt_adapter_set_scan_mode(g_bt_ins, msg->gap_req._bt_adapter_set_scan_mode.mode, 
                                     msg->gap_req._bt_adapter_set_scan_mode.bondable);
            break;
        case APP_BT_GAP_SET_IO_CAPABILITY:
            bt_adapter_set_io_capability(g_bt_ins, msg->gap_req._bt_adapter_set_io_capability.cap);
            break;
        case APP_BT_GAP_CONNECT_REQUEST_REPLY:
            bt_device_connect_request_reply(g_bt_ins, &msg->gap_req._bt_device_connect_request_reply.addr, 
                                            msg->gap_req._bt_device_connect_request_reply.accept);
            break;
        case APP_BT_GAP_CONNECT_DISCONNECT:
            bt_device_disconnect(g_bt_ins, &msg->gap_req._bt_device_disconnect.addr);
            break;
        default:
            break;
    }
}

void app_bt_gap_handle_callback(bt_instance_t* g_bt_ins, node_t* node)
{
    node_t* node_data;
    app_demo_message_t* msg = &node->data;
    switch(msg->msg_type) {
        case APP_BT_GAP_ON_GAP_STATE_CHANGED:
            syslog(LOG_INFO, "[app_demo] Adapter state changed: %d\n", msg->gap_cb._on_adapter_state_changed.state);
            if (msg->gap_cb._on_adapter_state_changed.state == BT_ADAPTER_STATE_ON) {
                bt_gap_init(g_bt_ins);
#ifdef CONFIG_BLUETOOTH_HID_DEVICE
                node_data = (node_t*)malloc(sizeof(node_t));
                node_data->data.msg_type = APP_BT_HID_DEVICE_REGISTER_APP;
                bt_msg_list_add_tail(&node_data->node);
#endif
            } else if (msg->gap_cb._on_adapter_state_changed.state == BT_ADAPTER_STATE_OFF) {
            }
            break;
        case APP_BT_GAP_ON_CONNECT_REQUEST:
            syslog(LOG_INFO, "[app_demo] Connect request\n");
            node_data = (node_t*)malloc(sizeof(node_t));
            node_data->data.msg_type = APP_BT_GAP_CONNECT_REQUEST_REPLY;
            memcpy(&node_data->data.gap_req._bt_device_connect_request_reply.addr, 
                   &msg->gap_cb._on_connect_request.addr, sizeof(bt_address_t));
            node_data->data.gap_req._bt_device_connect_request_reply.accept = true;
            bt_msg_list_add_tail(&node_data->node);
            break;
        case APP_BT_GAP_ON_CONNECTION_STATE_CHANGED:
            if (msg->gap_cb._on_connection_state_changed.state == CONNECTION_STATE_DISCONNECTED) {
                node_data = (node_t*)malloc(sizeof(node_t));
                node_data->data.msg_type = APP_BT_GAP_DISABLE;
                bt_msg_list_add_tail(&node_data->node);
            }
            break;
        case APP_BT_GAP_ON_BOND_STATE_CHANGED:
            syslog(LOG_INFO, "[app_demo] Bond state changed: %d\n", msg->gap_cb._on_bond_state_changed.state);
            if (msg->gap_cb._on_bond_state_changed.state == BOND_STATE_BONDED) {
                node_data = (node_t*)malloc(sizeof(node_t));
#ifdef CONFIG_BLUETOOTH_HID_DEVICE
                syslog(LOG_INFO, "[app_demo] Bonded, connect to HID device\n");
                node_data->data.msg_type = APP_BT_HID_DEVICE_CONNECT;
                memcpy(&node_data->data.hidd_req._bt_hid_device_connect.addr, 
                       &msg->gap_cb._on_bond_state_changed.addr, sizeof(bt_address_t));
#else
                syslog(LOG_INFO, "[app_demo] Bonded, disconnect\n");
                node_data->data.msg_type = APP_BT_GAP_CONNECT_DISCONNECT;
                memcpy(&node_data->data.gap_req._bt_device_disconnect.addr,
                    &msg->gap_cb._on_bond_state_changed.addr, sizeof(bt_address_t));
#endif
                bt_msg_list_add_tail(&node_data->node);
            }
            break;
        default:
            break;
    }
}