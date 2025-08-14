/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <stdint.h>
#include <stdlib.h>

#include <nuttx/circbuf.h>

#include "sal_a2dp_source_interface.h"
#include "sal_interface.h"

#include "a2dp_codec.h"
#include "a2dp_source.h"
#include "a2dp_source_audio.h"
#include "audio_control.h"
#include "utils.h"
#define LOG_TAG "a2dp_source_audio"
#include "utils/log.h"

#include "service_loop.h"

typedef enum {
    STATE_OFF, /* idle state */
    STATE_RUNNING, /* streaming */
    STATE_SUSPENDING, /* suspend after all data is send */
} stream_state_t;

typedef struct {
    bool offloading;
    uint16_t mtu;
    stream_state_t stream_state;
    uint8_t codec_info[10];
    uint32_t interval_ms;
    service_timer_t* media_alarm;
    uint32_t sequence_number;
    uint32_t max_tx_length;
    const a2dp_source_stream_interface_t* stream_interface;
} a2dp_source_stream_t;

static a2dp_source_stream_t a2dp_src_stream = { 0 };

static void a2dp_source_audio_handle_event(void* event)
{
    a2dp_audio_event_t* audio_event = (a2dp_audio_event_t*)event;

    switch (audio_event->type) {
    case A2DP_AUDIO_EVENT_STOPPED: {
        audio_control_stop(A2DP_SOURCE_PROFILE_ID);
        break;
    }
    case A2DP_AUDIO_EVENT_START_FAIL: {
        audio_control_reset(A2DP_SOURCE_PROFILE_ID);
        break;
    }
    default:
        break;
    }
    free(audio_event);
}

static const a2dp_source_stream_interface_t* get_stream_interface(void)
{
    a2dp_codec_config_t* config;

    config = a2dp_codec_get_config();
    if (config->codec_type == BTS_A2DP_TYPE_SBC)
        return get_a2dp_source_sbc_stream_interface();
#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    else if (config->codec_type == BTS_A2DP_TYPE_MPEG2_4_AAC)
        return get_a2dp_source_aac_stream_interface();
#endif

    abort();
    return NULL;
}

static void a2dp_source_send_callback(uint8_t* buf, uint16_t nbytes, uint8_t nb_frames, uint64_t timestamp)
{
    if (buf == NULL) {
        BT_LOGE("%s, buffer is null", __func__);
        return;
    }

    bt_sal_a2dp_source_send_data(PRIMARY_ADAPTER, a2dp_source_active_peer()->bd_addr,
        buf, nbytes, nb_frames, timestamp, a2dp_src_stream.sequence_number++);
}

// Read data from compress
static int a2dp_source_read_callback(uint8_t* buf, uint16_t frame_len)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    int ret = 0;

    ret = audio_control_read(A2DP_SOURCE_PROFILE_ID, buf, frame_len);
    if (ret <= 0 && stream->stream_state == STATE_SUSPENDING) {
        BT_LOGD("suspending and no data to send, stop stream");
        stream->stream_state = STATE_OFF;
        a2dp_source_stream_stop();
        return ret;
    }

    if (ret != frame_len) {
        BT_LOGD("read ret: %d, frame_len: %d\n", ret, frame_len);
    }

    return ret;
}

static void a2dp_source_audio_handle_timer(service_timer_t* timer, void* arg)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    if (stream->stream_state == STATE_RUNNING || stream->stream_state == STATE_SUSPENDING) {
        stream->stream_interface->send_frames(STREAM_DATA_RESERVED, bt_get_os_timestamp_us());
    }
}

static void a2dp_source_stop_audio_req()
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    BT_LOGD("%s", __func__);

    stream->stream_state = STATE_OFF;
    stream->sequence_number = 0;
    if (stream->stream_interface && stream->stream_interface->reset) {
        stream->stream_interface->reset();
    }

    if (stream->media_alarm) {
        service_loop_cancel_timer(stream->media_alarm);
        stream->media_alarm = NULL;
    }

    a2dp_audio_event_t* event = (a2dp_audio_event_t*)zalloc(sizeof(a2dp_audio_event_t));
    event->type = A2DP_AUDIO_EVENT_STOPPED;
    do_in_service_loop(a2dp_source_audio_handle_event, event);
}

static void a2dp_source_suspend_audio_req(void)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    if (stream->stream_state != STATE_RUNNING) {
        if (!stream->media_alarm) {
            BT_LOGD("%s, a2dp stream has off", __func__);
            a2dp_audio_event_t* event = (a2dp_audio_event_t*)zalloc(sizeof(a2dp_audio_event_t));
            event->type = A2DP_AUDIO_EVENT_STOPPED;
            do_in_service_loop(a2dp_source_audio_handle_event, event);
            stream->stream_state = STATE_OFF;
        }

        return;
    }

    stream->stream_state = STATE_SUSPENDING;
    BT_LOGD("%s, suspending a2dp stream", __func__);
}

bool a2dp_source_is_streaming(void)
{
    return a2dp_src_stream.media_alarm ? true : false;
}

bool a2dp_source_on_connection_changed(bool connected)
{
    BT_LOGD("%s, %d", __func__, connected);
    if (!connected) {
        a2dp_source_stop_audio_req();
    }

    return true;
}

void a2dp_source_on_started(bool started)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    BT_LOGD("%s: %d", __func__, started);
    audio_control_start(A2DP_SOURCE_PROFILE_ID, started);
    if (started) {
        if (stream->offloading) {
            return;
        }

        stream->stream_state = STATE_RUNNING;
        if (stream->stream_interface)
            stream->stream_interface->reset();

        if (stream->media_alarm) {
            service_loop_cancel_timer(stream->media_alarm);
            stream->media_alarm = NULL;
        }

        stream->media_alarm = service_loop_timer(stream->interval_ms,
            stream->interval_ms,
            a2dp_source_audio_handle_timer,
            NULL);
    }
}

void a2dp_source_on_stopped(void)
{
    BT_LOGD("%s", __func__);

    a2dp_source_stop_audio_req();
}

void a2dp_source_setup_codec(bt_address_t* bd_addr)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    a2dp_codec_config_t* config;
    a2dp_peer_t* peer;

    if (stream->offloading) {
        return;
    }

    stream->stream_interface = get_stream_interface();
    if (!stream->stream_interface) {
        BT_LOGE("get_stream_interface fail");
        return;
    }

    config = a2dp_codec_get_config();
    peer = a2dp_source_find_peer(bd_addr);
    if (peer == NULL) {
        BT_LOGE("%s, can't find peer:%s", __func__, bt_addr_str(bd_addr));
        return;
    }

    stream->stream_interface->init(&config->codec_param.sbc, peer->mtu,
        a2dp_source_send_callback,
        a2dp_source_read_callback);
}

static void a2dp_source_audio_start(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_source_stream_ready()) {
        a2dp_source_stream_start();
    } else {
        BT_LOGW("%s: A2DP is not ready", __func__);
        a2dp_audio_event_t* event = (a2dp_audio_event_t*)zalloc(sizeof(a2dp_audio_event_t));
        event->type = A2DP_AUDIO_EVENT_START_FAIL;
        do_in_service_loop(a2dp_source_audio_handle_event, event);
    }
}

static void a2dp_source_audio_stop(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_src_stream.offloading) {
        BT_LOGD("No stream data to handle, stop immediately");
        a2dp_source_stream_stop();
        return;
    }

    a2dp_source_suspend_audio_req();
}

static const audio_control_callbacks_t audio_control_callbacks = {
    .size = sizeof(audio_control_callbacks_t),
    .start_cb = a2dp_source_audio_start,
    .stop_cb = a2dp_source_audio_stop,
};

void a2dp_source_audio_open(bool offloading, bt_address_t* bd_addr)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    bt_audio_config_t* config;

    if (stream->stream_state != STATE_OFF) {
        BT_LOGE("%s: stream is running, open failed", __func__);
        return;
    }

    stream->offloading = offloading;
    stream->stream_interface = get_stream_interface();
    stream->interval_ms = stream->stream_interface->get_interval_ms();
    config = a2dp_codec_create_media_config(A2DP_SOURCE_PROFILE_ID);
    if (!config) {
        BT_LOGE("%s: create media config failed", __func__);
        return;
    }
    audio_control_open(A2DP_SOURCE_PROFILE_ID, config, &audio_control_callbacks);
    a2dp_source_codec_state_change();
    a2dp_codec_delete_media_config(config);
}

uint32_t a2dp_source_get_frame_size(void)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    if (!stream->stream_interface) {
        BT_LOGE("%s, stream_interface is null", __func__);
        return 0;
    }

    return stream->stream_interface->get_min_frame_size();
}

void a2dp_source_audio_init(bool offloading)
{
    BT_LOGD("%s, offloading: %d", __func__, offloading);
}

void a2dp_source_audio_cleanup(void)
{
    a2dp_source_stop_audio_req();
    memset(&a2dp_src_stream, 0, sizeof(a2dp_src_stream));
}
