/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_avrcp.h
Abstract:
    Bluetooth Stack Avrcp adapter interfaces
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_AVRCP_H
#define STACK_ADAPTER_AVRCP_H

#include "stack_adapter_common.h"

#ifdef __cplusplus
extern "C" {
#endif
/*******************************************************************************
 *
 * Avrcp connection state changed callback,
 * invoked in response to service_adapter_avrcp_connect()
 * and service_adapter_avrcp_disconnect()
 * @param       remote_addr - Remote BT address
 * @param       state   - avrcp connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*avrcp_connection_state_changed_callback)(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state);

/*******************************************************************************
 *
 * A panel response has been received from the remote target device callback
 * @param       remote_addr     - Remote BT address
 * @param       response    - panel response
 * @param       op          - panel operation
 * @param       state       - key state
 * @return      void
 *
 ******************************************************************************/
typedef void (*avrcp_received_panel_rsp_callback)(BD_ADDR remote_addr,
        SERVICE_AVRCP_RESPONSE response,
        SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state);

/*******************************************************************************
 *
 * The notification of a registered event has been received callback
 * @param       remote_addr - Remote BT address
 * @param       event   - notification event
 * @param       value   -
 * @return      void
 *
 ******************************************************************************/
typedef void (*avrcp_received_notification_callback)(BD_ADDR remote_addr,
        SERVICE_AVRCP_NOTIFICATION_EVENT event,
        void *value);

/*******************************************************************************
 *
 * Remote capabilities has been received callback
 * @param       remote_addr         - Remote BT address
 * @param       capabilities    -
 * @return      void
 *
 ******************************************************************************/
typedef void (*avrcp_received_remote_capabilities_callback)(BD_ADDR remote_addr,
        void *capabilities);

/*******************************************************************************
 *
 * Response of get element attributes has been received callback
 * @param       remote_addr  - Remote BT address
 * @param       attrs_count  - Number of attributes
 * @param       types        - IDs of the attributes
 * @param       chr_sets     - Character Set ID of the attributes
 * @param       attrs        - Value of the attributes
 * @return      void
 *
 ******************************************************************************/
typedef void (*avrcp_received_element_attributes_callback)(BD_ADDR remote_addr, uint8_t attrs_count,
        SERVICE_AVRCP_MEDIA_ATTR_TYPE *types,
        uint16_t *chr_sets, char *attrs[]);

/* * Stack Avrcp callback structure */
typedef struct {
    /* * set to sizeof(avrcp_callbacks) */
    uint8_t size;
    avrcp_connection_state_changed_callback avrcp_connection_state_changed_cb;
    avrcp_received_panel_rsp_callback avrcp_received_panel_rsp_cb;
    avrcp_received_notification_callback avrcp_received_notification_cb;
    avrcp_received_remote_capabilities_callback avrcp_received_remote_capabilities_cb;
    avrcp_received_element_attributes_callback avrcp_received_element_attributes_cb;
} AVRCP_CALLBACKS_S;

/* * Avrcp related API */

/*******************************************************************************
 *
 * Initialize Bluetooth stack
 * Register callback functions for Stack's AVRCP module
 * @param    cbs - callback fucntion struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_init(AVRCP_CALLBACKS_S *cbs);

/*******************************************************************************
 *
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 *
 ******************************************************************************/
void service_adapter_avrcp_cleanup(void);

/*******************************************************************************
 *
 * Register callback functions for Stack's AVRCP module
 * @param    cbs - callback fucntion struct
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
// BT_STATUS service_adapter_avrcp_register_callback(void);

/*******************************************************************************
 *
 * Create a service level connection
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_connect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Releases a service level connection
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_disconnect(BD_ADDR remote_addr);

/*******************************************************************************
 *
 * Send avrcp panel operation
 * @param    remote_addr    - Remote BT address
 * @param    op         - panel operation
 * @param    state      - key state
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_send_panel_operation(BD_ADDR remote_addr,
        SERVICE_AVRCP_PANEL_OPERATION op,
        SERVICE_AVRCP_PANEL_STATE state);

/*******************************************************************************
 *
 * Register for notification of events on the target device
 * @param    remote_addr    - Remote BT address
 * @param    event      - The event for which the CT requires notifications
 * @param    interval   - Only works in PLAY_POS_CHANGED event
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_register_notification(BD_ADDR remote_addr,
        SERVICE_AVRCP_NOTIFICATION_EVENT event,
        uint32_t interval);

/*******************************************************************************
 *
 * Send avrcp command
 * @param    remote_addr    - Remote BT address
 * @param    volume     - The absolute volume of the Sink (0 - 0x7F)
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_set_absolute_volume(BD_ADDR remote_addr, uint8_t volume);

/*******************************************************************************
 *
 * Get the capabilities of the target device.
 * @param    remote_addr       - Remote BT address
 * @param    cap_id            - CapabilityID
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_get_remote_capabilities(BD_ADDR remote_addr,
        uint8_t cap_id);

/*******************************************************************************
 *
 * Get the attributes of the current element of the target device.
 * @param    remote_addr    - Remote BT address
 * @param    attrs_count    - Count of the attribute IDs. If 0 is set,
 *                          attr_ids is ignored and gets all attributes.
 * @param    types          - ID of attributes to get information. If NULL
 *                          is set, attr_ids_count is ignored and gets all.
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
SERVICE_BT_STATUS service_adapter_avrcp_get_element_attributes(BD_ADDR remote_addr,
        uint8_t attrs_count, SERVICE_AVRCP_MEDIA_ATTR_TYPE *types);
#ifdef __cplusplus
}
#endif
#endif
