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
#ifndef __SAL_HID_HOST_INTERFACE_H__
#define __SAL_HID_HOST_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_status.h"

bt_status_t bt_sal_hid_host_init(void);
void bt_sal_hid_host_cleanup(void);
bt_status_t bt_sal_hid_host_connect(bt_address_t* addr, bt_transport_t transport);
bt_status_t bt_sal_hid_host_disconnect(bt_address_t* addr);
bt_status_t bt_sal_hid_host_get_report(bt_address_t* addr, uint8_t report_id, uint8_t report_type);
bt_status_t bt_sal_hid_host_set_report(bt_address_t* addr, uint8_t report_id, uint8_t report_type, const uint8_t* data, uint16_t len);
bt_status_t bt_sal_hid_host_set_protocol(bt_address_t* addr, uint8_t protocol_mode);
bt_status_t bt_sal_hid_host_suspend(bt_address_t* addr);
bt_status_t bt_sal_hid_host_exit_suspend(bt_address_t* addr);
int bt_sal_hid_host_get_supported_intervals(bt_address_t* addr, uint16_t* intervals, uint8_t max_count);
bt_status_t bt_sal_hid_host_set_mode(bt_address_t* addr, uint8_t mode, uint8_t policy, uint16_t iso_interval);
bt_status_t bt_sal_hid_host_get_mode(bt_address_t* addr, uint8_t* mode);

#endif /* __SAL_HID_HOST_INTERFACE_H__ */
