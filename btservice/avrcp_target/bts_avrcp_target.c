/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_avrcp.c
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "btm_manager.h"
#include "stack_adapter_common.h"
#include "stack_adapter_service_base.h"
#include "stack_adapter_avrcp_target.h"
#include "stack_adapter_common.h"
#define LOG_TAG "bts_avrcp_target"
#include "log.h"

void bts_avrcp_target_connection_state_changed_callback(BD_ADDR remote_addr,
        SERVICE_PROFILE_CONNECTION_STATE state)
{
    BT_LOGD("%s", __func__);  
    //TODO add uinput initlize
}
void bts_avrcp_target_received_register_notification_request_callback(BD_ADDR remote_addr,
        SERVICE_AVRCP_NOTIFICATION_EVENT event, uint32_t interval)
{
    BT_LOGD("%s", __func__);
}
void bts_avrcp_target_received_get_play_status_request_callback(BD_ADDR remote_addr)
{
    BT_LOGD("%s", __func__);
}
void bts_avrcp_target_received_get_element_attr_request_callback(BD_ADDR remote_addr)
{
    BT_LOGD("%s", __func__);
}
void bts_avrcp_target_received_set_volume_callback(BD_ADDR remote_addr, uint8_t volume)
{
    BT_LOGD("%s, volume is %d", __func__, volume);
    //TODO add set volume to system through uinput
}
void bts_avrcp_target_received_panel_operation_callback(BD_ADDR remote_addr,
        SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state)
{
    BT_LOGD("%s", __func__);
    //TODO add operation to system through uinput
    switch (op)
    {
    case AVRCP_OPERATION_SELECT:
      /* code */
      BT_LOGD("AVRCP_OPERATION_SELECT, state is %d  ", state);
      break;
     case AVRCP_OPERATION_VOLUME_UP:
      BT_LOGD("AVRCP_OPERATION_VOLUME_UP, state is %d  ", state);
      break;
    case AVRCP_OPERATION_VOLUME_DOWN:
      BT_LOGD("AVRCP_OPERATION_VOLUME_DOWN, state is %d  ", state);
      break;
    case AVRCP_OPERATION_MUTE:
      BT_LOGD("AVRCP_OPERATION_MUTE, state is %d  ", state);
      break;
    case AVRCP_OPERATION_PLAY:
      BT_LOGD("AVRCP_OPERATION_PLAY, state is %d  ", state);
      break;
    case AVRCP_OPERATION_STOP:
      BT_LOGD("AVRCP_OPERATION_STOP, state is %d  ", state);
      break;
    case AVRCP_OPERATION_PAUSE:
      BT_LOGD("AVRCP_OPERATION_PAUSE, state is %d  ", state);
      break;
    case AVRCP_OPERATION_RECORD:
      BT_LOGD("AVRCP_OPERATION_RECORD, state is %d  ", state);
      break;
    case AVRCP_OPERATION_REWIND:
      BT_LOGD("AVRCP_OPERATION_REWIND, state is %d  ", state);
      break;
    case AVRCP_OPERATION_FAST_FORWARD:
      BT_LOGD("AVRCP_OPERATION_FAST_FORWARD, state is %d  ", state);
      break;
    case AVRCP_OPERATION_EJECT:
      BT_LOGD("AVRCP_OPERATION_EJECT, state is %d  ", state);
      break;
    case AVRCP_OPERATION_FORWARD:
      BT_LOGD("AVRCP_OPERATION_FORWARD, state is %d  ", state);
      break;
    case AVRCP_OPERATION_BACKWARD:
      BT_LOGD("AVRCP_OPERATION_BACKWARD, state is %d  ", state);    
      break;
    default:
      break;
    }

}

AVRCP_TARGET_CALLBACKS_S g_avrcp_target = {
   .size = sizeof(g_avrcp_target),
  .avrcp_target_connection_state_changed_cb = bts_avrcp_target_connection_state_changed_callback,
  .avrcp_target_received_get_element_attr_request_cb = bts_avrcp_target_received_get_element_attr_request_callback,
  .avrcp_target_received_get_play_status_request_cb = bts_avrcp_target_received_get_play_status_request_callback,
  .avrcp_target_received_panel_operation_cb = bts_avrcp_target_received_panel_operation_callback,
  .avrcp_target_received_register_notification_request_cb = bts_avrcp_target_received_register_notification_request_callback,
  .avrcp_target_received_set_volume_cb = bts_avrcp_target_received_set_volume_callback
};

bt_result_code avrcp_target_init(void)
{
  bt_status ret = service_adapter_avrcp_target_init(&g_avrcp_target);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%d", __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}
