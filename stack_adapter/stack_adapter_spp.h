/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_spp.h
Abstract:
    Bluetooth Stack SPP adapter interfaces.
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_SPP_H
#define STACK_ADAPTER_SPP_H

#include "stack_adapter_common.h"

/*******************************************************************************
 *
 * SPP connection state changed callback,
 * when a connection has been established with a remote device
 * or a connection has been terminated for a remote device
 * @param       remote_addr - Remote BT address
 * @param       conn_port   - at least High 10bits
 * @param       state   - spp connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*spp_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_SPP_PORT conn_port,
        SERVICE_PROFILE_CONNECTION_STATE state);

/*******************************************************************************
 *
 * The data has been sent out. At this time the tx buffer can be released.
 * @param       conn_port   - spp port
 * @param       buffer  - write data buffer
 * @param       length  - buffer length
 * @return      void
 *
 ******************************************************************************/
typedef void (*spp_data_sent_callback)(SERVICE_SPP_PORT conn_port, uint8_t *buffer, uint16_t length,
                                       uint16_t sent_length);

/*******************************************************************************
 *
 * The data has been sent out. At this time the tx buffer can be released.
 * Data received callback, memcpy the data buffer
 * @param       remote_addr - Remote BT address
 * @param       conn_port   - spp port
 * @param       length  - buffer length
 * @param       uuid    - SPP UUID
 * @return       void
 *
 ******************************************************************************/
typedef void (*spp_data_received_callback)(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port,
        uint8_t *buffer, uint16_t length);

/*******************************************************************************
 *
 * SPP Server connection state changed callback,
 * when a connection has been established with a remote device
 * or a connection has been terminated for a remote device
 * @param       remote_addr     - Remote BT address
 * @param       svr_port             - low 6bits
 * @return      void
 *
 ******************************************************************************/
typedef void (*spp_server_connection_req_received_callback)(BD_ADDR remote_addr,
        SERVICE_SPP_PORT svr_port);

/* * Stack SPP callback structure */
typedef struct {
    /* * set to sizeof(spp_callbacks) */
    uint8_t size;
    spp_connection_state_changed_callback spp_connection_state_changed_cb;
    spp_data_sent_callback spp_data_sent_cb;
    spp_data_received_callback spp_data_received_cb;
    spp_server_connection_req_received_callback spp_server_connection_req_received_cb;
} SPP_CALLBACKS_S;

/* * SPP related API */

/*******************************************************************************
 *
 * Initialize Bluetooth stack
 * @param    cbs        - callback function struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_init(SPP_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 *
 ******************************************************************************/
void service_adapter_spp_cleanup(void);

/*******************************************************************************
 *
 * Opens a serial device for reading and writing. Requires an
 * existing ACL connection with the remote device for client devices
 * @param    svr_port        - low 6its
 * @param    uuid               - optional
 * @param    max_device_count
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_server_open(SERVICE_SPP_PORT svr_port, BT_UUID_T uuid,
        uint8_t max_device_count);

/*******************************************************************************
 *
 * Close the serial device. Requires the device to have been opened
 * previously by service_adapter_spp_open()
 * @param   svr_port        - low 6bits
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_server_close(SERVICE_SPP_PORT svr_port);

/*******************************************************************************
 *
 * Opens a serial device for reading and writing. Requires an
 * existing ACL connection with the remote device for client devices
 * @param    remote_addr - Remote BT address
 * @param    conn_port           -  High 10bits (svr port optional)
 * @param    uuid                     - optional
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_client_open(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port,
        BT_UUID_T uuid);

/*******************************************************************************
 *
 * Close the serial device. Requires the device to have been opened
 * previously by service_adapter_spp_open()
 * @param    conn_port          - High 10bits
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_client_close(SERVICE_SPP_PORT conn_port);

/*******************************************************************************
 *
 * disconnect serial connection by port
 * @param    conn_port          - High 10bits
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_disconnect_by_port(SERVICE_SPP_PORT conn_port);

/*******************************************************************************
 *
 * disconnect serial connection by port
 * @param    remote_addr - remote addr
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_disconnect_by_remote_addr(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Write to the serial device. Requires the device to have been opened
 * previously by service_adapter_spp_open()
 * @param    conn_port          - High 10bits
 * @param    buffer  - write data buffer
 * @param    length  - buffer length
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_write(SERVICE_SPP_PORT conn_port, uint8_t *buffer,
        uint8_t length);

/*******************************************************************************
 *
 * Give more credits to peer. Requires the device to have been opened
 * previously by service_adapter_spp_open()
 * @param    con_port - High 10bits
 * @param    credits  - Additional credits to give.
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_add_credits(SERVICE_SPP_PORT con_port, uint8_t credits);

/*******************************************************************************
 *
 * Given back the buffer of data received after the data is handled by the
 * application.
 * @param    conn_port          - High 10bits
 * @param    buffer  - received data buffers.
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_data_received_rsp(SERVICE_SPP_PORT conn_port,
        uint8_t *buffer);

/*******************************************************************************
 *
 * previously by service_adapter_spp_open()
 * @param    remote_addr - Remote BT address
 * @param    conn_port                     - full SPP port
 * @param    accept  - accept or reject
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_spp_send_connection_rsp(BD_ADDR remote_addr,
        SERVICE_SPP_PORT conn_port, bool accept);

#endif
