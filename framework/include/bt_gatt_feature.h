/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#ifndef _BT_GATT_FEATURE_H_
#define _BT_GATT_FEATURE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bt_async.h"
#include "bt_gattc.h"

/* ----------------------------- Struct Definitions ----------------------------- */

typedef struct gatt_service_t gatt_service_t;
typedef struct gatt_characteristic_t gatt_characteristic_t;
typedef struct gatt_descriptor_t gatt_descriptor_t;
typedef struct gatt_include_service_t gatt_include_service_t;

/**
 * @brief GATT Descriptor.
 */
struct gatt_descriptor_t {
    bt_uuid_t service_uuid; /**< Parent service UUID. */
    bt_uuid_t characteristic_uuid; /**< parent characteristic UUID. */
    bt_uuid_t uuid; /**< Descriptor UUID. */
    uint16_t attr_handle; /**< Descriptor attribute handle. */
    uint8_t* value; /**< Value buffer pointer. */
    size_t value_len; /**< Length of value. */
};

/**
 * @brief GATT Characteristic.
 */
struct gatt_characteristic_t {
    bt_uuid_t service_uuid; /**< Parent service UUID. */
    bt_uuid_t uuid; /**< Characteristic UUID. */
    uint16_t value_handle; /**< Characteristic Value attribute handle. */
    uint8_t* value; /**< Value buffer pointer. */
    size_t value_len; /**< Length of value. */
    uint32_t properties; /**< Properties bitmask. */
    gatt_descriptor_t* descriptors; /**< Array of descriptors. */
    size_t descriptor_count; /**< Number of descriptors. */
};

/**
 * @brief GATT Included Service.
 */
struct gatt_include_service_t {
    uint16_t attr_handle; /**< Include declaration attribute handle. */
    uint16_t start_handle; /**< Start handle of the referenced service */
    uint16_t end_handle; /**< End handle of the referenced service */
    bt_uuid_t included_service_uuid; /**< referenced service UUID */
};

/**
 * @brief GATT Service.
 */
struct gatt_service_t {
    bt_uuid_t uuid; /**< Service UUID. */
    uint16_t attr_handle; /**< Service declaration attribute handle. */
    bool is_primary; /**< True if primary service. */
    gatt_characteristic_t* characteristics; /**< Array of characteristics. */
    size_t characteristic_count; /**< Number of characteristics. */
    gatt_include_service_t* included_services; /**< Array of included services. */
    size_t included_service_count; /**< Number of included services. */
};

/* ----------------------------- Callback Typedefs ----------------------------- */

/**
 * @brief Status callback.
 * @param ins      Bluetooth instance.
 * @param status   Operation status.
 * @param userdata User context.
 */
typedef void (*bt_status_cb_t)(bt_instance_t* ins, bt_status_t status, void* userdata);

/**
 * @brief GATT client connection completion callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 */
typedef void (*bt_gattc_feature_on_connected_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle);

/**
 * @brief GATT client disconnected from the remote device or connect fail callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 */
typedef void (*bt_gattc_feature_on_disconnected_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle);

/**
 * @brief Create client completion callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 * @param userdata   User context.
 */
typedef void (*bt_gattc_feature_create_client_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle, void* userdata);

/**
 * @brief Delete client completion callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 * @param userdata   User context.
 */
typedef void (*bt_gattc_feature_delete_client_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle, void* userdata);

/**
 * @brief Get service callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 * @param service     Retrieved service (single entry).
 */
typedef void (*bt_gattc_feature_get_service_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle,
    const gatt_service_t* service);

/**
 * @brief Read characteristic callback.
 * @param ins             Bluetooth instance.
 * @param status          Operation status.
 * @param conn_handle     Connection handle.
 * @param characteristic  Retrieved characteristic (with value).
 */
typedef void (*bt_gattc_feature_read_characteristic_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle,
    const gatt_characteristic_t* characteristic);

/**
 * @brief Read descriptor callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 * @param descriptor  Retrieved descriptor (with value).
 */
typedef void (*bt_gattc_feature_read_descriptor_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle,
    const gatt_descriptor_t* descriptor);

/**
 * @brief Write characteristic callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 */
typedef void (*bt_gattc_feature_write_characteristic_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle);

/**
 * @brief Write descriptor callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 */
typedef void (*bt_gattc_feature_write_descriptor_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle);

/**
 * @brief Notification subscription state callback.
 * @param ins         Bluetooth instance.
 * @param status      Operation status.
 * @param conn_handle Connection handle.
 * @param enable      True if subscription was enabled, false if disabled.
 */
typedef void (*bt_gattc_feature_on_subscribed_cb_t)(bt_instance_t* ins, gatt_status_t status, gattc_handle_t conn_handle, bool enable);

/**
 * @brief MTU exchange result callback.
 * @param conn_handle Connection handle.
 * @param status      Operation status.
 * @param mtu         Negotiated MTU size.
 */
typedef void (*bt_gattc_feature_on_mtu_changed_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint32_t mtu);

/**
 * @brief Characteristic change notification callback.
 * @param ins             Bluetooth instance.
 * @param conn_handle     Connection handle.
 * @param characteristic  Updated characteristic (with value).
 */
typedef void (*bt_gattc_feature_characteristic_changed_cb_t)(bt_instance_t* ins, gattc_handle_t conn_handle,
    const gatt_characteristic_t* characteristic);

typedef struct {
    uint32_t size;
    bt_gattc_feature_on_connected_cb_t on_connected;
    bt_gattc_feature_on_disconnected_cb_t on_disconnected;
    bt_gattc_feature_get_service_cb_t on_discovered;
    bt_gattc_feature_read_characteristic_cb_t on_read_char;
    bt_gattc_feature_read_descriptor_cb_t on_read_desc;
    bt_gattc_feature_write_characteristic_cb_t on_write_char;
    bt_gattc_feature_write_descriptor_cb_t on_write_desc;
    bt_gattc_feature_on_subscribed_cb_t on_subscribed;
    bt_gattc_feature_characteristic_changed_cb_t on_notified;
    bt_gattc_feature_on_mtu_changed_cb_t on_mtu_updated;
} bt_gattc_feature_callbacks_t;

/* ----------------------------- API Declarations ----------------------------- */

/**
 * @brief Create GATT client.
 * @param ins         Bluetooth instance.
 * @param addr        Remote device address.
 * @param cb          Create completion callback.
 * @param callbacks   GATT client event callbacks.
 * @param userdata    User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_create_client_async(bt_instance_t* ins, bt_address_t* addr,
    bt_gattc_feature_create_client_cb_t cb,
    bt_gattc_feature_callbacks_t* callbacks,
    void* userdata);

/**
 * @brief Delete GATT client.
 * @param ins        Bluetooth instance.
 * @param addr       Remote device address.
 * @param cb         Delete completion callback.
 * @param userdata   User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_delete_client_async(bt_instance_t* ins, bt_address_t* addr,
    bt_gattc_feature_delete_client_cb_t cb, void* userdata);

/**
 * @brief Connect to GATT server.
 *
 * Initiates a GATT connection to the remote server using the specified connection handle.
 *
 * Important:
 * - Before calling this function, you must have successfully called
 *   @ref bt_gattc_feature_create_client_async() to create a GATT client and obtained the `conn_handle`.
 * - This function reuses the existing `conn_handle` to connect; it does not create a new client.
 * - Normally, after client creation, the stack will automatically attempt the first connection.
 *   You can call this function explicitly if you want to reconnect after a disconnect.
 *
 * @param conn_handle Connection handle (from create).
 * @param addr        Remote device address.
 * @param addr_type   Address type.
 * @param cb          Async call status callback.
 * @param userdata    User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_connect_async(gattc_handle_t conn_handle, bt_address_t* addr, ble_addr_type_t addr_type,
    bt_status_cb_t cb, void* userdata);

/**
 * @brief Disconnect from GATT server.
 *
 * Terminates the active GATT connection associated with the given connection handle.
 *
 * Important:
 * - The `conn_handle` must be a valid handle previously obtained from
 *   @ref bt_gattc_feature_create_client_async().
 * - After disconnection, the GATT client remains allocated.
 *   You can reconnect later using @ref bt_gattc_feature_connect_async(),
 *   or completely remove the client using @ref bt_gattc_feature_delete_client_async().
 *
 * @param conn_handle Connection handle.
 * @param cb          Async call status callback.
 * @param userdata    User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_disconnect_async(gattc_handle_t conn_handle, bt_status_cb_t cb, void* userdata);

/**
 * @brief Discover all GATT services (rebuilds local DB).
 *
 * @param conn_handle Connection handle.
 * @param cb          Async call status callback.
 * @param userdata    User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_get_service_async(gattc_handle_t conn_handle, bt_status_cb_t cb, void* userdata);

/**
 * @brief Read characteristic value.
 *
 * @param conn_handle         Connection handle.
 * @param service_uuid        Parent service UUID.
 * @param characteristic_uuid Target characteristic UUID.
 * @param cb                  Async call status callback.
 * @param userdata            User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_read_characteristic_value_async(gattc_handle_t conn_handle,
    const bt_uuid_t* service_uuid, const bt_uuid_t* characteristic_uuid,
    bt_status_cb_t cb, void* userdata);

/**
 * @brief Read descriptor value.
 *
 * @param conn_handle         Connection handle.
 * @param service_uuid        Parent service UUID.
 * @param characteristic_uuid Parent characteristic UUID.
 * @param descriptor_uuid     Target descriptor UUID.
 * @param cb                  Async call status callback.
 * @param userdata            User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_read_descriptor_value_async(gattc_handle_t conn_handle,
    const bt_uuid_t* service_uuid, const bt_uuid_t* characteristic_uuid,
    const bt_uuid_t* descriptor_uuid,
    bt_status_cb_t cb, void* userdata);

/**
 * @brief Write characteristic value.
 *
 * @param conn_handle    Connection handle.
 * @param characteristic Characteristic to write.
 * @param cb             Async call status callback.
 * @param userdata       User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_write_characteristic_value_async(gattc_handle_t conn_handle,
    const gatt_characteristic_t* characteristic, bt_status_cb_t cb, void* userdata);

/**
 * @brief Write descriptor value.
 *
 * @param conn_handle Connection handle.
 * @param descriptor  Descriptor to write.
 * @param cb          Async call status callback.
 * @param userdata    User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_write_descriptor_value_async(gattc_handle_t conn_handle,
    const gatt_descriptor_t* descriptor, bt_status_cb_t cb, void* userdata);

/**
 * @brief Exchange MTU size.
 *
 * @param conn_handle Connection handle.
 * @param mtu         Desired MTU size.
 * @param cb          Async call status callback.
 * @param userdata    User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_exchange_mtu_async(gattc_handle_t conn_handle, uint32_t mtu,
    bt_status_cb_t cb, void* userdata);

/**
 * @brief Enable or disable characteristic notification.
 *
 * @param conn_handle    Connection handle.
 * @param characteristic Target characteristic.
 * @param enable         True to enable, false to disable.
 * @param cb             Async call status callback.
 * @param userdata       User context.
 * @return bt_status_t
 */
bt_status_t bt_gattc_feature_set_notify_characteristic_changed_async(gattc_handle_t conn_handle,
    const gatt_characteristic_t* characteristic, bool enable, bt_status_cb_t cb, void* userdata);

#ifdef __cplusplus
}
#endif

#endif // _BT_GATT_FEATURE_H_
