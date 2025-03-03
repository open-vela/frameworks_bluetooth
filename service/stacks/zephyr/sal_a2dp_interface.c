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
#include "bt_utils.h"
#include "sal_a2dp.h"
#include "sal_a2dp_sink_interface.h"
#include "sal_a2dp_source_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"

#include "bt_list.h"
#include "bt_utils.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_A2DP
#include "a2dp_codec.h"

#ifndef MAX
#define MAX(a,b) (((UINT32)(a) > (UINT32)(b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) (((UINT32)(a) > (UINT32)(b)) ? (b) : (a))
#endif

struct bt_a2dp_stream stream;
static struct bt_a2dp_infor {
    struct bt_a2dp* a2dp;
    struct bt_conn* conn;
    struct bt_a2dp_stream *stream;
    bt_address_t bd_addr;
    bool is_cleanup;
    bool is_channel_disc;
    bool is_signaling_connected;
    bool is_media_connected;
    bool is_int;
};
static bt_list_t* bt_a2dp_conn = NULL;
static uint8_t peer_endpoint_count = 0;
static struct bt_a2dp_codec_ie peer_sbc_capabilities[10];
static struct bt_a2dp_ep found_peer_endpoint[10];
static void zblue_on_connected(struct bt_conn* conn, struct bt_a2dp *a2dp, int err);
static void zblue_on_disconnected(struct bt_a2dp* a2dp);
static int zblue_on_config_req(struct bt_a2dp* a2dp, struct bt_a2dp_ep* ep,
    struct bt_a2dp_codec_cfg* codec_cfg, struct bt_a2dp_stream** stream,
    uint8_t* rsp_err_code);
static int zblue_on_reconfig_req(struct bt_a2dp_stream *stream, struct bt_a2dp_codec_cfg *codec_cfg,
        uint8_t *rsp_err_code);
static void zblue_on_config_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code);
static int zblue_on_establish_req(struct bt_a2dp_stream* stream, uint8_t* rsp_err_code);
static void zblue_on_establish_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code);
static int zblue_on_release_req(struct bt_a2dp_stream* stream, uint8_t* rsp_err_code);
static void zblue_on_release_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code);
static int zblue_on_start_req(struct bt_a2dp_stream* stream, uint8_t* rsp_err_code);
static void zblue_on_start_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code);
static int zblue_on_suspend_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code);
static void zblue_on_suspend_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code);

static void zblue_on_stream_configured(struct bt_a2dp_stream *stream);
static void zblue_on_stream_established(struct bt_a2dp_stream *stream);
static void zblue_on_stream_released(struct bt_a2dp_stream *stream);
static void zblue_on_stream_started(struct bt_a2dp_stream *stream);
static void zblue_on_stream_suspended(struct bt_a2dp_stream *stream);
static void zblue_on_stream_recv(struct bt_a2dp_stream *stream,
    struct net_buf *buf, uint16_t seq_num, uint32_t ts);

static struct bt_a2dp_cb a2dp_cbks = {
    .connected = zblue_on_connected,
    .disconnected = zblue_on_disconnected,
    .config_req = zblue_on_config_req,
    .reconfig_req = NULL,
    .config_rsp = zblue_on_config_rsp,
    .establish_req = zblue_on_establish_req,
    .establish_rsp = zblue_on_establish_rsp,
    .release_req = zblue_on_release_req,
    .release_rsp = zblue_on_release_rsp,
    .start_req = zblue_on_start_req,
    .start_rsp = zblue_on_start_rsp,
    .suspend_req = zblue_on_suspend_req,
    .suspend_rsp = zblue_on_suspend_rsp,
    .abort_req = NULL,
    .abort_rsp = NULL,
};

static struct bt_a2dp_stream_ops stream_ops = {
	.configured = zblue_on_stream_configured,
	.established = zblue_on_stream_established,
	.released = zblue_on_stream_released,
	.started = zblue_on_stream_started,
	.suspended = zblue_on_stream_suspended,
	.aborted = NULL,
#if defined(CONFIG_BLUETOOTH_A2DP_SINK)
	.recv = zblue_on_stream_recv,
#endif
#if defined(CONFIG_BLUETOOTH_A2DP_SOURCE)
	.sent = NULL,
#endif
};

static bool bt_a2dp_infor_find_a2dp(void* data, void* context)
{
    struct bt_a2dp_infor* a2dp_infor = (struct bt_a2dp_infor*)data;
    if (!a2dp_infor) {
        return false;
    }

    return a2dp_infor->a2dp == context;
}

static bool bt_a2dp_infor_find_conn(void* data, void* context)
{
    struct bt_a2dp_infor* a2dp_infor = (struct bt_a2dp_infor*)data;
    if (!a2dp_infor) {
        return false;
    }

    return a2dp_infor->conn == context;
}

static uint8_t bt_a2dp_get_a2dp_role(struct bt_a2dp_stream* stream)
{
	return stream->local_ep->sep.sep_info.tsep;
}

uint8_t bt_avrcp_get_a2dp_role(struct bt_conn* conn)
{
    struct bt_a2dp_infor* a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, conn);

    return bt_a2dp_get_a2dp_role(a2dp_infor->stream);
}

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

static a2dp_codec_channel_mode_t zephyr_sbc_channel_mode_2_sal_channel_mode(
    struct bt_a2dp_codec_sbc_params *sbc_codec)
{
    if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_JOINT || 
        sbc_codec->config[0] & A2DP_SBC_CH_MODE_STREO ||
        sbc_codec->config[0] & A2DP_SBC_CH_MODE_DUAL) {
        return BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
	} else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_MONO) {
		return BTS_A2DP_CODEC_CHANNEL_MODE_MONO;
	} else {
		BT_LOGW("%s, invalid channel mode", __func__);
        return BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
	}
}

static uint8_t check_local_remote_codec_sbc(uint8_t* local_ie, uint8_t* remote_ie, uint8_t* prefered_ie) {
    uint8_t bit_map = 0;
    uint8_t bit_pool_min = 0;
    uint8_t bit_pool_max = 0;

    bit_map = (local_ie[0] & 0xF0) & (remote_ie[0] & 0xF0) & (prefered_ie[0] & 0xF0);
    if (!bit_map) {
        return 0;
    }

    bit_map = (local_ie[0] & 0x0F) & (remote_ie[0] & 0x0F) & (prefered_ie[0] & 0x0F);
    if (!bit_map) {
        return 0;
    }

    bit_map = (local_ie[1] & 0xF0) & (remote_ie[1] & 0xF0) & (prefered_ie[1] & 0xF0);
    if (!bit_map) {
        return 0;
    }

    bit_map = (local_ie[1] & 0x0C) & (remote_ie[1] & 0x0C) & (prefered_ie[1] & 0x0C);
    if (!bit_map) {
        return 0;
    }

    bit_map = (local_ie[1] & 0x03) & (remote_ie[1] & 0x03) & (prefered_ie[1] & 0x03);
    if (!bit_map) {
        return 0;
    }

    bit_pool_min = MAX(local_ie[2], MIN(remote_ie[2], prefered_ie[2]));
    bit_pool_max = MIN(local_ie[3], MIN(remote_ie[3], prefered_ie[3]));
    if (bit_pool_min > bit_pool_max) {
        return 0;
    }
    return 1;
}

static uint8_t check_local_remote_codec_aac(uint8_t* local_ie, uint8_t* remote_ie, uint8_t* prefered_ie) {
    //未实现
    return 1;
}

static uint8_t find_remote_codec(struct bt_a2dp_ep *local_ep, struct bt_a2dp* a2dp, struct bt_a2dp_codec_cfg *config)
{
    uint8_t index = 0;

    uint8_t flag;
    for (; index < peer_endpoint_count; index++) {
        if (local_ep->codec_type != found_peer_endpoint[index].codec_type) {
            continue;
        }
        if (local_ep->sep.sep_info.inuse!=0 || 
            found_peer_endpoint[index].sep.sep_info.inuse!=0 || 
            local_ep->sep.sep_info.media_type != found_peer_endpoint[index].sep.sep_info.media_type ||
            local_ep->sep.sep_info.tsep == found_peer_endpoint[index].sep.sep_info.tsep) {
            continue;
        }
        if (local_ep->codec_cap->len != found_peer_endpoint[index].codec_cap->len) {
            continue;
        }
        if (local_ep->codec_type == BT_A2DP_SBC) {    //uint8_t *codec_specific_ie
            flag = check_local_remote_codec_sbc(&local_ep->codec_cap->codec_ie, 
                &found_peer_endpoint[index].codec_cap->codec_ie, &config->codec_config->codec_ie);
            if (!flag) {
                return index;
            }
        } else if (local_ep->codec_type == BT_A2DP_MPEG2) {
            flag = check_local_remote_codec_aac(&local_ep->codec_cap->codec_ie, 
                &found_peer_endpoint[index].codec_cap->codec_ie, &config->codec_config->codec_ie);
            if (!flag) {
                return index;
            }
        }
    }
    return index;
}

static uint8_t bt_a2dp_discover_endpoint_cb(struct bt_a2dp* a2dp,
    struct bt_a2dp_ep_info* info, struct bt_a2dp_ep** ep)
{
	uint8_t remote_index = 0;
    uint8_t default_index = 0;
    struct bt_a2dp_infor* a2dp_infor;

    if (info && ep) {
        found_peer_endpoint[peer_endpoint_count].codec_cap = &peer_sbc_capabilities[peer_endpoint_count];
        *ep = &found_peer_endpoint[peer_endpoint_count++];
        return BT_A2DP_DISCOVER_EP_CONTINUE;
    }

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, a2dp);
    if (!a2dp_infor) {
        BT_LOGW("a2dp not found");
        return BT_A2DP_DISCOVER_EP_STOP;
    }
    bt_a2dp_stream_cb_register(a2dp_infor->stream, &stream_ops);

    if (info->codec_type == BT_A2DP_SBC) {
        while (default_index < ARRAY_SIZE(src_sbc_cfg_default)) {
            remote_index = find_remote_codec(&a2dp_sbc_src_endpoint, a2dp, 
                            &src_sbc_cfg_default[default_index]);
            if (remote_index <= peer_endpoint_count) {
                bt_a2dp_stream_config(a2dp, a2dp_infor->stream,
                    &a2dp_sbc_src_endpoint, &found_peer_endpoint[remote_index],
                    &src_sbc_cfg_default[default_index]);
                break;
            }
            default_index++;
        }
    } else if (info->codec_type == BT_A2DP_MPEG2) {
#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
        while (default_index < ARRAY_SIZE(src_aac_cfg_default)) {
            remote_index = find_remote_codec(&a2dp_aac_src_endpoint, a2dp,
                        &src_aac_cfg_default[default_index]);
            if (remote_index <= a2dp->peer_seps_count) {
                bt_a2dp_stream_config(a2dp, &a2dp_stream,
                    &a2dp_aac_src_endpoint, &found_peer_endpoint[remote_index],
                    &src_aac_cfg_default[default_index]);
                break;
            }
            default_index++;
#endif
        }
    }
#endif

    return BT_A2DP_DISCOVER_EP_STOP;
}

static struct bt_avdtp_sep_info peer_seps[10];
struct bt_a2dp_discover_param bt_discover_param = {
	.cb = bt_a2dp_discover_endpoint_cb,
	.seps_info = &peer_seps[0], /* it save endpoint info internally. */
	.sep_count = 10,
};

static void zblue_on_connected(struct bt_conn* conn, struct bt_a2dp *a2dp, int err)
{
    struct bt_a2dp_infor* a2dp_infor;

    if (err || !a2dp) {
        BT_LOGW("a2dp connecting fail");
        return;
    }

    a2dp_infor = bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, a2dp);

    if (!a2dp_infor) {
        a2dp_infor = (struct bt_a2dp_infor*)malloc(sizeof(struct bt_a2dp_infor));
        if (!a2dp_infor) {
            BT_LOGW("malloc fail");
            return;
        }
        a2dp_infor->a2dp = a2dp;
        a2dp_infor->conn = conn;
        a2dp_infor->stream = &stream;
        a2dp_infor->is_cleanup = false;
        a2dp_infor->is_channel_disc = false;
        a2dp_infor->is_signaling_connected = true;
        a2dp_infor->is_media_connected = false;
        a2dp_infor->is_int = false;

        if (bt_sal_get_remote_address(conn, &a2dp_infor->bd_addr) != BT_STATUS_SUCCESS) {
            return;
        }
        bt_list_add_tail(bt_a2dp_conn, a2dp_infor);
    } else {
        a2dp_infor->is_signaling_connected = true;

        bt_a2dp_discover(a2dp, &bt_discover_param);
    }

}

static void zblue_on_disconnected(struct bt_a2dp *a2dp)
{
    bt_address_t bd_addr;
    struct bt_a2dp_infor* a2dp_infor;
    struct bt_conn* conn;

    if (bt_a2dp_conn) {
        a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, a2dp);
        if (!a2dp_infor) {
            BT_LOGW("a2dp disconnecting fail");
            return;
        }
        memcpy(&bd_addr, &a2dp_infor->bd_addr, sizeof(bt_address_t));

        if (a2dp_infor->is_channel_disc) {
            bt_list_remove(bt_a2dp_conn, a2dp_infor);
            if (a2dp_infor->is_cleanup && !bt_list_length(bt_a2dp_conn)) {
                bt_list_clear(bt_a2dp_conn);
                bt_a2dp_conn = NULL;
            }
        } else {
            a2dp_infor->is_channel_disc = true;
            a2dp_infor->is_signaling_connected = false;
        }
    }

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    bt_sal_a2dp_source_event_callback(a2dp_event_new(DISCONNECTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_sal_a2dp_sink_event_callback(a2dp_event_new(DISCONNECTED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

static int zblue_on_config_req(struct bt_a2dp *a2dp, struct bt_a2dp_ep *ep,
    struct bt_a2dp_codec_cfg *codec_cfg, struct bt_a2dp_stream **stream,
    uint8_t *rsp_err_code)
{
    struct bt_a2dp_infor* a2dp_infor;
    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, a2dp);
    *stream = a2dp_infor->stream; /* The a2dp_stream saved in SAL is assigned a value in zblue. */
    bt_a2dp_stream_cb_register(a2dp_infor->stream, &stream_ops);
    *rsp_err_code = 0;
    return 0;
}
static void zblue_on_config_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
    if (!rsp_err_code) {
        BT_LOGE("%s, config fail: %d", __func__, rsp_err_code);
    }
}
static int zblue_on_establish_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
    *rsp_err_code = 0;
	return 0;
}
static void zblue_on_establish_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
    if (!rsp_err_code) {
        BT_LOGE("%s, open fail: %d", __func__, rsp_err_code);
    }
}
static int zblue_on_release_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
    *rsp_err_code = 0;
    return 0;
}
static void zblue_on_release_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
    if (!rsp_err_code) {
        BT_LOGE("%s, close fail: %d", __func__, rsp_err_code);
    }
}
static int zblue_on_start_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
    *rsp_err_code = 0;
	return 0;
}
static void zblue_on_start_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
    if (!rsp_err_code) {
        BT_LOGE("%s, start fail: %d", __func__, rsp_err_code);
    }
}

static int zblue_on_suspend_req(struct bt_a2dp_stream *stream, uint8_t *rsp_err_code)
{
    *rsp_err_code = 0;
    return 0;
}
static void zblue_on_suspend_rsp(struct bt_a2dp_stream *stream, uint8_t rsp_err_code)
{
    if (!rsp_err_code) {
        BT_LOGE("%s, suspend fail: %d", __func__, rsp_err_code);
    }
}

static void zblue_on_stream_configured(struct bt_a2dp_stream *stream)
{
    uint8_t role = bt_a2dp_get_a2dp_role(stream);
    a2dp_event_t* event;
    a2dp_codec_config_t codec_config;   /* framework codec */
    struct bt_a2dp_codec_ie *codec_cfg;   /* zblue codec */
    uint8_t sample_rate, channel_mode;
    struct bt_a2dp_infor* a2dp_infor;

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    SAL_CHECK_RET(bt_a2dp_stream_establish(stream), 0);
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, stream->a2dp);

    codec_config.codec_type = zephyr_codec_2_sal_codec(stream->local_ep->codec_type);
    codec_cfg = &stream->codec_config;

    switch (stream->local_ep->codec_type) {
    case BT_A2DP_SBC:
        codec_config.sample_rate = bt_a2dp_sbc_get_sampling_frequency(
            (struct bt_a2dp_codec_sbc_params *)&codec_cfg->codec_ie[0]);
        codec_config.bits_per_sample = BTS_A2DP_CODEC_BITS_PER_SAMPLE_16;
        codec_config.channel_mode = zephyr_sbc_channel_mode_2_sal_channel_mode(
            (struct bt_a2dp_codec_sbc_params *)&codec_cfg->codec_ie[0]);
        codec_config.packet_size = 1024;
        memcpy(codec_config.specific_info, codec_cfg->codec_ie, sizeof(codec_cfg->codec_ie));
        break;
    case BT_A2DP_MPEG2:
        break;
    default:
        BT_LOGE("%s, codec not supported: 0x%x", __func__, stream->local_ep->codec_type);
        return;
    }

    event = a2dp_event_new(CODEC_CONFIG_EVT, &a2dp_infor->bd_addr);
    event->event_data.data = malloc(sizeof(codec_config));
    memcpy(event->event_data.data, &codec_config, sizeof(codec_config));

    if (role == BT_AVDTP_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(event);
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(event);
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

static void zblue_on_stream_established(struct bt_a2dp_stream *stream)
{
    uint8_t role = bt_a2dp_get_a2dp_role(stream);
    struct bt_a2dp_infor* a2dp_infor;

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, stream->a2dp);

    a2dp_infor->is_media_connected = true;

    if (role == BT_AVDTP_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(a2dp_event_new(CONNECTED_EVT, &a2dp_infor->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(CONNECTED_EVT, &a2dp_infor->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}
static void zblue_on_stream_released(struct bt_a2dp_stream *stream)
{
    uint8_t role = bt_a2dp_get_a2dp_role(stream);
    struct bt_a2dp_infor* a2dp_infor;
    bt_address_t bd_addr;

    if (bt_a2dp_conn) {
        a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, stream->a2dp);
        if (!a2dp_infor) {
            BT_LOGW("a2dp disconnect done");
            return;
        }
        memcpy(&bd_addr, &a2dp_infor->bd_addr, sizeof(bt_address_t));

        if (a2dp_infor->is_channel_disc) {
            bt_list_remove(bt_a2dp_conn, a2dp_infor);
            if (a2dp_infor->is_cleanup && !bt_list_length(bt_a2dp_conn)) {
                bt_list_clear(bt_a2dp_conn);
                bt_a2dp_conn = NULL;
            }
        } else {
            a2dp_infor->is_channel_disc = true;
            a2dp_infor->is_media_connected = false;
        }
    }

    if (role == BT_AVDTP_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
            bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_CLOSED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
        } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
            bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_CLOSED_EVT, &bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
        }
}
static void zblue_on_stream_started(struct bt_a2dp_stream *stream)
{
    uint8_t role = bt_a2dp_get_a2dp_role(stream);
    struct bt_a2dp_infor* a2dp_infor;

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, stream->a2dp);

    if (role == BT_AVDTP_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
            /* TODO: check if a2dp stream should be accepted */
            bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_STARTED_EVT, &a2dp_infor->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
        } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
            bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_STARTED_EVT, &a2dp_infor->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
        }
}

static void zblue_on_stream_suspended(struct bt_a2dp_stream *stream)
{
    uint8_t role = bt_a2dp_get_a2dp_role(stream);
    struct bt_a2dp_infor* a2dp_infor;

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, stream->a2dp);

    if (role == BT_AVDTP_SOURCE) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_SUSPENDED_EVT, &a2dp_infor->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* BT_A2DP_CH_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_SUSPENDED_EVT, &a2dp_infor->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
static void zblue_on_stream_recv(struct bt_a2dp_stream* stream,
    struct net_buf* buf, uint16_t seq_num, uint32_t ts)
{
    a2dp_event_t* event;
    a2dp_sink_packet_t* packet;
    uint8_t num_of_frames;
    uint16_t seq;
    uint32_t timestamp;
    struct bt_a2dp_infor* a2dp_infor;

    if (buf == NULL || buf->data == NULL)
        return;

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_a2dp, stream->a2dp);

    if (!buf->len) {
        BT_LOGE("%s, invalid length: %d", __func__, buf->len);
        return;
    }

    seq = seq_num;
    timestamp = ts;
    
    packet = a2dp_sink_new_packet(timestamp, seq, buf->data, buf->len);

    if (packet == NULL) {
        BT_LOGE("%s, packet malloc failed", __func__);
        return;
    }
    event = a2dp_event_new(DATA_IND_EVT, &a2dp_infor->bd_addr);
    event->event_data.packet = packet;
    bt_sal_a2dp_sink_event_callback(event);
}
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */

bt_status_t bt_sal_a2dp_source_init(uint8_t max_connections)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    uint8_t media_type = 0x00; /* BT_AVDTP_AUDIO */
    uint8_t role = 0; /* BT_AVDTP_SOURCE */

    if (!bt_a2dp_conn) {
        bt_a2dp_conn = bt_list_new(NULL);
    }
    bt_sdp_register_service(&a2dp_source_rec);

    /* Mandatory support for SBC */
    SAL_CHECK_RET(bt_a2dp_register_ep(&a2dp_sbc_src_endpoint, media_type, role), 0);

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    /* Optional support for AAC */
    SAL_CHECK_RET(bt_a2dp_register_ep(&a2dp_aac_src_endpoint, media_type, role), 0);
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
    uint8_t media_type = 0x00; /* BT_AVDTP_AUDIO */
    uint8_t role = 1; /* BT_AVDTP_SINK */
    if (!bt_a2dp_conn) {
        bt_a2dp_conn = bt_list_new(NULL);
    }
    bt_sdp_register_service(&a2dp_sink_rec);
    /* Mandatory support for SBC */
    SAL_CHECK_RET(bt_a2dp_register_ep(&a2dp_sbc_snk_endpoint, media_type, role), 0);

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    /* Optional support for AAC */
    SAL_CHECK_RET(bt_a2dp_register_ep(&a2dp_aac_snk_endpoint, media_type, role), 0);
#endif /* CONFIG_BLUETOOTH_A2DP_AAC_CODEC */

    SAL_CHECK_RET(bt_a2dp_register_cb(&a2dp_cbks), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

bt_status_t bt_sal_a2dp_source_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    if (!conn) {
        bt_conn_unref(conn);
        BT_LOGW("%s, ACL connect is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    struct bt_a2dp* a2dp = bt_a2dp_connect(conn);

    if (a2dp && !bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, conn)) {
        struct bt_a2dp_infor* a2dp_infor = (struct bt_a2dp_infor*)malloc(sizeof(struct bt_a2dp_infor));
        if (a2dp_infor) {
            memcpy(&a2dp_infor->bd_addr, addr, sizeof(bt_address_t));
            a2dp_infor->a2dp = a2dp;
            a2dp_infor->conn = conn;
            a2dp_infor->stream = &stream;
            a2dp_infor->is_cleanup = false;
            a2dp_infor->is_channel_disc = false;
            a2dp_infor->is_signaling_connected = false;
            a2dp_infor->is_media_connected = false;
            a2dp_infor->is_int = true;
            if (bt_sal_get_remote_address(conn, &a2dp_infor->bd_addr) != BT_STATUS_SUCCESS) {
                bt_conn_unref(conn);
                free(a2dp_infor);
                return BT_STATUS_FAIL;
            }
            bt_list_add_tail(bt_a2dp_conn, a2dp_infor);
        } else {
            bt_conn_unref(conn);
            BT_LOGE("%s, malloc failed", __func__);
            return BT_STATUS_NOMEM;
        }
    }

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    
    if (!conn) {
        bt_conn_unref(conn);
        BT_LOGW("%s, ACL connect is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    struct bt_a2dp* a2dp = bt_a2dp_connect(conn);

    if (a2dp && !bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, conn)) {
        struct bt_a2dp_infor* a2dp_infor = (struct bt_a2dp_infor*)malloc(sizeof(struct bt_a2dp_infor));
        if (a2dp_infor) {
            memcpy(&a2dp_infor->bd_addr, addr, sizeof(bt_address_t));
            a2dp_infor->a2dp = a2dp;
            a2dp_infor->conn = conn;
            a2dp_infor->stream = &stream;
            a2dp_infor->is_cleanup = false;
            a2dp_infor->is_channel_disc = false;
            a2dp_infor->is_signaling_connected = false;
            a2dp_infor->is_media_connected = false;
            a2dp_infor->is_int = true;
            if (bt_sal_get_remote_address(conn, &a2dp_infor->bd_addr) != BT_STATUS_SUCCESS) {
                bt_conn_unref(conn);
                free(a2dp_infor);
                return BT_STATUS_FAIL;
            }
            bt_list_add_tail(bt_a2dp_conn, a2dp_infor);
        } else {
            bt_conn_unref(conn);
            BT_LOGE("%s, malloc failed", __func__);
            return BT_STATUS_NOMEM;
        }
    }
    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

static bt_status_t bt_sal_a2dp_disconnect(struct bt_a2dp_infor* a2dp_infor, bool is_cleanup)
{
    int res_media = 0;
    int res_signaling = 0;

    if (a2dp_infor->is_media_connected && a2dp_infor->stream) {
        res_media = bt_a2dp_stream_release(a2dp_infor->stream);
    }

    if (a2dp_infor->is_signaling_connected) {
        res_signaling = bt_a2dp_disconnect(a2dp_infor->a2dp);
    }

    if ((res_media != 0 && res_signaling != 0) || (!a2dp_infor->is_media_connected && res_signaling != 0)) {
        bt_list_remove(bt_a2dp_conn, a2dp_infor);

        if (is_cleanup && !bt_list_length(bt_a2dp_conn)) {
            bt_list_clear(bt_a2dp_conn);
            bt_a2dp_conn = NULL;
        }
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_source_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    struct bt_a2dp_infor* a2dp_infor;

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, conn);

    bt_conn_unref(conn);
    if (!a2dp_infor) {
        BT_LOGW("%s, ACL connect is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    return bt_sal_a2dp_disconnect(a2dp_infor, a2dp_infor->is_cleanup);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    struct bt_a2dp_infor* a2dp_infor;

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, conn);

    bt_conn_unref(conn);
    if (!a2dp_infor) {
        BT_LOGW("%s, ACL connect is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    return bt_sal_a2dp_disconnect(a2dp_infor, a2dp_infor->is_cleanup);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

bt_status_t bt_sal_a2dp_source_start_stream(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    struct bt_a2dp_infor* a2dp_infor;

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, conn);

    bt_conn_unref(conn);

    if (a2dp_infor->is_media_connected) {
        SAL_CHECK_RET(bt_a2dp_stream_start(a2dp_infor->stream), 0);
    }

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_source_suspend_stream(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    struct bt_a2dp_infor* a2dp_infor;
    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, conn);

    bt_conn_unref(conn);

    if (a2dp_infor->is_media_connected) {
        SAL_CHECK_RET(bt_a2dp_stream_suspend(a2dp_infor->stream), 0);
    }

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_source_send_data(bt_controller_id_t id, bt_address_t* remote_addr,
    uint8_t* buf, uint16_t nbytes, uint8_t nb_frames, uint64_t timestamp, uint32_t seq)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    uint8_t* data = buf;
    uint16_t len;
    struct net_buf *media_packet_buf;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)remote_addr);
    struct bt_a2dp_infor* a2dp_infor;

    if (!buf) {
        bt_conn_unref(conn);
        return BT_STATUS_PARM_INVALID;
    }

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

    /* The media packet header has 12 bytes mandatory field */
    /* media_packet_len = media_packet_header_len + media_payload_len */
    len = nbytes + AVDTP_RTP_HEADER_LEN;
    media_packet_buf = bt_l2cap_create_pdu(NULL, 0);
	if (!media_packet_buf) {
        bt_conn_unref(conn);
		return -ENOMEM;
	}
    net_buf_add_mem(media_packet_buf, data, len);

    a2dp_infor = (struct bt_a2dp_infor*)bt_list_find(bt_a2dp_conn, bt_a2dp_infor_find_conn, conn);

    bt_conn_unref(conn);
    
    SAL_CHECK_RET(bt_a2dp_stream_send(a2dp_infor->stream, media_packet_buf, seq, timestamp), 0);

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

void bt_sal_a2dp_source_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    bt_list_t* list = bt_a2dp_conn;
    bt_list_node_t* node;

    bt_sdp_unregister_service(&a2dp_source_rec);

    if (!bt_list_length(list)) {
        bt_list_free(bt_a2dp_conn);
        bt_a2dp_conn = NULL;
        return;
    }

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        struct bt_a2dp_infor* a2dp_infor = bt_list_node(node);

        if (bt_a2dp_get_a2dp_role(a2dp_infor->stream) != BT_AVDTP_SOURCE) {
            continue;
        }

        a2dp_infor->is_cleanup = true;

        bt_sal_a2dp_disconnect(a2dp_infor, a2dp_infor->is_cleanup);
    }

    return;
#else
    return;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

//失败的话，移除conn list
void bt_sal_a2dp_sink_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_list_t* list = bt_a2dp_conn;
    bt_list_node_t* node;

    bt_sdp_unregister_service(&a2dp_sink_rec);

    if (!bt_list_length(list)) {
        bt_list_free(bt_a2dp_conn);
        bt_a2dp_conn = NULL;
        return;
    }

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        struct bt_a2dp_infor* a2dp_infor = bt_list_node(node);

        if (bt_a2dp_get_a2dp_role(a2dp_infor->stream) != BT_AVDTP_SINK) {
            continue;
        }

        a2dp_infor->is_cleanup = true;

        bt_sal_a2dp_disconnect(a2dp_infor, a2dp_infor->is_cleanup);
    }

    return;
#else
    return;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

#endif /* CONFIG_BLUETOOTH_A2DP */continue;