/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/avrcp_target.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "btm_manager.h"
#include "bts_service.h"
#include "stack_adapter_avrcp_target.h"
#include "stack_adapter_service_base.h"
#include "bts_avrcp_target.h"


/*******************************************************************************
 *
 * Avrcp target connection state changed callback
 ******************************************************************************/
void bts_avrcp_target_connection_state_changed_callback(BD_ADDR remote_addr,
                                        SERVICE_PROFILE_CONNECTION_STATE state)
{

}

/*******************************************************************************
 *
 * Received register notification request callback
 *
 ******************************************************************************/
void bts_avrcp_target_received_register_notification_request_callback(BD_ADDR remote_addr,
                                        SERVICE_AVRCP_NOTIFICATION_EVENT event,
                                        uint32_t interval)
{
    
}

/*******************************************************************************
 *
 * Received Get Play Status request from CT
 * @param    remote_addr    - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 *
 ******************************************************************************/
void bts_avrcp_target_received_get_play_status_request_callback(BD_ADDR remote_addr)
{

}

/*******************************************************************************
 *
 * Received get Get Element Attr request from CT
 ******************************************************************************/
void bts_avrcp_target_received_get_element_attr_request_callback(BD_ADDR remote_addr)
{

}

/*******************************************************************************
 *
 * Received set Absolute volume from CT
 *
 ******************************************************************************/
void bts_avrcp_target_received_set_volume_callback(BD_ADDR remote_addr, uint8_t volume)
{

}

/*******************************************************************************
 *
 * Received panel operation from CT
 *
 ******************************************************************************/
void bts_avrcp_target_received_panel_operation_callback(BD_ADDR remote_addr, SERVICE_AVRCP_PANEL_OPERATION op,
                                                               SERVICE_AVRCP_PANEL_STATE state)
{

}

AVRCP_TARGET_CALLBACKS_S avrcp_target_callbakc = 
{
    6,
    bts_avrcp_target_connection_state_changed_callback,
    bts_avrcp_target_received_register_notification_request_callback,
    bts_avrcp_target_received_get_play_status_request_callback,
    bts_avrcp_target_received_get_element_attr_request_callback,
    bts_avrcp_target_received_set_volume_callback,
    bts_avrcp_target_received_panel_operation_callback
};

void process_loop_in_avrcp(excute_service_context_t * context, size_t data_size)
{
    //int common_size = 0;
    //char * a2dp_buff = NULL;

    // if (NULL == context)
    // {
    //     return;
    // }

    // if(context->command_id > AVRCP_COMMAND_MAX_ID)
    // {
    //     switch (context->command_id)
    //     {
    //     case AVRCP_RESPONSE_CONNECTION_STATE_CHANGED:
    //         a2dp_buff = package_common_buffer_to_manager(INIT_SERVICE_DONE_RESPONSE, NULL, 0, &common_size);    
    //         send_pb_command_buffer_to_manager(BT_PROFILE_AV_RC_TARGET_ID, a2dp_buff, common_size); 

    //         break;
    //     default:
    //         break;
    //     }
    // }

    free(context);
}

void avrcp_process_command_from_manager(void * profile_buff, size_t profile_size)
{
    bool status = false;
    if (false == status) 
    {
        printf("Decoding failed \n");
    }
    // switch (profile_data.command_id)
    // {
    // case AVRCP_COMMAND_NOTIFY_TRACK_CHANGED:
    //   /* code */    
    //   break;
    // default:
    //   break;
    // }
}

bt_result_code bts_avrcp_init(void)
{
    bt_result_code result = BT_RESULT_FAILED;

    SERVICE_BT_STATUS service_status = SERVICE_BT_STATUS_FAIL;
    printf(" bts_avrcp_init coming \n");
    register_process_func_to_service(BT_PROFILE_AV_RC_TARGET_ID, avrcp_process_command_from_manager);
    service_status = service_adapter_avrcp_target_init(&avrcp_target_callbakc);

    if (SERVICE_BT_STATUS_SUCCESS == service_status)
    {
        result = BT_RESULT_SUCCESS;
    }
    return result;
}

bt_result_code avrcp_deinit(void)
{
    bt_result_code result = BT_RESULT_FAILED;

    SERVICE_BT_STATUS service_status = SERVICE_BT_STATUS_FAIL;
    printf(" bts_avrcp_deinit coming \n");
    //unregister_process_func_to_service(BT_PROFILE_AV_RC_TARGET_ID);
    service_adapter_avrcp_target_cleanup();
    result = BT_RESULT_SUCCESS;
    return result;
}