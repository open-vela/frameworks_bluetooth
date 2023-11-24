/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#ifdef __BT_MESSAGE_CODE__
    BT_HID_DEVICE_MESSAGE_START,
    BT_HID_DEVICE_REGISTER_CALLBACK,
    BT_HID_DEVICE_UNREGISTER_CALLBACK,
    BT_HID_DEVICE_REGISTER_APP,
    BT_HID_DEVICE_UNREGISTER_APP,
    BT_HID_DEVICE_CONNECT,
    BT_HID_DEVICE_DISCONNECT,
    BT_HID_DEVICE_SEND_REPORT,
    BT_HID_DEVICE_RESPONSE_REPORT,
    BT_HID_DEVICE_REPORT_ERROR,
    BT_HID_DEVICE_VIRTUAL_UNPLUG,
    BT_HID_DEVICE_MESSAGE_END,

    BT_HID_DEVICE_CALLBACK_START,
    BT_HID_DEVICE_APP_STATE,
    BT_HID_DEVICE_CONNECTION_STATE,
    BT_HID_DEVICE_ON_GET_REPORT,
    BT_HID_DEVICE_ON_SET_REPORT,
    BT_HID_DEVICE_ON_RECEIVE_REPORT,
    BT_HID_DEVICE_ON_VIRTUAL_UNPLUG,
    BT_HID_DEVICE_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_HID_DEVICE_H__
#define _BT_MESSAGE_HID_DEVICE_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_hid_device.h"

typedef union {
    bt_status_t status;
    bool value_bool;
} bt_hid_device_result_t;

typedef union {
    struct {
        bool le_hid;
        hid_device_sdp_settings_t sdp;
    } _bt_hid_device_register_app;

    struct {
        bt_address_t addr;
    } _bt_hid_device_connect;

    struct {
        bt_address_t addr;
    } _bt_hid_device_disconnect;

    struct {
        bt_address_t addr;
        uint8_t rpt_id;
        int rpt_size;
        uint8_t rpt_data[256];
    } _bt_hid_device_send_report;

    struct {
        bt_address_t addr;
        uint8_t rpt_type;
        int rpt_size;
        uint8_t rpt_data[256];
    } _bt_hid_device_response_report;

    struct {
        bt_address_t addr;
        hid_status_error_t error;
    } _bt_hid_device_report_error;

    struct {
        bt_address_t addr;
    } _bt_hid_device_virtual_unplug;

} bt_message_hid_device_t;

typedef union {
    struct {
        hid_app_state_t state;
    } _app_state;

    struct {
        bt_address_t addr;
        bool le_hid;
        profile_connection_state_t state;
    } _connection_state;

    struct {
        bt_address_t addr;
        uint8_t rpt_type;
        uint8_t rpt_id;
        uint16_t buffer_size;
    } _on_get_report;

    struct {
        bt_address_t addr;
        uint8_t rpt_type;
        uint16_t rpt_size;
        uint8_t rpt_data[256];
    } _on_set_report;

    struct {
        bt_address_t addr;
        uint8_t rpt_type;
        uint16_t rpt_size;
        uint8_t rpt_data[256];
    } _on_receive_report;

    struct {
        bt_address_t addr;
    } _on_virtual_unplug;

} bt_message_hid_device_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_HID_DEVICE_H__ */
