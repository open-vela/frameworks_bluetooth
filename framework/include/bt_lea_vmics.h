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
#ifndef __BT_LEA_VMICS_H__
#define __BT_LEA_VMICS_H__

#include <stddef.h>

#include "bt_device.h"

/**
 * @brief LE Audio vmics test callback
 *
 * @param cookie - callback cookie.
 */
typedef void (*lea_vmics_test_callback)(void* cookie, int unused);

typedef struct
{
    size_t size;
    lea_vmics_test_callback test_cb;
} lea_vmics_callbacks_t;

/**
 * @brief Register LE Audio vmics callback functions
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - LE Audio vmics callback functions.
 * @return void* - callback cookie.
 */
void* bt_lea_vmics_register_callbacks(bt_instance_t* ins, const lea_vmics_callbacks_t* callbacks);

/**
 * @brief Unregister LE Audio vmics callback functions
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callback cookie.
 * @return true - on unregister success.
 * @return false - on callback cookie not found.
 */
bool bt_lea_vmics_unregister_callbacks(bt_instance_t* ins, void* cookie);

/**
 * @brief Set Volume. Users use this function to tell the client the current value.
 * @param ins - Bluetooth client instance.
 * @param vol - Current Volume.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vcs_volume_set(bt_instance_t* ins, int vol);

/**
 * @brief Set Mute state. Users use this function to tell the client the current Mute state.
 * @param ins - Bluetooth client instance.
 * @param mute - Current Mute state(0:unmute, 1:mute).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vcs_mute_set(bt_instance_t* ins, int mute);

/**
 * @brief Set Volume flag. Users use this function to tell the client the current Volume flag.
 * # this is a optional function.
 * @param ins - Bluetooth client instance.
 * @param flags - Current Volume flag(0:reset, 1:setted).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_vcs_volume_flags_set(bt_instance_t* ins, int flags);

/**
 * @brief Set Mic state. Users use this function to tell the client the current Mic state.
 * @param ins - Bluetooth client instance.
 * @param mute - Current Mic state(0:unmute, 1:mute, 2:disable).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_mics_mute_set(bt_instance_t* ins, int mute);

#endif