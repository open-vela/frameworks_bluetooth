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
#ifndef __AVRCP_CONTROL_SERVICE_H__
#define __AVRCP_CONTROL_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_avrcp_control.h"

typedef struct {
    size_t size;

    /**
     * @brief Register the a2dp_sink event callback
     * @param[in] callbacks  a2dp_sink event callback function.
     */
    void* (*register_callbacks)(void* remote, const avrcp_control_callbacks_t* callbacks);

    /**
     * @brief Unregister the a2dp_sink event callback
     */
    bool (*unregister_callbacks)(void** remote, void* cookie);

    /** send pass through command to target */
    bt_status_t (*send_pass_through_cmd)(bt_address_t* bd_addr,
        avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state);

    /** get the playback state */
    bt_status_t (*get_playback_state)(bt_address_t* bd_addr);

    /** notify volume changed */
    bt_status_t (*volume_changed_notify)(bt_address_t* bd_addr, uint8_t volume);

    /** get element attributes */
    bt_status_t (*avrcp_control_get_element_attributes)(bt_address_t* bd_addr);

} avrcp_control_interface_t;

/*
 * register profile to service manager
 */
void register_avrcp_control_service(void);

#endif /* __AVRCP_CONTROL_SERVICE_H__ */
