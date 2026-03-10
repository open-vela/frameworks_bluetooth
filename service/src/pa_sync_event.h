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

#ifndef __PA_SYNC_EVENT_H__
#define __PA_SYNC_EVENT_H__

#include "bluetooth.h"

/** AoA Constant Tone Extension */
#define BT_LE_PA_SYNC_EVENT_CTE_TYPE_AOA (0x00)
/** AoD Constant Tone Extension with 1 μs slots */
#define BT_LE_PA_SYNC_EVENT_CTE_TYPE_AOD_1US (0x01)
/** AoD Constant Tone Extension with 2 μs slots */
#define BT_LE_PA_SYNC_EVENT_CTE_TYPE_AOD_2US (0x02)
/** No Constant Tone Extension */
#define BT_LE_PA_SYNC_EVENT_CTE_TYPE_NONE (0xFF)

/** Data complete */
#define BT_LE_PA_SYNC_EVENT_DATA_COMPLETE (0x00)
/** Data incomplete, more data to come */
#define BT_LE_PA_SYNC_EVENT_DATA_MORE (0x01)
/** Data incomplete, data truncated, no more to come */
#define BT_LE_PA_SYNC_EVENT_DATA_TRUNCATED (0x02)
/** Failed to receive an AUX_SYNC_SUBEVENT_IND PDU */
#define BT_LE_PA_SYNC_EVENT_DATA_FAILED (0xFF)

typedef enum {
    SYNC_ESTABLISHED,
    SYNC_TERMINATED,
    SYNC_REPORT,
} pa_sync_event_type_t;

typedef struct {
    /** TxPower in dBm (-127 to +20). 0x7F if unavailable */
    int8_t tx_power;

    /** RSSI in dBm (-127 to +20). 0x7F if unavailable */
    int8_t rssi;

    /** CTE_Type, e.g., `BT_LE_PA_SYNC_EVENT_CTE_TYPE_NONE` */
    uint8_t cte;

    /** Periodic_Event_Counter */
    uint16_t cnt;

    /** The subevent number, range from 0x00 to 0x7F. 0xFF if no subevents */
    uint8_t subevent;

    /** Data_Status, e.g., `BT_LE_PA_SYNC_EVENT_DATA_COMPLETE` */
    uint8_t status;

    /** Data_Length */
    uint8_t adv_data_len;

    /** Data received from a Periodic Advertising packet */
    uint8_t adv_data[0];
} pa_sync_event_report_data_t;

typedef struct {
    pa_sync_event_type_t event;
    bt_controller_id_t id;
    bt_le_address_t addr;
    uint8_t data[0]; /**< Event specific data, e.g., @ref pa_sync_event_report_data_t */
} pa_sync_event_t;

#endif /* __PA_SYNC_EVENT_H__ */