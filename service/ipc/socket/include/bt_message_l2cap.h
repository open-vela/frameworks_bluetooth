/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
BT_L2CAP_MESSAGE_START,
    BT_L2CAP_REGISTER_CALLBACKS,
    BT_L2CAP_UNREGISTER_CALLBACKS,
    BT_L2CAP_LISTEN,
    BT_L2CAP_CONNECT,
    BT_L2CAP_DISCONNECT,
    BT_L2CAP_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_L2CAP_CALLBACK_START,
    BT_L2CAP_CONNECTED_CB,
    BT_L2CAP_DISCONNECTED_CB,
    BT_L2CAP_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_L2CAP_H__
#define _BT_MESSAGE_L2CAP_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_l2cap.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t value_bool; /* boolean */
    } bt_l2cap_result_t;

    typedef union {

        struct {
            l2cap_config_option_t option;
        } _bt_l2cap_listen;

        struct {
            bt_address_t addr;
            l2cap_config_option_t option;
        } _bt_l2cap_connect;

        struct {
            uint16_t cid;
        } _bt_l2cap_disconnect;

    } bt_message_l2cap_t;

    typedef union {

        struct {
            bt_address_t addr;
            uint8_t transport; /* bt_transport_t */
            uint8_t pad[1];
            uint16_t cid;
            uint16_t psm;
            uint16_t incoming_mtu;
            uint16_t outgoing_mtu;
            char pty_name[64];
        } _connected_cb;

        struct {
            bt_address_t addr;
            uint16_t cid;
            uint32_t reason;
        } _disconnected_cb;

    } bt_message_l2cap_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_L2CAP_H__ */
