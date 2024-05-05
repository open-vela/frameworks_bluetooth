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
#endif

#ifdef __BT_CALLBACK_CODE__
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

#define MAX_BT_HID_DEVICE_REGISTER_APP_SDP 512

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t value_bool; /* boolean */
    } bt_hid_device_result_t;

    typedef union {
        struct {
            uint8_t sdp[MAX_BT_HID_DEVICE_REGISTER_APP_SDP];
            uint8_t le_hid; /* boolean */
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
            uint8_t pad[1];
            uint32_t rpt_size;
            uint8_t rpt_data[256];
        } _bt_hid_device_send_report;

        struct {
            bt_address_t addr;
            uint8_t rpt_type;
            uint8_t padp[1];
            uint32_t rpt_size;
            uint8_t rpt_data[256];
        } _bt_hid_device_response_report;

        struct {
            bt_address_t addr;
            uint8_t error; /* hid_status_error_t */
        } _bt_hid_device_report_error;

        struct {
            bt_address_t addr;
        } _bt_hid_device_virtual_unplug;

    } bt_message_hid_device_t;

    typedef union {
        struct {
            uint8_t state; /* hid_app_state_t */
        } _app_state;

        struct {
            bt_address_t addr;
            uint8_t le_hid; /* boolean */
            uint8_t state; /* profile_connection_state_t */
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
            uint8_t pad[1];
            uint16_t rpt_size;
            uint8_t rpt_data[256];
        } _on_set_report;

        struct {
            bt_address_t addr;
            uint8_t rpt_type;
            uint8_t pad[1];
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
