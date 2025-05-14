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
#define LOG_TAG "sal_a2dp"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bluetooth.h"
#include "bt_addr.h"
#include "sal_a2dp_sink_interface.h"
#include "sal_a2dp_source_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/zephyr3/a2dp.h>

#include "bt_utils.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_A2DP
#include "a2dp_codec.h"

static void zblue_on_connected(struct bt_conn* conn);
static void zblue_on_disconnected(struct bt_conn* conn);
static void zblue_on_media_handler(struct bt_conn* conn, uint8_t* data, uint16_t len);
static int zblue_on_media_state_req(struct bt_conn* conn, uint8_t state);
static void zblue_on_seted_codec(struct bt_conn* conn, struct bt_a2dp_media_codec* codec, uint8_t cp_type);

static struct bt_a2dp_app_cb a2dp_cbks = {
    .connected = zblue_on_connected,
    .disconnected = zblue_on_disconnected,
    .media_handler = zblue_on_media_handler,
    .media_state_req = zblue_on_media_state_req,
    .seted_codec = zblue_on_seted_codec,
};

static a2dp_codec_index_t zephyr_codec_2_sal_codec(uint8_t codec)
{
    switch (codec) {
    case BT_A2DP_SBC:
        return BTS_A2DP_TYPE_SBC;
    case BT_A2DP_MPEG1:
        return BTS_A2DP_TYPE_MPEG1_2_AUDIO;
    case BT_A2DP_MPEG2:
        return BTS_A2DP_TYPE_MPEG2_4_AAC;
    case BT_A2DP_VENDOR:
        return BTS_A2DP_TYPE_NON_A2DP;
    default:
        BT_LOGW("%s, invalid codec: 0x%x", __func__, codec);
        return BTS_A2DP_TYPE_SBC;
    }
}

#define CASE_RETURN_SBC_SAMPLE_RATE(sbc_sample_rate) \
    case BT_A2DP_SBC_##sbc_sample_rate:              \
        return sbc_sample_rate;

static uint32_t zephyr_sbc_sample_rate_2_sal_sample_rate(uint8_t sbc_sample_rate)
{
    switch (sbc_sample_rate) {
        CASE_RETURN_SBC_SAMPLE_RATE(48000)
        CASE_RETURN_SBC_SAMPLE_RATE(44100)
        CASE_RETURN_SBC_SAMPLE_RATE(32000)
        CASE_RETURN_SBC_SAMPLE_RATE(16000)
        DEFAULT_BREAK()
    }
    BT_LOGW("%s, invalid sample rate: 0x%x", __func__, sbc_sample_rate);
    return 44100;
}

#define CHECK_RETURN_AAC_SAMPLE_RATE(aac_sample_rate)    \
    if (aac_sample_rate & BT_A2DP_AAC_##aac_sample_rate) \
        return aac_sample_rate;

static uint32_t zephyr_aac_sample_rate_2_sal_sample_rate(uint16_t aac_sample_rate)
{
    CHECK_RETURN_AAC_SAMPLE_RATE(96000)
    CHECK_RETURN_AAC_SAMPLE_RATE(88200)
    CHECK_RETURN_AAC_SAMPLE_RATE(64000)
    CHECK_RETURN_AAC_SAMPLE_RATE(48000)
    CHECK_RETURN_AAC_SAMPLE_RATE(44100)
    CHECK_RETURN_AAC_SAMPLE_RATE(32000)
    CHECK_RETURN_AAC_SAMPLE_RATE(24000)
    CHECK_RETURN_AAC_SAMPLE_RATE(22050)
    CHECK_RETURN_AAC_SAMPLE_RATE(16000)
    CHECK_RETURN_AAC_SAMPLE_RATE(12000)
    CHECK_RETURN_AAC_SAMPLE_RATE(11025)
    CHECK_RETURN_AAC_SAMPLE_RATE(8000)

    BT_LOGW("%s, invalid sample rate: 0x%x", __func__, aac_sample_rate);
    return 44100;
}

static a2dp_codec_channel_mode_t zephyr_sbc_channel_mode_2_sal_channel_mode(uint8_t sbc_channel_mode)
{
    switch (sbc_channel_mode) {
    case BT_A2DP_SBC_JOINT_STEREO:
    case BT_A2DP_SBC_STEREO:
    case BT_A2DP_SBC_DUAL_CHANNEL:
        return BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
    case BT_A2DP_SBC_MONO:
        return BTS_A2DP_CODEC_CHANNEL_MODE_MONO;
    default:
        BT_LOGW("%s, invalid channel mode: 0x%x", __func__, sbc_channel_mode);
        return BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
    }
}

static a2dp_codec_channel_mode_t zephyr_aac_channel_mode_2_sal_channel_mode(uint8_t aac_channel_mode)
{
    switch (aac_channel_mode) {
    case BT_A2DP_AAC_CHANNELS_1:
        return BTS_A2DP_CODEC_CHANNEL_MODE_MONO;
    case BT_A2DP_AAC_CHANNELS_2:
        return BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
    default:
        BT_LOGW("%s, invalid channel mode: 0x%x", __func__, aac_channel_mode);
        return BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
    }
}

static void zblue_on_connected(struct bt_conn* conn)
{
    uint8_t role = bt_a2dp_get_a2dp_role(conn);
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    if (role == BT_A2DP_CH_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(a2dp_event_new(CONNECTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(CONNECTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

static void zblue_on_disconnected(struct bt_conn* conn)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    bt_sal_a2dp_source_event_callback(a2dp_event_new(DISCONNECTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_sal_a2dp_sink_event_callback(a2dp_event_new(DISCONNECTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
static void zblue_on_media_handler(struct bt_conn* conn, uint8_t* data, uint16_t len)
{
    bt_address_t bd_addr;
    a2dp_event_t* event;
    a2dp_sink_packet_t* packet;
    uint8_t* p;
    uint8_t offset;
    uint16_t seq, pktlen;
    uint32_t timestamp;

    if (data == NULL)
        return;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    p = data;
    offset = 12 + (*p & 0x0F) * 4; // rtp header + ssrc + csrc
    if (len < offset) {
        BT_LOGE("%s, invalid length: %d", __func__, len);
        return;
    }

    pktlen = len - offset;
    p += 2;
    BE_STREAM_TO_UINT16(seq, p);
    BE_STREAM_TO_UINT32(timestamp, p);
    packet = a2dp_sink_new_packet(timestamp, seq, data + offset, pktlen);
    if (packet == NULL) {
        BT_LOGE("%s, packet malloc failed", __func__);
        return;
    }
    event = a2dp_event_new(DATA_IND_EVT, &bd_addr);
    event->event_data.packet = packet;
    bt_sal_a2dp_sink_event_callback(event);
}
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */

static int zblue_on_media_state_req(struct bt_conn* conn, uint8_t state)
{
    uint8_t role = bt_a2dp_get_a2dp_role(conn);
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return -1;

    switch (state) {
    case BT_A2DP_MEDIA_STATE_OPEN:
        if (role == BT_A2DP_CH_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
            // bt_sal_a2dp_source_event_callback(a2dp_event_new(CONNECTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
        } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
            // bt_sal_a2dp_sink_event_callback(a2dp_event_new(CONNECTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
        }
        return 0;

    case BT_A2DP_MEDIA_STATE_START:
        if (role == BT_A2DP_CH_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
            /* TODO: check if a2dp stream should be accepted */
            bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_STARTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
        } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
            bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_STARTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
        }
        return 0;

    case BT_A2DP_MEDIA_STATE_CLOSE:
        if (role == BT_A2DP_CH_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
            bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_CLOSED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
        } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
            bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_CLOSED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
        }
        return 0;

    case BT_A2DP_MEDIA_STATE_SUSPEND:
        if (role == BT_A2DP_CH_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
            bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_SUSPENDED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
        } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
            bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_SUSPENDED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
        }
        return 0;

    default:
        return -1;
    }

    return -1;
}

static void zblue_on_seted_codec(struct bt_conn* conn, struct bt_a2dp_media_codec* codec, uint8_t cp_type)
{
    uint8_t role = bt_a2dp_get_a2dp_role(conn);
    bt_address_t bd_addr;
    a2dp_event_t* event;
    a2dp_codec_config_t codec_config;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    codec_config.codec_type = zephyr_codec_2_sal_codec(codec->head.codec_type);

    switch (codec->head.codec_type) {
    case BT_A2DP_SBC:
        codec_config.sample_rate = zephyr_sbc_sample_rate_2_sal_sample_rate(codec->sbc.freq);
        codec_config.bits_per_sample = BTS_A2DP_CODEC_BITS_PER_SAMPLE_16;
        codec_config.channel_mode = zephyr_sbc_channel_mode_2_sal_channel_mode(codec->sbc.channel_mode);
        codec_config.packet_size = 1024;
        memcpy(codec_config.specific_info, &codec->sbc + sizeof(struct bt_a2dp_media_codec_head),
            sizeof(struct bt_a2dp_media_sbc_codec) - sizeof(struct bt_a2dp_media_codec_head));
        break;
    case BT_A2DP_MPEG2:
        codec_config.sample_rate = zephyr_aac_sample_rate_2_sal_sample_rate(codec->aac.freq0 << 4 | codec->aac.freq1);
        codec_config.bits_per_sample = BTS_A2DP_CODEC_BITS_PER_SAMPLE_16;
        codec_config.channel_mode = zephyr_aac_channel_mode_2_sal_channel_mode(codec->aac.channels);
        codec_config.packet_size = 1024;
        memcpy(codec_config.specific_info, &codec->aac + sizeof(struct bt_a2dp_media_codec_head),
            sizeof(struct bt_a2dp_media_aac_codec) - sizeof(struct bt_a2dp_media_codec_head));
        break;
    default:
        BT_LOGE("%s, codec not supported: 0x%x", __func__, codec->head.codec_type);
        return;
    }

    event = a2dp_event_new(CODEC_CONFIG_EVT, &bd_addr);
    event->event_data.data = malloc(sizeof(codec_config));
    memcpy(event->event_data.data, &codec_config, sizeof(codec_config));

    if (role == BT_A2DP_CH_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(event);
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(event);
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
static const uint8_t a2dp_sbc_src_codec[] = {
    0x00, /* BT_A2DP_AUDIO << 4 */
    0x00, /* BT_A2DP_SBC */
    0x23, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
    0xFF, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
    0x02, /* min bitpool */
    0x35, /* max bitpool */
};

static struct bt_a2dp_endpoint a2dp_sbc_src_endpoint = {
    .info.codec = (struct bt_a2dp_media_codec*)&a2dp_sbc_src_codec,
    .info.a2dp_cp_scms_t = 0,
    .info.a2dp_delay_report = 0,
};
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
static const uint8_t a2dp_sbc_snk_codec[] = {
    0x00, /* BT_A2DP_AUDIO << 4 */
    0x00, /* BT_A2DP_SBC */
    0x33, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
    0xFF, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
    0x02, /* min bitpool */
    0x35, /* max bitpool */
};

static struct bt_a2dp_endpoint a2dp_sbc_snk_endpoint = {
    .info.codec = (struct bt_a2dp_media_codec*)&a2dp_sbc_snk_codec,
    .info.a2dp_cp_scms_t = 0,
    .info.a2dp_delay_report = 0,
};
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
static const uint8_t a2dp_aac_src_codec[] = {
    0x00, /* BT_A2DP_AUDIO << 4 */
    0x02, /* BT_A2DP_MPEG2 */
    0x80, /* MPEG2 AAC LC | MPEG4 AAC LC | MPEG AAC LTP | MPEG4 AAC Scalable | MPEG4 HE-AAC | MPEG4 HE-AACv2 | MPEG4 HE-AAC-ELDv2 */
    0x01, /* 8000 | 11025 | 12000 | 16000 | 22050 | 24000 | 32000 | 44100 */
    0x0C, /* 48000 | 64000 | 88200 | 96000 | Channels 1 | Channels 2 | Channels 5.1 | Channels 7.1 */
#ifdef A2DP_AAC_MAX_BIT_RATE
    0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
    ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
    (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
    0xFF, /* VBR | bit rate[22:16] */
    0xFF, /* bit rate[15:8] */
    0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
};

static struct bt_a2dp_endpoint a2dp_aac_src_endpoint = {
    .info.codec = (struct bt_a2dp_media_codec*)&a2dp_aac_src_codec,
    .info.a2dp_cp_scms_t = 0,
    .info.a2dp_delay_report = 0,
};
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
static const uint8_t a2dp_aac_snk_codec[] = {
    0x00, /* BT_A2DP_AUDIO << 4 */
    0x02, /* BT_A2DP_MPEG2 */
    0x80, /* MPEG2 AAC LC | MPEG4 AAC LC | MPEG AAC LTP | MPEG4 AAC Scalable | MPEG4 HE-AAC | MPEG4 HE-AACv2 | MPEG4 HE-AAC-ELDv2 */
    0x01, /* 8000 | 11025 | 12000 | 16000 | 22050 | 24000 | 32000 | 44100 */
    0x8C, /* 48000 | 64000 | 88200 | 96000 | Channels 1 | Channels 2 | Channels 5.1 | Channels 7.1 */
#ifdef A2DP_AAC_MAX_BIT_RATE
    0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
    ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
    (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
    0xFF, /* VBR | bit rate[22:16] */
    0xFF, /* bit rate[15:8] */
    0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
};

static struct bt_a2dp_endpoint a2dp_aac_snk_endpoint = {
    .info.codec = (struct bt_a2dp_media_codec*)&a2dp_aac_snk_codec,
    .info.a2dp_cp_scms_t = 0,
    .info.a2dp_delay_report = 0,
};
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
#endif /* CONFIG_BLUETOOTH_A2DP_AAC_CODEC */

bt_status_t bt_sal_a2dp_source_init(uint8_t max_connections)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    uint8_t media_type = BT_A2DP_AUDIO;
    uint8_t role = BT_A2DP_EP_SOURCE;

    /* Mandatory support for SBC */
    SAL_CHECK_RET(bt_a2dp_register_endpoint(&a2dp_sbc_src_endpoint, media_type, role), 0);

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    /* Optional support for AAC */
    SAL_CHECK_RET(bt_a2dp_register_endpoint(&a2dp_aac_src_endpoint, media_type, role), 0);
#endif /* CONFIG_BLUETOOTH_A2DP_AAC_CODEC */

    SAL_CHECK_RET(bt_a2dp_register_cb(&a2dp_cbks), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_init(uint8_t max_connections)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    uint8_t media_type = BT_A2DP_AUDIO;
    uint8_t role = BT_A2DP_EP_SINK;

    /* Mandatory support for SBC */
    SAL_CHECK_RET(bt_a2dp_register_endpoint(&a2dp_sbc_snk_endpoint, media_type, role), 0);

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    /* Optional support for AAC */
    SAL_CHECK_RET(bt_a2dp_register_endpoint(&a2dp_aac_snk_endpoint, media_type, role), 0);
#endif /* CONFIG_BLUETOOTH_A2DP_AAC_CODEC */

    SAL_CHECK_RET(bt_a2dp_register_cb(&a2dp_cbks), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

void bt_sal_a2dp_source_cleanup(void)
{
    /* Not supported */
}

void bt_sal_a2dp_sink_cleanup(void)
{
    /* Not supported */
}

bt_status_t bt_sal_a2dp_source_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET(bt_a2dp_connect(conn, BT_A2DP_CH_SOURCE), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET(bt_a2dp_connect(conn, BT_A2DP_CH_SINK), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

bt_status_t bt_sal_a2dp_source_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET(bt_a2dp_disconnect(conn), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET(bt_a2dp_disconnect(conn), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

bt_status_t bt_sal_a2dp_source_start_stream(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET(bt_a2dp_start(conn), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_source_suspend_stream(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET(bt_a2dp_suspend(conn), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_source_send_data(bt_controller_id_t id, bt_address_t* remote_addr,
    uint8_t* buf, uint16_t nbytes, uint8_t nb_frames, uint64_t timestamp, uint32_t seq)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)remote_addr);
    uint8_t* data = buf;
    uint16_t len;

    if (!buf)
        return BT_STATUS_PARM_INVALID;

    data[0] = 0x80; /* Version 0b10, Padding 0b0, Extension 0b0, CSRC 0b0000 */
    data[1] = 0x60; /* Marker 0b0, Payload Type 0b1100000 */
    data[2] = (uint8_t)(seq >> 8);
    data[3] = (uint8_t)(seq);
    data[4] = (uint8_t)(timestamp >> 24);
    data[5] = (uint8_t)(timestamp >> 16);
    data[6] = (uint8_t)(timestamp >> 8);
    data[7] = (uint8_t)(timestamp);
    data[8] = 0x00; /* SSRC(MSB) */
    data[9] = 0x00; /* SSRC */
    data[10] = 0x00; /* SSRC */
    data[11] = 0x01; /* SSRC(LSB) */

    len = nbytes + AVDTP_RTP_HEADER_LEN;

    SAL_CHECK_RET(bt_a2dp_send_audio_data(conn, data, len), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_start_stream(bt_controller_id_t id, bt_address_t* addr)
{
    /* Note: this interface is used to accept an AVDTP Start Request */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

#endif /* CONFIG_BLUETOOTH_A2DP */
