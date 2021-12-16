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
#ifndef _MGR_INC_BLUETOOTH_LE_ADVERTISE_H
#define _MGR_INC_BLUETOOTH_LE_ADVERTISE_H

#include <stddef.h>

#include "btm_manager.h"

typedef void (*leadv_started_callback)(void* handle);
typedef void (*leadv_stopped_callback)(void* handle);
typedef void (*leadv_failed_callback)(void* handle, int error);

typedef struct {
    leadv_started_callback le_advertise_started_cb;
    leadv_stopped_callback le_advertise_stopped_cb;
    leadv_failed_callback le_advertise_failed_cb;
} btm_le_advertise_callbacks;

typedef struct {
    size_t size;

    bt_result_code (*start_advertising)(void** handle, advertise_param_t* param,
        btm_le_advertise_callbacks* cb);
    bt_result_code (*stop_advertising)(void* handle);
} btm_le_advertise_interface_t;

btm_le_advertise_interface_t* get_btm_leadv_interface(void* bt_mgr_interface);

#endif