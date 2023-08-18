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

#include "bluetooth.h"
#include "sal.h"
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
#include "sal_a2dp_source_interface.h"
#endif
#include "sal_bluelet.h"

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE

static void adp_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state);
static void adp_stream_state_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state, uint16_t sink_cid);
static void adp_stream_config_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S *config);
static void adp_stream_channel_mtu_cb(BD_ADDR remote_addr, uint16_t stream_chnl_mtu);
#ifdef CONFIG_BLUETOOTH_A2DP_PEER_PARTIAL_RECONN
static void adp_peer_partial_reconnect_cb(BD_ADDR remote_addr);
#endif

static A2DP_SOURCE_CALLBACKS_S a2dp_source_cbks = {
    sizeof(a2dp_source_cbks),
    .a2dp_source_connection_state_changed_cb = adp_connection_state_changed_cb,
    .a2dp_source_stream_state_changed_cb = adp_stream_state_changed_cb,
    .a2dp_source_stream_config_changed_cb = adp_stream_config_changed_cb,
    .a2dp_source_stream_channel_mtu_cb = adp_stream_channel_mtu_cb,
#ifdef CONFIG_BLUETOOTH_A2DP_PEER_PARTIAL_RECONN
    .a2dp_source_peer_partial_reconnect_cb = adp_peer_partial_reconnect_cb,
#else
    .a2dp_source_peer_partial_reconnect_cb = NULL,
#endif
};

static void adp_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    a2dp_event_type_t event;

    switch (state) {
    case SERVICE_PROFILE_DISCONNECTED:
        event = DISCONNECTED_EVT;
        break;
    case SERVICE_PROFILE_CONNECTED:
        BT_LOGD("PERFORMANCE-A2DP-SRC-BLUELET-CONNECTED");
        event = CONNECTED_EVT;
        break;
    default:
        return;
    }

    bt_sal_a2dp_source_event_callback(a2dp_event_new(event, (void *)remote_addr));
}

static void adp_stream_state_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_STATE state, uint16_t sink_cid)
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

    bt_sal_a2dp_source_event_callback(a2dp_event_new(event, (void *)remote_addr));
}

static void adp_stream_config_changed_cb(BD_ADDR remote_addr, SERVICE_A2DP_STREAM_CONFIG_S *config)
{
    a2dp_event_t *event;
    a2dp_codec_config_t codec_config = { 0 };

    codec_config.codec_type = config->codec;
    codec_config.sample_rate = config->sample_rate;
    codec_config.channel_mode = config->channel;
    codec_config.bits_per_sample = config->bit_width;
    codec_config.packet_size = 1024;
    memcpy(codec_config.specific_info, config->codec_info, config->codec_info_len);
    event = a2dp_event_new(CODEC_CONFIG_EVT, (void *)remote_addr);
    event->event_data.data = malloc(sizeof(codec_config));
    memcpy(event->event_data.data, &codec_config, sizeof(codec_config));

    bt_sal_a2dp_source_event_callback(event);
}

static void adp_stream_channel_mtu_cb(BD_ADDR remote_addr, uint16_t stream_chnl_mtu)
{
    a2dp_event_t *event;

    event = a2dp_event_new(STREAM_MTU_CONFIG_EVT, (void *)remote_addr);
    event->event_data.mtu = stream_chnl_mtu;

    bt_sal_a2dp_source_event_callback(event);
}

#ifdef CONFIG_BLUETOOTH_A2DP_PEER_PARTIAL_RECONN
static void adp_peer_partial_reconnect_cb(BD_ADDR remote_addr)
{
    a2dp_event_t *event;

    event = a2dp_event_new(PEER_PARTIAL_RECONN_EVT, remote_addr);

    bt_sal_a2dp_source_event_callback(event);
}
#endif

bt_status_t bt_sal_a2dp_source_init(uint8_t max_connection)
{
    SAL_CHECK_RET(service_adapter_a2dp_source_init(max_connection, &a2dp_source_cbks),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_a2dp_source_cleanup(void)
{
    service_adapter_a2dp_source_cleanup();
}

bt_status_t bt_sal_a2dp_source_connect(bt_address_t *addr)
{
    SAL_CHECK_RET(service_adapter_a2dp_source_connect((void *)addr, A2DP_PREFERRED_CODEC),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_source_disconnect(bt_address_t *addr)
{
    SAL_CHECK_RET(service_adapter_a2dp_source_disconnect((void *)addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_source_set_silence_device(bt_address_t *addr, bool silence)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_source_set_active_device(bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_source_start_stream(bt_address_t *remote_addr)
{
    SAL_CHECK_RET(service_adapter_a2dp_source_start_stream((void *)remote_addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_source_suspend_stream(bt_address_t *remote_addr)
{
    SAL_CHECK_RET(service_adapter_a2dp_source_suspend_stream((void *)remote_addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_source_send_data(bt_address_t *remote_addr, uint8_t *buf,
                                         uint16_t nbytes, uint8_t nb_frames, uint64_t timestamp, uint32_t seq)
{
    SERVICE_A2DP_SOURCE_PACKET_S *packet;

    if (buf == NULL) {
        BT_LOGE("%s, buffer is null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    packet = (SERVICE_A2DP_SOURCE_PACKET_S *)buf;
    packet->data_length = nbytes;
    packet->header.version = 2;
    packet->header.padding = 0;
    packet->header.csrcCount = 0;
    packet->header.marker = 0;
    packet->header.payloadType = 96;
    packet->header.sequenceNumber = seq;
    packet->header.ssrc = 1;
    packet->header.timestamp = timestamp;

    SAL_CHECK_RET(service_adapter_a2dp_source_send_data((void *)remote_addr, packet),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
