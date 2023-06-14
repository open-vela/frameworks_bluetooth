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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stack_adapter_a2dp_sink.h"
#include "stack_adapter_common.h"
#include "stack_adapter_service_base.h"

#include "bluetooth.h"
#include "sal.h"
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
#include "sal_a2dp_sink_interface.h"
#endif
#include "sal_bluelet.h"

#ifdef CONFIG_BLUETOOTH_A2DP_SINK

#ifndef CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#define A2DP_MAX_CONNECTION (1)
#else
#define A2DP_MAX_CONNECTION CONFIG_BLUETOOTH_A2DP_MAX_CONNECTIONS
#endif

static void adpt_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state);
static void adpt_stream_state_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state, uint16_t sink_cid);
static void adpt_stream_config_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S *config);
static void adpt_packet_received_cb(BD_ADDR remote_addr, SERVICE_A2DP_SINK_DATA_S *data);
static void adpt_stream_req_received_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_REQUEST request);

static A2DP_SINK_CALLBACKS_S a2dp_sink_cbks = {
    sizeof(a2dp_sink_cbks),
    adpt_connection_state_changed_cb,
    adpt_stream_state_changed_cb,
    adpt_stream_config_changed_cb,
    // CONFIG_SBC should be disabled, we dont need it.
    NULL,
    adpt_packet_received_cb,
    adpt_stream_req_received_cb,
};

static void adpt_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    a2dp_event_type_t event;

    switch (state) {
    case SERVICE_PROFILE_DISCONNECTED:
        event = DISCONNECTED_EVT;
        break;
    case SERVICE_PROFILE_CONNECTED:
        event = CONNECTED_EVT;
        break;
    default:
        return;
    }

    bt_sal_a2dp_sink_event_callback(a2dp_event_new(event, (void *)remote_addr));
}

static void adpt_stream_state_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state, uint16_t sink_cid)
{
    a2dp_event_type_t event;

    switch (state) {
    case A2DP_STREAM_UNKNOWN:
    case A2DP_STREAM_IDLE:
    case A2DP_STREAM_OPENED:
        return;
    case A2DP_STREAM_CLOSED:
        event = STREAM_CLOSED_EVT;
        break;
    case A2DP_STREAM_SUSPENDED:
        event = STREAM_SUSPENDED_EVT;
        break;
    case A2DP_STREAM_STREAMING:
        event = STREAM_STARTED_EVT;
        break;
    default:
        return;
    }

    bt_sal_a2dp_sink_event_callback(a2dp_event_new(event, (void *)remote_addr));
}

static void adpt_stream_config_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S *config)
{
    a2dp_event_t *event;
    a2dp_codec_config_t codec_config = { 0 };

    codec_config.codec_type = config->codec;
    codec_config.sample_rate = config->sample_rate;
    codec_config.channel_mode = config->channel;
    codec_config.bits_per_sample = config->bit_width;
    memcpy(codec_config.specific_info, config->codec_info, config->codec_info_len);
    event = a2dp_event_new(CODEC_CONFIG_EVT, (void *)remote_addr);
    event->event_data.data = malloc(sizeof(codec_config));
    memcpy(event->event_data.data, &codec_config, sizeof(codec_config));

    bt_sal_a2dp_sink_event_callback(event);
}

static void adpt_packet_received_cb(BD_ADDR remote_addr, SERVICE_A2DP_SINK_DATA_S *data)
{
    uint8_t *p;
    uint8_t offset;
    uint16_t seq, pktlen;
    uint32_t timestamp;
    a2dp_event_t *event;
    a2dp_sink_packet_t *packet;
    if (data == NULL || data->p_buffer == NULL)
        return;

    p = data->p_buffer;
    offset = 12 + (*p & 0x0F) * 4; // rtp header + ssrc
    pktlen = data->length - offset;
    p += 2;
    BE_STREAM_TO_UINT16(seq, p);
    BE_STREAM_TO_UINT32(timestamp, p);
    packet = a2dp_sink_new_packet(timestamp, seq, data->p_buffer + offset, pktlen);
    if (packet == NULL) {
        BT_LOGE("%s, packet malloc failed", __func__);
        return;
    }
    event = a2dp_event_new(DATA_IND_EVT, (void *)remote_addr);
    event->event_data.packet = packet;

    bt_sal_a2dp_sink_event_callback(event);
}

static void adpt_stream_req_received_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_REQUEST request)
{
    bt_sal_a2dp_sink_event_callback(a2dp_event_new(PEER_STREAM_START_REQ, (void *)remote_addr));
}

bt_status_t bt_sal_a2dp_sink_init(uint8_t max_connection)
{
    SAL_CHECK_RET(service_adapter_a2dp_sink_init(max_connection, &a2dp_sink_cbks),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_a2dp_sink_cleanup(void)
{
    service_adapter_a2dp_sink_cleanup();
}

bt_status_t bt_sal_a2dp_sink_connect(bt_address_t *addr)
{
    SAL_CHECK_RET(service_adapter_a2dp_sink_connect((void *)addr, A2DP_PREFERRED_CODEC),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_sink_disconnect(bt_address_t *addr)
{
    SAL_CHECK_RET(service_adapter_a2dp_sink_disconnect((void *)addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_sink_set_active_device(bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_sink_start_stream(bt_address_t *addr)
{
    SAL_CHECK_RET(service_adapter_a2dp_sink_accept_start_stream_req((void *)addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
