/****************************************************************************
 * frameworks/bluetooth/src/btmanager/a2dp_source.c
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

#include "btm_a2dp_source.h"

#define BT_A2DP_INTERFACE_NUM (11)

bt_a2dp_source_callbacks_t* a2dp_source_callback = NULL;


void a2dp_source_process_func(char * buff, size_t size)
{
    //bt_result_code result = BT_RESULT_FAILED;

    bool status = false;
    printf(" a2dp_source_process_func size is %d \n", size);

    if ((NULL == buff)  ||(0 == size))
    {
        goto Exit;
    }
    status = nano_pb_decode(a2dp_src_message_fields, &a2dp_source_message, buff, size);
    if (false == status)
    {
        goto Exit;
    }
    if (a2dp_source_message.message_type != A2DP_MSG_TYPE_RESPONSE)
    {
        return;
    }    
    switch (a2dp_source_message.a2dp_message.response.res_id)
    {
    //TODO ,case need to be merged
    case A2DP_RES_ID_CONNECTION_STATE:
      /* code */
    printf(" common_process_func INIT_SERVICE_DONE_RESPONSE \n");
      a2dp_src_res_connection_state_changed  state_changed = a2dp_src_res_connection_state_changed_init_default;
      state_changed = a2dp_source_message.a2dp_message.response.response.connection_state;
      break;

    default:
      break;
    }

Exit:
    printf(" common_process_func exit \n");
}

bt_result_code a2dp_source_init(bt_a2dp_source_callbacks_t* callbacks, bt_a2dp_stream_config_t stream_config)
{
    bt_result_code result = BT_RESULT_FAILED;
    a2dp_source_callback = callbacks;
    register_process_func_to_manager(BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID, a2dp_source_process_func);
    return result;    
}
bt_result_code a2dp_source_deinit(void *p)
{
    bt_result_code result = BT_RESULT_FAILED;
    a2dp_source_callback = NULL;
    return result;    
}

/** connect to remote device */
bt_result_code a2dp_source_connect(void *p, const bt_address address)
{
    bt_result_code result = BT_RESULT_FAILED;
    
#ifdef CONFIG_BLUETOOTH_LOCAL_THREAD
    result = send_pb_buffer_to_service(NULL, BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID, ENABLE_BT, NULL);
#endif

    return result;
}

/** dis-connect from remote device */
bt_result_code a2dp_source_disconnect(void *p, const bt_address address)
{
    bt_result_code result = BT_RESULT_FAILED;
    return result;
}

/** sets the connected device as active */
bt_result_code a2dp_source_set_active_device(void *p, const bt_address address)
{
    bt_result_code result = BT_RESULT_FAILED;
    return result;
}

bt_result_code a2dp_source_get_connected_device(void *p, const bt_address address)
{
    bt_result_code result = BT_RESULT_FAILED;
    return result;
}

/** start stream */
bt_result_code a2dp_source_start_stream(void *p, const bt_address bd_addr)
{
    bt_result_code result = BT_RESULT_FAILED;
    return result;
}

/** start stream */
bt_result_code a2dp_source_stop_stream(void *p, const bt_address bd_addr)
{
    bt_result_code result = BT_RESULT_FAILED;
    return result;
}

/** start stream */
bt_result_code a2dp_source_is_playing(void *p, const bt_address bd_addr)
{
    bt_result_code result = BT_RESULT_FAILED;
    return result;
}

/** configure the codecs settings preferences */
bt_result_code a2dp_source_config_codec(void *p, const bt_address address, bt_a2dp_stream_config_t streamConfig)
{
    bt_result_code result = BT_RESULT_FAILED;
    return result;
}

bt_result_code a2dp_source_send_data(void *p, const bt_address bd_addr, bt_a2dp_packet_t data)
{
    bt_result_code result = BT_RESULT_FAILED;
    return result;
}

const bt_a2dp_source_interface_t *a2dp_source_interface = 
{
    BT_A2DP_INTERFACE_NUM,
    a2dp_source_init,
    a2dp_source_deinit,
    a2dp_source_connect,
    a2dp_source_disconnect,
    a2dp_source_set_active_device,
    a2dp_source_set_active_device,
    a2dp_source_get_connected_device,
    a2dp_source_start_stream,
    a2dp_source_stop_stream,
    a2dp_source_is_playing,
    a2dp_source_config_codec,
    a2dp_source_send_data,
};
