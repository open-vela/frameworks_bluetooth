
#ifndef _SRV_INC_GATT_CLIENT_MANAGER_H
#define _SRV_INC_GATT_CLIENT_MANAGER_H

#include <nuttx/list.h>
#include <stddef.h>

#include "btm_manager.h"
#include "bts_common.h"

typedef void (*bts_gattc_connect_state_changed_callback)(void* handle, bt_connection_state state);
typedef void (*bts_gattc_services_discovered_callback)(void* handle, gatt_element_t* element, uint16_t size);
typedef void (*bts_gattc_read_rsp_callback)(void* handle, gatt_element_t* element, uint8_t* value, uint16_t size, gatt_service_status_t status);
typedef void (*bts_gattc_write_req_callback)(void* handle, gatt_element_t* element, gatt_service_status_t status);
typedef void (*bts_gattc_nofity_req_callback)(void* handle, gatt_element_t* element, uint8_t* value, uint16_t size);
typedef void (*bts_gattc_read_rssi_callback)(void* handle, int32_t rssi, gatt_service_status_t status);
typedef void (*bts_gattc_read_phy_callback)(void* handle, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*bts_gattc_update_phy_callback)(void* handle, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*bts_gattc_mtu_updated_callback)(void* handle, uint32_t mtu);

typedef struct {
    bts_gattc_connect_state_changed_callback bts_gattc_connection_state_changed_cb;
    bts_gattc_services_discovered_callback bts_gattc_service_discovered_cb;
    bts_gattc_read_rsp_callback bts_gattc_read_result_cb;
    bts_gattc_write_req_callback bts_gattc_write_result_cb;
    bts_gattc_nofity_req_callback bts_gattc_nofity_request_cb;
    bts_gattc_read_rssi_callback bts_gattc_rssi_read_cb;
    bts_gattc_read_phy_callback bts_gattc_phy_read_cb;
    bts_gattc_update_phy_callback bts_gattc_phy_update_cb;
    bts_gattc_mtu_updated_callback bts_gattc_mtu_changed_cb;
} bts_gatt_client_callbacks;

typedef struct
{
    struct list_node node;

    bts_gatt_client_callbacks* callbacks;
    bd_addr_t remote_addr;
    void* btm_handle;
} bts_gattc_hdl_t;

typedef struct {
    size_t size;

    bt_result_code (*connect)(bts_gattc_hdl_t handle);
    bt_result_code (*disconnect)(bd_addr_t addr);
    bt_result_code (*discover_services)(bd_addr_t addr, bt_uuid_t uuid);
    bt_result_code (*read_request)(bd_addr_t addr, gatt_element_t* element);
    bt_result_code (*write_request)(bd_addr_t addr, gatt_element_t* element, uint8_t* value, uint16_t length);
    bt_result_code (*register_notification)(bd_addr_t addr, gatt_element_t* element, bool enable);
    bt_result_code (*read_rssi)(bd_addr_t addr);
    bt_result_code (*read_phy)(bd_addr_t addr);
    bt_result_code (*update_phy)(bd_addr_t addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
    bt_result_code (*update_mtu)(bd_addr_t addr, uint32_t mtu);
    bt_result_code (*update_connection_parameter)(bd_addr_t addr, uint32_t min_interval, uint32_t max_interval,
        uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length);
} bts_gattc_interface_t;

const bts_gattc_interface_t* get_bts_gattc_instance(void);

#endif