/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __SAL_AVRCP_CONTROL_INTERFACE_H__
#define __SAL_AVRCP_CONTROL_INTERFACE_H__

#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_TARGET)

#include "avrcp_msg.h"
#include "bt_avrcp.h"
#include "bt_device.h"

bt_status_t bt_sal_avrcp_control_init(void);
void bt_sal_avrcp_control_cleanup(void);
bt_status_t bt_sal_avrcp_control_send_pass_through_cmd(bt_controller_id_t id,
    bt_address_t* bd_addr, avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state);
bt_status_t bt_sal_avrcp_control_get_playback_state(bt_controller_id_t id, bt_address_t* bd_addr);
bt_status_t bt_sal_avrcp_control_volume_changed_notify(bt_controller_id_t id,
    bt_address_t* bd_addr, uint8_t volume);
bt_status_t bt_sal_avrcp_control_connect(bt_controller_id_t id, bt_address_t* bd_addr);
bt_status_t bt_sal_avrcp_control_disconnect(bt_controller_id_t id, bt_address_t* bd_addr);
bt_status_t bt_sal_avrcp_control_get_capabilities(bt_controller_id_t id, bt_address_t* bd_addr,
    uint8_t cap_id);
bt_status_t bt_sal_avrcp_control_register_notification(bt_controller_id_t id,
    bt_address_t* bd_addr, avrcp_notification_event_t event, uint32_t interval);
bt_status_t bt_sal_avrcp_control_get_element_attributes(bt_controller_id_t id,
    bt_address_t* bd_addr, uint8_t attrs_count, avrcp_media_attr_type_t* types);

void bt_sal_avrcp_control_event_callback(avrcp_msg_t* msg);

#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL || CONFIG_BLUETOOTH_AVRCP_TARGET */
#endif /* __SAL_AVRCP_CONTROL_INTERFACE_H__ */