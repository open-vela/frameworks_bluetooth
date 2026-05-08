/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#include "sal_hid_device_interface.h"

bt_status_t bt_sal_hid_device_init(void)
{
    return BT_STATUS_NOT_SUPPORTED;
}

void bt_sal_hid_device_cleanup(void)
{
}

bt_status_t bt_sal_hid_device_register_app(hid_device_sdp_settings_t* sdp, bool le_hid)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_hid_device_unregister_app(void)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_hid_device_connect(bt_address_t* addr)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_hid_device_disconnect(bt_address_t* addr)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_hid_device_get_report_response(bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_hid_device_report_error(bt_address_t* addr, hid_status_error_t error)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_hid_device_send_report(bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_hid_device_virtual_unplug(bt_address_t* addr)
{
    return BT_STATUS_NOT_SUPPORTED;
}
