/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#include "bt_hid_host.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "hid_host_service.h"
#include "service_manager.h"
#include "utils/log.h"

static hid_host_interface_t* get_profile_service(void)
{
    return (hid_host_interface_t*)service_manager_get_profile(PROFILE_HID_HOST);
}

void* BTSYMBOLS(bt_hid_host_register_callbacks)(bt_instance_t* ins, const hid_host_callbacks_t* callbacks)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->register_callbacks(ins, callbacks);
}

bool BTSYMBOLS(bt_hid_host_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks((void**)&ins, cookie);
}

bt_status_t BTSYMBOLS(bt_hid_host_connect)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->connect(addr, transport);
}

bt_status_t BTSYMBOLS(bt_hid_host_disconnect)(bt_instance_t* ins, bt_address_t* addr)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->disconnect(addr);
}

bt_status_t BTSYMBOLS(bt_hid_host_get_report)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t report_id, uint8_t report_type)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->get_report(addr, report_id, report_type);
}

bt_status_t BTSYMBOLS(bt_hid_host_set_report)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t report_id, uint8_t report_type,
    const uint8_t* data, uint16_t len)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->set_report(addr, report_id, report_type, data, len);
}

bt_status_t BTSYMBOLS(bt_hid_host_set_protocol)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t protocol_mode)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->set_protocol(addr, protocol_mode);
}

bt_status_t BTSYMBOLS(bt_hid_host_suspend)(bt_instance_t* ins, bt_address_t* addr)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->suspend(addr);
}

bt_status_t BTSYMBOLS(bt_hid_host_exit_suspend)(bt_instance_t* ins, bt_address_t* addr)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->exit_suspend(addr);
}

bt_status_t BTSYMBOLS(bt_hid_host_set_mode)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t mode, uint8_t level)
{
    hid_host_interface_t* profile = get_profile_service();

    return profile->set_mode(addr, mode, level);
}
