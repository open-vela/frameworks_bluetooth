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
#ifndef _SRV_INC_HID_DEVICE_H
#define _SRV_INC_HID_DEVICE_H

#include <nuttx/list.h>
#include <stddef.h>

#include "btm_manager.h"

#define XK_MISCELLANY 1
#define XK_LATIN1 1
#define XK_XKB_KEYS 1

typedef void (*bts_hidd_app_state_changed_callback)(void* handle, uint8_t device_id, hid_app_state registered);
typedef void (*bts_hidd_connection_state_changed_callback)(void* handle, bt_address remote_addr, bool le_hid, profile_connection_state state);

typedef struct {
    bts_hidd_app_state_changed_callback bts_hidd_app_state_changed_cb;
    bts_hidd_connection_state_changed_callback bts_hidd_connection_state_changed_cb;
} bts_hid_device_callbacks;

typedef struct
{
    struct list_node node;

    const bts_hid_device_callbacks* callbacks;
    uint8_t device_id;
    bt_address remote_addr;
    void* btm_handle;
} bts_hidd_hdl_t;

typedef struct {
    size_t size;
    bt_result_code (*init)(void);
    void (*clean_up)(void);
    bt_result_code (*register_device)(bts_hidd_hdl_t handle, bt_hidd_sdp_settings_t sdp, bt_hidd_qos_settings_t tx_qos, bt_hidd_qos_settings_t rx_qos);
    bt_result_code (*unregister_device)(uint16_t device_id);
    bt_result_code (*connect)(uint16_t device_id, bt_address remote_addr);
    bt_result_code (*disconnect)(uint8_t device_id, bt_address remote_addr);
    bt_result_code (*send_report)(uint8_t device_id, bt_address remote_addr, uint8_t report_id, uint8_t* buffer, size_t size);
    bt_result_code (*unplug)(uint8_t device_id, bt_address remote_addr);
} bts_hidd_interface_t;

const bts_hidd_interface_t* get_bts_hidd_interface(void);
#endif