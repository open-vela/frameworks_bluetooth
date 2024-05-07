/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef __LEA_VMICP_SERVICE_H__
#define __LEA_VMICP_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_lea_vmicp.h"
#include "sal_lea_vmicp_interface.h"

/****************************************************************************
 * sal callback
 ****************************************************************************/
void lea_vmicp_on_volume_state_changed(bt_address_t* addr, uint8_t volume, uint8_t mute);
void lea_vmicp_on_volume_flags_changed(bt_address_t* addr, uint8_t flags);
void lea_vmicp_on_mic_state_changed(bt_address_t* addr, uint8_t mute);

/****************************************************************************
 * lea vmicp interface
 ****************************************************************************/
typedef struct {
    size_t size;

    bt_status_t (*vol_get)(bt_address_t* remote_addr);
    bt_status_t (*flags_get)(bt_address_t* remote_addr);
    bt_status_t (*vol_change)(bt_address_t* remote_addr, int dir);
    bt_status_t (*vol_unmute_change)(bt_address_t* remote_addr, int dir);
    bt_status_t (*vol_set)(bt_address_t* remote_addr, int vol);
    bt_status_t (*mute_state_set)(bt_address_t* remote_addr, int state);

    bt_status_t (*mic_mute_get)(bt_address_t* remote_addr);
    bt_status_t (*mic_mute_set)(bt_address_t* remote_addr, int mute);

    void* (*register_callbacks)(void* handle, lea_vmicp_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** handle, void* cookie);
} lea_vmicp_interface_t;

/*
 * register profile to service manager
 */
void register_lea_vmicp_service(void);

#endif /* __LEA_VMICP_SERVICE_H__ */
