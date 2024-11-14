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
BT_SPP_MESSAGE_START,
    BT_SPP_REGISTER_APP,
    BT_SPP_UNREGISTER_APP,
    BT_SPP_SERVER_START,
    BT_SPP_SERVER_STOP,
    BT_SPP_CONNECT,
    BT_SPP_DISCONNECT,
    BT_SPP_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_SPP_CALLBACK_START,
    BT_SPP_PROXY_STATE_CB,
    BT_SPP_CONNECTION_STATE_CB,
    BT_SPP_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_SPP_H__
#define _BT_MESSAGE_SPP_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bluetooth.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t pad[3];
        uint64_t handle;
    } bt_spp_result_t;

    typedef union {
        struct {
            uint32_t name_len;
            char name[64];
        } _bt_spp_register_app;

        struct {
            uint32_t handle;
            bt_uuid_t uuid;
            uint16_t scn;
            uint8_t max_connection;
        } _bt_spp_server_start;

        struct {
            uint32_t handle;
            uint16_t scn;
        } _bt_spp_server_stop;

        struct {
            uint32_t handle;
            bt_address_t addr;
            int16_t scn;
            bt_uuid_t uuid;
            uint16_t port;
        } _bt_spp_connect;

        struct {
            uint32_t handle;
            bt_address_t addr;
            uint16_t port;
        } _bt_spp_disconnect;

    } bt_message_spp_t;

    typedef struct
    {
        struct {
            uint32_t handle;
            bt_address_t addr;
            uint8_t state;
            uint16_t scn;
            char name[64];
            uint16_t port;
        } _proxy_state_cb;

        struct {
            uint32_t handle;
            bt_address_t addr;
            uint16_t scn;
            uint16_t port;
            uint8_t state; /* profile_connection_state_t */
        } _connection_state_cb;
    } bt_message_spp_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_SPP_H__ */
