/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#ifndef __BT_LEA_MCP_H__
#define __BT_LEA_MCP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include <stddef.h>

#define OBJ_ID_SIZE 6

typedef uint8_t lea_mcp_object_id[OBJ_ID_SIZE];

typedef void (*lea_mcp_test_callback)(void* cookie, bt_address_t* addr, uint8_t event);

typedef struct
{
    size_t size;
    lea_mcp_test_callback test_cb;
} lea_mcp_callbacks_t;

void* bt_lea_mcp_register_callbacks(bt_instance_t* ins, const lea_mcp_callbacks_t* callbacks);
bool bt_lea_mcp_unregister_callbacks(bt_instance_t* ins, void* cookie);
bt_status_t bt_lea_mcp_read_info(bt_instance_t* ins, bt_address_t* addr, uint8_t opcode);
bt_status_t bt_lea_mcp_media_control_request(bt_instance_t* ins, bt_address_t* addr,
    uint32_t opcode, int32_t n);
bt_status_t bt_lea_mcp_search_control_request(bt_instance_t* ins, bt_address_t* addr, uint8_t number,
    uint32_t type, uint8_t* parameter);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LEA_MCP_H__ */
