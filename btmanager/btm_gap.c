/****************************************************************************
 * frameworks/blluetooth/src/btmanager/gap.c
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
#include "btm_manager.h"
#include "bts_gap.h"

bt_result_code package_gap_buffer_to_service(int command_id, void* command_buffer, int command_size)
{
    bt_result_code result = BT_RESULT_FAILED;
    char * gap_buff = NULL;
    int gap_size = 0;
    bt_profile data = bt_profile_init_default;

    data.command_id = command_id;
    if (NULL != command_buffer) 
    {
        //nano_pb_bytes(data, buffer, command_buffer, command_size);
    }

    gap_buff = nano_pb_encode(bt_profile_fields, &data, &gap_size);
#ifdef CONFIG_BLUETOOTH_LOCAL_THREAD
    result = send_pb_buffer_to_service(NULL, BT_PROFILE_GAP_ID, gap_buff, gap_size);
#endif
    return result;
}


bt_result_code bt_set_local_address(void *p ,bt_address addr)
{
    size_t size = 0;
    char *buffer = NULL;
    bt_result_code result = BT_RESULT_FAILED;
    bt_profile data = bt_profile_init_default;

    if (NULL == addr) 
    {
        result = BT_RESULT_PARAMETER_ERROR;
        goto Exit;
    }
    
    command_set_address set_address = command_set_address_init_default;
    set_address.address = malloc(BT_ADDR_LENGTH);
    memcpy(set_address.address, addr, BT_ADDR_LENGTH);
    buffer = nano_pb_encode(command_set_address_fields, &set_address, &size);
    if (NULL == buffer)
    {
        result = BT_RESULT_ALLOC_BUFFER_FAILED;
        goto Exit;
    }
    result = package_common_buffer_to_service(SET_ADDRESS, buffer, size);

Exit:
    return result;
}
