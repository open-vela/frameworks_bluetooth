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

typedef struct bt_pa_sync_data {
    uint16_t length;
    uint8_t data[0];
} bt_pa_sync_data_t;

bt_status_t pa_sync_init(void);
bt_status_t pa_sync_cleanup(void);
bt_status_t pa_sync_create(const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_create_param_t* params, const bt_pa_sync_callbacks_t* cbs,
    const void* context);
bt_status_t pa_sync_terminate(const bt_le_address_t* addr, uint8_t sid);
const bt_pa_sync_data_t* pa_sync_get_report_cache(const bt_le_address_t* addr, uint8_t sid);

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
 * @param[in] sid Advertising SID
 */
void pa_sync_on_terminated(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid);

/**
 * @brief Callback from SAL when a sync is terminated.
 *
 * @param[in] id The Controller ID
 * @param[in] addr Advertiser's address (public, random, public identity, or random static identity)
 * @param[in] sid Advertising SID; `BLE_SCAN_SID_NOT_PROVIDED` if unavailable
 * @param[in] tx_power TxPower in dBm (-127 to +20). `BT_POWER_UNAVAILABLE` if unavailable
 * @param[in] rssi RSSI in dBm (-127 to +20). `BT_POWER_UNAVAILABLE` if unavailable
 * @param[in] cte CTE_Type, e.g., `BT_LE_PA_SYNC_EVENT_CTE_TYPE_NONE`
 * @param[in] cnt Periodic_Event_Counter
 * @param[in] subevent The subevent number, range from 0x00 to 0x7F, or `BT_PA_SYNC_SUBEVENT_NONE`
 * @param[in] status Data_Status, e.g., `BT_LE_PA_SYNC_EVENT_DATA_COMPLETE`
 * @param[in] adv_data_len Length of `adv_data`
 * @param[in] adv_data Data received from a Periodic Advertising packet
 */
void pa_sync_on_received(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid,
    int tx_power, int rssi, uint8_t cte, uint16_t cnt, uint8_t subevent, uint8_t status, 
    uint8_t adv_data_len, const uint8_t* adv_data);

#endif /* __PA_SYNC_SERVICE_H__ */