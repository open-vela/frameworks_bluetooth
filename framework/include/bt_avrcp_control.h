/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#ifndef __BT_AVRCP_CONTROL_H__
#define __BT_AVRCP_CONTROL_H__

#include "bt_avrcp.h"

/**
 * @cond
 */

typedef struct {
    uint32_t attr_id;
    uint16_t chr_set;
    uint8_t* text;
} avrcp_element_attr_val_t;

typedef struct {
    uint8_t title[AVRCP_ATTR_MAX_TIELE_LEN + 1];
    uint8_t artist[AVRCP_ATTR_MAX_ARTIST_LEN + 1];
    uint8_t album[AVRCP_ATTR_MAX_ALBUM_LEN + 1];
    uint8_t track_number[AVRCP_ATTR_MAX_TRACK_NUMBER_LEN + 1];
    uint8_t total_track_number[AVRCP_ATTR_MAX_TOTAL_TRACK_NUMBER_LEN + 1];
    uint8_t gener[AVRCP_ATTR_MAX_GENER_LEN + 1];
    uint8_t playing_time_ms[AVRCP_ATTR_MAX_PLAYING_TIMES_LEN + 1];
    uint8_t cover_art_handle[AVRCP_ATTR_MAX_COVER_ART_HANDLE_LEN + 1];
} avrcp_socket_element_attr_val_t;

typedef void (*avrcp_get_element_attribute_cb)(void* cookie, bt_address_t* addr, uint8_t attrs_count, avrcp_element_attr_val_t* attrs);
typedef struct {
    size_t size;
    avrcp_connection_state_callback connection_state_cb;
    avrcp_get_element_attribute_cb get_element_attribute_cb;
} avrcp_control_callbacks_t;

/**
 * @endcond
 */

/**
 * @brief Register callback functions to AVRCP Control
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - AVRCP Control callback functions.
 * @return void* - Callbacks cookie.
 */
void* BTSYMBOLS(bt_avrcp_control_register_callbacks)(bt_instance_t* ins, const avrcp_control_callbacks_t* callbacks);

/**
 * @brief Unregister callback functions to AVRCP Control
 *
 * @param ins - Bluetooth client instance.
 * @param cookie - Callbacks cookie.
 * @return bool - True, if unregister success, false otherwise.
 */
bool BTSYMBOLS(bt_avrcp_control_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Get element attribute from peer device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - Remote BT address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_avrcp_control_get_element_attributes)(bt_instance_t* ins, bt_address_t* addr);

#endif /* __BT_AVRCP_CONTROL_H__ */
