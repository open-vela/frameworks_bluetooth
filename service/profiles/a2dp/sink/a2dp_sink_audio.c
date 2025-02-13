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
#include <stdio.h>
#include <stdlib.h>

#include "a2dp_control.h"
#include "a2dp_sink.h"
#include "a2dp_sink_audio.h"
#include "audio_transport.h"
#include "bt_list.h"
#include "bt_utils.h"

#include "service_loop.h"

#define LOG_TAG "a2dp_snk_stream"
#include "utils/log.h"

#define A2DP_SINK_MEDIA_TICK_MS 20
#define A2DP_MAX_DELAY_PACKET_COUNT 5
#define A2DP_MAX_ENQUEUE_PACKET_COUNT 14
#define A2DP_ASYNC_SEND_COUNT 14

typedef enum {
    STATE_OFF,
    STATE_FLUSHING,
    STATE_RUNNING,
    STATE_SUSPENDING,
    STATE_WAIT4_SUSPENDED,
} stream_state_t;

typedef struct {
    uint8_t codec_info[10];
    uint8_t packet_sending_cnt;
    uint64_t underflow_ts;
    uint64_t last_ts;
    uint32_t block_ticks;
    bool ready;
    stream_state_t state;
    uv_mutex_t queue_lock;
    service_timer_t* media_alarm;
    struct list_node packet_queue;
    const a2dp_sink_stream_interface_t* stream_interface;
} a2dp_sink_stream_t;

extern audio_transport_t* a2dp_transport;
a2dp_sink_stream_t sink_stream = { 0 };

static const a2dp_sink_stream_interface_t* get_stream_interface(void)
{
    a2dp_codec_config_t* config;

    config = a2dp_codec_get_config();
    if (config->codec_type == BTS_A2DP_TYPE_SBC)
        return get_a2dp_sink_sbc_stream_interface();
#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    else if (config->codec_type == BTS_A2DP_TYPE_MPEG2_4_AAC)
        return get_a2dp_sink_aac_stream_interface();
#endif

    abort();
    return NULL;
}

static void a2dp_sink_write_done(uint8_t ch_id, uint8_t* buffer)
{
    a2dp_sink_stream_t* stream = &sink_stream;

    if (stream->packet_sending_cnt > 0)
        stream->packet_sending_cnt--;
}

static void a2dp_sink_flush_packet_queue(void)
{
    struct list_node *node, *tmp;

    list_for_every_safe(&sink_stream.packet_queue, node, tmp)
    {
        list_delete(node);
        free(node);
    }
}

static void a2dp_sink_audio_handle_timer(service_timer_t* timer, void* arg)
{
    a2dp_sink_stream_t* stream = &sink_stream;
    struct list_node* queue = &stream->packet_queue;
    a2dp_sink_packet_t* packet = NULL;
    struct list_node *node, *tmp;
    int ret;

    uint64_t now_us = get_os_timestamp_us();
#ifndef CONFIG_ARCH_SIM
    if (stream->last_ts && ((now_us - stream->last_ts) > 30000))
        BT_LOGD("===a2dp cpu busy time:%lld, buff_cnt:%d===", now_us - stream->last_ts, list_length(&sink_stream.packet_queue));
    stream->last_ts = now_us;
#endif

    uv_mutex_lock(&stream->queue_lock);
    if (list_is_empty(queue) == true) {
        if (!stream->underflow_ts)
            stream->underflow_ts = now_us;
        goto out;
    }

    if (stream->underflow_ts) {
        uint64_t miss_tick = (now_us - stream->underflow_ts) / (uint64_t)(A2DP_SINK_MEDIA_TICK_MS * 1000);
        if (miss_tick > 2)
            BT_LOGD("%s underflow, miss ticks: %" PRIu64, __func__, miss_tick);
        stream->underflow_ts = 0;
    }

    list_for_every_safe(queue, node, tmp)
    {
        if (stream->packet_sending_cnt == A2DP_ASYNC_SEND_COUNT) {
            if (stream->block_ticks++ > 2)
                BT_LOGD("%s ipc blocking, block ticks:%" PRIu32, __func__, stream->block_ticks);
            goto out;
        }

        stream->block_ticks = 0;
        packet = (a2dp_sink_packet_t*)node;
        ret = audio_transport_write(a2dp_transport,
            AUDIO_TRANS_CH_ID_AV_SINK_AUDIO,
            packet->data, packet->length,
            a2dp_sink_write_done);
        if (ret != 0) {
            BT_LOGE("%s, packet write failed", __func__);
            goto out;
        }

        stream->packet_sending_cnt++;
        list_delete(node);
        if (sink_stream.stream_interface)
            sink_stream.stream_interface->packet_send_done(packet);
    }

out:
    uv_mutex_unlock(&stream->queue_lock);
}

void a2dp_sink_packet_recieve(a2dp_sink_packet_t* packet)
{
    a2dp_sink_stream_t* stream = &sink_stream;
    struct list_node* queue = &stream->packet_queue;

    if (packet == NULL)
        return;

    if (stream->state != STATE_RUNNING) {
        free(packet);
        return;
    }

    uv_mutex_lock(&stream->queue_lock);
    if (list_length(queue) == A2DP_MAX_ENQUEUE_PACKET_COUNT) {
        BT_LOGD("%s queue is full, drop head packet", __func__);
        struct list_node* pkt = list_remove_head(queue);
        free(pkt);
        list_add_tail(queue, &packet->node);
        uv_mutex_unlock(&stream->queue_lock);
        return;
    }

    list_add_tail(queue, &packet->node);
    if (list_length(queue) >= A2DP_MAX_DELAY_PACKET_COUNT && !stream->media_alarm) {
        BT_LOGD("%s start trans packet", __func__);
        stream->underflow_ts = 0;
        stream->last_ts = 0;
        stream->block_ticks = 0;
        sink_stream.media_alarm = service_loop_timer(10,
            A2DP_SINK_MEDIA_TICK_MS, a2dp_sink_audio_handle_timer, NULL);
        if (sink_stream.media_alarm == NULL)
            BT_LOGE("%s, media_alarm start error", __func__);
    }
    uv_mutex_unlock(&stream->queue_lock);
}

a2dp_sink_packet_t* a2dp_sink_new_packet(uint32_t timestamp, uint16_t seq, uint8_t* data, uint16_t length)
{
    (void)seq;
    (void)timestamp;

    if (sink_stream.stream_interface)
        return sink_stream.stream_interface->repackage(data, length);
    else
        return NULL;
}

// TODO: check active peer
bool a2dp_sink_on_connection_changed(bool connected)
{
    transport_conn_state_t state;

    BT_LOGD("%s, %d", __func__, connected);
    state = a2dp_control_get_state(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL);
    if (state != IPC_CONNTECTED) {
        return false;
    }

    if (connected) {
        a2dp_control_update_audio_config(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL, 1);
    } else {
        /* When disconnected, the Media framework should send AUDIO_CTRL_CMD_STOP to notify us that
        it is no longer ready to receive data via audio data channel. However, due to a logic issue,
        the media currently cannot send AUDIO_CTRL_CMD_STOP upon disconnection. As a workaround,
        we are proactively setting sink_stream.ready to false in such scenario.  */
        sink_stream.ready = false;
        a2dp_sink_on_stopped();
        a2dp_control_update_audio_config(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SINK_CTRL, 0);
    }

    return true;
}

void a2dp_sink_on_started(bool started)
{
    BT_LOGD("%s: %d", __func__, started);
    if (!sink_stream.ready)
        return;

    if (sink_stream.state != STATE_RUNNING) {
        sink_stream.state = STATE_RUNNING;
        a2dp_sink_flush_packet_queue();
    }
}

void a2dp_sink_on_stopped(void)
{
    a2dp_sink_stream_t* stream = &sink_stream;

    BT_LOGD("%s", __func__);

    if (sink_stream.state == STATE_OFF)
        return;

    service_loop_cancel_timer(stream->media_alarm);
    stream->media_alarm = NULL;
    a2dp_sink_flush_packet_queue();
    stream->state = STATE_OFF;
}

void a2dp_sink_prepare_suspend(void)
{
    BT_LOGD("%s", __func__);
    a2dp_sink_on_stopped(); /* stop and suspend act the same at audio sink*/
}

void a2dp_sink_mute(void)
{
    BT_LOGD("%s", __func__);

    sink_stream.ready = false;
    a2dp_sink_prepare_suspend();
}

void a2dp_sink_resume(void)
{
    BT_LOGD("%s", __func__);

    sink_stream.ready = true;
    a2dp_sink_on_started(true);
}

void a2dp_sink_setup_codec(bt_address_t* bd_addr)
{
    sink_stream.stream_interface = get_stream_interface();
    if (!sink_stream.stream_interface)
        BT_LOGE("get_stream_interface fail");
}

void a2dp_sink_audio_init(void)
{
    sink_stream.media_alarm = NULL;
    sink_stream.state = STATE_OFF;
    uv_mutex_init(&sink_stream.queue_lock);
    list_initialize(&sink_stream.packet_queue);
    a2dp_control_init(AUDIO_TRANS_CH_ID_AV_SINK_CTRL, AUDIO_TRANS_CH_ID_AV_SINK_AUDIO);
}

void a2dp_sink_audio_cleanup(void)
{
    a2dp_sink_on_stopped();
    a2dp_sink_flush_packet_queue();
    uv_mutex_destroy(&sink_stream.queue_lock);
    list_delete(&sink_stream.packet_queue);
    a2dp_control_ch_close(AUDIO_TRANS_CH_ID_AV_SINK_CTRL, AUDIO_TRANS_CH_ID_AV_SINK_AUDIO);
}
