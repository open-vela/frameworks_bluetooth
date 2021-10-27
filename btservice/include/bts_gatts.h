
#ifndef _SRV_INC_GATT_SERVER_MANAGER_H
#define _SRV_INC_GATT_SERVER_MANAGER_H

#include "btm_manager.h"
#include "bts_common.h"
#include <stddef.h>

typedef void (*server_connection_state_changed_callback)(bd_addr_t remote_addr, bt_connection_state state);
typedef void (*server_registered_callback)(uint8_t server_if);
typedef void (*server_element_added_callback)(uint8_t server_if, gatt_service_status_t status, gatt_element_t* element,
    size_t size);
typedef void (*server_element_removed_callback)(uint8_t server_if, gatt_service_status_t status, gatt_element_t* element,
    size_t size);
typedef void (*server_phy_read_callback)(uint8_t server_if, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*server_phy_update_callback)(uint8_t server_if, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*server_read_request_callback)(uint8_t server_if, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element);
typedef void (*server_write_request_callback)(uint8_t server_if, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size);
typedef void (*server_mtu_changed_callback)(uint8_t server_if, bd_addr_t remote_addr, uint32_t mtu);
typedef void (*server_notify_sent_callback)(uint8_t server_if, bd_addr_t remote_addr, gatt_service_status_t status);

typedef struct {
    server_connection_state_changed_callback _server_connection_state_changed_cb;
    server_registered_callback _server_registered_cb;
    server_element_added_callback _server_elements_added_cb;
    server_element_removed_callback _server_elements_removed_cb;
    server_phy_read_callback _server_phy_read_cb;
    server_phy_update_callback _server_phy_update_cb;
    server_read_request_callback _server_read_request_cb;
    server_write_request_callback _server_write_request_cb;
    server_mtu_changed_callback _server_mtu_changed_cb;
    server_notify_sent_callback _server_notify_sent_cb;
} ble_gatt_server_callbacks;

typedef struct {
    size_t size;

    bt_result_code (*register_server)(ble_gatt_server_callbacks* callbacks);
    bt_result_code (*unregister_server)(uint8_t server_if);
    bt_result_code (*connect)(uint8_t server_if, bd_addr_t addr, bool auto_connect);
    bt_result_code (*disconnect)(uint8_t server_if, bd_addr_t addr);
    bt_result_code (*add_element)(uint8_t server_if, gatt_element_t* element);
    bt_result_code (*remove_element)(uint8_t server_if, gatt_element_t* element);
    bt_result_code (*clear_service)(uint8_t server_if);
    gatt_element_t* (*get_service)(uint8_t server_if);
    bt_result_code (*get_connection_state)(uint8_t server_if, bt_connection_state* state);
    bt_result_code (*read_phy)(uint8_t server_if, bd_addr_t addr);
    bt_result_code (*update_phy)(uint8_t server_if, bd_addr_t addr, ble_phy_type_t tx_type, ble_phy_type_t rx_type);
    bt_result_code (*send_notify)(uint8_t server_if, bd_addr_t addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_indicate)(uint8_t server_if, bd_addr_t addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_response)(uint8_t server_if, bd_addr_t addr, uint32_t id, uint8_t* value, size_t size);
} ble_gatt_server_interface;

const ble_gatt_server_interface* get_ble_gatt_server_instance();
#endif