/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __AURACAST_SINK_EVENT_H__
#define __AURACAST_SINK_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_auracast_sink.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
typedef enum {
    AURACAST_SINK_STARTUP,
    AURACAST_SINK_SHUTDOWN,
    AURACAST_SINK_CREATE_SYNC,
    AURACAST_SINK_TERMINATE_SYNC,
    AURACAST_SINK_SYNC_ESTABLISHED,
    AURACAST_SINK_SYNC_TERMINATED,
    AURACAST_SINK_CONFIG_DONE,
    AURACAST_SINK_DATA_IN,
    AURACAST_SINK_DUMP,
} auracast_sink_event_t;

typedef struct {
    uint32_t bitfield;
    uint8_t encrypted;
    uint8_t broadcast_code[BT_AURACAST_BROADCAST_CODE_LEN];
} auracast_sink_event_create_sync_t;

typedef struct {
    uint32_t bitfield;
    uint32_t timestamp;
    uint16_t sequence_number;
    uint16_t length;
    uint8_t data[0];
} auracast_sink_event_packet_t;

typedef struct {
    uint16_t size;
    uint8_t data[0];
} auracast_sink_msg_priv_data_t;

typedef struct {
    auracast_sink_event_t event;
    bt_controller_id_t id;
    bt_le_address_t addr;
    uint8_t sid;
    void* context;
    auracast_sink_msg_priv_data_t data;
} auracast_sink_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
auracast_sink_msg_t* auracast_sink_msg_new(auracast_sink_event_t event, bt_controller_id_t id,
    const bt_le_address_t* addr, uint8_t sid);
auracast_sink_msg_t* auracast_sink_msg_new_ext(auracast_sink_event_t event, bt_controller_id_t id,
    const bt_le_address_t* addr, uint8_t sid, size_t size);
void auracast_sink_msg_destory(auracast_sink_msg_t* msg);
void auracast_sink_send_message(void* msg);

#endif /* __AURACAST_SINK_EVENT_H__ */