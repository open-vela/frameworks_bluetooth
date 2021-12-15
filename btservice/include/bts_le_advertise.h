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
#ifndef _SRV_INC_GATT_ADVERTISE_MANAGER_H
#define _SRV_INC_GATT_ADVERTISE_MANAGER_H

#include <nuttx/list.h>
#include <stddef.h>

#include "btm_manager.h"

typedef void (*bts_le_advertise_started_callback)(void* context, uint8_t adv_id);
typedef void (*bts_le_advertise_stopped_callback)(void* context, uint8_t adv_id);
typedef void (*bts_le_advertise_failed_callback)(void* context, int error);
typedef struct {
    bts_le_advertise_started_callback bts_le_advertise_started_cb;
    bts_le_advertise_stopped_callback bts_le_advertise_stopped_cb;
    bts_le_advertise_failed_callback bts_le_advertise_failed_cb;
} bts_ble_advertiser_callbacks;

typedef struct {
    struct list_node node;

    uint8_t advertiser_id;
    advertise_param_t* param;
    const bts_ble_advertiser_callbacks* callbacks;

    void* btm_handle;
} bts_leadv_hdl_t;

typedef void (*ble_advertise_started_callback)(uint8_t adv_id);
typedef void (*ble_advertise_stopped_callback)(uint8_t adv_id);
typedef struct {
    ble_advertise_started_callback ble_advtise_started_cb;
    ble_advertise_stopped_callback ble_advtise_stopped_cb;
} stack_le_advertise_callbacks;

typedef struct {
    size_t size;

    const stack_le_advertise_callbacks* callbacks;
    BT_RESULT_CODE (*start_adv)(bts_leadv_hdl_t client);
    BT_RESULT_CODE (*stop_adv)(uint8_t advertiser_id);
} bts_le_advertise_interface_t;

const bts_le_advertise_interface_t* get_bts_bleadv_instance(void);
#endif