/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_hfp_ag.h
Abstract:
    Bluetooth Stack HFP AG functions
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_HFP_AG_H
#define STACK_ADAPTER_HFP_AG_H

#include "stack_adapter_common.h"

/*******************************************************************************
 *
 * HFP AG connection state changed callback
 * @param       remote_addr - Remote BT address
 * @param       state   - hfp connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state);

/*******************************************************************************
 *
 * SCO connection state changed callback
 * @param       remote_addr - Remote BT address
 * @param       state   - hfp connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_sco_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_HFP_SCO_STATE state);

/*******************************************************************************
 *
 * HPF AG Codec changed callback
 * @param       remote_addr - Remote BT address
 * @param       config  - hfp config
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_codec_changed_callback)(BD_ADDR remote_addr, SERVICE_HFP_CONFIG_S *config);

/*******************************************************************************
 *
 * HFP AG volume changed callback
 * @param       remote_addr - Remote BT address
 * @param       volume  - volume value
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_volume_changed_callback)(BD_ADDR remote_addr, SERVICE_HFP_VOLUME_TYPE type,
        uint8_t volume);

/*******************************************************************************
 *
 * HFP AG received CIND request callback
 * @param       remote_addr - Remote BT address
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_cind_request_callback)(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * HFP AG received CLCC request callback
 * @param       remote_addr - Remote BT address
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_clcc_request_callback)(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * HFP AG received COPS request callback
 * @param       remote_addr - Remote BT address
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_cops_request_callback)(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * HFP AG voice recognition state changed callback
 * @param       remote_addr - Remote BT address
 * @param       enabled - is enabled or not
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_voice_recognition_enabled_changed_callback)(BD_ADDR remote_addr,
        bool enabled);

/*******************************************************************************
 *
 * HFP AG voice recognition state changed callback
 * @param       remote_addr - Remote BT address
 * @param       value   - battery level
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_remote_battery_level_callback)(BD_ADDR remote_addr, uint8_t value);

/*******************************************************************************
 *
 * HFP AG received answer call
 * @param       remote_addr - Remote BT address
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_answer_call_callback)(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * HFP AG received reject call
 * @param       remote_addr - Remote BT address
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_reject_call_callback)(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * HFP AG received hangup call
 * @param       remote_addr - Remote BT address
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_hangup_call_callback)(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * HFP AG received vs at command callback
 * @param       remote_addr - Remote BT address
 * @param       at_string   - at cmd
 * @param       at_length   - at cmd length
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_at_cmd_callback)(BD_ADDR remote_addr, char *at_string,
        uint16_t at_length);

/*******************************************************************************
 *
 * HFP AG received eSCO/SCO connection request callback
 * @param       bt_addr     - Remote BT address
 * @return      void
 *
 ******************************************************************************/
typedef void (*hfp_ag_received_sco_connection_req_callback)(BD_ADDR remote_addr);

/* * Stack HFP AG callback structure */
typedef struct {
    /* * set to sizeof(hfp_ag_callbacks) */
    uint8_t size;
    hfp_ag_connection_state_changed_callback hfp_ag_connection_state_changed_cb;
    hfp_ag_sco_connection_state_changed_callback hfp_ag_sco_connection_state_changed_cb;
    hfp_ag_codec_changed_callback hfp_ag_codec_changed_cb;
    hfp_ag_volume_changed_callback hfp_ag_volume_changed_cb;
    hfp_ag_received_cind_request_callback hfp_ag_received_cind_request_cb;
    hfp_ag_received_clcc_request_callback hfp_ag_received_clcc_request_cb;
    hfp_ag_received_cops_request_callback hfp_ag_received_cops_request_cb;
    hfp_ag_voice_recognition_enabled_changed_callback hfp_ag_voice_recognition_enabled_changed_cb;
    hfp_ag_received_remote_battery_level_callback hfp_ag_received_remote_battery_level_cb;
    hfp_ag_received_answer_call_callback hfp_ag_received_answer_call_cb;
    hfp_ag_received_reject_call_callback hfp_ag_received_reject_call_cb;
    hfp_ag_received_hangup_call_callback hfp_ag_received_hangup_call_cb;
    hfp_ag_received_at_cmd_callback hfp_ag_received_at_cmd_cb;
    hfp_ag_received_sco_connection_req_callback hfp_ag_received_sco_connection_req_cb;
} HFP_AG_CALLBACKS_S;

/* * HFP AG related API */

/*******************************************************************************
 *
 * Initialize Bluetooth stack
 * @param    features           - HFP AG features
 * @param    max_connection     - max connection count
 * @param    cbs                - callback function struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_init(uint32_t features, uint8_t max_connection,
        HFP_AG_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 *
 ******************************************************************************/
void service_adapter_hfp_ag_cleanup(void);

/*******************************************************************************
 *
 * Create a service level connection
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_connect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Releases a service level connection
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_disconnect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Create a sco link
 * Only used in Audio connection transfer towards the HF
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_create_sco(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Releases a sco link
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_disconnect_sco(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Enable voice recognition
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_enable_voice_recognition(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Disable voice recognition
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_disable_voice_recognition(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * send phone state change
 * @param    remote_addr        - Remote BT address
 * @param    num_active     - active call count
 * @param    num_held       - held call count
 * @param    call_state     - call state
 * @param    number         - remote number struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_phone_state_change(BD_ADDR remote_addr, uint8_t num_active,
        uint8_t num_held,
        SERVICE_HFP_AG_CALL_STATE call_state,
        SERVICE_HFP_AG_PHONE_NUMBER_S *number);

/*******************************************************************************
 *
 * send CIND response
 * @param    remote_addr        - Remote BT address
 * @param    response       - CIND response struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_cind_response(BD_ADDR remote_addr,
        SERVICE_HFP_AG_CIND_RESPONSE_S response);

/*******************************************************************************
 *
 * send CLCC response
 * @param    remote_addr        - Remote BT address
 * @param    response       - CLCC response struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_clcc_response(BD_ADDR remote_addr,
        SERVICE_HFP_AG_CLCC_RESPONSE_S *response);

/*******************************************************************************
 *
 * send COPS response
 * @param    remote_addr            - Remote BT address
 * @param    operator_name      - operator name
 * @param    length             - operator name length
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_cops_response(BD_ADDR remote_addr, char *operator_name,
        uint16_t length);

/*******************************************************************************
 *
 * send deice status
 * @param    remote_addr        - Remote BT address
 * @param    status         - service, signal, roam, battery
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_notify_device_status_changed(BD_ADDR remote_addr,
        SERVICE_HFP_AG_DEVICE_STATUS_S status);

/*******************************************************************************
 *
 * send +BSIR
 * @param    remote_addr        - Remote BT address
 * @param    enable         - TRUE, enable inband ring
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_set_inband_ring_enable(BD_ADDR remote_addr, bool enable);

/*******************************************************************************
 *
 * Set volume
 * When host get response, put it into at_cmd and send it back through cb
 * @param    remote_addr    - Remote BT address
 * @param    type       - volume type
 * @param    volume     - value
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_set_volume(BD_ADDR remote_addr,
        SERVICE_HFP_VOLUME_TYPE type, uint8_t volume);

/*******************************************************************************
 *
 * Send at command
 * When host get response, put it into at_cmd and send it back through cb
 * @param    remote_addr    - Remote BT address
 * @param    at_cmd     - AT command struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_hfp_ag_send_at_cmd(BD_ADDR remote_addr,
        SERVICE_HFP_AG_AT_CMD_S *at_cmd);

#endif  // STACK_ADAPTER_HFP_AG_H
