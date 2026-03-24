/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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

#ifndef __AURACAST_SINK_SERVICE_H__
#define __AURACAST_SINK_SERVICE_H__

#include "bt_auracast_sink.h"

/** TODO: Move to a proper place so both auracast/unicast and source/sink can access it */
#define BT_ISO_FRAMING_MODE_UNFRAMED 0x00
#define BT_ISO_FRAMING_MODE_FRAMED_SEGMENTABLE 0x01
#define BT_ISO_FRAMING_MODE_FRAMED_UNSEGMENTED 0x02

#define BT_AURACAST_SINK_DEFAULT_TIMEOUT_MS 500

#define HACK_BEFORE_TINYCOMPRESS_DONE 1

typedef struct auracast_sink_interface {
    void* (*register_callbacks)(void* remote, const bt_auracast_sink_callbacks_t* cbs);
    bool (*unregister_callbacks)(void** remote, void* cookie);
    bt_status_t (*create_sync)(const bt_le_address_t* addr, uint8_t sid, uint32_t bitfield,
        const uint8_t* broadcast_code);
    bt_status_t (*terminate_sync)(const bt_le_address_t* addr, uint8_t sid);
    bt_status_t (*dump)(void);
} auracast_sink_interface_t;

void register_auracast_sink_service(void);
void auracast_sink_service_notify_sync_established(const void* context);
void auracast_sink_service_notify_sync_terminated(const void* context);

#if HACK_BEFORE_TINYCOMPRESS_DONE
void auracast_sink_send_message_delayed(const void* stm, int event, uint16_t size,
    const uint8_t* payload, void* context);
#endif

/**
 * @brief Callback from SAL when a sync is established.
 *
 * @param[in] id The Controller ID
 * @param[in] addr Advertiser's address (public, random, public identity, or random static identity)
 * @param[in] sid Advertising SID; `BLE_SCAN_SID_NOT_PROVIDED` if unavailable
 */
void auracast_sink_on_established(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid);

/**
 * @brief Callback from SAL when a sync is terminated.
 *
 * @param[in] id The Controller ID
 * @param[in] addr Advertiser's address (public, random, public identity, or random static identity)
 * @param[in] sid Advertising SID
 */
void auracast_sink_on_terminated(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid);

/**
 * @brief Callback from SAL when a ISO data is received.
 *
 * @param[in] id The Controller ID
 * @param[in] addr Advertiser's address (public, random, public identity, or random static identity)
 * @param[in] sid Advertising SID
 * @param[in] bis Bitwise value of which BIS is to synchronize, e.g., BIS[x] is synchronized if bit
 *                x is set. The value of x ranges from 0x1 to 0x1F. Bit 0 is reserved
 * @param[in] ts The timestamp, in microseconds
 * @param[in] seq The sequence number of the SDU
 * @param[in] len The length of ISO data in octets
 * @param[in] data The ISO SDU
 */
void auracast_sink_on_data_received(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid,
    uint32_t bis, uint32_t ts, uint16_t seq, uint16_t len, const uint8_t* data);
#endif /* __AURACAST_SINK_SERVICE_H__ */