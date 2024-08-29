/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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

#ifndef __BT_SPP_H__
#define __BT_SPP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "bluetooth.h"
#include "bt_device.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief Unknow server channel number
 *
 */
#define UNKNOWN_SERVER_CHANNEL_NUM -1

/**
 * @brief Serial Port Profile (SPP) UUID
 *
 */
#define BT_UUID_SERVCLASS_SERIAL_PORT 0x1101

/**
 * @brief Spp pty mode
 *
 */
typedef enum {
    SPP_PTY_MODE_NORMAL,
    SPP_PTY_MODE_RAW
} spp_pty_mode_t;

typedef enum {
    SPP_PORT_TYPE_TTY,
    SPP_PORT_TYPE_RPMSG_UART,
} spp_port_type_t;

/**
 * @brief Spp connection state callback
 *
 * @param handle - spp app handle, the return value of bt_spp_register_app.
 * @param addr - address of peer device.
 * @param scn - server channel number, range in <1-28>.
 * @param port - unique port of connection.
 * @param state - spp connection state
 */
typedef void (*spp_connection_state_callback)(void* handle, bt_address_t* addr,
    uint16_t scn, uint16_t port,
    profile_connection_state_t state);

/**
 * @brief Spp pty opened notification
 *
 * @param handle - spp app handle, the return value of bt_spp_register_app.
 * @param addr - address of peer device.
 * @param scn - server channel number, range in <1-28>.
 * @param port - unique port of connection.
 * @param name - pty slave device name, like "/dev/pts/0"
 */
typedef void (*spp_pty_open_callback)(void* handle, bt_address_t* addr, uint16_t scn, uint16_t port, char* name);

/**
 * @brief SPP event callbacks structure
 *
 */
typedef struct {
    size_t size;
    spp_pty_open_callback pty_open_cb;
    spp_connection_state_callback connection_state_cb;
} spp_callbacks_t;

/**
 * @brief Register spp app
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - spp callback functions.
 * @return void* - spp app handle, NULL on failure.
 */
void* BTSYMBOLS(bt_spp_register_app)(bt_instance_t* ins, const spp_callbacks_t* callbacks);

/**
 * @brief Register spp app with params
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - spp callback functions.
 * @param name - spp app name.
 * @param port_type - spp port type.
 * @return void* - spp app handle, NULL on failure.
 */
void* BTSYMBOLS(bt_spp_register_app_ext)(bt_instance_t* ins, const char* name, int port_type, const spp_callbacks_t* callbacks);

/**
 * @brief Unregister spp app
 *
 * @param ins - bluetooth client instance.
 * @param handle - spp app handle.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_spp_unregister_app)(bt_instance_t* ins, void* handle);

/**
 * @brief Start spp server
 *
 * @param ins - bluetooth client instance.
 * @param handle - spp app handle.
 * @param scn - server channel number, range in <1-28>.
 * @param uuid - server uuid, default:0x1101.
 * @param max_connection - maximum of client connections.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_spp_server_start)(bt_instance_t* ins, void* handle, uint16_t scn, bt_uuid_t* uuid, uint8_t max_connection);

/**
 * @brief Stop spp server
 *
 * @param ins - bluetooth client instance.
 * @param handle - spp app handle.
 * @param scn - server channel number, range in <1-28>.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_spp_server_stop)(bt_instance_t* ins, void* handle, uint16_t scn);

/**
 * @brief Connect to spp server
 *
 * @param[in] ins - bluetooth client instance.
 * @param[in] handle - spp app handle.
 * @param[in] addr - address of peer device.
 * @param[in] scn - server channel number, range in <1-28>.
 *                - UNKNOWN_SERVER_CHANNEL_NUM: Not specify scn.
 * @param[in] uuid - server uuid, default:0x1101.
 * @param[out] port - point to unique port of connection.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_spp_connect)(bt_instance_t* ins, void* handle, bt_address_t* addr, int16_t scn, bt_uuid_t* uuid, uint16_t* port);

/**
 * @brief Disconnect to spp server
 *
 * @param ins - bluetooth client instance.
 * @param handle - spp app handle.
 * @param addr - address of peer device.
 * @param port unique port of connection.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_spp_disconnect)(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t port);

#ifdef __cplusplus
}
#endif

#endif /* __BT_SPP_H__ */
