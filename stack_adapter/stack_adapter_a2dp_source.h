/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_a2dp_source.h
Abstract:
    Bluetooth Stack A2DP Source adapter interfaces
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_A2DP_SOURCE_H
#define STACK_ADAPTER_A2DP_SOURCE_H

#include "stack_adapter_common.h"

#ifdef __cplusplus
extern "C" {
#endif
/*******************************************************************************
 *
 * A2DP Source connection state changed callback,
 * invoked in response to service_adapter_a2dp_sink_connect()
 * and service_adapter_a2dp_sink_disconnect()
 * @param       remote_addr - Remote BT address
 * @param       state   - a2dp source connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_source_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state);
/*******************************************************************************
 *
 * A2DP Source stream state changed callback
 * @param       remote_addr - Remote BT address
 * @param       state   - a2dp stream state
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_source_stream_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_A2DP_STREAM_STATE state);

/*******************************************************************************
 *
 * A2DP Source stream config changed callback
 * @param       remote_addr - Remote BT address
 * @param       config  - stream config info
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_source_stream_config_changed_callback)(BD_ADDR remote_addr,
        SERVICE_A2DP_STREAM_CONFIG_S *config);

/*******************************************************************************
 *
 * A2DP Source stream config changed callback
 * @param       remote_addr - Remote BT address
 * @param       stream_chnl_mtu  - MTU of AVDTP stream channel
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_source_stream_channel_mtu_callback)(BD_ADDR remote_addr,
        uint16_t stream_chnl_mtu);

/* * Stack A2DP Source callback structure */
typedef struct {
    /* * set to sizeof(a2dp_source_callbacks) */
    uint8_t size;
    a2dp_source_connection_state_changed_callback a2dp_source_connection_state_changed_cb;
    a2dp_source_stream_state_changed_callback a2dp_source_stream_state_changed_cb;
    a2dp_source_stream_config_changed_callback a2dp_source_stream_config_changed_cb;
    a2dp_source_stream_channel_mtu_callback a2dp_source_stream_channel_mtu_cb;
} A2DP_SOURCE_CALLBACKS_S;

/* * A2DP Source related API */

/*******************************************************************************
 *
 * Initialize Bluetooth stack
 * Register callback functions for Stack's A2DP Source module
 * @param    max_connection     - max connection count
 * @param    cbs                - callback fucntion struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_source_init(uint8_t max_connection,
        A2DP_SOURCE_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 *
 ******************************************************************************/
void service_adapter_a2dp_source_cleanup(void);

/*******************************************************************************
 *
 * Register callback functions for Stack's A2DP Source module
 * @param    cbs - callback function struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_source_register_callback(void);

/*******************************************************************************
 *
 * Create a service level connection
 * @param    remote_addr - Remote BT address
 * @param    codec_type - codec type
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_source_connect(BD_ADDR remote_addr,
        SERVICE_AVDTP_CODEC_TYPE codec_type);

/*******************************************************************************
 *
 * Releases a service level connection
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_source_disconnect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Set stream codec
 * @param    remote_addr - Remote BT address
 * @param    config  - stream config info ?types?
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_source_set_stream_config(BD_ADDR remote_addr, a2dp_stream_config_t *config);
// // raw data

/*******************************************************************************
 *
 * Send open stream request to remote device
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_source_open_stream(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Send close stream request to remote device
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_source_close_stream(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Send start stream request to remote device
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_source_start_stream(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Send suspend stream request to remote device
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_source_suspend_stream(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Send data to remote device
 * @param    remote_addr         - Remote BT address
 * @param    package        - package
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_source_send_data(BD_ADDR remote_addr,
        SERVICE_A2DP_SOURCE_PACKET_S *packet);

#ifdef __cplusplus
}
#endif
#endif
