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
#include "sal_hid_host_interface.h"

#ifdef CONFIG_BLUETOOTH_HID_HOST
#include "sal_hogp_host_interface.h"
#endif

bt_status_t bt_sal_hid_host_init(void)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_init();
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_hid_host_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    bt_sal_hogp_host_cleanup();
#endif
}

bt_status_t bt_sal_hid_host_connect(bt_address_t* addr, bt_transport_t transport)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    if (transport == BT_TRANSPORT_BREDR)
        return BT_STATUS_NOT_SUPPORTED;

    return bt_sal_hogp_host_connect(addr);
#else
    (void)addr;
    (void)transport;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_hid_host_disconnect(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_disconnect(addr);
#else
    (void)addr;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_hid_host_get_report(bt_address_t* addr, uint8_t report_id,
    uint8_t report_type)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_get_report(addr, report_id, report_type);
#else
    (void)addr;
    (void)report_id;
    (void)report_type;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_hid_host_set_report(bt_address_t* addr, uint8_t report_id,
    uint8_t report_type, const uint8_t* data, uint16_t len)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_set_report(addr, report_id, report_type, data, len);
#else
    (void)addr;
    (void)report_id;
    (void)report_type;
    (void)data;
    (void)len;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_hid_host_set_protocol(bt_address_t* addr, uint8_t protocol_mode)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_set_protocol(addr, protocol_mode);
#else
    (void)addr;
    (void)protocol_mode;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_hid_host_suspend(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_suspend(addr);
#else
    (void)addr;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_hid_host_exit_suspend(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_exit_suspend(addr);
#else
    (void)addr;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

int bt_sal_hid_host_get_supported_intervals(bt_address_t* addr,
    uint16_t* intervals, uint8_t max_count)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_get_supported_intervals(addr, intervals, max_count);
#else
    (void)addr;
    (void)intervals;
    (void)max_count;
    return 0;
#endif
}

bt_status_t bt_sal_hid_host_set_mode(bt_address_t* addr, uint8_t mode,
    uint8_t policy, uint16_t iso_interval)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_set_mode(addr, mode, policy, iso_interval);
#else
    (void)addr;
    (void)mode;
    (void)policy;
    (void)iso_interval;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_hid_host_get_mode(bt_address_t* addr, uint8_t* mode)
{
#ifdef CONFIG_BLUETOOTH_HID_HOST
    return bt_sal_hogp_host_get_mode(addr, mode);
#else
    (void)addr;
    (void)mode;
    return BT_STATUS_NOT_SUPPORTED;
#endif
}
