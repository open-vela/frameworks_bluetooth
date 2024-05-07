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
#ifndef __BT_LEA_VMICP_H__
#define __BT_LEA_VMICP_H__

#include <stddef.h>

#include "bt_device.h"

/**
 * @brief LE Audio vmicp test callback
 *
 * @param cookie - callback cookie.
 */
typedef void (*lea_vmicp_volume_state_callback)(void* cookie, bt_address_t* addr, int volume, int mute);
typedef void (*lea_vmicp_volume_flags_callback)(void* cookie, bt_address_t* addr, int flags);
typedef void (*lea_vmicp_mic_state_callback)(void* cookie, bt_address_t* addr, int mute);

typedef struct
{
    size_t size;
    lea_vmicp_volume_state_callback volume_state_cb;
    lea_vmicp_volume_flags_callback volume_flags_cb;
    lea_vmicp_mic_state_callback mic_state_cb;
} lea_vmicp_callbacks_t;

/**
 * @brief Register LE Audio vmicp callback functions
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - LE Audio vmicp callback functions.
 * @return void* - callback cookie.
 */
void* bt_lea_vmicp_register_callbacks(bt_instance_t* ins, const lea_vmicp_callbacks_t* callbacks);

/**
 * @brief Unregister LE Audio vmicp callback functions
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callback cookie.
 * @return true - on unregister success.
 * @return false - on callback cookie not found.
 */
bool bt_lea_vmicp_unregister_callbacks(bt_instance_t* ins, void* cookie);

/**
 * @brief Read volume state. Value is returned by
 * #vmicp_volume_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vmicp_get_volume_state(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read volume flags. Value is returned by
 * #vmicp_volume_flags_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vmicp_get_volume_flags(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Change volume. Value is returned by
 * #vmicp_volume_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @param dir - up or down the volume
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vmicp_change_volume(bt_instance_t* ins, bt_address_t* addr, int dir);

/**
 * @brief Chnage and unmute volume. Value is returned by
 * #vmicp_volume_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @param dir - up or down the volume
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vmicp_change_unmute_volume(bt_instance_t* ins, bt_address_t* addr, int dir);

/**
 * @brief Set absolute volume. Value is returned by
 * #vmicp_volume_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @param vol - value of volume
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vmicp_set_volume(bt_instance_t* ins, bt_address_t* addr, int vol);

/**
 * @brief Set volume mute. Value is returned by
 * #vmicp_volume_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @param mute - mute or unmute
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vmicp_set_volume_mute(bt_instance_t* ins, bt_address_t* addr, int mute);

/**
 * @brief Read mic state. Value is returned by
 * #vmicp_mic_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vmicp_get_mic_state(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set Mic mute. Value is returned by
 * #vmicp_volume_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @param mute - mute or unmute
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vmicp_set_mic_mute(bt_instance_t* ins, bt_address_t* addr, int mute);

#endif