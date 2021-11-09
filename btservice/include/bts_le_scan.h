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
#ifndef _SRV_INC_LESCAN_MANAGER_H
#define _SRV_INC_LESCAN_MANAGER_H

#include <nuttx/list.h>
#include <stdbool.h>
#include <stddef.h>

#include "btm_manager.h"
#include "bts_common.h"

typedef void (*bts_le_scan_result_callback)(void* handle, const scan_result_t* scan_result_data);
typedef void (*bts_le_scan_failed_callback)(void* handle, int error);
typedef void (*bts_le_scan_started_callback)(void* handle, uint8_t scanner_id);
typedef void (*bts_le_scan_stopped_callback)(void* handle);

typedef struct {
    bts_le_scan_result_callback bts_le_scan_result_cb;
    bts_le_scan_failed_callback bts_ble_scan_failed_cb;
    bts_le_scan_started_callback bts_ble_scan_started_cb;
    bts_le_scan_stopped_callback bts_ble_scan_stopped_cb;
} bts_ble_scanner_callbacks;

typedef struct {
    struct list_node node;

    uint8_t scanner_id;
    ble_scan_filter_t* filter;
    scan_params_t* settings;
    const bts_ble_scanner_callbacks* callbacks;

    void* btm_handle;
} bts_lescan_hdl_t;

typedef void (*ble_scan_result_callback)(const scan_result_t* result);

typedef struct {
    ble_scan_result_callback ble_scan_result;
} stack_le_scan_callbacks;
typedef struct {
    size_t size;

    const stack_le_scan_callbacks* callbacks;
    bt_result_code (*start_scan)(bts_lescan_hdl_t client);
    bt_result_code (*stop_scan)(uint8_t scanner_id);
} bts_le_scan_interface_t;

const bts_le_scan_interface_t* get_bts_lescan_instance(void);
#endif