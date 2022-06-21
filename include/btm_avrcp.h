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
#ifndef __BTM_AVRCP_H__
#define __BTM_AVRCP_H__

typedef enum {
    AVRC_CONNECTION_STATE_DISCONNECTED = 0,
    AVRC_CONNECTION_STATE_CONNECTING,
    AVRC_CONNECTION_STATE_CONNECTED,
    AVRC_CONNECTION_STATE_DISCONNECTING
} avrcp_connection_state_t;

typedef enum {
    PASSTHROUGH_CMD_ID_SELECT,
    PASSTHROUGH_CMD_ID_UP,
    PASSTHROUGH_CMD_ID_DOWN,
    PASSTHROUGH_CMD_ID_LEFT,
    PASSTHROUGH_CMD_ID_RIGHT,
    PASSTHROUGH_CMD_ID_RIGHT_UP,
    PASSTHROUGH_CMD_ID_RIGHT_DOWN,
    PASSTHROUGH_CMD_ID_LEFT_UP,
    PASSTHROUGH_CMD_ID_LEFT_DOWN,
    PASSTHROUGH_CMD_ID_ROOT_MENU,
    PASSTHROUGH_CMD_ID_SETUP_MENU,
    PASSTHROUGH_CMD_ID_CONTENTS_MENU,
    PASSTHROUGH_CMD_ID_FAVORITE_MENU,
    PASSTHROUGH_CMD_ID_EXIT,
    PASSTHROUGH_CMD_ID_0,
    PASSTHROUGH_CMD_ID_1,
    PASSTHROUGH_CMD_ID_2,
    PASSTHROUGH_CMD_ID_3,
    PASSTHROUGH_CMD_ID_4,
    PASSTHROUGH_CMD_ID_5,
    PASSTHROUGH_CMD_ID_6,
    PASSTHROUGH_CMD_ID_7,
    PASSTHROUGH_CMD_ID_8,
    PASSTHROUGH_CMD_ID_9,
    PASSTHROUGH_CMD_ID_DOT,
    PASSTHROUGH_CMD_ID_ENTER,
    PASSTHROUGH_CMD_ID_CLEAR,
    PASSTHROUGH_CMD_ID_CHANNEL_UP,
    PASSTHROUGH_CMD_ID_CHANNEL_DOWN,
    PASSTHROUGH_CMD_ID_PREVIOUS_CHANNEL,
    PASSTHROUGH_CMD_ID_SOUND_SELECT,
    PASSTHROUGH_CMD_ID_INPUT_SELECT,
    PASSTHROUGH_CMD_ID_DISPLAY_INFO,
    PASSTHROUGH_CMD_ID_HELP,
    PASSTHROUGH_CMD_ID_PAGE_UP,
    PASSTHROUGH_CMD_ID_PAGE_DOWN,
    PASSTHROUGH_CMD_ID_POWER,
    PASSTHROUGH_CMD_ID_VOLUME_UP,
    PASSTHROUGH_CMD_ID_VOLUME_DOWN,
    PASSTHROUGH_CMD_ID_MUTE,
    PASSTHROUGH_CMD_ID_PLAY,
    PASSTHROUGH_CMD_ID_STOP,
    PASSTHROUGH_CMD_ID_PAUSE,
    PASSTHROUGH_CMD_ID_RECORD,
    PASSTHROUGH_CMD_ID_REWIND,
    PASSTHROUGH_CMD_ID_FAST_FORWARD,
    PASSTHROUGH_CMD_ID_EJECT,
    PASSTHROUGH_CMD_ID_FORWARD,
    PASSTHROUGH_CMD_ID_BACKWARD,
    PASSTHROUGH_CMD_ID_ANGLE,
    PASSTHROUGH_CMD_ID_SUBPICTURE,
    PASSTHROUGH_CMD_ID_F1,
    PASSTHROUGH_CMD_ID_F2,
    PASSTHROUGH_CMD_ID_F3,
    PASSTHROUGH_CMD_ID_F4,
    PASSTHROUGH_CMD_ID_F5,
    PASSTHROUGH_CMD_ID_VENDOR_UNIQUE,
    PASSTHROUGH_CMD_ID_NEXT_GROUP,
    PASSTHROUGH_CMD_ID_PREV_GROUP,
    PASSTHROUGH_CMD_ID_RESERVED
} avrcp_passthr_cmd_t;

typedef enum {
    AVRCP_KEY_PRESSED,
    AVRCP_KEY_RELEASED
} avrcp_key_state_t;

typedef enum{
    PLAY_STATUS_STOPPED,
    PLAY_STATUS_PLAYING,
    PLAY_STATUS_PAUSED,
    PLAY_STATUS_FWD_SEEK,
    PLAY_STATUS_REV_SEEK,
    PLAY_STATUS_ERROR
} play_status_t;

typedef void (*avrcp_connection_state_callback)(bt_address addr, avrcp_connection_state_t state);
typedef void (*avrcp_get_play_status_callback)(bt_address addr);
typedef void (*avrcp_playback_register_notification_callback)(bt_address addr);

typedef struct {
  size_t size;
  avrcp_connection_state_callback connection_state_cb;
  avrcp_get_play_status_callback get_play_status_cb;
  avrcp_playback_register_notification_callback playback_register_notification_cb;
} avrcp_tg_callbacks_t;

/* avrcp target interface structure */
typedef struct {
    size_t size;

    /**
     * @brief Response get playback status request
     * @param[in] addr      address of peer device.
     * @param[in] status    current playback status.
     * @param[in] song_len  length of song which is playing
     * @param[in] song_pos  position of song which is playing
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*get_play_status_rsp)(bt_address addr,
                                          play_status_t status,
                                          uint32_t song_len, uint32_t song_pos);

    /**
     * @brief notify playback status if peer had register playback notification
     * @param[in] addr      address of peer device.
     * @param[in] status    current playback status.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*play_status_notify)(bt_address addr, play_status_t status);

    /**
     * @brief notify volume change if peer had register volumechanged notification
     * @param[in] addr      address of peer device.
     * @param[in] volume    volume of mediaplayer, range in <0-0x7F>.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*volume_changed_notify)(bt_address addr, uint8_t volume);

    /**
     * @brief set absolute volume
     * @param[in] addr      address of peer device.
     * @param[in] volume    volume of mediaplayer, range in <0-0x7F>.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*set_absolute_volume)(bt_address addr, uint8_t volume);

    /**
     * @brief Set the avrcp target event callback
     * @param[in] callbacks  avrcp target event callback function.
     */
    bt_result_code (*set_callbacks)(avrcp_tg_callbacks_t* callbacks);

    /**
     * @brief Reset the avrcp target event callback
     */
    void (*reset_callbacks)(void);
} avrcp_tg_interface_t;

/**
 * @brief Get the avrcp target interface
 * @return Pointer to avrcp target interface.
 */
const avrcp_tg_interface_t *get_avrcp_tg_interface(void);
#endif