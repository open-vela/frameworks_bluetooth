/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_hfp.h
Abstract:
    Bluetooth Stack HFP adapter interfaces
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_HFP_H
#define STACK_ADAPTER_HFP_H

#include "stack_adapter_common.h"

/*******************************************************************************
 *
 * HFP connection state changed callback,
 * invoked in response to service_adapter_hfp_connect()
 * and service_adapter_hfp_disconnect()
 * @param       remote_addr - Remote BT address
 * @param       state   - hfp connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state);

/*******************************************************************************
 *
 * SCO connection state changed callback,
 * invoked in response to service_adapter_hfp_create_sco()
 * and service_adapter_hfp_disconnect_sco()
 * @param       remote_addr - Remote BT address
 * @param       state   - hfp connection state
 * @param       sco_connection_handle   - SCO connection handle
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_sco_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_HFP_SCO_STATE state, uint16_t sco_connection_handle);

/*******************************************************************************
 *
 * HPF Codec changed callback
 * @param       remote_addr - Remote BT address
 * @param       config  - hfp config
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_codec_changed_callback)(BD_ADDR remote_addr, SERVICE_HFP_CONFIG_S *config);

/*******************************************************************************
 *
 * HFP call setup state changed callback
 * @param       remote_addr - Remote BT address
 * @param       state   - call setup state
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_call_setup_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_HFP_CALL_SETUP_STATE state);

/*******************************************************************************
 *
 * HFP call active state changed callback
 * @param       remote_addr - Remote BT address
 * @param       state   - call active state
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_call_active_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_HFP_CALL_ACTIVE_STATE state);

/*******************************************************************************
 *
 * HFP call held state changed callback
 * @param       remote_addr - Remote BT address
 * @param       state   - call held state
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_call_held_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_HFP_CALL_HELD_STATE state);

/*******************************************************************************
 *
 * HFP volume changed callback
 * @param       remote_addr - Remote BT address
 * @param       type    - volume type
 * @param       volume  - volume value
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_volume_changed_callback)(BD_ADDR remote_addr, SERVICE_HFP_VOLUME_TYPE type,
        uint8_t volume);

/*******************************************************************************
 *
 * HFP ring state changed callback
 * @param       remote_addr         - Remote BT address
 * @param       active          - is active or not
 * @param       in_band_ring    - is inband ring or not
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ring_active_state_changed_callback)(BD_ADDR remote_addr, bool active,
        bool inband_ring);

/*******************************************************************************
 *
 * HFP voice recognition state changed callback
 * @param       remote_addr - Remote BT address
 * @param       enabled - is enabled or not
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_voice_recognition_enabled_changed_callback)(BD_ADDR remote_addr, bool enabled);

/*******************************************************************************
 *
 * HFP received vs at command callback
 * @param       remote_addr     - Remote BT address
 * @param       at_string   - at cmd
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_received_at_cmd_callback)(BD_ADDR remote_addr, char *response,
        uint16_t response_length);

/*******************************************************************************
 *
 * HFP received eSCO/SCO connection request callback
 * @param       bt_addr     - Remote BT address
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_received_sco_connection_req_callback)(BD_ADDR remote_addr);

/* * Stack HFP callback structure */
typedef struct {
    /* * set to sizeof(hfp_callbacks) */
    uint8_t size;
    hfp_connection_state_changed_callback hfp_connection_state_changed_cb;
    hfp_sco_connection_state_changed_callback hfp_sco_connection_state_changed_cb;
    hfp_codec_changed_callback hfp_codec_changed_cb;
    hfp_call_setup_state_changed_callback hfp_call_setup_state_changed_cb;
    hfp_call_active_state_changed_callback hfp_call_active_state_changed_cb;
    hfp_call_held_state_changed_callback hfp_call_held_state_changed_cb;
    hfp_volume_changed_callback hfp_volume_changed_cb;
    hfp_ring_active_state_changed_callback hfp_ring_active_state_changed_cb;
    hfp_voice_recognition_enabled_changed_callback hfp_voice_recognition_enabled_changed_cb;
    hfp_received_at_cmd_callback hfp_received_at_cmd_cb;
    hfp_received_sco_connection_req_callback hfp_received_sco_connection_req_cb;
} HFP_CALLBACKS_S;

/* * HFP related API */

/*******************************************************************************
 *
 * Initialize Bluetooth stack
 * @param    features           - HFP features
 * @param    max_connection     - max connection count
 * @param    cbs                - callback function struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_init(uint32_t features, uint8_t max_connection,
        HFP_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 *
 ******************************************************************************/
void service_adapter_hfp_cleanup(void);

/*******************************************************************************
 *
 * Register callback functions for Stack's HFP module
 * @param    cbs - callback function struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// SERVICE_BT_STATUS service_adapter_hfp_register_callback(HFP_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Create a service level connection
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_connect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Releases a service level connection
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_disconnect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Create a sco link
 * Only used in Audio connection transfer towards the HF
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_create_sco(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Releases a sco link
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_disconnect_sco(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Answer an incoming call
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_answer_call(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Reject an incoming call
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_reject_call(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Terminates an active call
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_hangup_call(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Set volume
 * @param       remote_addr - Remote BT address
 * @param       type    - volume type
 * @param       volume  - volume value
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_set_volume(BD_ADDR remote_addr, SERVICE_HFP_VOLUME_TYPE type,
        uint8_t volume);

/*******************************************************************************
 *
 * Enable voice recognition
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_enable_voice_recognition(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Disable voice recognition
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_disable_voice_recognition(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Disable voice recognition
 * @param    remote_addr - Remote BT address
 * @param    value   - battery value
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_send_battery_value(BD_ADDR remote_addr, uint8_t value);

/*******************************************************************************
 *
 * Send at command
 * When host get response, put it into at_cmd and send it back through cb
 * @param    remote_addr    - Remote BT address
 * @param    at_cmd     - AT command struct
 * @param    cb         - call this func after get response
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_send_at_cmd(BD_ADDR remote_addr, SERVICE_HFP_AT_CMD_S *at_cmd,
        hfp_received_at_cmd_callback cb);

#endif
