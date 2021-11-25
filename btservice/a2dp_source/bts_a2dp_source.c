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

#include <queue.h>
#include <stdio.h>
#include <stdlib.h>

#include "stack_adapter_a2dp_source.h"
#include "stack_adapter_service_base.h"

#include "btm_manager.h"
#include "bts_a2dp.h"
#include "bts_service.h"

#define A2DP_MAX_CONNECTION (2)

typedef struct {
    dq_entry_t entry;
    bt_address remote_addr;
    SERVICE_PROFILE_CONNECTION_STATE connection_state;
    SERVICE_A2DP_STREAM_CONFIG_S stream_config;
    SERVICE_A2DP_STREAM_STATE stream_state;
    SERVICE_AVDTP_CODEC_TYPE codec_type;
    uint16_t stream_chnl_mtu;
} a2dp_device_t;

typedef struct {
    bt_address remote_addr;
    SERVICE_PROFILE_CONNECTION_STATE state;
} a2dp_connection_state_changed_parameter_t;

dq_queue_t a2dp_device_list;

a2dp_device_t* find_a2dp_device_by_addr(bt_address remote_addr)
{
    int list_size = 0;
    a2dp_device_t* result = NULL;
    list_size = dq_count(&a2dp_device_list);
    if (0 == list_size) {
        return NULL;
    }
    a2dp_device_t* device = (a2dp_device_t*)dq_peek(&a2dp_device_list);

    for (int i = 0; i < list_size; i++) {
        if (NULL == device) {
            break;
        }

        if (memcmp(result->remote_addr, remote_addr, BT_ADDR_LENGTH) == 0) {
            result = device;
            break;
        } else {
            device = (a2dp_device_t*)dq_next(&(device->entry));
        }
    }

    return result;
}

void process_loop_in_a2dp(void* data, size_t data_size)
{
    //int a2dp_size = 0;
    //char * a2dp_buff = NULL;
    //char * command_buff = NULL;
    //int command_size = 0;
    if (NULL == data) {
        return;
    }

    // if(context->command_id > A2DP_COMMAND_MAX_ID)
    // {
    //     switch (context->command_id)
    //     {
    //     case A2DP_RESPONSE_CONNECTION_STATE_CHANGED:
    //         printf("process_loop_in_a2dp A2DP_RESPONSE_CONNECTION_STATE_CHANGED \n");
    //         a2dp_connection_state_changed_parameter_t* parameter = context->data;
    //         if (NULL == parameter)
    //         {
    //             return;
    //         }
    //         //find device in device list

    //         //change state for device

    //         response_a2dp_state_changed response = response_a2dp_state_changed_init_default;

    //         memcpy(response.address, parameter->remote_addr, BT_ADDR_LENGTH);

    //         response.state = parameter->state;

    //         command_buff = nano_pb_encode(response_a2dp_state_changed_fields, &response, &command_size);

    //         a2dp_buff = package_common_buffer_to_manager(INIT_SERVICE_DONE_RESPONSE, NULL, 0, &a2dp_size);
    //         send_pb_command_buffer_to_manager(BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID, a2dp_buff, a2dp_size);

    //         break;
    //     default:
    //         break;
    //     }
    // }

    free(data);
}

void bts_a2dp_connect(void* data)
{
    if (NULL == data) {
        goto Exit;
    }

    //service_adapter_a2dp_source_connect(a2dp_connect_data->remote_addr, a2dp_connect_data->);
Exit:
    printf("process_loop_in_a2dp exit \n");
}

/*******************************************************************************
 * A2DP connection state changed callback
 ******************************************************************************/
void bts_a2dp_source_connection_state_changed_callback(BD_ADDR remote_addr,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    excute_service_context_t* context = (excute_service_context_t*)malloc(sizeof(excute_service_context_t));
    a2dp_connection_state_changed_parameter_t* parameter = (a2dp_connection_state_changed_parameter_t*)malloc(sizeof(a2dp_connection_state_changed_parameter_t));
    context->loop_func = process_loop_in_a2dp;
    context->data = (void*)parameter;
    context->command_id = A2DP_RESPONSE_CONNECTION_STATE_CHANGED;
    //process_in_loop(context);
}
/*******************************************************************************
 * A2DP Source stream state changed callback
 ******************************************************************************/
void bts_a2dp_source_stream_state_changed_callback(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state)
{
}

/*******************************************************************************
 * A2DP Source stream config changed callback
 ******************************************************************************/
void bts_a2dp_source_stream_config_changed_callback(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S* config)
{
}
/*******************************************************************************
 * A2DP Source stream config changed callback
 ******************************************************************************/
void bts_a2dp_source_stream_channel_mtu_callback(BD_ADDR remote_addr, uint16_t stream_chnl_mtu)
{
}

A2DP_SOURCE_CALLBACKS_S a2dp_callback = {
    4,
    bts_a2dp_source_connection_state_changed_callback,
    bts_a2dp_source_stream_state_changed_callback,
    bts_a2dp_source_stream_config_changed_callback,
    bts_a2dp_source_stream_channel_mtu_callback
};

bt_result_code bts_a2dp_init(void)
{
    SERVICE_BT_STATUS service_status = SERVICE_BT_STATUS_FAIL;
    bt_result_code result = BT_RESULT_FAILED;
    printf(" bts_a2dp_init coming \n");

    service_status = service_adapter_a2dp_source_init(A2DP_MAX_CONNECTION, &a2dp_callback);
    if (SERVICE_BT_STATUS_SUCCESS == service_status) {
        result = BT_RESULT_SUCCESS;
    }
    return result;
}

bt_result_code bts_a2dp_deinit(void)
{
    SERVICE_BT_STATUS service_status = SERVICE_BT_STATUS_FAIL;
    bt_result_code result = BT_RESULT_FAILED;
    printf(" bts_a2dp_deinit coming \n");

    service_adapter_a2dp_source_cleanup();
    if (SERVICE_BT_STATUS_SUCCESS == service_status) {
        result = BT_RESULT_SUCCESS;
    }
    return result;
}