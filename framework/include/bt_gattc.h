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

#ifndef __BT_GATTC_H__
#define __BT_GATTC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_gatt_defs.h"
#include "bt_status.h"
#include "bt_uuid.h"
#include <stddef.h>

/**
 * @cond
 */

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

typedef void* gattc_handle_t;

typedef struct {
    uint8_t type; /* gatt_attr_type_t */
    uint8_t pad[1];
    uint16_t handle;
    bt_uuid_t uuid;
    uint32_t properties;

} gatt_attr_desc_t;

/**
 * @endcond
 */

/**
 * @brief callback for connected as GATT client.
 *
 * This callback is triggered when gattc connected. BT Service use this callback to notify the application calling
 * bt_gattc_connect that gattc connection has been established.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param addr - remote device address.
 * @return void.
 *
 * **Example:**
 * @code
static void connect_callback(void* conn_handle, bt_address_t* addr)
{
    gattc_device_t* device = find_gattc_device(conn_handle);

    assert(device);
    memcpy(&device->remote_address, addr, sizeof(bt_address_t));
    device->conn_state = CONNECTION_STATE_CONNECTED;
    PRINT_ADDR("gattc_connect_callback, addr:%s", addr);
}
 * @endcode
 */
typedef void (*gattc_connected_cb_t)(gattc_handle_t conn_handle, bt_address_t* addr);

/**
 * @brief callback for disconnected as GATT client.
 *
 * This callback is triggered when gattc disconnected. BT Service use this callback to notify the application that
 * the gattc connection is destroyed.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param addr - remote device address.
 * @return void.
 *
 * **Example:**
 * @code
static void disconnect_callback(void* conn_handle, bt_address_t* addr)
{
    gattc_device_t* device = find_gattc_device(conn_handle);

    assert(device);
    device->conn_state = CONNECTION_STATE_DISCONNECTED;
    PRINT_ADDR("gattc_disconnect_callback, addr:%s", addr);
}
 * @endcode
 */
typedef void (*gattc_disconnected_cb_t)(gattc_handle_t conn_handle, bt_address_t* addr);

/**
 * @brief callback for GATT client execute Services discover operation.
 *
 * This callback is triggered when application initiating service discovery procedure to peer GATT server.
 * BT Service use this callback to report the result of attributes.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param status - bt_status_t - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param uuid - uuid of the attribute. uuid == NULL or uuid->type == 0 means discovery procedure completed.
 * @param start_handle - start handle of Primary services(Secondary services).
 * @param end_handle - end handle of Primary services(Secondary services).
 * @return void.
 *
 * **Example:**
 * @code
static void discover_callback(void* conn_handle, gatt_status_t status, bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle)
{
    PRINT("gattc_discover_callback result, attr_handle: 0x%04x - 0x%04x", start_handle, end_handle);
}
 * @endcode
 */
typedef void (*gattc_discover_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle);

/**
 * @brief callback for GATT client excute Exchange MTU operation.
 *
 * This callback is triggered when application initiating Exchange MTU procedure to peer GATT server.
 * BT Service use this callback to Report the results of MTU negotiation.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param status - bt_status_t - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param mtu - negotiated MTU.
 * @return void.
 *
 * **Example:**
 * @code
static void mtu_updated_callback(void* conn_handle, gatt_status_t status, uint32_t mtu)
{
    gattc_device_t* device = find_gattc_device(conn_handle);

    assert(device);
    if (status == GATT_STATUS_SUCCESS) {
        device->gatt_mtu = mtu;
    }
    PRINT("gattc_mtu_updated_callback, status:%d, mtu:%" PRIu32, status, mtu);
}
 * @endcode
 */
typedef void (*gattc_mtu_updated_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint32_t mtu);

/**
 * @brief callback for GATT client excute attribute read operation.
 *
 * This callback is triggered when application initiating read procedure to peer GATT server.
 * BT Service use this callback to Report the attribute value of corresponding attribute.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param status - bt_status_t - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param attr_handle - attribute handle.
 * @param value - attribute value.
 * @param length - attribute value length.
 * @return void.
 *
 * **Example:**
 * @code
static void read_complete_callback(void* conn_handle, gatt_status_t status, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    PRINT("gattc connection read complete, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
}
 * @endcode
 */
typedef void (*gattc_read_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle, uint8_t* value, uint16_t length);

/**
 * @brief callback for GATT client excute attribute read operation.
 *
 * This callback is triggered when application initiating write procedure to peer GATT server.
 * BT Service use this callback to Report the status of write procedure to corresponding attribute.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param status - bt_status_t - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param attr_handle - attribute handle.
 * @return void.
 *
 * **Example:**
 * @code
static void write_complete_callback(void* conn_handle, gatt_status_t status, uint16_t attr_handle)
{
    if (status != GATT_STATUS_SUCCESS) {
        PRINT("gattc connection write failed, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
        return;
    }

    if (throughtput_cursor) {
        throughtput_cursor--;
    } else {
        PRINT("gattc connection write complete, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
    }
}
 * @endcode
 */
typedef void (*gattc_write_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle);

/**
 * @brief callback for GATT client excute enable/disable cccd operation.
 *
 * This callback is triggered when application initiating enable/disable to corresponding cccd.
 * BT Service use this callback to Report the value of corresponding cccd.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param status - bt_status_t - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param attr_handle - attribute handle.
 * @param enable - enable/disable flag.
 * @return void.
 *
 * **Example:**
 * @code
static void subscribe_complete_callback(void* conn_handle, gatt_status_t status, uint16_t attr_handle, bool enable)
{
    PRINT("gattc connection subscribe complete, handle 0x%" PRIx16 ", status:%d, enable:%d", attr_handle, status, enable);
}
 * @endcode
 */
typedef void (*gattc_subscribe_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle, bool enable);

/**
 * @brief callback for GATT client recv notification/indication from peer GATT server.
 *
 * This callback is triggered when Peer GATT Server initiating notify/indicate procedure. BT Service use this callback
 * to Report the notify/indicate value received.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param attr_handle - attribute handle.
 * @param value - notify/indicate value.
 * @param length - notify/indicate value length.
 * @return void.
 *
 * **Example:**
 * @code
static void notify_received_callback(void* conn_handle, uint16_t attr_handle,
    uint8_t* value, uint16_t length)
{
    PRINT("gattc connection receive notify, handle 0x%" PRIx16, attr_handle);
}
 * @endcode
 */
typedef void (*gattc_notify_cb_t)(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);

/**
 * @brief callback for GATT client execute read PHY.
 *
 * This callback is triggered when application initiating read PHY procedure. BT Service use this callback to Report
 * the Tx PHY & Rx PHY.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param tx_phy - current Tx PHY.
 * @param rx_phy - current Rx PHY.
 * @return void.
 *
 * **Example:**
 * @code
static void phy_read_callback(void* conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    PRINT("gattc read phy complete, tx:%d, rx:%d", tx_phy, rx_phy);
}
 * @endcode
 */
typedef void (*gattc_phy_read_cb_t)(gattc_handle_t conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);

/**
 * @brief callback for GATT client execute update PHY operation.
 *
 * This callback is triggered when application initiating update PHY procedure. BT Service use this callback to Report
 * the result of update Tx PHY & Rx PHY.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param status - bt_status_t - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param tx_phy - Tx PHY after update. If update operation failed, keep original value.
 * @param rx_phy - Rx PHY after update. If update operation failed, keep original value.
 * @return void.
 *
 * **Example:**
 * @code
static void phy_updated_callback(void* conn_handle, gatt_status_t status, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    PRINT("gattc_phy_updated_callback, status:%d, tx:%d, rx:%d", status, tx_phy, rx_phy);
}
 * @endcode
 */
typedef void (*gattc_phy_updated_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);

/**
 * @brief callback for GATT client execute read RSSI of remote device.
 *
 * This callback is triggered when application initiating read remote RSSI procedure. BT Service use this callback to
 * Report the RSSI of remote device.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param status - bt_status_t - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param RSSI - value of remote RSSI.
 * @return void.
 *
 * **Example:**
 * @code
static void rssi_read_callback(void* conn_handle, gatt_status_t status, int32_t rssi)
{
    PRINT("gattc read rssi complete, status:%d, rssi:%" PRIi32, status, rssi);
}
 * @endcode
 */
typedef void (*gattc_rssi_read_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, int32_t rssi);

/**
 * @brief callback for GATT client execute connnection parameter update operation.
 *
 * This callback is triggered when application initiating connnection parameter update procedure. BT Service use this
 *  callback to Report the connection parameter.
 *
 * @param conn_handle - gattc connection handle(void*). The Caller use gattc_connection_t as real parameter.
 * @param status - bt_status_t - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param connection_interval - connection interval(n * 1.25ms).
 * @param peripheral_latency - peripheral latency.
 * @param supervision_timeout - supervision timeout(n * 10ms).
 * @return void.
 *
 * **Example:**
 * @code
static void conn_param_updated_callback(void* conn_handle, bt_status_t status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout)
{
    PRINT("gattc connection paramter updated, status:%d, interval:%" PRIu16 ", latency:%" PRIu16 ", timeout:%" PRIu16,
        status, connection_interval, peripheral_latency, supervision_timeout);
}
 * @endcode
 */
typedef void (*gattc_connection_parameter_updated_cb_t)(gattc_handle_t conn_handle, bt_status_t status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout);

/**
 * @cond
 */
typedef struct {
    uint32_t size;
    gattc_connected_cb_t on_connected;
    gattc_disconnected_cb_t on_disconnected;
    gattc_discover_cb_t on_discovered;
    gattc_read_cb_t on_read;
    gattc_write_cb_t on_written;
    gattc_subscribe_cb_t on_subscribed;
    gattc_notify_cb_t on_notified;
    gattc_mtu_updated_cb_t on_mtu_updated;
    gattc_phy_read_cb_t on_phy_read;
    gattc_phy_updated_cb_t on_phy_updated;
    gattc_rssi_read_cb_t on_rssi_read;
    gattc_connection_parameter_updated_cb_t on_conn_param_updated;
} gattc_callbacks_t;

/**
 * @endcond
 */

/**
 * @brief create a gatt client.
 *
 * This function is used to create a gatt client.
 *
 * @param ins - Bluetooth client instance.
 * @param phandle - pointer of gattc connection handle(void*).
 * @param callbacks - gattc callback table.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_create_connect(handle, &g_gattc_devies[conn_id].handle, &gattc_cbs) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_create_connect)(bt_instance_t* ins, gattc_handle_t* phandle, gattc_callbacks_t* callbacks);

/**
 * @brief delete a gatt client.
 *
 * This function is used to delete a gatt client.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_delete_connect(g_gattc_devies[conn_id].handle) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_delete_connect)(gattc_handle_t conn_handle);

/**
 * @brief create a ATT bearer with peer device.
 *
 * This function is used to establish a ATT bearer.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param addr - peer bluetooth device address.
 * @param addr_type - peer address type(ble_addr_type_t).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_connect(g_gattc_devies[conn_id].handle, &addr, BT_LE_ADDR_TYPE_UNKNOWN) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_connect)(gattc_handle_t conn_handle, bt_address_t* addr, ble_addr_type_t addr_type);

/**
 * @brief dicconnect ATT bearer.
 *
 * This function is used to initiate a disconnection.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_disconnect(g_gattc_devies[conn_id].handle) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_disconnect)(gattc_handle_t conn_handle);

/**
 * @brief discover all GATT services or service with specific UUIDs on peer GATT Server.
 *
 * This function is used to discover GATT services. If filter_uuid is NULL, all services will be discovered.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param filter_uuid - UUID of service to be discovered. filter_uuid is NULL for discover all service of peer device
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_discover_service(g_gattc_devies[conn_id].handle, NULL) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_discover_service)(gattc_handle_t conn_handle, bt_uuid_t* filter_uuid);

/**
 * @brief get attribute by specific Attribute handle.
 *
 * This function is used to get attribute by specific Attribute handle, which is stored after Discover Services procedure.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param attr_handle - attribute handle being found.
 * @param attr_desc - attribute structure(Type, handle, UUID, property). The desired result is stored in this pointer.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_get_attribute_by_handle(conn_handle, attr_handle, &attr_desc) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_get_attribute_by_handle)(gattc_handle_t conn_handle, uint16_t attr_handle, gatt_attr_desc_t* attr_desc);

/**
 * @brief get attribute by specific UUID.
 *
 * This function is used to get attribute by specific handle, which is stored after Discover Services procedure.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param start_handle - start of the Attribute handle range being found.
 * @param end_handle - end of the Attribute handle range being found.
 * @param attr_uuid - attribute UUID being found.
 * @param attr_desc - attribute structure(Type, handle, UUID, property). The desired result is stored in this pointer.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_get_attribute_by_handle(conn_handle, start_handle, end_handle， &attr_desc.uuid, &attr_desc) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_get_attribute_by_uuid)(gattc_handle_t conn_handle, uint16_t start_handle, uint16_t end_handle, bt_uuid_t* attr_uuid, gatt_attr_desc_t* attr_desc);

/**
 * @brief read attribute value by specific handle.（If This Attribute's permissions is readable）
 *
 * This function is used to read a attribute value by specific handle. This function will initiate a ATT Read Request procedure.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param attr_handle - attribute handle being read.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_read(g_gattc_devies[conn_id].handle, attr_handle) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_read)(gattc_handle_t conn_handle, uint16_t attr_handle);

/**
 * @brief write data to a specific attribute. （If This Attribute's permissions is Writable）
 *
 * This function is used to write data to a specific attribute. This function will initiate a ATT write Request procedure.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param attr_handle - attribute handle being written.
 * @param value - data to be written.
 * @param length - the length of value.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_write_without_response(g_gattc_devies[conn_id].handle, attr_handle, value, len) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_write)(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);

/**
 * @brief write data to a specific attribute. （If Write Without Response flag of This Attribute's property is set to 1）
 *
 * This function is used to write data to a specific attribute. This function will initiate a ATT write command procedure.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param attr_handle - attribute handle being written.
 * @param value - data to be written.
 * @param length - the length of value.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_write_without_response(g_gattc_devies[conn_id].handle, attr_handle, value, len) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_write_without_response)(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);

/**
 * @brief enable a centain CCCD(Client Characteristic Configuration Description).
 *
 * This function is used to enable CCCD. Once enabled, it can receive Indication and Notification
 * from the Server successfully.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param attr_handle - attribute handle of the characteristic you want to enable CCCD.
 * @param ccc_value - bit 0 is used to enable Notification, bit 1 is used to enable Indication.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_subscribe(g_gattc_devies[conn_id].handle, attr_handle, ccc_value) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_subscribe)(gattc_handle_t conn_handle, uint16_t attr_handle, uint16_t ccc_value);

/**
 * @brief disable a centain CCCD(Client Characteristic Configuration Description).
 *
 * This function is used to disable CCCD. Once disable, it cannot receive Indication and Notification
 * from the Server.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param attr_handle - attribute handle of the characteristic you want to disable CCCD.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_unsubscribe(g_gattc_devies[conn_id].handle, attr_handle) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_unsubscribe)(gattc_handle_t conn_handle, uint16_t attr_handle);

/**
 * @brief exchange ATT_MTU.
 *
 * This function is used to initiate an ATT_MTU negotiation, which will initiate a Exchcange MTU procedure.
 * In Bluetooth Core Spec, this procedure can be initiated once per connection. If it is initiated more than
 * once, BT_STATUS_FAIL will be returned.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param mtu - MTU size.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_exchange_mtu(g_gattc_devies[conn_id].handle, mtu) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_exchange_mtu)(gattc_handle_t conn_handle, uint32_t mtu);

/**
 * @brief Modify BLE connection parameters.
 *
 * This function is used to modify BLE connection parameters.
 * Note: The following conditions must be met:
 *       min_interval <= max_interval.
 *       min_connection_event_length <= max_connection_event_length.
 *       timeout > (1 + latency) * max_interval * 2
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param min_interval - min connection interval(n * 1.25ms).
 * @param max_interval - max connection interval(n * 1.25ms).
 * @param latency - peripheral latency.
 * @param timeout - supervision timeout(n * 10ms).
 * @param min_connection_event_length - min connection event length(n * 0.625ms).
 * @param max_connection_event_length - max connection event length(n * 0.625ms).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_update_connection_parameter(g_gattc_devies[conn_id].handle, min_interval, max_interval, latency,
            timeout, min_connection_event_length, max_connection_event_length)
        != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_update_connection_parameter)(gattc_handle_t conn_handle, uint32_t min_interval, uint32_t max_interval,
    uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length,
    uint32_t max_connection_event_length);

/**
 * @brief read Tx & Rx PHY.
 *
 * This function is used to read PHY.(1M, 2M, Coded)
 *
 * @param conn_handle - gattc connection handle(void*).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_read_phy(g_gattc_devies[conn_id].handle) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_read_phy)(gattc_handle_t conn_handle);

/**
 * @brief update PHY.
 *
 * This function is used to update PHY type(1M, 2M, Coded).
 *
 * @param conn_handle - gattc connection handle(void*).
 * @param tx_phy - Tx PHY type wants to update.
 * @param rx_phy - Rx PHY type wants to update.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_update_phy(g_gattc_devies[conn_id].handle, tx, rx) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_update_phy)(gattc_handle_t conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);

/**
 * @brief read RSSI.
 *
 * This function is used to read RSSI value.
 *
 * @param conn_handle - gattc connection handle(void*).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gattc_read_rssi(g_gattc_devies[conn_id].handle) != BT_STATUS_SUCCESS) {
    // Handle Error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gattc_read_rssi)(gattc_handle_t conn_handle);

#ifdef __cplusplus
}
#endif

#endif /* __BT_GATTC_H__ */
