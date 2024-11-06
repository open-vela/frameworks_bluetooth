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
 * @brief Register callback functions to AVRCP Controller service.
 *
 * When initializing an AVRCP Controller, an application should register callback functions
 * to the AVRCP Controller service. Subsequently, the AVRCP Controller service will
 * notify the application of any state changes via the registered callback functions.
 *
 * Callback functions includes:
 *   * connection_state_cb
 *   * get_element_attribute_cb
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - AVRCP Controller callback functions.
 * @return void* - Callbacks cookie, if the callback is registered successfuly. NULL,
 *                 if the callback is already registered or registration fails.
 *                 To obtain more information, refer to bt_remote_callbacks_register().
 *
 * **Example:**
 * @code
static const avrcp_control_callbacks_t avrcp_control_cbs = {
    .size = sizeof(avrcp_control_cbs),
    .connection_state_cb = avrcp_control_connection_state_cb,
    .get_element_attribute_cb = avrcp_control_get_element_attribute_cb,
};

void avrcp_control_init(void* ins)
{
    static void* control_cbks_cookie;

    control_cbks_cookie = bt_avrcp_control_register_callbacks(ins, &avrcp_control_cbs);
}
 * @endcode
 */
void* BTSYMBOLS(bt_avrcp_control_register_callbacks)(bt_instance_t* ins, const avrcp_control_callbacks_t* callbacks);

/**
 * @brief Unregister callback functions from AVRCP Controller service.
 *
 * An application may use this interface to stop listening on the AVRCP Controller
 * callbacks and to release the associated resources.
 *
 * @param ins - Bluetooth client instance.
 * @param cookie - Callbacks cookie.
 * @return true - Callback unregistration successful.
 * @return false - Callback cookie not found or callback unregistration failed.
 *
 * **Example:**
 * @code
void avrcp_control_uninit(void* ins)
{
    bt_avrcp_control_unregister_callbacks(ins, control_cbks_cookie);
}
 * @endcode
 */
bool BTSYMBOLS(bt_avrcp_control_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Get element attributes from AVRCP Target.
 *
 * This function is used when an application wants to obtain song information
 * from an AVRCP Target device, including title, artist name, album name, track
 * number, total number of tracks, genre, playing time.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
bt_status_t start_get_element_attributes(bt_instance_t* ins, bt_address_t* addr)
{
    bt_status_t ret = bt_avrcp_control_get_element_attributes(ins, addr);

    return ret;
}
 */
bt_status_t BTSYMBOLS(bt_avrcp_control_get_element_attributes)(bt_instance_t* ins, bt_address_t* addr);

#endif /* __BT_AVRCP_CONTROL_H__ */
