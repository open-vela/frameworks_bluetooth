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
#ifndef __A2DP_SOURCE_SERVICE_H__
#define __A2DP_SOURCE_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_a2dp_source.h"

/* A2DP source interface structure */
typedef struct {
    size_t size;

    /**
     * @brief Register the a2dp_source event callback
     * @param[in] callbacks  a2dp_source event callback function.
     */
    void* (*register_callbacks)(void* remote, const a2dp_source_callbacks_t* callbacks);

    /**
     * @brief Unregister the a2dp_source event callback
     */
    bool (*unregister_callbacks)(void** remote, void* cookie);

    /**
     * @brief Check a2dp source connection is connected
     * @param addr - address of peer device.
     * @return true - connected.
     * @return false - not connected.
     */
    bool (*is_connected)(bt_address_t* addr);

    /**
     * @brief Check a2dp source audio stream is started
     * @param addr - address of peer device.
     * @return true - playing.
     * @return false - stopped or suspend.
     */
    bool (*is_playing)(bt_address_t* addr);

    /**
     * @brief Get a2dp source connection state
     * @param addr - address of peer device.
     * @return profile_connection_state_t - connection state.
     */
    profile_connection_state_t (*get_connection_state)(bt_address_t* addr);

    /**
     * @brief Connect to the headset
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*connect)(bt_address_t* addr);

    /**
     * @brief Dis-connect from headset
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*disconnect)(bt_address_t* addr);

    /**
     * @brief Sets the connected device silence state
     * @note  Not implemented, will be realized in the future
     * @param[in] addr      address of peer device.
     * @param[in] silence   true on enable silence, false on disable
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*set_silence_device)(bt_address_t* addr, bool silence);

    /**
     * @brief Sets the connected device as active
     * @note  Not implemented, will be realized in the future
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_status_t (*set_active_device)(bt_address_t* addr);

} a2dp_source_interface_t;

/*
 * register profile to service manager
 */
void register_a2dp_source_service(void);

#endif /* __A2DP_SOURCE_SERVICE_H__ */
