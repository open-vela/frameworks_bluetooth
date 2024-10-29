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

#ifndef __BT_HID_DEVICE_H__
#define __BT_HID_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "bt_addr.h"
#include "bt_device.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/* * Descriptor types in the SDP record */
#define HID_SDP_DESCRIPTOR_REPORT (0x22)
#define HID_SDP_DESCRIPTOR_PHYSICAL (0x23)

/* * HID supported features - bit mask */
#define HID_ATTR_MASK_VIRTUAL_CABLE 0x0001
#define HID_ATTR_MASK_RECONNECT_INITIATE 0x0002
#define HID_ATTR_MASK_BOOT_DEVICE 0x0004
#define HID_ATTR_MASK_BATTERY_POWER 0x0010
#define HID_ATTR_MASK_REMOTE_WAKE 0x0020
#define HID_ATTR_MASK_SUPERVISION_TIMEOUT 0x0080
#define HID_ATTR_MASK_NORMALLY_CONNECTABLE 0x0100
#define HID_ATTR_MASK_SSR_MAX_LATENCY 0x0200
#define HID_ATTR_MASK_SSR_MIN_TIMEOUT 0x0400
#define HID_ATTR_MASK_BREDR 0x8000

/**
 * @cond
 */

/**
 * @brief HID descriptor information
 *
 */
typedef struct {
    uint32_t attr_mask; /* BTHID_ATTR_MASK_VIRTUAL_CABLE etc. */
    uint8_t sub_class;
    uint8_t country_code;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t version;
    uint16_t supervision_timeout;
    uint16_t ssr_max_latency;
    uint16_t ssr_min_timeout;
    uint16_t dsc_list_length; /* Length of desc_list */
    uint8_t pad[4];
    uint8_t* dsc_list; /* List of descriptors. Each descriptor is constructed as: Type(1 Byte), Length(2 Bytes, Little Endian), Values(Length Bytes) */
} hid_info_t;

/**
 * @brief HID device settings for SDP server
 *
 */
typedef struct {
    const char* name;
    const char* description;
    const char* provider;
    hid_info_t hids_info;
} hid_device_sdp_settings_t;

/**
 * @brief HID report type
 *
 */
typedef enum {
    HID_REPORT_RESRV, /* reserved */
    HID_REPORT_INPUT, /* input report */
    HID_REPORT_OUTPUT, /* output report */
    HID_REPORT_FEATURE, /* feature report */
} hid_report_type_t;

/**
 * @brief HID status code
 *
 */
typedef enum {
    HID_STATUS_OK = 0,
    HID_STATUS_HANDSHAKE_NOT_READY,
    HID_STATUS_HANDSHAKE_INVALID_REPORT_ID,
    HID_STATUS_HANDSHAKE_UNSUPPORTED_REQ,
    HID_STATUS_HANDSHAKE_INVALID_PARAM,
    HID_STATUS_HANDSHAKE_UNSPECIFIED_ERROR,
    HID_STATUS_UNSPECIFIED_ERROR,
    HID_ERROR_SDP,
    HID_ERROR_SET_PROTOCOL,
    HID_ERROR_DATABASE_FULL,
    HID_ERROR_DEVICE_TYPE_UNSUPPORTED,
    HID_ERROR_NO_RESOURCES,
    HID_ERROR_AUTHENTICATION_FAILED,
    HID_ERROR_OPERATION_NOT_ALLOWED,
} hid_status_error_t;

/**
 * @brief HID app state
 *
 */
typedef enum {
    HID_APP_STATE_NOT_REGISTERED,
    HID_APP_STATE_REGISTERED,
} hid_app_state_t;
/**
 * @endcond
 */


/**
 * @brief HID device application state callback.
 *
 * Callback function invoked when the HID application state changes. This callback
 * is used to notify the HID device about the state transition in interaction 
 * with the remote HID host.
 *
 * @param cookie - `remote_callback_t*`. 
 *                 See `bt_hid_device_register_callbacks`.
 * @param state  - New HID application state, see @ref hid_app_state_t.
 *
 * **Example:**
 * @code
 * void hidd_app_state_callback(void* cookie, hid_app_state_t state)
 * {
 *     // Handle HID application state change
 * }
 * @endcode
 */
typedef void (*hidd_app_state_callback)(void* cookie, hid_app_state_t state);

/**
 * @brief HID device connection state callback.
 *
 * Callback function invoked when the HID connection state changes. This callback
 * is used to notify the HID device about connection state changes with the remote 
 * HID host.
 *
 * @param cookie - `remote_callback_t*`. 
 *                 See `bt_hid_device_register_callbacks`.
 * @param addr   - Address of the peer device, see @ref bt_address_t.
 * @param le_hid - TRUE if the connection is over LE; FALSE if over BR/EDR.
 * @param state  - New HID connection state, see @ref profile_connection_state_t.
 *
 * **Example:**
 * @code
 * void hidd_connection_state_callback(void* cookie, bt_address_t* addr, bool le_hid, profile_connection_state_t state)
 * {
 *     // Handle HID connection state change
 * }
 * @endcode
 */
typedef void (*hidd_connection_state_callback)(void* cookie, bt_address_t* addr, bool le_hid,
    profile_connection_state_t state);

/**
 * @brief Callback for getting a specified report from the remote HID host.
 *
 * Callback function invoked when a GET_REPORT request is received from the remote HID host.
 *
 * @param cookie - `remote_callback_t*`. 
 *                 See `bt_hid_device_register_callbacks`.
 * @param addr   - Address of the peer device, see @ref bt_address_t.
 * @param rpt_type - Report type, see @ref hid_report_type_t.
 * @param rpt_id - Report ID.
 * @param buffer_size - Maximum size of the report data to return.
 *
 * **Example:**
 * @code
 * void hidd_get_report_callback(void* cookie, bt_address_t* addr, uint8_t rpt_type,
 *     uint8_t rpt_id, uint16_t buffer_size)
 * {
 *     // Provide the requested report to the remote HID host
 * }
 * @endcode
 */
typedef void (*hidd_get_report_callback)(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint8_t rpt_id, uint16_t buffer_size);

/**
 * @brief Callback for setting a specified report from the remote HID host.
 *
 * Callback function invoked when a SET_REPORT request is received from the remote HID host.
 *
 * @param cookie - `remote_callback_t*`. 
 *                 See `bt_hid_device_register_callbacks`.
 * @param addr   - Address of the peer device, see @ref bt_address_t.
 * @param rpt_type - Report type, see @ref hid_report_type_t.
 * @param rpt_size - Size of the report data.
 * @param rpt_data - Pointer to the report data.
 *
 * **Example:**
 * @code
 * void hidd_set_report_callback(void* cookie, bt_address_t* addr, uint8_t rpt_type,
 *     uint16_t rpt_size, uint8_t* rpt_data)
 * {
 *     // Process the report data received from the remote HID host
 * }
 * @endcode
 */
typedef void (*hidd_set_report_callback)(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data);

/**
 * @brief Callback for receiving reports from the remote HID host.
 *
 * Callback function invoked when an INPUT report is received from the remote HID host.
 *
 * @param cookie - `remote_callback_t*`. 
 *                 See `bt_hid_device_register_callbacks`.
 * @param addr   - Address of the peer device, see @ref bt_address_t.
 * @param rpt_type - Report type, see @ref hid_report_type_t.
 * @param rpt_size - Size of the report data.
 * @param rpt_data - Pointer to the report data.
 *
 * **Example:**
 * @code
 * void hidd_receive_report_callback(void* cookie, bt_address_t* addr, uint8_t rpt_type,
 *     uint16_t rpt_size, uint8_t* rpt_data)
 * {
 *     // Handle the report data received from the remote HID host
 * }
 * @endcode
 */
typedef void (*hidd_receive_report_callback)(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data);

/**
 * @brief HID device virtual cable unplug callback.
 *
 * Callback function invoked when a virtual cable unplug request is received from the remote HID host.
 *
 * @param cookie - `remote_callback_t*`. 
 *                 See `bt_hid_device_register_callbacks`.
 * @param addr   - Address of the peer device, see @ref bt_address_t.
 *
 * **Example:**
 * @code
 * void hidd_virtual_unplug_callback(void* cookie, bt_address_t* addr)
 * {
 *     // Handle virtual cable unplug event initiated by the remote HID host
 * }
 * @endcode
 */
typedef void (*hidd_virtual_unplug_callback)(void* cookie, bt_address_t* addr);

/**
 * @cond
 */

/**
 * @brief HID device event callbacks structure
 *
 */
typedef struct {
    size_t size;
    hidd_app_state_callback app_state_cb;
    hidd_connection_state_callback connection_state_cb;
    hidd_get_report_callback get_report_cb;
    hidd_set_report_callback set_report_cb;
    hidd_receive_report_callback receive_report_cb;
    hidd_virtual_unplug_callback virtual_unplug_cb;
} hid_device_callbacks_t;
/**
 * @endcond
 */

/**
 * @brief Register callback functions with the HID device service.
 *
 * Registers application callbacks to receive HID device events.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param callbacks - Pointer to the HID device callbacks structure, see @ref hid_device_callbacks_t.
 * @return void* - Callback cookie to be used in future calls; NULL on failure.
 *
 * **Example:**
 * @code
void* cookie = bt_hid_device_register_callbacks(ins, &my_hid_device_callbacks);
if (cookie == NULL) {
    // Handle error
}
 * @endcode
 */
void* BTSYMBOLS(bt_hid_device_register_callbacks)(bt_instance_t* ins, const hid_device_callbacks_t* callbacks);

/**
 * @brief Unregister HID device callback functions.
 *
 * Unregisters the application callbacks from the HID device service.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param cookie - Callback cookie obtained from registration.
 * @return true - Unregistration successful.
 * @return false - Callback cookie not found or unregistration failed.
 *
 * **Example:**
 * @code
if (bt_hid_device_unregister_callbacks(ins, cookie)) {
    // Unregistered successfully
} else {
    // Handle error
}
 * @endcode
 */
bool BTSYMBOLS(bt_hid_device_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Register the HID application.
 *
 * Registers the HID device application.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param sdp_setting - Pointer to the HID device SDP settings, see @ref hid_device_sdp_settings_t.
 * @param le_hid - TRUE to register as an LE HID device; FALSE for BR/EDR.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
hid_device_sdp_settings_t sdp_settings = {
    .name = "My HID Device",
    .description = "Example HID Device",
    .provider = "My Company",
    // Initialize hids_info...
};
if (bt_hid_device_register_app(ins, &sdp_settings, false) == BT_STATUS_SUCCESS) {
    // HID application registered
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hid_device_register_app)(bt_instance_t* ins, hid_device_sdp_settings_t* sdp_setting, bool le_hid);

/**
 * @brief Unregister the HID application.
 *
 * Unregisters the HID device application.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_hid_device_unregister_app(ins) == BT_STATUS_SUCCESS) {
    // HID application unregistered
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hid_device_unregister_app)(bt_instance_t* ins);

/**
 * @brief Connect to a HID host.
 *
 * Initiates a connection to a HID host device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the peer device, see @ref bt_address_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_hid_device_connect(ins, &host_addr) == BT_STATUS_SUCCESS) {
    // Connection initiated
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hid_device_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from a HID host.
 *
 * Terminates the connection with a HID host device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the peer device, see @ref bt_address_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_hid_device_disconnect(ins, &host_addr) == BT_STATUS_SUCCESS) {
    // Disconnection initiated
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hid_device_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Send a report to the HID host.
 *
 * Sends a HID report to the connected HID host device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the peer device, see @ref bt_address_t.
 * @param rpt_id - Report ID.
 * @param rpt_data - Pointer to the report data.
 * @param rpt_size - Size of the report data.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
uint8_t report_data[] = { report data };
if (bt_hid_device_send_report(ins, &host_addr, rpt_id, report_data, sizeof(report_data)) == BT_STATUS_SUCCESS) {
    // Report sent
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hid_device_send_report)(bt_instance_t* ins, bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size);

/**
 * @brief Respond with a report to the host's GET_REPORT command.
 *
 * Sends a report in response to a GET_REPORT request from the host.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the peer device, see @ref bt_address_t.
 * @param rpt_type - Report type, see @ref hid_report_type_t.
 * @param rpt_data - Pointer to the report data.
 * @param rpt_size - Size of the report data.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_response_report)(bt_instance_t* ins, bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size);

/**
 * @brief Send local HID device error response to remote HID host.
 *
 * Send local HID device error response to remote HID host.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the peer device, see @ref bt_address_t.
 * @param error - Error code, see @ref hid_status_error_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_report_error)(bt_instance_t* ins, bt_address_t* addr, hid_status_error_t error);

/**
 * @brief Perform a virtual cable unplug with the current HID host.
 *
 * Simulates the physical disconnection of the HID device from the host.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the peer device, see @ref bt_address_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_virtual_unplug)(bt_instance_t* ins, bt_address_t* addr);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HID_DEVICE_H__ */