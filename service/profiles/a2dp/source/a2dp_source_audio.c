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
#include "a2dp_control.h"
#include "a2dp_source.h"
#include "a2dp_source_audio.h"

#include "audio_transport.h"
#include "utils.h"
#define LOG_TAG "a2dp_src_stream"
#include "utils/log.h"

#include "service_loop.h"

#define MAX_FRAME_NUM_PER_TICK 14
#define STREAM_DELAY_MS 10
#define STREAM_FLUSH_SIZE (1024)
#define UNDERFLOW_TICKS_TO_SUSPEND (100)
#define UNDERFLOW_TICKS_TO_FLUSH (2)
typedef enum {
    STATE_OFF, /* idle state */
    STATE_FLUSHING, /* flush remaining data */
    STATE_RUNNING, /* streaming */
    STATE_SUSPENDING, /* suspend after all data is send */
    STATE_WAIT4_SUSPENDED /* wait for stream suspended */
} stream_state_t;

typedef enum {
    UNDERFLOW_STATE_NONE = 0,
    UNDERFLOW_STATE_PAUSED,
    UNDERFLOW_STATE_RESUMING,
} underflow_state_t;

typedef struct {
    uint32_t ticks;
    underflow_state_t state;
} a2dp_source_underflow_t;

typedef struct {
    bool offloading;
    uint16_t mtu;
    stream_state_t stream_state;
    uint8_t codec_info[10];
    uint32_t interval_ms;
    service_timer_t* media_alarm;
    uint32_t sequence_number;
    uint32_t max_tx_length;
    struct circbuf_s stream_pool;
    uint8_t read_congest;
    a2dp_source_underflow_t underflow;
    const a2dp_source_stream_interface_t* stream_interface;
} a2dp_source_stream_t;

static a2dp_source_stream_t a2dp_src_stream = { 0 };
extern audio_transport_t* a2dp_transport;

static void a2dp_source_read_congest(uint8_t ch_id);

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

static void a2dp_audio_data_alloc(uint8_t ch_id, uint8_t** buffer, size_t* len)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    int space, next_to_read;
    uint8_t* alloc_buffer;

    if (stream->stream_state == STATE_FLUSHING) {
        *len = STREAM_FLUSH_SIZE;
        *buffer = (uint8_t*)malloc(STREAM_FLUSH_SIZE);
        return;
    }

    // check pool buffer space enough to read one frame
    space = circbuf_space(&stream->stream_pool);
    if (space == 0) {
        BT_LOGE("%s, no enough space to read", __func__);
        *buffer = NULL;
        return;
    }

    next_to_read = space > stream->max_tx_length ? stream->max_tx_length : space;

    alloc_buffer = (void*)malloc(next_to_read);
    if (!alloc_buffer) {
        *buffer = NULL;
        audio_transport_read_stop(a2dp_transport, ch_id);
        return;
    }

    *buffer = alloc_buffer;
    *len = next_to_read;
}

static void a2dp_audio_data_received(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    int space;

    if (buffer == NULL)
        return;

    if (len <= 0) {
        BT_LOGD("%s, status:%" PRIuPTR, __func__, len);
        if (len < 0)
            audio_transport_read_stop(a2dp_transport, ch_id);

        goto out;
    }

    space = circbuf_space(&stream->stream_pool);
    if (len > space) {
        BT_LOGE("%s, unexpected len:%" PRIuPTR, __func__, len);
        goto out;
    }

    circbuf_write(&stream->stream_pool, buffer, len);

    space = circbuf_space(&stream->stream_pool);
    if (space == 0) {
        a2dp_source_read_congest(ch_id);
    }

    if (stream->underflow.state == UNDERFLOW_STATE_PAUSED) {
        /**
         * Patch for audio channel control:
         * An extra A2DP suspened is sent due to data underflow.
         * A compensatory A2DP start is now created when data recovers.
         */
        a2dp_source_stream_start();
        stream->underflow.state = UNDERFLOW_STATE_RESUMING;
    }

out:
    free(buffer);
}

static void a2dp_audio_data_flush(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    free(buffer);
}

static void a2dp_source_start_read(void)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    if (stream->stream_state != STATE_OFF && stream->read_congest != 1)
        return;

    if (circbuf_space(&stream->stream_pool) == 0)
        return;

    stream->read_congest = 0;
    audio_transport_read_start(a2dp_transport,
        AUDIO_TRANS_CH_ID_AV_SOURCE_AUDIO,
        a2dp_audio_data_alloc,
        a2dp_audio_data_received);
}

static void a2dp_source_read_congest(uint8_t ch_id)
{
    a2dp_src_stream.read_congest = 1;
    audio_transport_read_stop(a2dp_transport, ch_id);
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

static int a2dp_source_read_callback(uint8_t* buf, uint16_t frame_len)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    uint16_t remaining_size;

    remaining_size = circbuf_used(&stream->stream_pool);
    if (remaining_size < frame_len) {
        return 0;
    }

    circbuf_read(&stream->stream_pool, buf, frame_len);

    return frame_len;
}

static void a2dp_source_audio_handle_timer(service_timer_t* timer, void* arg)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    if ((stream->stream_state != STATE_RUNNING) && (stream->stream_state != STATE_SUSPENDING))
        return;

    /* Handle stream underflow */
    if (circbuf_used(&stream->stream_pool) == 0) {
        if (!stream->underflow.ticks)
            BT_LOGD("a2dp src send frame, underflowed");

        stream->underflow.ticks++;
    } else if (stream->underflow.ticks) {
        BT_LOGD("a2dp src send frame resume, underflowed %" PRIu32 "ticks", stream->underflow.ticks);
        stream->underflow.ticks = 0;
        stream->underflow.state = UNDERFLOW_STATE_NONE;
    }

    /* Send/flush buffered data and read new data */
    switch (stream->stream_state) {
    case STATE_RUNNING:
        if ((stream->underflow.state == UNDERFLOW_STATE_NONE) && (stream->underflow.ticks > UNDERFLOW_TICKS_TO_SUSPEND)) {
            /**
             * Patch for audio channel control:
             * Audio stream may end without a control command received.
             * An A2DP suspend is created by Bluetooth service to send to the audio SNK.
             * An underflow state is marked so we can re-start the stream in the future.
             */
            a2dp_source_stream_stop();
            stream->underflow.state = UNDERFLOW_STATE_PAUSED;
            return;
        }
        stream->stream_interface->send_frames(STREAM_DATA_RESERVED, get_os_timestamp_us());
        a2dp_source_start_read();
        break;
    case STATE_SUSPENDING:
        if (stream->underflow.ticks > UNDERFLOW_TICKS_TO_FLUSH) {
            audio_transport_read_stop(a2dp_transport, AUDIO_TRANS_CH_ID_AV_SOURCE_AUDIO);
            circbuf_reset(&stream->stream_pool);
            a2dp_source_stream_stop();
            stream->stream_state = STATE_WAIT4_SUSPENDED;
            return;
        }
        stream->stream_interface->send_frames(STREAM_DATA_RESERVED, get_os_timestamp_us());
        a2dp_source_start_read();
        break;
    default:
        return;
    }
}

static void a2dp_source_start_flush(void)
{
    if (a2dp_src_stream.stream_state == STATE_OFF) {
        a2dp_src_stream.stream_state = STATE_FLUSHING;
        audio_transport_read_start(a2dp_transport,
            AUDIO_TRANS_CH_ID_AV_SOURCE_AUDIO,
            a2dp_audio_data_alloc,
            a2dp_audio_data_flush);
    }
}

static void a2dp_source_stop_flush(void)
{
    a2dp_src_stream.stream_state = STATE_OFF;
    audio_transport_read_stop(a2dp_transport, AUDIO_TRANS_CH_ID_AV_SOURCE_AUDIO);
}

static void a2dp_source_start_delay(service_timer_t* timer, void* arg)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    service_loop_cancel_timer(stream->media_alarm);
    stream->media_alarm = NULL;
    stream->media_alarm = service_loop_timer(stream->interval_ms,
        stream->interval_ms,
        a2dp_source_audio_handle_timer,
        NULL);
    if (stream->stream_interface)
        stream->stream_interface->reset();
}

static void a2dp_source_start_audio_req(void)
{
    BT_LOGD("%s", __func__);

    a2dp_source_stream_t* stream = &a2dp_src_stream;
    a2dp_peer_t* peer = a2dp_source_active_peer();

    if (!stream->stream_interface) {
        BT_LOGE("stream interface is NULL");
        return;
    }

    if (stream->stream_state == STATE_FLUSHING) {
        /* Issue: when a flushing is ongoing, there might be
         * incompleted audio frame remains in the audio channel.
         * This would lead to decoding failure at audio sink.
         */
        BT_LOGE("the previous flush is ongoing");
        a2dp_source_stop_flush();
    }

    stream->mtu = peer->mtu > 0 ? peer->mtu : MAX_2MBPS_AVDTP_MTU;
    stream->interval_ms = stream->stream_interface->get_interval_ms();
    stream->max_tx_length = stream->mtu;

    if (stream->underflow.state == UNDERFLOW_STATE_NONE) {
        stream->read_congest = 0;
        circbuf_reset(&stream->stream_pool);
        a2dp_source_start_read();
    }
    stream->underflow.ticks = 0;
    stream->underflow.state = UNDERFLOW_STATE_NONE;

    if (stream->media_alarm == NULL) { /* delay start, wait stream pool filling */
        stream->media_alarm = service_loop_timer(STREAM_DELAY_MS,
            0, a2dp_source_start_delay, NULL);
    } /* else, keep the previous timer running */

    stream->stream_state = STATE_RUNNING;
}

static void a2dp_source_stop_audio_req(bool cleanup)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    BT_LOGD("%s, remaining:%" PRIuPTR, __func__, circbuf_used(&stream->stream_pool));

    if (cleanup) { /* Audio stream is closed, no need to maintain the underflow status */
        memset(&stream->underflow, 0, sizeof(a2dp_source_underflow_t));
    }

    stream->sequence_number = 0;
    if (stream->stream_interface && stream->stream_interface->reset) {
        stream->stream_interface->reset();
    }

    stream->stream_state = STATE_OFF;
    if (stream->media_alarm) {
        service_loop_cancel_timer(stream->media_alarm);
        stream->media_alarm = NULL;
    }
}

static void a2dp_source_suspend_audio_req(void)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    BT_LOGD("%s, remaining:%" PRIuPTR, __func__, circbuf_used(&stream->stream_pool));

    if (stream->stream_state != STATE_RUNNING)
        return;

    stream->stream_state = STATE_SUSPENDING;
}

static void a2dp_source_close_audio(void)
{
    a2dp_source_stop_audio_req(true);
    audio_transport_read_stop(a2dp_transport, AUDIO_TRANS_CH_ID_AV_SOURCE_AUDIO);
}

bool a2dp_source_is_streaming(void)
{
    return a2dp_src_stream.media_alarm ? true : false;
}

bool a2dp_source_on_connection_changed(bool connected)
{
    transport_conn_state_t state;
    BT_LOGD("%s, %d", __func__, connected);

    state = a2dp_control_get_state(AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL);
    if (state != IPC_CONNTECTED) {
        return false;
    }

    a2dp_control_update_audio_config(AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL, connected ? 1 : 0);
    if (a2dp_src_stream.offloading) {
        return true;
    }

    if (!connected) {
        a2dp_source_stop_audio_req(true);
    }

    return true;
}

void a2dp_source_on_started(bool started)
{
    BT_LOGD("%s: %d", __func__, started);

    a2dp_control_event(AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL, started ? A2DP_CTRL_EVT_STARTED : A2DP_CTRL_EVT_START_FAIL);
    if (a2dp_src_stream.offloading)
        return;

    if (!started)
        return;

    if ((a2dp_src_stream.stream_state == STATE_OFF)
        || (a2dp_src_stream.stream_state == STATE_FLUSHING)
        || (a2dp_src_stream.stream_state == STATE_SUSPENDING)
        || (a2dp_src_stream.stream_state == STATE_WAIT4_SUSPENDED)) {
        a2dp_source_start_audio_req();
    }
}

void a2dp_source_on_stopped(void)
{
    BT_LOGD("%s", __func__);

    if (a2dp_src_stream.offloading) {
        return;
    }

    a2dp_source_stop_audio_req(false);
    a2dp_control_event(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL, A2DP_CTRL_EVT_STOPPED);
}

bool a2dp_source_prepare_start(void)
{
    BT_LOGD("%s", __func__);

    if (a2dp_src_stream.offloading) {
        return true;
    }

    if (a2dp_src_stream.stream_state == STATE_SUSPENDING) {
        /*
         * An A2DP_CTRL_CMD_START is received during STATE_SUSPENDING, withdraw the
         * STATE_SUSPENDING and no need to send the redundant start request.
         */
        BT_LOGD("Recover from STATE_SUSPENDING");
        a2dp_src_stream.stream_state = STATE_RUNNING;
        return false;
    }

    return true;
}

void a2dp_source_prepare_suspend(void)
{
    BT_LOGD("%s", __func__);

    if (a2dp_src_stream.offloading) {
        BT_LOGD("No stream data to handle, stop immediately");
        a2dp_source_stream_stop();
        return;
    }
    a2dp_source_suspend_audio_req();
}

void a2dp_source_setup_codec(bt_address_t* bd_addr)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    a2dp_codec_config_t* config;
    a2dp_peer_t* peer;

    if (a2dp_src_stream.offloading) {
        return;
    }

    circbuf_reset(&stream->stream_pool);
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
    a2dp_source_start_flush();
}

void a2dp_source_audio_init(bool offloading)
{
    if (!offloading) {
        a2dp_src_stream.stream_state = STATE_OFF;
        circbuf_init(&a2dp_src_stream.stream_pool, NULL, 2048);
    }

    a2dp_src_stream.offloading = offloading;
    a2dp_control_init(AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL, offloading ? AUDIO_TRANS_CH_ID_AV_INVALID : AUDIO_TRANS_CH_ID_AV_SOURCE_AUDIO);
}

void a2dp_source_audio_cleanup(void)
{
    if (!a2dp_src_stream.offloading) {
        a2dp_source_close_audio();
        circbuf_uninit(&a2dp_src_stream.stream_pool);
    }

    memset(&a2dp_src_stream, 0, sizeof(a2dp_src_stream));
    a2dp_control_ch_close(AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL, AUDIO_TRANS_CH_ID_AV_SOURCE_AUDIO);
}
