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
BT_MANAGER_MESSAGE_START,
    BT_MANAGER_CREATE_INSTANCE,
    BT_MANAGER_DELETE_INSTANCE,
    BT_MANAGER_GET_INSTANCE,
    BT_MANAGER_START_SERVICE,
    BT_MANAGER_STOP_SERVICE,
    BT_MANAGER_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_MANAGER_CALLBACK_START,
    BT_MANAGER_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_MANAGER_H__
#define _BT_MESSAGE_MANAGER_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bluetooth.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t pad[3];
        uint32_t v32;
        uint64_t v64;
    } bt_manager_result_t;

    typedef union {
        struct {
            pid_t pid;
            uint32_t handle;
            uint32_t type;
            char cpu_name[64];
        } _bluetooth_create_instance;

        struct {
            pid_t pid;
            char cpu_name[64];
        } _bluetooth_get_instance;

        struct {
            uint32_t v32;
        } _bluetooth_delete_instance;

        struct {
            uint8_t id; /* enum profile_id */
            uint8_t pad[3];
            uint32_t appid;
        } _bluetooth_start_service,
            _bluetooth_stop_service;

    } bt_message_manager_t;

    typedef struct
    {

    } bt_message_manager_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_MANAGER_H__ */
