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
#include "bt_lea_mcp.h"
#include "bt_profile.h"
#include "lea_mcp_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static lea_mcp_interface_t* get_profile_service(void)
{
    return (lea_mcp_interface_t*)service_manager_get_profile(PROFILE_LEAUDIO_MCP);
}

void* bt_lea_mcp_register_callbacks(bt_instance_t* ins, const lea_mcp_callbacks_t* callbacks)
{
    lea_mcp_interface_t* profile = get_profile_service();

    return profile->set_callbacks(NULL, (lea_mcp_callbacks_t*)callbacks);
}

bool bt_lea_mcp_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    lea_mcp_interface_t* profile = get_profile_service();

    return profile->reset_callbacks(NULL, cookie);
}

bt_status_t bt_lea_mcp_read_info(bt_instance_t* ins, bt_address_t* addr, uint8_t opcode)
{
    lea_mcp_interface_t* profile = get_profile_service();

    return profile->read_remote_mcs_info(addr, opcode);
}

bt_status_t bt_lea_mcp_media_control_request(bt_instance_t* ins, bt_address_t* addr, uint32_t opcode, int32_t n)
{
    lea_mcp_interface_t* profile = get_profile_service();

    return profile->media_control_request(addr, opcode, n);
}

bt_status_t bt_lea_mcp_search_control_request(bt_instance_t* ins, bt_address_t* addr, uint8_t number, uint32_t type, uint8_t* parameter)
{
    lea_mcp_interface_t* profile = get_profile_service();

    return profile->search_control_request(addr, number, type, parameter);
}
