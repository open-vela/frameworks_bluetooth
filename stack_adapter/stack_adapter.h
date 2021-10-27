
#ifndef _ADAPTER_INC_STACK_BLE_H
#define _ADAPTER_INC_STACK_BLE_H

#include "stack_adapter_gap.h"
#include "stack_adapter_gatt.h"



typedef void (*ble_scan_result_callback)(const SERVICE_SCAN_RESULT_DATA_S* result);

typedef struct {
    ble_scan_result_callback ble_scan_result;
} stack_le_scan_callbacks;

typedef void (*ble_advertise_started_callback)(uint8_t adv_id);
typedef void (*ble_advertise_stopped_callback)(uint8_t adv_id);
typedef struct {
    ble_advertise_started_callback ble_advtise_started_cb;
    ble_advertise_stopped_callback ble_advtise_stopped_cb;
} stack_le_advertise_callbacks;

typedef void (*gatt_client_connection_state_changed_callback)(bd_addr_t remote_addr,
    bt_connection_state state);
typedef void (*gatt_client_service_discovered_callback)(bd_addr_t remote_addr, gatt_element_t* elements,
    uint16_t size);
typedef void (*gatt_client_element_read_callback)(bd_addr_t remote_addr, gatt_element_t* element, uint8_t* value,
    uint16_t length, gatt_status status);
typedef void (*gatt_client_element_written_callback)(bd_addr_t remote_addr, gatt_element_t* element,
    gatt_status status);
typedef void (*gatt_client_element_changed_callback)(bd_addr_t remote_addr, gatt_element_t* element,
    uint8_t* value, uint16_t length);
typedef void (*gatt_client_remote_rssi_read_callback)(bd_addr_t remote_addr, int32_t rssi, gatt_status status);
typedef void (*gatt_client_phy_read_callback)(bd_addr_t remote_addr, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy);
typedef void (*gatt_client_phy_update_callback)(bd_addr_t remote_addr, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy, gatt_status status);
typedef void (*gatt_client_mtu_changed_callback)(bd_addr_t remote_addr, uint32_t mtu, gatt_status status);

typedef struct {
    uint8_t size;
    gatt_client_connection_state_changed_callback gatt_client_connection_state_changed_cb;
    gatt_client_service_discovered_callback gatt_client_service_discovered_cb;
    gatt_client_element_read_callback gatt_client_element_read_cb;
    gatt_client_element_written_callback gatt_client_element_written_cb;
    gatt_client_element_changed_callback gatt_client_element_changed_cb;
    gatt_client_remote_rssi_read_callback gatt_client_remote_rssi_read_cb;
    gatt_client_phy_read_callback gatt_client_phy_read_cb;
    gatt_client_phy_update_callback gatt_client_phy_update_cb;
    gatt_client_mtu_changed_callback gatt_client_mtu_changed_cb;
} stack_gatt_client_callbacks;

typedef void (*gatt_server_connection_state_changed_callback)(bd_addr_t remote_addr,
    bt_connection_state state);
typedef void (*gatt_server_elements_added_callback)(gatt_status status, gatt_element_t* elements,
    uint16_t size);
typedef void (*gatt_server_elements_removed_callback)(gatt_status status, gatt_element_t* elements,
    uint16_t size);
typedef void (*gatt_server_phy_read_callback)(bd_addr_t remote_addr, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy);
typedef void (*gatt_server_phy_update_callback)(bd_addr_t remote_addr, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy, gatt_status status);
typedef void (*gatt_server_received_element_read_request_callback)(bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element);
typedef void (*gatt_server_received_element_write_request_callback)(bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value,
    uint16_t offset, uint16_t length);
typedef void (*gatt_server_mtu_changed_callback)(bd_addr_t remote_addr, uint32_t mtu);
typedef void (*gatt_server_notification_sent_callback)(bd_addr_t remote_addr, gatt_status status);

typedef struct {
    uint8_t size;
    gatt_server_connection_state_changed_callback gatt_server_connection_state_changed_cb;
    gatt_server_elements_added_callback gatt_server_elements_added_cb;
    gatt_server_elements_removed_callback gatt_server_elements_removed_cb;
    gatt_server_phy_read_callback gatt_server_phy_read_cb;
    gatt_server_phy_update_callback gatt_server_phy_update_cb;
    gatt_server_received_element_read_request_callback gatt_server_received_element_read_request_cb;
    gatt_server_received_element_write_request_callback gatt_server_received_element_write_request_cb;
    gatt_server_mtu_changed_callback gatt_server_mtu_changed_cb;
    gatt_server_notification_sent_callback gatt_server_notification_sent_cb;
} stack_gatt_server_callbacks;

typedef struct {
    stack_le_advertise_callbacks* gatt_advertise_callbacks;
    stack_le_scan_callbacks* gatt_scan_callbacks;
    stack_gatt_client_callbacks* client_callbacks;
    stack_gatt_server_callbacks* server_callbacks;
} stack_bt_gatt_callbacks;

typedef struct
{
    bt_result_code (*ble_set_scan_parameters)(scan_settings_t* setting);
    bt_result_code (*ble_set_scan_filter)(scan_filter_t* filter);
    bt_result_code (*ble_start_scan)();
    bt_result_code (*ble_stop_scan)();
} stack_ble_scanner_interface;

typedef struct
{
    bt_result_code (*ble_start_adv)(advertise_settings settings, advertise_data data, uint8_t* advertise_id);
    bt_result_code (*ble_stop_adv)(uint8_t adv_id);
} stack_ble_advertiser_interface;

typedef struct
{
    uint8_t size;

    bt_result_code (*connect)(bd_addr_t addr, stack_gatt_client_callbacks* callbacks);
    bt_result_code (*disconnect)(bd_addr_t addr);
    bt_result_code (*discover_services)(bd_addr_t addr, bt_uuid_t uuid);
    bt_result_code (*read_request)(bd_addr_t addr, gatt_element_t* element);
    bt_result_code (*write_request)(bd_addr_t addr, gatt_element_t* element, uint8_t* value,
        uint16_t length);
    bt_result_code (*register_notification)(bd_addr_t addr, gatt_element_t element, bool enable);
    bt_result_code (*read_rssi)(bd_addr_t addr);
    bt_result_code (*read_phy)(bd_addr_t addr);
    bt_result_code (*update_phy)(bd_addr_t addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
    bt_result_code (*update_mtu)(bd_addr_t addr, uint32_t mtu);
    bt_result_code (*update_connection_parameter)(bd_addr_t addr, uint32_t max_interval,
        uint32_t latency, uint32_t timeout,
        uint32_t min_connection_event_length,
        uint32_t max_connection_event_length);
} stack_gatt_client_interface;

typedef struct
{
    uint8_t size;

    bt_result_code (*open)(stack_gatt_server_callbacks* callbacks);
    bt_result_code (*close)();
    bt_result_code (*connect)(bd_addr_t remote_addr, bool auto_connect);
    bt_result_code (*disconnect)(bd_addr_t remote_addr);
    bt_result_code (*add_service)(gatt_element_t* element);
    bt_result_code (*remove_service)(gatt_element_t* element);
    bt_result_code (*clear_service)();
    gatt_element_t* (*get_service)();
    bt_connection_state (*get_connection_state)();
    bt_result_code (*read_phy)(bd_addr_t remote_addr);
    bt_result_code (*update_phy)(bd_addr_t remote_addr, ble_phy_type_t tx_type, ble_phy_type_t rx_type);
    bt_result_code (*send_notify)(bd_addr_t remote_addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_indicate)(bd_addr_t remote_addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_response)(bd_addr_t remote_addr, uint32_t id, uint8_t* value, size_t size);
} stack_gatt_server_interface;

typedef void (*bthd_application_state_callback)(bd_addr_t* bd_addr_t,
    bthd_application_state_t state);
typedef void (*bthd_connection_state_callback)(bd_addr_t* bd_addr_t,
    bthd_connection_state_t state);
typedef void (*bthd_get_report_callback)(uint8_t type, uint8_t id,
    uint16_t buffer_size);
typedef void (*bthd_set_report_callback)(uint8_t type, uint8_t id, uint16_t len,
    uint8_t* p_data);
typedef void (*bthd_set_protocol_callback)(uint8_t protocol);
typedef void (*bthd_intr_data_callback)(uint8_t report_id, uint16_t len,
    uint8_t* p_data);
typedef void (*bthd_vc_unplug_callback)(void);
 
typedef struct
{
    size_t size;

    bt_result_code (*init)(stack_bt_gatt_callbacks* callbacks);
    void (*cleanup)(void);
    bt_result_code (*register_callback)(stack_le_scan_callbacks* scan_cb,
        stack_le_advertise_callbacks* adv_cb, stack_gatt_client_callbacks* client_cb,
        stack_gatt_server_callbacks* server_cb);
    bt_result_code (*unregister_callback)();

    const stack_ble_scanner_interface* scanner;
    const stack_ble_advertiser_interface* advertiser;
    const stack_gatt_client_interface* client;
    const stack_gatt_server_interface* server;
} stack_bt_gatt_interface;

typedef SERVICE_BT_STACK_STATE bt_state_t;
typedef void (*adapter_state_changed_callback)(bt_state_t state);

typedef struct {
  size_t size;
  adapter_state_changed_callback adapter_state_changed_cb;
} stack_bt_callbacks;

typedef struct {
    size_t size;

    bt_result_code (*init)(stack_bt_callbacks* callback);
    bt_result_code (*clean_up)();
    const void* (*get_profile_interface)(const char* profile_id);
} stack_bluetooth_adapter;

stack_bluetooth_adapter* get_stack_bluetooth_adapter();
#endif