#ifndef _MGR_INC_BLUETOOTH_GATT_C_H
#define _MGR_INC_BLUETOOTH_GATT_C_H

#include <stddef.h>

#include "btm_manager.h"
#include "bts_common.h"

typedef void (*btm_gattc_connection_state_changed_callback)(void* handle, bd_addr_t remote_addr, bt_state_t state);
typedef void (*btm_gattc_service_discovered_callback)(void* handle, bd_addr_t remote_addr, gatt_element_t* element, uint16_t size);
typedef void (*btm_gattc_read_result_callback)(void* handle, bd_addr_t remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size, gatt_service_status_t status);
typedef void (*btm_gattc_write_result_callback)(void* handle, bd_addr_t remote_addr, gatt_element_t* element, gatt_service_status_t status);
typedef void (*btm_gattc_nofity_request_callback)(void* handle, bd_addr_t remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size);
typedef void (*btm_gattc_rssi_read_callback)(void* handle, bd_addr_t remote_addr, int32_t rssi, gatt_service_status_t status);
typedef void (*btm_gattc_phy_read_callback)(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*btm_gattc_phy_update_callback)(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*btm_gattc_mtu_changed_callback)(void* handle, bd_addr_t remote_addr, uint32_t mtu);

typedef struct {
    btm_gattc_connection_state_changed_callback gattc_connection_state_changed_cb;
    btm_gattc_service_discovered_callback gattc_service_discovered_cb;
    btm_gattc_read_result_callback gattc_read_result_cb;
    btm_gattc_write_result_callback gattc_write_result_cb;
    btm_gattc_nofity_request_callback gattc_nofity_request_cb;
    btm_gattc_rssi_read_callback gattc_rssi_read_cb;
    btm_gattc_phy_read_callback gattc_phy_read_cb;
    btm_gattc_phy_update_callback gattc_phy_update_cb;
    btm_gattc_mtu_changed_callback gattc_mtu_changed_cb;
} gatt_client_callbacks;

typedef struct {
    size_t size;

    bt_result_code (*connect)(void** handle, bd_addr_t remote_addr, gatt_client_callbacks* callbacks);
    bt_result_code (*disconnect)(void* handle);
    bt_result_code (*discover_services)(void* handle, bt_uuid_t uuid);
    bt_result_code (*read_request)(void* handle, gatt_element_t* element);
    bt_result_code (*write_request)(void* handle, gatt_element_t* element, uint8_t* value, uint16_t length);
    bt_result_code (*register_notification)(void* handle, gatt_element_t* element, bool enable);
    bt_result_code (*read_rssi)(void* handle);
    bt_result_code (*read_phy)(void* handle);
    bt_result_code (*update_phy)(void* handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
    bt_result_code (*update_mtu)(void* handle, uint32_t mtu);
    bt_result_code (*update_connection_parameter)(void* handle, uint32_t min_interval, uint32_t max_interval,
        uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length);
} btm_gatt_client_interface_t;

btm_gatt_client_interface_t* get_btm_gattc_interface(void* bt_mgr_interface);

#endif