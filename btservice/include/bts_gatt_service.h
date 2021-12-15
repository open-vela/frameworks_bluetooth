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

#ifndef _SRV_INC_GATT_SERVICE_H
#define _SRV_INC_GATT_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "bts_gatt_client.h"
#include "bts_gatt_server.h"
#include "bts_le_advertise.h"
#include "bts_le_scan.h"

typedef struct {
    size_t size;

    const bts_ble_scanner_callbacks* scanner;
    const bts_ble_advertiser_callbacks advertiser;
    const bts_gatt_client_callbacks* client;
    const bts_gatt_server_callbacks* server;
} btgatt_callbacks;

typedef struct {
    size_t size;

    BT_RESULT_CODE (*init)(void);
    void (*cleanup)(void);

    const bts_le_scan_interface_t* scanner;
    const bts_le_advertise_interface_t* advertiser;
    const bts_gattc_interface_t* client;
    const bts_gatts_interface_t* server;
} gatt_interface_t;

const gatt_interface_t* gatt_get_interface(void);
#endif