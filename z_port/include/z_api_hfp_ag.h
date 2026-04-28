/***********************************************************************
 *
 * Copyright 2026 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#ifndef __Z_API_HFP_AG_H__
#define __Z_API_HFP_AG_H__

#include <stdint.h>

/* Initialize HFP AG z_api layer (register callbacks with framework) */
int z_bt_hfp_ag_init(void);

/* SLC connection management */
int z_bt_hfp_ag_slc_connect(const uint8_t* addr);
int z_bt_hfp_ag_slc_disconnect(const uint8_t* addr);

/* Audio (SCO) connection management */
int z_bt_hfp_ag_connect_audio(const uint8_t* addr);
int z_bt_hfp_ag_disconnect_audio(const uint8_t* addr);

/* Virtual call (SCO via virtual call) */
int z_bt_hfp_ag_start_virtual_call(const uint8_t* addr);
int z_bt_hfp_ag_stop_virtual_call(const uint8_t* addr);

/* Phone state change notification */
int z_bt_hfp_ag_phone_state_change(const uint8_t* addr,
    uint8_t num_active, uint8_t num_held,
    uint8_t call_state, uint8_t addr_type,
    const char* number, const char* name);

/* Volume control */
int z_bt_hfp_ag_volume_control(const uint8_t* addr, uint8_t type,
    uint8_t volume);

/* Voice recognition */
int z_bt_hfp_ag_start_voice_recognition(const uint8_t* addr);
int z_bt_hfp_ag_stop_voice_recognition(const uint8_t* addr);

/* Device status notification */
int z_bt_hfp_ag_device_status(const uint8_t* addr,
    uint8_t network, uint8_t roam, uint8_t signal, uint8_t battery);

/* AT command */
int z_bt_hfp_ag_send_at_cmd(const uint8_t* addr, const char* cmd);

/* CLCC response */
int z_bt_hfp_ag_clcc_response(const uint8_t* addr, uint32_t index,
    uint8_t dir, uint8_t call_state, uint8_t mode, uint8_t mpty,
    uint8_t addr_type, const char* number);

/* CIND response */
int z_bt_hfp_ag_cind_response(const uint8_t* addr,
    uint8_t network, uint8_t call, uint8_t callsetup,
    uint8_t callheld, uint8_t signal, uint8_t roam, uint8_t battery);

/* Dial response */
int z_bt_hfp_ag_dial_response(uint8_t result);

#endif /* __Z_API_HFP_AG_H__ */
