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
#ifndef __SAL_HFP_HF_INTERFACE_H__
#define __SAL_HFP_HF_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "hfp_hf_service.h"

bt_status_t bt_sal_hfp_hf_init(uint32_t hf_features, uint8_t max_connection);
void bt_sal_hfp_hf_cleanup(void);
bt_status_t bt_sal_hfp_hf_connect(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_disconnect(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_connect_audio(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_disconnect_audio(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_answer_call(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_reject_call(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_hold_call(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_hangup_call(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_dial_number(bt_address_t* addr, const char* number);
bt_status_t bt_sal_hfp_hf_dial_memory(bt_address_t* addr, uint32_t memory);
bt_status_t bt_sal_hfp_hf_call_control(bt_address_t* addr, hfp_call_control_t chld, uint32_t index);
bt_status_t bt_sal_hfp_hf_get_current_calls(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);
bt_status_t bt_sal_hfp_hf_start_voice_recognition(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_stop_voice_recognition(bt_address_t* addr);
bt_status_t bt_sal_hfp_hf_send_battery_level(bt_address_t* addr, uint8_t value);
bt_status_t bt_sal_hfp_hf_send_at_cmd(bt_address_t* addr, const char* cmd, uint16_t len);
bt_status_t bt_sal_hfp_hf_send_dtmf(bt_address_t* addr, char dtmf);
bt_status_t bt_sal_hfp_hf_get_subscriber_number(bt_address_t* addr);

#endif /* __SAL_HFP_HF_INTERFACE_H__ */