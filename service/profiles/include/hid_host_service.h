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
#ifndef __HID_HOST_SERVICE_H__
#define __HID_HOST_SERVICE_H__

#include "bt_device.h"
#include "bt_hid_host.h"

typedef struct hid_host_interface {
    size_t size;
    void* (*register_callbacks)(void* remote, const hid_host_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** remote, void* cookie);
    bt_status_t (*connect)(bt_address_t* addr, bt_transport_t transport);
    bt_status_t (*disconnect)(bt_address_t* addr);
    bt_status_t (*get_report)(bt_address_t* addr, uint8_t report_id, uint8_t report_type);
    bt_status_t (*set_report)(bt_address_t* addr, uint8_t report_id, uint8_t report_type, const uint8_t* data, uint16_t len);
    bt_status_t (*set_protocol)(bt_address_t* addr, uint8_t protocol_mode);
    bt_status_t (*suspend)(bt_address_t* addr);
    bt_status_t (*exit_suspend)(bt_address_t* addr);
    bt_status_t (*set_mode)(bt_address_t* addr, uint8_t mode, uint8_t level);
} hid_host_interface_t;

/* SAL callbacks */
void hid_host_on_connection_state_changed(bt_address_t* addr, bt_transport_t transport, profile_connection_state_t state);
void hid_host_on_report_map(bt_address_t* addr, uint8_t service_index, const uint8_t* data, uint16_t len);
void hid_host_on_input_report(bt_address_t* addr, uint8_t service_index, uint8_t report_id, const uint8_t* data, uint16_t len);
void hid_host_on_get_report_result(bt_address_t* addr, uint8_t report_id, uint8_t report_type, const uint8_t* data, uint16_t len);
void hid_host_on_pnp_id(bt_address_t* addr, uint8_t vid_src, uint16_t vid, uint16_t pid, uint16_t version);
void hid_host_on_battery_level(bt_address_t* addr, uint8_t bat_index, uint8_t level);
void hid_host_on_mode_changed(bt_address_t* addr, uint8_t mode, int status);

void register_hid_host_service(void);

#endif /* __HID_HOST_SERVICE_H__ */
