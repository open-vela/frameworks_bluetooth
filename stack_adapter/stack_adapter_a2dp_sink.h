/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_a2dp_sink.h
Abstract:
    Bluetooth Stack A2DP Sink adapter interfaces
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_A2DP_SINK_H
#define STACK_ADAPTER_A2DP_SINK_H

#include "stack_adapter_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *
 * A2DP Sink connection state changed callback,
 * invoked in response to service_adapter_a2dp_sink_connect()
 * and service_adapter_a2dp_sink_disconnect()
 * @param       bt_addr - Remote BT address
 * @param       state   - a2dp sink connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_sink_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state);

/*******************************************************************************
 *
 * A2DP Sink stream state changed callback
 * @param       bt_addr - Remote BT address
 * @param       state   - a2dp stream state
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_sink_stream_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_A2DP_STREAM_STATE state);

/*******************************************************************************
 *
 * A2DP Sink stream config changed callback
 * @param       bt_addr - Remote BT address
 * @param       config  - stream config info
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_sink_stream_config_changed_callback)(BD_ADDR remote_addr,
        SERVICE_A2DP_STREAM_CONFIG_S *config);

/*******************************************************************************
 *
 * A2DP Sink pcm data received callback
 * When data->length changed,
 * the first package data->buffer == NULL and data->length == NEW LENGTH
 * Service will provide new data->buffer in callback
 * @param       bt_addr - Remote BT address
 * @param       data    - stream data
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_sink_pcm_data_received_callback)(BD_ADDR remote_addr,
        SERVICE_A2DP_SINK_DATA_S *data);

/*******************************************************************************
 *
 * A2DP Sink package received callback
 * stack malloc buffer, and then after callback free buffer
 * @param       bt_addr - Remote BT address
 * @param       data    - stream data p_buffer raw data, len raw data len
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_sink_packet_received_callback)(BD_ADDR remote_addr,
        SERVICE_A2DP_SINK_DATA_S *data);

/*******************************************************************************
 *
 * A2DP Sink received stream request callback
 * @param       bt_addr     - Remote BT address
 * @param       request     - request type
 * @return      void
 *
 ******************************************************************************/
typedef void (*a2dp_sink_stream_req_received_callback)(BD_ADDR remote_addr,
        SERVICE_A2DP_STREAM_REQUEST request);

/* * Stack A2DP Sink callback structure */
typedef struct {
    /* * set to sizeof(a2dp_sink_callbacks) */
    uint8_t size;
    a2dp_sink_connection_state_changed_callback a2dp_sink_connection_state_changed_cb;
    a2dp_sink_stream_state_changed_callback a2dp_sink_stream_state_changed_cb;
    a2dp_sink_stream_config_changed_callback a2dp_sink_stream_config_changed_cb;
    a2dp_sink_pcm_data_received_callback a2dp_sink_pcm_data_received_cb;
    a2dp_sink_packet_received_callback a2dp_sink_packet_received_cb;
    a2dp_sink_stream_req_received_callback a2dp_sink_stream_req_received_cb;
} A2DP_SINK_CALLBACKS_S;

/* * A2DP Sink related API */

/*******************************************************************************
 *
 * Initialize Bluetooth stack
 * Register callback functions for Stack's A2DP Sink module
 * @param    max_connection     - max connection count
 * @param    cbs                - callback fucntion struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_sink_init(uint8_t max_connection,
        A2DP_SINK_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 *
 ******************************************************************************/
void service_adapter_a2dp_sink_cleanup(void);

/*******************************************************************************
 *
 * Register callback functions for Stack's A2DP Sink module
 * @param    cbs - callback fucntion struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_sink_register_callback(void);

/*******************************************************************************
 *
 * Create a service level connection
 * @param    bt_addr    - Remote BT address
 * @param    codec_type - codec type
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_sink_connect(BD_ADDR remote_addr,
        SERVICE_AVDTP_CODEC_TYPE codec_type);

/*******************************************************************************
 *
 * Releases a service level connection
 * @param    bt_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_sink_disconnect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Accept open stream request from remote device//stack做完了
 * @param    bt_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_sink_accept_open_stream_req(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Accept close stream request from remote device//stack做完了
 * @param    bt_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_sink_accept_close_stream_req(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Accept suspend stream request from remote device
 * @param    bt_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_sink_accept_suspend_stream_req(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Accept start stream request from remote device
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_a2dp_sink_accept_start_stream_req(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Accept config stream request from remote device//stack做完了
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_sink_accept_config_stream_req(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Accept reconfig stream request from remote device//stack做完了
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_a2dp_sink_accept_reconfig_stream_req(BD_ADDR remote_addr);

#ifdef __cplusplus
}
#endif
#endif  // STACK_ADAPTER_A2DP_SINK_H
