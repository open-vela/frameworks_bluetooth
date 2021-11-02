#ifndef _MGR_INC_BLUETOOTH_GATT_S_H
#define _MGR_INC_BLUETOOTH_GATT_S_H

#include <stddef.h>

#include "btm_manager.h"
#include "bts_common.h"

typedef void (*server_connection_state_changed_callback)(void* handle, bd_addr_t remote_addr, bt_state_t state);
typedef void (*server_opened_callback)(void* handle);
typedef void (*server_closed_callback)(void* handle);
typedef void (*server_service_added_callback)(void* handle, gatt_service_status_t status, gatt_element_t* element,
    size_t size);
typedef void (*server_service_removed_callback)(void* handle, gatt_service_status_t status, gatt_element_t* element,
    size_t size);
typedef void (*server_phy_read_callback)(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*server_phy_update_callback)(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx, gatt_service_status_t status);
typedef void (*server_read_request_callback)(void* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element);
typedef void (*server_write_request_callback)(void* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size);
typedef void (*server_mtu_changed_callback)(void* handle, bd_addr_t remote_addr, uint32_t mtu);
typedef void (*server_notify_sent_callback)(void* handle, bd_addr_t remote_addr, gatt_service_status_t status);

typedef struct {
    server_connection_state_changed_callback le_server_connection_state_changed_cb;
    server_opened_callback le_server_opened_cb;
    server_closed_callback le_server_closed_cb;
    server_service_added_callback le_server_service_added_cb;
    server_service_removed_callback le_server_service_removed_cb;
    server_phy_read_callback le_server_phy_read_cb;
    server_phy_update_callback le_server_phy_update_cb;
    server_read_request_callback le_server_read_request_cb;
    server_write_request_callback le_server_write_request_cb;
    server_mtu_changed_callback le_server_mtu_changed_cb;
    server_notify_sent_callback le_server_notify_sent_cb;
} gatt_server_callbacks;

typedef struct
{
    uint8_t server_if;
    gatt_server_callbacks* callbacks;
} gatt_server_t;

typedef struct {
    size_t size;

    bt_result_code (*open)(gatt_server_t** handle, gatt_server_callbacks* callbacks);
    bt_result_code (*close)(gatt_server_t* handle);
    bt_result_code (*connect)(gatt_server_t* handle, bd_addr_t remote_addr, bool auto_connect);
    bt_result_code (*disconnect)(gatt_server_t* handle, bd_addr_t remote_addr);
    bt_result_code (*add_service)(gatt_server_t* handle, gatt_element_t* element, uint16_t size);
    bt_result_code (*remove_service)(gatt_server_t* handle, gatt_element_t* element);
    bt_result_code (*read_phy)(gatt_server_t* handle, bd_addr_t remote_addr);
    bt_result_code (*update_phy)(gatt_server_t* handle, bd_addr_t remote_addr, ble_phy_type_t tx_type, ble_phy_type_t rx_type);
    bt_result_code (*send_notify)(gatt_server_t* handle, bd_addr_t remote_addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_indicate)(gatt_server_t* handle, bd_addr_t remote_addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_response)(gatt_server_t* handle, bd_addr_t remote_addr, gatt_response_t* response);
} btm_le_gatts_interface_t;

btm_le_gatts_interface_t* get_le_gatts_interface(void* bt_mgr_interface);

#endif