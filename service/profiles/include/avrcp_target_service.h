/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#ifndef __AVRCP_TARGET_SERVICE_H__
#define __AVRCP_TARGET_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_avrcp_target.h"

/* avrcp target interface structure */
typedef struct {
    size_t size;

    /**
     * @brief Register the a2dp_sink event callback
     * @param[in] callbacks  a2dp_sink event callback function.
     */
    void* (*register_callbacks)(void* remote, const avrcp_target_callbacks_t* callbacks);

    /**
     * @brief Unregister the a2dp_sink event callback
     */
    bool (*unregister_callbacks)(void** remote, void* cookie);

    /**
     * @brief Response get playback status request
     * @param[in] addr      address of peer device.
     * @param[in] status    current playback status.
     * @param[in] song_len  length of song which is playing
     * @param[in] song_pos  position of song which is playing
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*get_play_status_rsp)(bt_address_t* addr,
        avrcp_play_status_t status,
        uint32_t song_len, uint32_t song_pos);

    /**
     * @brief notify playback status if peer had register playback notification
     * @param[in] addr      address of peer device.
     * @param[in] status    current playback status.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*play_status_notify)(bt_address_t* addr, avrcp_play_status_t status);

    /**
     * @brief set absolute volume
     * @param[in] addr      address of peer device.
     * @param[in] volume    volume of mediaplayer, range in <0-0x7F>.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*set_absolute_volume)(bt_address_t* addr, uint8_t volume);

} avrcp_target_interface_t;

/*
 * register profile to service manager
 */
void register_avrcp_target_service(void);

#endif /* __AVRCP_TARGET_SERVICE_H__ */
