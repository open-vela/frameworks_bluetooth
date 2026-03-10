/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#ifndef __PA_SYNC_SERVICE_H__
#define __PA_SYNC_SERVICE_H__

#include "bt_pa_sync.h"

bt_status_t pa_sync_init(void);
bt_status_t pa_sync_cleanup(void);
bt_status_t pa_sync_create(const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_create_param_t* params, const bt_pa_sync_callbacks_t* cbs,
    const void* context);
bt_status_t pa_sync_terminate(void);

/**
 * @brief Callback from SAL when a sync is established.
 *
 * @param[in] id The Controller ID
 * @param[in] addr Advertiser's address (public, random, public identity, or random static identity)
 * @param[in] sid Advertising SID; `BLE_SCAN_SID_NOT_PROVIDED` if unavailable
 */
void pa_sync_on_established(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid);

/**
 * @brief Callback from SAL when a sync is terminated.
 *
 * @param[in] id The Controller ID
 * @param[in] addr Advertiser's address (public, random, public identity, or random static identity)
 * @param[in] sid Advertising SID; `BLE_SCAN_SID_NOT_PROVIDED` if unavailable
 */
void pa_sync_on_terminated(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid);

#endif /* __PA_SYNC_SERVICE_H__ */