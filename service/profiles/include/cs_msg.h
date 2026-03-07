/****************************************************************************
 *
 *   Copyright (C) 2025 Xiaomi InC. All rights reserved.
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
#ifndef __CS_MSG_H__
#define __CS_MSG_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_addr.h"

typedef enum {
    CS_STARTUP,
    CS_SHUTDOWN,
    START_REQ,
    STOP_REQ,
    CAPABILITIES_RECEIVED_EVT,
    DISCONNECTED_EVT,
    CONNECTED_EVT,
    CONFIG_DONE_EVT,
    SECURITY_DONE_EVT,
    PROCEDURE_DONE_EVT,
    SUBEVENT_RESULT_EVT,
    LOCAL_SUPPORTED_CAPABILITIES_EVT,
} cs_msg_id_t;

typedef struct {
    bt_address_t bd_addr;
    void* data;
    void* cb;
} cs_msg_data_t;

typedef struct {
    cs_msg_id_t id;
    cs_msg_data_t cs_data;
} cs_msg_t;

cs_msg_t* cs_msg_new(cs_msg_id_t msg, bt_address_t* bd_addr);
void cs_msg_destroy(cs_msg_t* cs_msg);

#endif
