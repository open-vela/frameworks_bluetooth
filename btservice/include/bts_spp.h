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
#ifndef __BTS_SPP_H__
#define __BTS_SPP_H__

#include "btm_manager.h"
#include "btm_spp.h"

typedef struct spp_handle spp_handle_t;
typedef spp_callbacks_t spp_service_callbacks_t;

bt_result_code bts_spp_init(spp_callbacks_t* callbacks);
spp_handle_t*  bts_spp_register_app(int app_id, spp_callbacks_t *callbacks);
bt_result_code bts_spp_server_start(spp_handle_t *handle, uint16_t scn, uint16_t uuid);
bt_result_code bts_spp_server_stop(spp_handle_t *handle, uint16_t scn);
bt_result_code bts_spp_client_connect(spp_handle_t *handle, bt_address addr, int16_t scn, uint16_t uuid, uint16_t *port);
bt_result_code bts_spp_disconnect(spp_handle_t *handle, bt_address addr, uint16_t port);
void bts_spp_unregister_app(spp_handle_t *handle);
void bts_spp_cleanup(void);

extern bt_result_code spp_service_start(void);
extern void spp_service_stop(void);
extern spp_interface_t* get_spp_service_interface(void);
extern void bts_spp_state_dump(void);

#endif
