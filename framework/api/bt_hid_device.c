/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#define LOG_TAG "hid_device_api"

#include <stdint.h>

#include "bt_hid_device.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "hid_device_service.h"
#include "service_manager.h"
#include "utils/log.h"

static hid_device_interface_t* get_profile_service(void)
{
    return (hid_device_interface_t*)service_manager_get_profile(PROFILE_HID_DEV);
}

void* BTSYMBOLS(bt_hid_device_register_callbacks)(bt_instance_t* ins, const hid_device_callbacks_t* callbacks)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool BTSYMBOLS(bt_hid_device_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t BTSYMBOLS(bt_hid_device_register_app)(bt_instance_t* ins, hid_device_sdp_settings_t* sdp, bool le_hid)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->register_app(sdp, le_hid);
}

bt_status_t BTSYMBOLS(bt_hid_device_unregister_app)(bt_instance_t* ins)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->unregister_app();
}

bt_status_t BTSYMBOLS(bt_hid_device_connect)(bt_instance_t* ins, bt_address_t* addr)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->connect(addr);
}

bt_status_t BTSYMBOLS(bt_hid_device_disconnect)(bt_instance_t* ins, bt_address_t* addr)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->disconnect(addr);
}

bt_status_t BTSYMBOLS(bt_hid_device_send_report)(bt_instance_t* ins, bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->send_report(addr, rpt_id, rpt_data, rpt_size);
}

bt_status_t BTSYMBOLS(bt_hid_device_response_report)(bt_instance_t* ins, bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->response_report(addr, rpt_type, rpt_data, rpt_size);
}

bt_status_t BTSYMBOLS(bt_hid_device_report_error)(bt_instance_t* ins, bt_address_t* addr, hid_status_error_t error)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->report_error(addr, error);
}

bt_status_t BTSYMBOLS(bt_hid_device_virtual_unplug)(bt_instance_t* ins, bt_address_t* addr)
{
    hid_device_interface_t* profile = get_profile_service();

    return profile->virtual_unplug(addr);
}
