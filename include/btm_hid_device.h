/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#ifndef _SDK_INC_BLUETOOTH_HID_DEVICE_H
#define _SDK_INC_BLUETOOTH_HID_DEVICE_H

#include <stddef.h>

#include "btm_manager.h"

typedef void (*btm_hidd_device_state_changed_callback)(void* handle, HID_APP_STATE registered);
typedef void (*btm_hidd_connection_state_changed_callback)(void* handle, bt_address remote_addr, PROFILE_CONNECTION_STATE state);

typedef struct {
    btm_hidd_device_state_changed_callback hidd_app_state_changed_cb;
    btm_hidd_connection_state_changed_callback hidd_connection_state_changed_cb;
} bt_hid_device_callbacks;

typedef struct {
    size_t size;

    BT_RESULT_CODE (*register_device)(void** handle, bt_hidd_sdp_settings_t sdp, bt_hidd_qos_settings_t tx_qos,
        bt_hidd_qos_settings_t rx_qos, bt_hid_device_callbacks* callbacks);
    BT_RESULT_CODE (*unregister_device)(void* handle);
    BT_RESULT_CODE (*connect)(void* handle, bt_address remote_addr);
    BT_RESULT_CODE (*disconnect)(void* handle, bt_address remote_addr);
    BT_RESULT_CODE (*send_report_test)(void* handle, uint8_t report_id, uint8_t* buffer, size_t size);
    BT_RESULT_CODE (*unplug)(void* handle, bt_address remote_addr);
} btm_hid_device_interface_t;

btm_hid_device_interface_t* get_btm_hid_device_interface(void* bt_mgr_interface);

#endif