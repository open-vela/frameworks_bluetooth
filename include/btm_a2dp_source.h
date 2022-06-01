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
#ifndef __BTM_A2DP_SOURCE_H__
#define __BTM_A2DP_SOURCE_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "btm_a2dp.h"
#include "btm_manager.h"

typedef void (*a2dp_audio_source_config_callback)(bt_address addr);

/* A2DP sink interface structure */
typedef struct {
    /** set to sizeof(a2dp_source_callbacks_t) */
    size_t size;
    a2dp_connection_state_callback connection_state_cb;
    a2dp_audio_state_callback audio_state_cb;
    a2dp_audio_source_config_callback audio_source_config_cb;
} a2dp_source_callbacks_t;

/* A2DP source interface structure */
typedef struct {
    size_t size;

    /**
     * @brief Connect to the headset
     * @param[in] handle    the A2DP handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*connect)(void* handle, bt_address addr);

    /**
     * @brief Dis-connect from headset
     * @param[in] handle    the A2DP handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*disconnect)(void* handle, bt_address addr);

    /**
     * @brief Sets the connected device silence state
     * @note  Not implemented, will be realized in the future
     * @param[in] handle    the A2DP handle (unused).
     * @param[in] addr      address of peer device.
     * @param[in] silence   true on enable silence, false on disable
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*set_silence_device)(void* handle, bt_address addr, bool silence);

    /**
     * @brief Sets the connected device as active
     * @note  Not implemented, will be realized in the future
     * @param[in] handle    the A2DP handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*set_active_device)(void* handle, bt_address addr);

    /**
     * @brief Set the a2dp source event callback
     * @param[in] handle    The A2DP handle (unused).
     * @param[in] callbacks a2dp source event callback function.
     */
    void (*set_callbacks)(void* handle, a2dp_source_callbacks_t* callbacks);

} a2dp_source_interface_t;

/**
 * @brief Get the a2dp source interface
 * @return Pointer to a2dp source interface.
 */
extern const a2dp_source_interface_t* get_a2dp_source_interface(void);

#endif
