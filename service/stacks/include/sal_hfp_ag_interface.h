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
#ifndef __SAL_HFP_AG_INTERFACE_H__
#define __SAL_HFP_AG_INTERFACE_H__

#include "bt_addr.h"
#include "bt_status.h"
#include "hfp_ag_service.h"
#include <stdint.h>

bt_status_t bt_sal_hfp_ag_init(uint32_t features, uint8_t max_connection);
void bt_sal_hfp_ag_cleanup(void);
bt_status_t bt_sal_hfp_ag_connect(bt_address_t* addr);
bt_status_t bt_sal_hfp_ag_disconnect(bt_address_t* addr);
bt_status_t bt_sal_hfp_ag_connect_audio(bt_address_t* addr);
bt_status_t bt_sal_hfp_ag_disconnect_audio(bt_address_t* addr);
bt_status_t bt_sal_hfp_ag_start_voice_recognition(bt_address_t* addr);
bt_status_t bt_sal_hfp_ag_stop_voice_recognition(bt_address_t* addr);
bt_status_t bt_sal_hfp_ag_phone_state_change(bt_address_t* addr, uint8_t num_active,
    uint8_t num_held, hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
    const char* number, const char* name);
bt_status_t bt_sal_hfp_ag_cind_response(bt_address_t* addr, hfp_ag_cind_resopnse_t* response);
bt_status_t bt_sal_hfp_ag_clcc_response(bt_address_t* addr, uint32_t index,
    hfp_call_direction_t dir, hfp_ag_call_state_t call, hfp_call_mode_t mode,
    hfp_call_mpty_type_t mpty, hfp_call_addrtype_t type, const char* number);
bt_status_t bt_sal_hfp_ag_dial_response(bt_address_t* addr, hfp_atcmd_result_t result);
bt_status_t bt_sal_hfp_ag_cops_response(bt_address_t* addr, const char* operator_name, uint16_t length);
bt_status_t bt_sal_hfp_ag_notify_device_status_changed(bt_address_t* addr, hfp_network_state_t network,
    hfp_roaming_state_t roam, uint8_t signal, uint8_t battery);
bt_status_t bt_sal_hfp_ag_set_inband_ring_enable(bt_address_t* addr, bool enable);
bt_status_t bt_sal_hfp_ag_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);
bt_status_t bt_sal_hfp_ag_send_at_cmd(bt_address_t* addr, const char* atcmd, uint16_t length);
bt_status_t bt_sal_hfp_ag_manufacture_id_response(bt_address_t* addr,
    const char* manufacturer_id, uint16_t length);
bt_status_t bt_sal_hfp_ag_model_id_response(bt_address_t* addr, const char* model_id, uint16_t length);
bt_status_t bt_sal_hfp_ag_error_response(bt_address_t* addr, hfp_atcmd_result_t result);
bt_status_t bt_sal_hfp_ag_call_sync(bt_address_t* addr, hfp_call_direction_t dir,
    hfp_ag_call_state_t call, hfp_call_mode_t mode, hfp_call_mpty_type_t mpty,
    hfp_call_addrtype_t type, const char* number);
#endif /* __SAL_HFP_AG_INTERFACE_H__ */