
#ifndef _SRV_INC_GATT_CLIENT_MANAGER_H
#define _SRV_INC_GATT_CLIENT_MANAGER_H

#include "gatt_common.h"

#include <stddef.h>

typedef void (*client_connection_state_changed_callback)(bd_addr_t remote_addr, bt_connection_state state);
typedef void (*client_service_discovered_callback)(bd_addr_t remote_addr, gatt_element_t* element,
    uint16_t size);
typedef void (*client_read_result_callback)(bd_addr_t remote_addr, gatt_element_t* element, uint8_t* value,
    uint16_t size, gatt_service_status_t status);
typedef void (*client_write_result_callback)(bd_addr_t remote_addr, gatt_element_t* element,
    gatt_service_status_t status);
typedef void (*client_nofity_request_callback)(bd_addr_t remote_addr, gatt_element_t* element,
    uint8_t* value, uint16_t size);
typedef void (*client_rssi_read_callback)(bd_addr_t remote_addr, int32_t rssi,
    gatt_service_status_t status);
typedef void (*client_phy_read_callback)(bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*client_phy_update_callback)(bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*client_mtu_changed_callback)(bd_addr_t remote_addr, uint32_t mtu);

typedef struct {
    client_connection_state_changed_callback _client_connection_state_changed_cb;
    client_service_discovered_callback _client_service_discovered_cb;
    client_read_result_callback _client_read_result_cb;
    client_write_result_callback _client_write_result_cb;
    client_nofity_request_callback _client_nofity_request_cb;
    client_rssi_read_callback _client_rssi_read_cb;
    client_phy_read_callback _client_phy_read_cb;
    client_phy_update_callback _client_phy_update_cb;
    client_mtu_changed_callback _client_mtu_changed_cb;
} ble_gatt_client_callbacks;

typedef struct {
    size_t size;

    bt_result_code (*connect)(bd_addr_t addr, ble_gatt_client_callbacks* callbacks);
    bt_result_code (*disconnect)(bd_addr_t addr);
    bt_result_code (*discover_services)(bd_addr_t addr, bt_uuid_t uuid);
    bt_result_code (*read_request)(bd_addr_t addr, gatt_element_t* element);
    bt_result_code (*write_request)(bd_addr_t addr, gatt_element_t* element, uint8_t* value,
        uint16_t length);
    bt_result_code (*register_notification)(bd_addr_t addr, gatt_element_t* element, bool enable);
    bt_result_code (*read_rssi)(bd_addr_t addr);
    bt_result_code (*read_phy)(bd_addr_t addr);
    bt_result_code (*update_phy)(bd_addr_t addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
    bt_result_code (*update_mtu)(bd_addr_t addr, uint32_t mtu);
    bt_result_code (*update_connection_parameter)(bd_addr_t addr, uint32_t max_interval,
        uint32_t latency, uint32_t timeout,
        uint32_t min_connection_event_length,
        uint32_t max_connection_event_length);
} ble_gatt_client_interface;

const ble_gatt_client_interface* get_ble_gatt_client_instance();
#endif