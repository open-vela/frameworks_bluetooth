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

#ifndef __BT_GATTS_H__
#define __BT_GATTS_H__

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

typedef void* gatts_handle_t;

/**
 * @endcond
 */

/**
 * @brief callback for GATT server recveived read request.
 *
 * This callback is triggered when peer GATT client initiate read request to characteristic with property
 * contains the read flag bit.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address. Address of the peer device that initiated the read request.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @param req_handle - request handle. Identifies the order in which the read request was made.
 * @return uint16_t.
 *
 * **Example:**
 * @code
uint16_t rx_char_on_read(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle)
{
    bt_status_t ret = bt_gatts_response(srv_handle, addr, req_handle, read_char_value, sizeof(read_char_value));
    PRINT("gatts service RX char response. status: %d", ret);
    return 0;
}
 * @endcode
 */
typedef uint16_t (*attribute_read_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle);

/**
 * @brief callback for GATT server recveived write request.
 *
 * This callback is triggered when peer GATT client initiate write request to characteristic with property
 * contains the write flag bit.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address. Address of the peer device that initiated the read request.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @param value - attribute value.
 * @param length - value length.
 * @param offset - value offset.
 * @return uint16_t. value length.
 *
 * **Example:**
 * @code
uint16_t rx_char_on_write(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, const uint8_t* value, uint16_t length, uint16_t offset)
{
    PRINT_ADDR("gatts service RX char received write request, addr:%s", addr);
    return length;
}
 * @endcode
 */
typedef uint16_t (*attribute_written_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset);

/**
 * @cond
 */

typedef struct {
    uint16_t handle;
    bt_uuid_t uuid;
    gatt_attr_type_t type;
    uint32_t properties;
    uint32_t permissions;
    gatt_attr_rsp_t rsp_type;
    attribute_read_cb_t read_cb;
    attribute_written_cb_t write_cb;
    uint32_t attr_length;
    uint8_t* attr_value;

} gatt_attr_db_t;

typedef struct {
    int32_t attr_num;
    gatt_attr_db_t* attr_db;
} gatt_srv_db_t;

/**
 * @endcond
 */

/**
 * @brief callback for connected as GATT server.
 *
 * This callback is triggered when gatts connected. BT Service use this callback to notify the application calling
 * bt_gatts_connect that gatts connection has been established.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address.
 * @return void.
 *
 * **Example:**
 * @code
static void connect_callback(void* srv_handle, bt_address_t* addr)
{
    PRINT_ADDR("gatts_connect_callback, addr:%s", addr);
    add_gatts_device(addr);
}
 * @endcode
 */
typedef void (*gatts_connected_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr);

/**
 * @brief callback for disconnected as GATT server.
 *
 * This callback is triggered when gatts disconnected. BT Service use this callback to notify the application that
 * the gatts connection is destroyed.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address.
 * @return void.
 *
 * **Example:**
 * @code
static void disconnect_callback(void* srv_handle, bt_address_t* addr)
{
    gatts_device_t* device = find_gatts_device(addr);
    remove_gatts_device(device);
    PRINT_ADDR("gatts_disconnect_callback, addr:%s", addr);
}
 * @endcode
 */
typedef void (*gatts_disconnected_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr);

/**
 * @brief callback for Addition of GATT service.
 *
 * This callback is triggered when GATT server add a GATT service.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param status - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @return void.
 *
 * **Example:**
 * @code
static void attr_table_added_callback(void* srv_handle, gatt_status_t status, uint16_t attr_handle)
{
    PRINT("gatts add attribute table complete, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
}
 * @endcode
 */
typedef void (*gatts_attr_table_added_cb_t)(gatts_handle_t srv_handle, gatt_status_t status, uint16_t attr_handle);

/**
 * @brief callback for Deletion of GATT service.
 *
 * This callback is triggered when GATT server remove a GATT service.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param status - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @return void.
 *
 * **Example:**
 * @code
static void attr_table_removed_callback(void* srv_handle, gatt_status_t status, uint16_t attr_handle)
{
    PRINT("gatts remove attribute table complete, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
}
 * @endcode
 */
typedef void (*gatts_attr_table_removed_cb_t)(gatts_handle_t srv_handle, gatt_status_t status, uint16_t attr_handle);

/**
 * @brief callback for GATT server receive Exchange MTU request.
 *
 * This callback is triggered as server receive Exchange MTU request from peer GATT client.
 * BT Service use this callback to Report the results of MTU negotiation.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address.
 * @param mtu - negotiated MTU.
 * @return void.
 *
 * **Example:**
 * @code
static void mtu_changed_callback(void* srv_handle, bt_address_t* addr, uint32_t mtu)
{
    gatts_device_t* device = find_gatts_device(addr);
    if (device) {
        device->gatt_mtu = mtu;
    }
    PRINT_ADDR("gatts_mtu_changed_callback, addr:%s, mtu:%" PRIu32, addr, mtu);
}
 * @endcode
 */
typedef void (*gatts_mtu_changed_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t mtu);

/**
 * @brief callback for GATT server excute attribute notify operation.
 *
 * This callback is triggered when GATT server initiate notify to characteristic with property
 * contains the notify flag bit.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address.
 * @param status - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @return void.
 *
 * **Example:**
 * @code
static void notify_complete_callback(void* srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle)
{
    if (status != GATT_STATUS_SUCCESS) {
        PRINT_ADDR("gatts service notify failed, addr:%s, handle 0x%" PRIx16 ", status:%d", addr, attr_handle, status);
        return;
    }

    if (throughtput_cursor) {
        throughtput_cursor--;
    } else {
        PRINT_ADDR("gatts service notify complete, addr:%s, handle 0x%" PRIx16 ", status:%d", addr, attr_handle, status);
    }
}
 * @endcode
 */
typedef void (*gatts_nofity_complete_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle);

/**
 * @brief callback for GATT server execute read PHY.
 *
 * This callback is triggered when application initiating read PHY procedure. BT Service use this callback to Report
 * the Tx PHY & Rx PHY.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param tx_phy - current Tx PHY.
 * @param rx_phy - current Rx PHY.
 * @return void.
 *
 * **Example:**
 * @code
static void phy_read_callback(void* srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    PRINT_ADDR("gatts read phy complete, addr:%s, tx:%d, rx:%d", addr, tx_phy, rx_phy);
}
 * @endcode
 */
typedef void (*gatts_phy_read_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);

/**
 * @brief callback for GATT server execute update PHY operation.
 *
 * This callback is triggered when application initiating update PHY procedure. BT Service use this callback to Report
 * the result of update Tx PHY & Rx PHY.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address.
 * @param status - GATT_STATUS_SUCCESS on success, and other error codes on failure.
 * @param tx_phy - Tx PHY after update. If update operation failed, keep original value.
 * @param rx_phy - Rx PHY after update. If update operation failed, keep original value.
 * @return void.
 *
 * **Example:**
 * @code
static void phy_updated_callback(void* srv_handle, bt_address_t* addr, gatt_status_t status, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    PRINT_ADDR("gatts phy updated, addr:%s, status:%d, tx:%d, rx:%d", addr, status, tx_phy, rx_phy);
}
 * @endcode
 */
typedef void (*gatts_phy_updated_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy);

/**
 * @brief callback for connnection parameter update.
 *
 * This callback is triggered when peer device initiating connnection parameter update procedure. BT Service use this
 *  callback to Report the connection parameter.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address.
 * @param connection_interval - connection interval(n * 1.25ms).
 * @param peripheral_latency - peripheral latency.
 * @param supervision_timeout - supervision timeout(n * 10ms).
 * @return void.
 *
 * **Example:**
 * @code
static void conn_param_changed_callback(void* srv_handle, bt_address_t* addr, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout)
{
    PRINT_ADDR("gatts_conn_param_changed_callback, addr:%s, interval:%" PRIu16 ", latency:%" PRIu16 ", timeout:%" PRIu16,
        addr, connection_interval, peripheral_latency, supervision_timeout);
}
 * @endcode
 */
typedef void (*gatts_connection_parameter_changed_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout);

/**
 * @cond
 */

typedef struct {
    uint32_t size;
    gatts_connected_cb_t on_connected;
    gatts_disconnected_cb_t on_disconnected;
    gatts_attr_table_added_cb_t on_attr_table_added;
    gatts_attr_table_removed_cb_t on_attr_table_removed;
    gatts_nofity_complete_cb_t on_notify_complete;
    gatts_mtu_changed_cb_t on_mtu_changed;
    gatts_phy_read_cb_t on_phy_read;
    gatts_phy_updated_cb_t on_phy_updated;
    gatts_connection_parameter_changed_cb_t on_conn_param_changed;

} gatts_callbacks_t;

/**
 * @endcond
 */

/**
 * @brief regsiter a service.
 *
 * @param ins - Bluetooth client instance.
 * @param phandle - pointer of gatts connection handle(void*).
 * @param callbacks - gatts service callback table.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
ret = bt_gatts_register_service(handle, &g_dis_handle, &gatts_cbs);
if (ret != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_register_service)(bt_instance_t* ins, gatts_handle_t* phandle, gatts_callbacks_t* callbacks);

/**
 * @brief unregsiter a service.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_unregister_service(service_handle) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_unregister_service)(gatts_handle_t srv_handle);

/**
 * @brief create a connect with peer device.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address.
 * @param addr_type - peer address type(ble_addr_type_t).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_connect(service_handle, &addr, BT_LE_ADDR_TYPE_UNKNOWN) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_connect)(gatts_handle_t srv_handle, bt_address_t* addr, ble_addr_type_t addr_type);

/**
 * @brief initiate a disconnect with peer device.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_disconnect(service_handle, &addr) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_disconnect)(gatts_handle_t srv_handle, bt_address_t* addr);

/**
 * @brief add GATT service attribute to attribute table.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param srv_db - GATT service attributes.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_add_attr_table(service_handle, service_db) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_add_attr_table)(gatts_handle_t srv_handle, gatt_srv_db_t* srv_db);

/**
 * @brief remove GATT service attribute from attribute table.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param attr_handle - GATT service attributes.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_remove_attr_table(service_handle, attr_handle) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_remove_attr_table)(gatts_handle_t srv_handle, uint16_t attr_handle);

/**
 * @brief set GATT service attribute value.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @param value - attribute value.
 * @param length - value length.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_set_attr_value(g_bas_handle, BAS_BATTERY_LEVEL_CHR_ID, (uint8_t*)&battery_level, sizeof(battery_level)) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_set_attr_value)(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);

/**
 * @brief get GATT service attribute value.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @param value - attribute value.
 * @param length - value length.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_get_attr_value(g_bas_handle, BAS_BATTERY_LEVEL_CHR_ID, buffer, sizeof(buffer)) != BT_STATUS_SUCCESS)
    // Handle Error
// Handle attribute value
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_get_attr_value)(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t* length);

/**
 * @brief response attribute value for GATT server recveived read request.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address. Address of the peer device that initiated the read request.
 * @param req_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @param value - attribute value.
 * @param length - value length.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
uint16_t rx_char_on_read(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle)
{
    PRINT_ADDR("gatts service RX char received read request, addr:%s", addr);
    bt_status_t ret = bt_gatts_response(srv_handle, addr, req_handle, read_char_value, sizeof(read_char_value));
    PRINT("gatts service RX char response. status: %d", ret);
    return 0;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_response)(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t req_handle, uint8_t* value, uint16_t length);

/**
 * @brief server notify attribute value to client.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address. Address of the peer device that initiated the read request.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @param value - attribute value.
 * @param length - value length.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_notify(g_bas_handle, &addr, BAS_BATTERY_LEVEL_CHR_ID, (uint8_t*)&battery_level, sizeof(battery_level)) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_notify)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length);

/**
 * @brief server indicate attribute value to client.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address. Address of the peer device that initiated the read request.
 * @param attr_handle - attribute handle. The handle identified by each service, not the Attribute handle in spec.
 * @param value - attribute value.
 * @param length - value length.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_indicate(g_custom_handle, &addr, IOT_SERVICE_TX_CHR_ID, (uint8_t*)argv[1], strlen(argv[1])) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_indicate)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length);

/**
 * @brief read Tx & Rx PHY.
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address. Address of the peer device that initiated the read request.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_read_phy(service_handle, &addr) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_read_phy)(gatts_handle_t srv_handle, bt_address_t* addr);

/**
 * @brief update PHY
 *
 * @param srv_handle - gatts handle(void*). Each GATT service has its own handle.
 * @param addr - remote device address. Address of the peer device that initiated the read request.
 * @param tx_phy - Tx PHY type wants to update.
 * @param rx_phy - Rx PHY type wants to update.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, and other error codes on failure.
 *
 * **Example:**
 * @code
if (bt_gatts_update_phy(service_handle, &addr, tx, rx) != BT_STATUS_SUCCESS)
    // Handle Error
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_gatts_update_phy)(gatts_handle_t srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);

#ifdef __cplusplus
}
#endif

#endif /* __BT_GATTS_H__ */
