
#ifndef _SRV_INC_GATT_SERVER_MANAGER_H
#define _SRV_INC_GATT_SERVER_MANAGER_H

#include <nuttx/list.h>
#include <stddef.h>

#include "btm_manager.h"
#include "bts_common.h"

typedef void (*server_connection_state_changed_cb)(void* handle, bd_addr_t remote_addr, bt_state_t state);
typedef void (*server_opened_cb)(void* handle, uint8_t server_if);
typedef void (*server_closed_cb)(void* handle);

typedef void (*server_element_added_cb)(void* handle, gatt_service_status_t status, gatt_element_t* element,
    size_t size);
typedef void (*server_element_removed_cb)(void* handle, gatt_service_status_t status, gatt_element_t* element,
    size_t size);
typedef void (*server_phy_read_cb)(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*server_phy_update_cb)(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*server_read_request_cb)(void* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element);
typedef void (*server_write_request_cb)(void* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size);
typedef void (*server_mtu_changed_cb)(void* handle, bd_addr_t remote_addr, uint32_t mtu);
typedef void (*server_notify_sent_cb)(void* handle, bd_addr_t remote_addr, gatt_service_status_t status);

typedef struct {
    server_connection_state_changed_cb _server_connection_state_changed_cb;
    server_opened_cb _server_opened_cb;
    server_closed_cb _server_closed_cb;
    server_element_added_cb _server_elements_added_cb;
    server_element_removed_cb _server_elements_removed_cb;
    server_phy_read_cb _server_phy_read_cb;
    server_phy_update_cb _server_phy_update_cb;
    server_read_request_cb _server_read_request_cb;
    server_write_request_cb _server_write_request_cb;
    server_mtu_changed_cb _server_mtu_changed_cb;
    server_notify_sent_cb _server_notify_sent_cb;
} ble_gatt_server_callbacks;

typedef struct {
    struct list_node node;

    uint8_t server_if;
    ble_gatt_server_callbacks* callbacks;

    void* btm_handle;
} gatts_hdl_t;

typedef struct {
    size_t size;

    bt_result_code (*init)();
    void (*clean_up)();

    bt_result_code (*open_server)(gatts_hdl_t handle);
    bt_result_code (*close_server)(uint8_t server_if);
    bt_result_code (*connect)(uint8_t server_if, bd_addr_t addr, bool auto_connect);
    bt_result_code (*disconnect)(uint8_t server_if, bd_addr_t addr);
    bt_result_code (*add_element)(uint8_t server_if, gatt_element_t* element, uint16_t size);
    bt_result_code (*remove_element)(uint8_t server_if, uint32_t* ids, uint16_t size);
    bt_result_code (*read_phy)(uint8_t server_if, bd_addr_t addr);
    bt_result_code (*update_phy)(uint8_t server_if, bd_addr_t addr, ble_phy_type_t tx_type, ble_phy_type_t rx_type);
    bt_result_code (*send_notify)(uint8_t server_if, bd_addr_t addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_indicate)(uint8_t server_if, bd_addr_t addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_response)(uint8_t server_if, bd_addr_t remote_addr, gatt_response_t* response)

} gatt_server_interface_t;

const gatt_server_interface_t* get_gatt_server_instance();
#endif