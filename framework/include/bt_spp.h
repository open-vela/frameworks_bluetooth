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
 * @cond
 */

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
 * @brief Spp proxy state
 *
 */
typedef enum {
    SPP_PROXY_STATE_CONNECTED,
    SPP_PROXY_STATE_DISCONNECTED,
} spp_proxy_state_t;

/**
 * @brief Callback used to notify SPP connection states.
 *
 * When the SPP connection state changes, this callback will be triggered.
 * SPP connection states include DISCONNECTED, CONNECTING, CONNECTED, and
 * DISCONNECTING. After the callback is triggered, the application will be
 * notified with the APP handle corresponding to the SPP connection state,
 * the Bluetooth address of the peer device, the server channel number used
 * for the SPP connection, the SPP connection port, and the latest SPP connection
 * state.
 *
 * @param handle - SPP APP handle, the return value of bt_spp_register_app.
 * @param addr - The Bluetooth address of the peer device.
 * @param scn - Server channel number, range in 1-28.
 * @param port - The The unique port of connection.
 * @param state - SPP connection state
 *
 * **Example:**
 * @code
void spp_connection_state_cb(void* handle, bt_address_t* addr,
    uint16_t scn, uint16_t port,
    profile_connection_state_t state)
{
    printf("spp_connection_state_cb, state: %d\n", state);
}
 * @endcode
 */
typedef void (*spp_connection_state_callback)(void* handle, bt_address_t* addr,
    uint16_t scn, uint16_t port,
    profile_connection_state_t state);

/**
 * @brief Callback used to notify SPP PTY open. [deprecated]
 *
 * After a successful establishment of the SPP connection, the PTY will be
 * opened. If the PTY is successfully opened, this callback will be triggered
 * to notify the application that the PTY has been opened.The APP handle
 * corresponding to the SPP connection, the Bluetooth address of the peer device,
 * the server channel number, the connection port, and the PTY name also be
 * notified.
 *
 * @note This callback is deprecated, use spp_proxy_state_callback instead.
 *
 * @param handle - SPP APP handle, the return value of bt_spp_register_app.
 * @param addr - The Bluetooth address of the peer device.
 * @param scn - Server channel number, range in 1-28.
 * @param port - The unique port of connection.
 * @param name - PTY slave device name, like "/dev/pts/0"
 *
 * **Example:**
 * @code
void spp_pty_open_cb(void* handle, bt_address_t* addr, uint16_t scn, uint16_t port, char* name)
{
    printf("spp_pty_open_cb, scn: %d, port: %d, name: %s\n", scn, port, name);
}
 * @endcode
 */
typedef void (*spp_pty_open_callback)(void* handle, bt_address_t* addr, uint16_t scn, uint16_t port, char* name);

/**
 * @brief Callback used to notify SPP proxy states.
 *
 * When the SPP proxy state changes, this callback will be triggered. The SPP
 * proxy states include CONNECTED and DISCONNECTED. After the callback is triggered,
 * the application will be notified with the APP handle corresponding to the SPP
 * proxy state, the Bluetooth address of the peer device, the server channel number
 * used for the SPP connection, the SPP connection port, and the latest SPP proxy
 * state.
 *
 * @param handle - SPP APP handle, the return value of bt_spp_register_app.
 * @param addr - The Bluetooth address of the peer device.
 * @param state - SPP proxy state.
 * @param scn - Server channel number, range in 1-28.
 * @param port - The unique port of connection.
 * @param name - Proxy name, like "btspp-srv0"
 *
 * **Example:**
 * @code
void spp_proxy_state_cb(void* handle, bt_address_t* addr, spp_proxy_state_t state, uint16_t scn, uint16_t port, char* name)
{
    printf("spp_proxy_state_cb, state: %d, scn: %d, port: %d, name: %s\n", state, scn, port, name);
}
 * @endcode
 */
typedef void (*spp_proxy_state_callback)(void* handle, bt_address_t* addr, spp_proxy_state_t state, uint16_t scn, uint16_t port, char* name);

/**
 * @brief SPP event callbacks structure
 *
 */
typedef struct {
    size_t size;
    spp_pty_open_callback pty_open_cb; /* [deprecated] */
    spp_connection_state_callback connection_state_cb;
    spp_proxy_state_callback proxy_state_cb;
} spp_callbacks_t;

/**
 * @endcond
 */

/**
 * @brief Register an SPP service for applications.
 *
 * The application can register an SPP service through this function. Before using
 * this function, the application should prepare callback functions for receiving
 * SPP connection states notifications and SPP proxy states notifications or PTY open
 * information notifications to register the service. After calling this function,
 * the application will receive a handle to identify the application entity in the
 * SPP service, which is used by the SPP service to find the corresponding application.
 *
 * @note It should be noted that the callback used for PTY open is not recommended, the
 *       callback used for SPP proxy states is recommended.
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - SPP callback functions.
 * @return void* - SPP APP handle, NULL represents fail.
 *
 * **Example:**
 * @code
void* spp_handle;
const static spp_callbacks_t callbacks = {
    .size = sizeof(spp_callbacks_t),
    .connection_state_cb = spp_connection_state_cb,
    .spp_proxy_state_cb = spp_proxy_state_cb,
};

void app_init_spp_1(bt_instance_t* ins)
{
    spp_handle = bt_spp_register_app(ins, &callbacks);
    if(!spp_handle)
        printf("register spp app failed\n");
    else
        printf("register spp app success\n");
}
 * @endcode
 */
void* BTSYMBOLS(bt_spp_register_app)(bt_instance_t* ins, const spp_callbacks_t* callbacks);

/**
 * @brief Register an SPP service with specified extended parameters for applications.
 *
 * The application can register the SPP service with the specified name and port
 * type used in the SPP service through this function. Before using this function,
 * the application should prepare callback functions for receiving SPP connection
 * states notifications and SPP proxy states notifications or PTY open information
 * notifications to register the service. After calling this function, the application
 * will receive a handle to identify the application entity in the SPP service, which
 * is used by the SPP service to find the corresponding application.
 *
 * @note It should be noted that the callback used for PTY open is not recommended, the
 *       callback used for SPP proxy states is recommended.
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - SPP callback functions.
 * @param name - SPP application name.
 * @param port_type - SPP port type used in SPP service, not used.
 * @return void* - SPP application handle, NULL represents fail.
 *
 * **Example:**
 * @code
void* spp_handle;
const static spp_callbacks_t callbacks = {
    .size = sizeof(spp_callbacks_t),
    .connection_state_cb = spp_connection_state_cb,
    .spp_proxy_state_cb = spp_proxy_state_cb,
};

void app_init_spp_2(bt_instance_t* ins)
{
    char* name = "spp_app_name";

    spp_handle = bt_spp_register_app_ext(ins, name, 0, &callbacks);
    if(!spp_handle)
        printf("register spp app failed\n");
    else
        printf("register spp app success\n");
}
 * @endcode
 */
void* BTSYMBOLS(bt_spp_register_app_ext)(bt_instance_t* ins, const char* name, int port_type, const spp_callbacks_t* callbacks);

/**
 * @brief Register an SPP service with specified parameters for applications.
 *
 * The application can register the SPP service with the specified name and port
 * type used in the SPP service through this function. Before using this function,
 * the application should prepare callback functions for receiving SPP connection
 * states notifications and SPP proxy states notifications or PTY open information
 * notifications to register the service. After calling this function, the application
 * will obtain a handle to identify the application entity in the SPP service, which
 * will be used for the SPP service to find the corresponding application.
 *
 * @note It should be noted that the callback used for PTY open is not recommended, the
 *       callback used for SPP proxy states is recommended.
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - SPP callback functions.
 * @param name - SPP application name.
 * @return void* - SPP application handle, NULL represents fail.
 *
 * **Example:**
 * @code
void* spp_handle;
const static spp_callbacks_t callbacks = {
    .size = sizeof(spp_callbacks_t),
    .connection_state_cb = spp_connection_state_cb,
    .spp_proxy_state_cb = spp_proxy_state_cb,
};

void app_init_spp_3(bt_instance_t* ins)
{
    char* name = "spp_app_name";

    spp_handle = bt_spp_register_app_with_name(ins, name, &callbacks);
    if(!spp_handle)
        printf("register spp app failed\n");
    else
        printf("register spp app success\n");
}
 * @endcode
 */
void* BTSYMBOLS(bt_spp_register_app_with_name)(bt_instance_t* ins, const char* name, const spp_callbacks_t* callbacks);

/**
 * @brief Unregister SPP service for applications.
 *
 * This function is used to unregister the SPP service for an application. Before
 * using this function, the application should have completed registration
 * of the Bluetooth service and obtained a valid handle. After the function returns
 * a successful result, the application have unregistered the SPP service.
 *
 * @param ins - Bluetooth client instance.
 * @param handle - SPP APP handle.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
void app_unregister_spp_app(bt_instance_t* ins, void* handle)
{
    bt_status_t status;

    if(spp_handle)
        return;

    status = bt_spp_unregister_app(ins, spp_handle);
    if(status != BT_STATUS_SUCCESS)
        printf("unregister spp app failed\n");
    else
        printf("unregister spp app success\n");
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_spp_unregister_app)(bt_instance_t* ins, void* handle);

/**
 * @brief Start the SPP server.
 *
 * This function is used to start an SPP server for a application. Before
 * using this function, the application should have completed registration
 * of the Bluetooth service and obtained a valid handle. In addition, the application
 * needs to specify the server channel number, UUID and maximum number of supported
 * connections using this function. After the function returns a successful result,
 * the application will have started an SPP server that can be connected by SPP clients
 * on other devices.
 *
 * @param ins - Bluetooth client instance.
 * @param handle - SPP application handle.
 * @param scn - Server channel number, range in 1-28.
 * @param uuid - Server uuid, default:0x1101.
 * @param max_connection - Maximum of client connections.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
void app_start_spp_server(bt_instance_t* ins, void* handle)
{
    bt_status_t status;
    uint16_t scn = 1;
    bt_uuid_t uuid = {
        .type = BT_UUID_TYPE_16,
        .value = {0x11, 0x01},
    };
    uint8_t max_connection = 1;

    status = bt_spp_server_start(ins, handle, scn, &uuid, max_connection);
    if(status != BT_STATUS_SUCCESS)
        printf("start spp server failed\n");
    else
        printf("start spp server success\n");
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_spp_server_start)(bt_instance_t* ins, void* handle, uint16_t scn, bt_uuid_t* uuid, uint8_t max_connection);

/**
 * @brief Stop the SPP server.
 *
 * This function is used to stop an SPP server for a application. Before
 * using this function, the application should have started SPP server.
 * In addition, the application needs to specify the server channel number using
 * this function. After the function returns a successful result, the application
 * will have stopped the SPP server.
 *
 * @param ins - Bluetooth client instance.
 * @param handle - SPP application handle.
 * @param scn - Server channel number, range in 1-28.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
void app_stop_spp_server(bt_instance_t* ins, void* handle)
{
    bt_status_t status;
    uint16_t scn = 1;

    status = bt_spp_server_stop(ins, handle, scn);
    if(status != BT_STATUS_SUCCESS)
        printf("stop spp server failed\n");
    else
        printf("stop spp server success\n");
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_spp_server_stop)(bt_instance_t* ins, void* handle, uint16_t scn);

/**
 * @brief Connect to the SPP server
 *
 * This function is used to initiate a connection to the SPP server of a specified
 * device. Before using this function, the application needs to complete SPP service
 * registration and obtain a valid handle. In addition, when the application uses
 * this function, it needs to specify the address of the remote device, the server
 * channel number, UUID, and port used. If this function returns a successful result,
 * an SPP connection will be established with the remote device.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] handle - SPP application handle.
 * @param[in] addr - The Bluetooth address of the peer device.
 * @param[in] scn - Server channel number, range in 1-28.
 *                - UNKNOWN_SERVER_CHANNEL_NUM: Not specify scn.
 * @param[in] uuid - Server uuid, default:0x1101.
 * @param[out] port - The unique port of connection.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
void app_connect_spp_server(bt_instance_t* ins, void* handle)
{
    bt_status_t status;
    bt_address_t addr = {
        .address = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    };
    uint16_t port;

    status = bt_spp_connect(ins, handle, &addr, UNKNOWN_SERVER_CHANNEL_NUM, NULL, &port);
    if(status != BT_STATUS_SUCCESS)
        printf("connect spp server failed\n");
    else
        printf("connect spp server success\n");
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_spp_connect)(bt_instance_t* ins, void* handle, bt_address_t* addr, int16_t scn, bt_uuid_t* uuid, uint16_t* port);

/**
 * @brief Disconnect to SPP server
 *
 * This function is used to initiate an SPP disconnection to a specified device.
 * Before using this function, an SPP connection should have been successfully
 * established with the remote device. In addition, the function requires providing
 * the address of the remote device and the port used.
 *
 * @param ins - Bluetooth client instance.
 * @param handle - SPP application handle.
 * @param addr - The Bluetooth address of the peer device.
 * @param port The unique port of connection.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
void app_disconnect_spp_server(bt_instance_t* ins, void* handle)
{
    bt_status_t status;
    bt_address_t addr = {
        .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    };
    uint16_t port = 1;

    status = bt_spp_disconnect(ins, handle, &addr, port);
    if(status != BT_STATUS_SUCCESS)
        printf("disconnect spp server failed\n");
    else
        printf("disconnect spp server success\n");
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_spp_disconnect)(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t port);

#ifdef __cplusplus
}
#endif

#endif /* __BT_SPP_H__ */
