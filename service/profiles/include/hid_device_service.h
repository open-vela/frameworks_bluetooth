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
#ifndef __HID_DEVICE_SERVICE_H__
#define __HID_DEVICE_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_hid_device.h"

typedef struct hid_device_interface {
    size_t size;
    void* (*register_callbacks)(void* remote, const hid_device_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** remote, void* cookie);
    bt_status_t (*register_app)(hid_device_sdp_settings_t* sdp, bool le_hid);
    bt_status_t (*unregister_app)(void);
    bt_status_t (*connect)(bt_address_t* addr);
    bt_status_t (*disconnect)(bt_address_t* addr);
    bt_status_t (*send_report)(bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size);
    bt_status_t (*response_report)(bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size);
    bt_status_t (*report_error)(bt_address_t* addr, hid_status_error_t error);
    bt_status_t (*virtual_unplug)(bt_address_t* addr);
} hid_device_interface_t;

void hid_device_on_app_state_changed(hid_app_state_t state);
void hid_device_on_connection_state_changed(bt_address_t* addr, bool le_hid, profile_connection_state_t state);
void hid_device_on_get_report(bt_address_t* addr, uint8_t rpt_type, uint8_t rpt_id, uint16_t buffer_size);
void hid_device_on_set_report(bt_address_t* addr, uint8_t rpt_type, uint16_t rpt_size, uint8_t* rpt_data);
void hid_device_on_receive_report(bt_address_t* addr, uint8_t rpt_type, uint16_t rpt_size, uint8_t* rpt_data);
void hid_device_on_virtual_cable_unplug(bt_address_t* addr);

/*
 * register profile to service manager
 */
void register_hid_device_service(void);

#endif /* __HID_DEVICE_SERVICE_H__ */