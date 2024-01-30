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
#ifndef __SAL_AVRCP_CONTROL_INTERFACE_H__
#define __SAL_AVRCP_CONTROL_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "avrcp_control_service.h"
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL

#include "avrcp_msg.h"

bt_status_t bt_sal_avrcp_control_init(void);
void bt_sal_avrcp_control_cleanup(void);
bt_status_t bt_sal_avrcp_control_send_pass_through_cmd(bt_address_t *bd_addr,
    avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state);
bt_status_t bt_sal_avrcp_control_get_playback_state(bt_address_t *bd_addr);
bt_status_t bt_sal_avrcp_control_volume_changed_notify(bt_address_t *bd_addr, uint8_t volume);
bt_status_t bt_sal_avrcp_control_get_capabilities(bt_address_t *bd_addr, uint8_t cap_id);
bt_status_t bt_sal_avrcp_control_register_notification(bt_address_t *bd_addr,
                                                       avrcp_notification_event_t event,
                                                       uint32_t interval);
void bt_sal_avrcp_control_event_callback(avrcp_msg_t *msg);
#endif
#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_TARGET)
bt_status_t bt_sal_avrcp_control_connect(bt_address_t *bd_addr);
bt_status_t bt_sal_avrcp_control_disconnect(bt_address_t *bd_addr);
#endif
#endif /* __SAL_AVRCP_CONTROL_INTERFACE_H__ */
