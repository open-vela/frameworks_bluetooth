
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
#ifndef __BTM_AVRCP_CT_H__
#define __BTM_AVRCP_CT_H__

#include "btm_manager.h"
#include "btm_avrcp.h"

typedef void (*avrcp_passthrough_rsp_callback)(bt_address addr, avrcp_passthr_cmd_t key_code,
                                              avrcp_key_state_t key_state, uint8_t response);
typedef void (*avrcp_play_position_changed_callback)(bt_address addr, uint32_t song_len, uint32_t song_pos);
typedef void (*avrcp_play_status_changed_callback)(bt_address addr, play_status_t play_status);

typedef struct {
    size_t size;
    avrcp_connection_state_callback connection_state_cb;
    avrcp_passthrough_rsp_callback passthrough_rsp_cb;
    avrcp_play_position_changed_callback play_position_changed_cb;
    avrcp_play_status_changed_callback play_status_changed_cb;
} avrc_ctrl_callbacks_t;

typedef struct {
    size_t size;

    /** send pass through command to target */
    bt_result_code (*send_pass_through_cmd)(bt_address bd_addr,
                                        avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state);

    /** get the playback state */
    bt_result_code (*get_playback_state)(bt_address bd_addr);

    /**
     * @brief Set the avrcp ctrl event callback
     * @param[in] callbacks  avrcp ctrl event callback function.
     */
    bt_result_code (*set_callbacks)(const avrc_ctrl_callbacks_t* callbacks);

    /**
     * @brief Reset the avrcp ctrl event callback
     */
    void (*reset_callbacks)(void);
} avrc_ctrl_interface_t;

const avrc_ctrl_interface_t* get_avrcp_ctrl_interface(void);
#endif