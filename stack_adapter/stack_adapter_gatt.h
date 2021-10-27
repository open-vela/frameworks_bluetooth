/******************************************************************************
 *
 *  Copyright (C) Barrot Technology Limited
 *
 ******************************************************************************/

/*******************************************************************************
 *
 *  Filename:      stack_adapter_gatt.h
 *
 *  Description:   Bluetooth Stack GATT adapter interfaces
 *
 *  Author:        Y.D.X
 *
 ******************************************************************************/

#ifndef STACK_ADAPTER_GATT_H
#define STACK_ADAPTER_GATT_H

#include "stack_adapter_common.h"

/*******************************************************************************
 *
 * GATT server connection state changed callback
 * @param       remote_addr     - Remote address
 * @param       state           - connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state);

/*******************************************************************************
 *
 * Indicates whether a local service has been added successfully
 * @param       status   - gatt status
 * @param       elements - added elements. It shall be the same buffer as the first
 *                         parameter of service_adapter_gatt_server_add_elements.
 * @param       size     - elements size
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_elements_added_callback)(SERVICE_GATT_STATUS status,
        SERVICE_GATT_ELEMENT_S *elements,
        uint16_t size);

/*******************************************************************************
 *
 * Indicates whether a local service has been removed successfully
 * @param       status   - gatt status
 * @param       elements - Full elements of the removed service.
 *                         It shall be the same buffer as the first parameter of
 *                         service_adapter_gatt_server_add_elements.
 * @param       size     - elements size
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_elements_removed_callback)(SERVICE_GATT_STATUS status,
        SERVICE_GATT_ELEMENT_S *elements,
        uint16_t size);

/*******************************************************************************
 *
 * PHY read callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_phy_read_callback)(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
        SERVICE_BLE_PHY_TYPE rx_phy);

/*******************************************************************************
 *
 * PHY changed callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_phy_update_callback)(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
        SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_GATT_STATUS status);

/*******************************************************************************
 *
 * A remote client has requested to read a local characteristic or descriptor
 * @param       remote_addr         - Remote address
 * @param       request_id          - request id
 * @param       element             - characteristic or descriptor
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_received_element_read_request_callback)(BD_ADDR remote_addr,
        uint32_t request_id,
        SERVICE_GATT_ELEMENT_S *element);

/*******************************************************************************
 *
 * A remote client has requested to write a local characteristic or descriptor
 * @param       remote_addr         - Remote address
 * @param       request_id          - request id
 * @param       element             - characteristic or descriptor
 * @param       value               - buffer, keeps valid until service_adapter_gatt_server_send_response is called.
 * @param       offset              - offset of the value to write
 * @param       length              - buffer length
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_received_element_write_request_callback)(BD_ADDR remote_addr,
        uint32_t request_id,
        SERVICE_GATT_ELEMENT_S *element, uint8_t *value,
        uint16_t offset, uint16_t length);

/*******************************************************************************
 *
 * mtu changed callback
 * @param       remote_addr         - Remote address
 * @param       mtu                 - The new (ATT_MTU-3) value negotiated.
 *                                    The default mtu value is (23-3).
 *                                    3 is the ATT PDU header size.
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_mtu_changed_callback)(BD_ADDR remote_addr, uint32_t mtu);

/*******************************************************************************
 *
 * a notification or indication has been sent to a remote device
 * @param       remote_addr         - Remote address
 * @param       status              - GATT status
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_server_notification_sent_callback)(BD_ADDR remote_addr,
        SERVICE_GATT_STATUS status);

/*******************************************************************************
 *
 * GATT client connection state changed callback
 * @param       remote_addr     - Remote address
 * @param       state           - connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state);

/*******************************************************************************
 *
 * the list of remote services, characteristics and descriptors
 * for the remote device have been updated
 * @param       remote_addr     - Remote address
 * @param       elements        - full list of elements. A NULL pointer means
 *                              end of the services discovery procedure.
 * @param       size            - elements size
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_service_discovered_callback)(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *elements,
        uint16_t size);

/*******************************************************************************
 *
 * Callback reporting the result of a characteristic or descriptor read operation
 * @param       remote_addr     - Remote address
 * @param       element         - characteristic or descriptor
 * @param       value           - buffer
 * @param       length          - buffer length
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_element_read_callback)(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *element, uint8_t *value,
        uint16_t length, SERVICE_GATT_STATUS status);

/*******************************************************************************
 *
 * Callback reporting the result of a characteristic or descriptor write operation
 * @param       remote_addr     - Remote address
 * @param       element         - characteristic or descriptor
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_element_written_callback)(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *element,
        SERVICE_GATT_STATUS status);

/*******************************************************************************
 *
 * Callback triggered as a result of a remote characteristic notification
 * @param       remote_addr     - Remote address
 * @param       element         - characteristic
 * @param       value           - buffer
 * @param       length          - buffer length
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_element_changed_callback)(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *element,
        uint8_t *value, uint16_t length);

/*******************************************************************************
 *
 * Callback reporting the RSSI for a remote device connection
 * @param       remote_addr     - Remote address
 * @param       rssi            - rssi
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_remote_rssi_read_callback)(BD_ADDR remote_addr, int32_t rssi,
        SERVICE_GATT_STATUS status);

/*******************************************************************************
 *
 * PHY read callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_phy_read_callback)(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
        SERVICE_BLE_PHY_TYPE rx_phy);

/*******************************************************************************
 *
 * PHY changed callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_phy_update_callback)(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
        SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_GATT_STATUS status);

/*******************************************************************************
 *
 * mtu changed callback
 * @param       remote_addr     - Remote address
 * @param       mtu             - The new (ATT_MTU-3) value negotiated.
 *                                The default mtu value is (23-3).
 *                                3 is the ATT PDU header size.
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*gatt_client_mtu_changed_callback)(BD_ADDR remote_addr, uint32_t mtu,
        SERVICE_GATT_STATUS status);

/* * Stack Gatt Server callback structure */
typedef struct {
    /* * set to sizeof(GATT_SERVER_CALLBACKS_S) */
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
} GATT_SERVER_CALLBACKS_S;

/* * Stack Gatt Client callback structure */
typedef struct {
    /* * set to sizeof(GATT_CLIENT_CALLBACKS_S) */
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
} GATT_CLIENT_CALLBACKS_S;

/* * GATT related API */

/*******************************************************************************
 *
 * Initialize Bluetooth stack
 * @param    void
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_init(void);

/*******************************************************************************
 *
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 *
 ******************************************************************************/
void service_adapter_gatt_cleanup(void);

/*******************************************************************************
 *
 * Open gatt server
 * @param    cbs    - callback function struct
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_open(GATT_SERVER_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Close gatt server
 * @param    void
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_close(void);

/*******************************************************************************
 *
 * Add a service to the list of services to be hosted.
 * The first element shall be the service (primary or secondary).
 * @param    elements  - service,[include],[characteristic, descriptor],
 *                       [characteristic, descriptor]. The elements buffer
 *                       shall keep valid until it is returned to the application
 *                       using gatt_server_elements_removed_cb.
 * @param    size      - elements size
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_add_elements(SERVICE_GATT_ELEMENT_S *elements,
        uint16_t size);

/*******************************************************************************
 *
 * Remove specified service(s) from the list of services hosted.
 * If a service included by other service(s) is removed, all the host services
 * shall also be removed manually.
 * @param    ids  - Each item in the ids specifies the ID of the service to remove.
 *                  If ids is NULL, all services are removed.
 * @param    size - Number of IDs in the ids.
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_remove_elements(uint32_t *ids, uint16_t size);

/*******************************************************************************
 *
 * Initiate a connection to a Bluetooth GATT capable device
 * @param    remote_addr    - Remote address
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_connect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Disconnects an established connection,
 * or cancels a connection attempt currently in progress.
 * @param    remote_addr    - Remote address
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_cancel_connection(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Read the current transmitter PHY and receiver PHY of the connection.
 * @param    remote_addr    - Remote BT address
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_read_phy(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Set the current transmitter PHY and receiver PHY of the connection.
 * @param    remote_addr    - Remote address
 * @param    tx_phy         - transmitter PHY
 * @param    rx_phy         - receiver PHY
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_set_phy(BD_ADDR remote_addr,
        SERVICE_BLE_PHY_TYPE tx_phy,
        SERVICE_BLE_PHY_TYPE rx_phy);

/*******************************************************************************
 *
 * Send a response to a read or write request to a remote device.
 * @param    remote_addr    - Remote address
 * @param    response       - response
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_send_response(BD_ADDR remote_addr,
        SERVICE_GATT_RESPONSE_S *response);

/*******************************************************************************
 *
 * Send an indication that a local characteristic has been updated.
 * @param    remote_addr    - Remote address
 * @param    element        - characteristic. It shall be within the elements
                              parameter of service_adapter_gatt_server_add_elements.
 * @param    value          - buffer
 * @param    length         - buffer length
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_send_indication(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *element,
        uint8_t *value, uint16_t length);

/*******************************************************************************
 *
 * Send an indication that a local characteristic has been updated.
 * @param    remote_addr    - Remote address
 * @param    element        - characteristic. It shall be within the elements
                              parameter of service_adapter_gatt_server_add_elements.
 * @param    value          - buffer
 * @param    length         - buffer length. It shall be at most (ATT_MTU-3).
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_server_send_notification(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *element,
        uint8_t *value, uint16_t length);

/*******************************************************************************
 *
 * Connect to remote device
 * @param    remote_addr    - Remote address
 * @param    cbs            - callback function struct
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_connect(BD_ADDR remote_addr,
        GATT_CLIENT_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Disconnects an established connection,
 * or cancels a connection attempt currently in progress.
 * @param    remote_addr    - Remote address
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_disconnect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Discovers services offered by a remote device
 * as well as their characteristics and descriptors.
 * @param    remote_addr    - Remote address
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_discover_services(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Discovers service offered by a remote device
 * as well as its characteristics and descriptors.
 * @param    remote_addr    - Remote address
 * @param    uuid           - Service uuid
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_discover_service(BD_ADDR remote_addr,
        BT_UUID_T uuid);

/*******************************************************************************
 *
 * Reads the value for characteristic or descriptor from the associated remote device.
 * @param    remote_addr    - Remote address
 * @param    element        - characteristic or descriptor
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_read_element(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *element);

/*******************************************************************************
 *
 * Writes the value for characteristic or descriptor from the associated remote device.
 * @param    remote_addr    - Remote address
 * @param    element        - characteristic or descriptor
 * @param    value          - buffer
 * @param    length         - buffer length
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_write_element(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *element,
        uint8_t *value, uint16_t length);

/*******************************************************************************
 *
 * Enable or disable notifications/indications for a given characteristic
 * @param    remote_addr    - Remote address
 * @param    element        - characteristic
 * @param    enable         - Set to true to enable notifications/indications
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_register_notifications(BD_ADDR remote_addr,
        SERVICE_GATT_ELEMENT_S *element, bool enable);

/*******************************************************************************
 *
 * Read the RSSI for a connected remote device.
 * @param    remote_addr    - Remote address
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_read_remote_rssi(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Read the current transmitter PHY and receiver PHY of the connection.
 * @param    remote_addr    - Remote BT address
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_read_phy(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Set the current transmitter PHY and receiver PHY of the connection.
 * @param    remote_addr    - Remote address
 * @param    tx_phy         - transmitter PHY
 * @param    rx_phy         - receiver PHY
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_set_phy(BD_ADDR remote_addr,
        SERVICE_BLE_PHY_TYPE tx_phy,
        SERVICE_BLE_PHY_TYPE rx_phy);

/*******************************************************************************
 *
 * Set mtu size
 * @param    remote_addr - Remote address
 * @param    mtu         - The new mtu value to set. It shall be (ATT_MTU-3).
 *                         The ATT_MTU is the actual MTU value to set using ATT
 *                         Exchange MTU Request. 3 is the ATT PDU header size.
 *                         The minimum mtu value is (23-3). 23 is the default
 *                         ATT_MTU.
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_set_mtu(BD_ADDR remote_addr, uint32_t mtu);

/*******************************************************************************
 *
 * update connection parameter
 * @param    remote_addr                            - Remote address
 * @param    min_interval                           - min interval
 * @param    max_interval                           - max interval
 * @param    latency                                - latency
 * @param    timeout                                - timeout
 * @param    min_connection_event_length            -
 * @param    max_connection_event_length            -
 * @return   GATT Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_GATT_STATUS service_adapter_gatt_client_update_connection_parameter(BD_ADDR remote_addr,
        uint32_t min_interval,
        uint32_t max_interval, uint32_t latency,
        uint32_t timeout,
        uint32_t min_connection_event_length,
        uint32_t max_connection_event_length);

#endif  // STACK_ADAPTER_GATT_H
