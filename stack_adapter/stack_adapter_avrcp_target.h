/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_avrcp_target.h
Abstract:
    Bluetooth Stack Avrcp Target adapter interfaces
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_AVRCP_TARGET_H
#define STACK_ADAPTER_AVRCP_TARGET_H

#include "stack_adapter_common.h"

/*******************************************************************************
 *
 * Avrcp target connection state changed callback
 * @param       remote_addr - Remote BT address
 * @param       state   - avrcp connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*avrcp_target_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state);

/*******************************************************************************
 *
 * Received register notification request callback
 * @param    remote_addr    - Remote BT address
 * @param    event      - The event for which the CT requires notifications
 * @param    interval   - Only works in PLAY_POS_CHANGED event
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
typedef void (*avrcp_target_received_register_notification_request_callback)(BD_ADDR remote_addr,
        SERVICE_AVRCP_NOTIFICATION_EVENT event,
        uint32_t interval);

/*******************************************************************************
 *
 * Received Get Play Status request from CT
 * @param    remote_addr    - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
typedef void (*avrcp_target_received_get_play_status_request_callback)(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Received get Get Element Attr request from CT
 * @param    remote_addr    - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
typedef void (*avrcp_target_received_get_element_attr_request_callback)(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Received set Absolute volume from CT
 * @param    remote_addr    - Remote BT address
 * @param    volume     - volume value
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
typedef void (*avrcp_target_received_set_volume_callback)(BD_ADDR remote_addr, uint8_t volume);

/*******************************************************************************
 *
 * Received panel operation from CT
 * @param    remote_addr    - Remote BT address
 * @param    op         - panel operation
 * @param    state      - key state
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
typedef void (*avrcp_target_received_panel_operation_callback)(BD_ADDR remote_addr,
        SERVICE_AVRCP_PANEL_OPERATION op,
        SERVICE_AVRCP_PANEL_STATE state);

/* * Stack Avrcp Target callback structure */
typedef struct {
    /* * set to sizeof(avrcp_target_callbacks) */
    uint8_t size;
    avrcp_target_connection_state_changed_callback avrcp_target_connection_state_changed_cb;
    avrcp_target_received_register_notification_request_callback
    avrcp_target_received_register_notification_request_cb;
    avrcp_target_received_get_play_status_request_callback
    avrcp_target_received_get_play_status_request_cb;
    avrcp_target_received_get_element_attr_request_callback
    avrcp_target_received_get_element_attr_request_cb;
    avrcp_target_received_set_volume_callback avrcp_target_received_set_volume_cb;
    avrcp_target_received_panel_operation_callback avrcp_target_received_panel_operation_cb;
} AVRCP_TARGET_CALLBACKS_S;

/* * Avrcp Target related API */

/*******************************************************************************
 *
 * Initialize Bluetooth stack
 * Register callback functions for Stack's AVRCP Target module
 * @param    cbs - callback fucntion struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_target_init(AVRCP_TARGET_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 *
 ******************************************************************************/
void service_adapter_avrcp_target_cleanup(void);

/*******************************************************************************
 *
 * Send response of Get Play Status request to CT
 * @param    remote_addr    - Remote BT address
 * @param    media_status   - current play status
 * @param    song_length    - song length in ms
 * @param    position       - current position in ms
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_target_get_play_status_response(BD_ADDR remote_addr,
        SERVICE_AVRCP_MEDIA_STATUS media_status,
        uint32_t song_length, uint32_t position);

/*******************************************************************************
 *
 * Send response of Get Element Attr request to CT
 * @param    remote_addr    - Remote BT address
 * @param    attrs_count    - attrs count
 * @param    types          - attrs id
 * @param    attrs          - attrs, each attr end with "\0"
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_target_get_element_attr_response(BD_ADDR remote_addr,
        uint8_t attrs_count,
        SERVICE_AVRCP_MEDIA_ATTR_TYPE *types,
        char *attrs[]);

/*******************************************************************************
 *
 * Notify play status changed
 * @param    remote_addr    - Remote BT address
 * @param    status         - current play status
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_target_notify_play_status_changed(BD_ADDR remote_addr,
        SERVICE_AVRCP_MEDIA_STATUS status);

/*******************************************************************************
 *
 * Notify track changed
 * @param    remote_addr    - Remote BT address
 * @param    selected       - TRUE track is selected
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_target_notify_track_changed(BD_ADDR remote_addr,
        bool selected);

/*******************************************************************************
 *
 * Notify play position changed
 * @param    remote_addr    - Remote BT address
 * @param    position       - current position in ms
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_target_notify_play_position_changed(BD_ADDR remote_addr,
        uint32_t position);

/*******************************************************************************
 *
 * Notify volume changed
 * @param    remote_addr    - Remote BT address
 * @param    volume         - volume value
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_target_notify_volume_changed(BD_ADDR remote_addr,
        uint8_t volume);

#endif  // STACK_ADAPTER_AVRCP_TARGET_H
