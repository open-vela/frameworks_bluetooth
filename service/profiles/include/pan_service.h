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
#ifndef __PAN_SERVICE_H__
#define __PAN_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_pan.h"

typedef struct {
    size_t size;

    /**
     * @brief Register the pan event callback
     * @param[in] callbacks  pan event callback function.
     */
    void* (*register_callbacks)(void* remote, const pan_callbacks_t* callbacks);

    /**
     * @brief Unregister the pan event callback
     */
    bool (*unregister_callbacks)(void** remote, void* cookie);

    /**
     * @brief Connect to the NAP server
     * @param[in] handle    the pan handle (unused).
     * @param[in] addr      address of peer device.
     * @param[in] dst_role  remote pan server role.
     * @param[in] src_role  local pan role.
     * @note Only support PANU connect to NAP server, so dst_role must be 1, src_role must be 2.
     * @return BT_STATUS_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*connect)(bt_address_t* addr, uint8_t dst_role, uint8_t src_role);

    /**
     * @brief Dis-connect from NAP server
     * @param[in] handle    the pan handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_STATUS_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*disconnect)(bt_address_t* addr);
} pan_interface_t;

void pan_on_connection_state_changed(bt_address_t* addr, pan_role_t remote_role,
    pan_role_t local_role, profile_connection_state_t state);
void pan_on_data_received(bt_address_t* addr, uint16_t protocol,
    uint8_t* dst_addr, uint8_t* src_addr,
    uint8_t* data, uint16_t length);

/*
 * register profile to service manager
 */
void register_pan_service(void);

#endif /* __PAN_SERVICE_H__ */