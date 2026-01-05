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

#include "a2dp_sink.h"
#include "a2dp_sink_audio.h"
#include "audio_control.h"
#include "bt_list.h"
#include "bt_utils.h"

#include "service_loop.h"

#include "a2dp_audio.h"
#define LOG_TAG "a2dp_snk_audio"
#include "utils/log.h"

#define A2DP_SINK_MEDIA_TICK_MS 20
#define A2DP_MAX_DELAY_PACKET_COUNT 5
#define A2DP_MAX_ENQUEUE_PACKET_COUNT 14
#define A2DP_ASYNC_SEND_COUNT 14

typedef enum {
    STATE_OFF,
    STATE_RUNNING,
} stream_state_t;

typedef struct {
    uint8_t codec_info[10];
    uint64_t underflow_ts;
    uint64_t last_ts;
    uint32_t block_ticks;
    stream_state_t state;
    uv_mutex_t queue_lock;
    service_timer_t* media_alarm;
    struct list_node packet_queue;
    const a2dp_sink_stream_interface_t* stream_interface;
    bool opened;
} a2dp_sink_stream_t;

static a2dp_sink_stream_t sink_stream = { 0 };

static void a2dp_sink_stop_audio_req();

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

    if (stream->state != STATE_RUNNING) {
        return;
    }

    uint64_t now_us = bt_get_os_timestamp_us();
#ifndef CONFIG_ARCH_SIM
    if (stream->last_ts && ((now_us - stream->last_ts) > 30000))
        BT_LOGD("===a2dp cpu busy time:%" PRIu64 ", buff_cnt:%zu===", now_us - stream->last_ts, list_length(&sink_stream.packet_queue));
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
        if (stream->block_ticks == 2) {
            BT_LOGD("%s ipc blocking, waiting for resume", __func__);
        }

        packet = (a2dp_sink_packet_t*)node;
        ret = audio_control_write(A2DP_SINK_PROFILE_ID, packet->data, packet->length);
        if (ret < 0) {
            stream->block_ticks++;
            goto out;
        } else if (ret < packet->length) {
            packet->is_partial = true;
            memmove(packet->data, packet->data + ret, packet->length - ret);
            packet->length -= ret;
            stream->block_ticks++;
            goto out;
        } else {
            if (stream->block_ticks > 0) {
                BT_LOGD("%s ipc blocking resume, block ticks:%" PRIu32, __func__, stream->block_ticks);
                stream->block_ticks = 0;
            }
        }

        list_delete(node);
        if (sink_stream.stream_interface)
            sink_stream.stream_interface->packet_send_done(packet);
    }
out:
    uv_mutex_unlock(&stream->queue_lock);
}

void a2dp_sink_packet_receive(a2dp_sink_packet_t* packet)
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
        a2dp_sink_packet_t* pkt = (a2dp_sink_packet_t*)list_peek_head(queue);

        if (pkt && pkt->is_partial) {
            // The list head element has already been partially sent and cannot be deleted. Delete the next packet.
            FAR struct list_node* node = list_next(queue, &pkt->node);

            if (node) {
                list_delete(node);
                free(node);
                list_add_tail(queue, &packet->node);
            } else {
                free(packet);
            }
        } else {
            free(list_remove_head(queue));
            list_add_tail(queue, &packet->node);
        }

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
    a2dp_sink_packet_t* packet = NULL;

    (void)seq;
    (void)timestamp;

    if (!sink_stream.stream_interface) {
        return NULL;
    }

    packet = sink_stream.stream_interface->repackage(data, length);

    if (!packet) {
        return NULL;
    }

    packet->is_partial = false;
    return packet;
}

// TODO: check active peer
bool a2dp_sink_on_connection_changed(bool connected)
{
    BT_LOGD("%s, %d", __func__, connected);
    return true;
}

void a2dp_sink_on_started(bool started)
{
    a2dp_sink_stream_t* stream = &sink_stream;

    BT_LOGD("%s: %d", __func__, started);
    audio_control_start(A2DP_SINK_PROFILE_ID, started);
    if (started) {
        if (stream->state != STATE_RUNNING) {
            stream->state = STATE_RUNNING;
        }
    }
}

static void a2dp_sink_audio_handle_event(void* event)
{
    a2dp_audio_event_t* audio_event = (a2dp_audio_event_t*)event;

    switch (audio_event->type) {
    case A2DP_AUDIO_EVENT_START:
        if (!a2dp_sink_stream_ready() && !a2dp_sink_stream_started()) {
            BT_LOGW("%s: can not start when sink stream is not ready", __func__);
            break;
        }

        a2dp_sink_on_started(true);
        break;
    case A2DP_AUDIO_EVENT_STOP:
        a2dp_sink_stop_audio_req();
        break;
    case A2DP_AUDIO_EVENT_STOPPED:
        audio_control_stop(A2DP_SINK_PROFILE_ID);
        break;
    default:
        break;
    }
    free(audio_event);
}

static void a2dp_sink_stop_audio_req()
{
    a2dp_sink_stream_t* stream = &sink_stream;
    a2dp_audio_event_t* event;

    BT_LOGD("%s", __func__);
    if (stream->state == STATE_OFF) {
        goto out;
    }

    stream->state = STATE_OFF;
    if (stream->media_alarm) {
        service_loop_cancel_timer(stream->media_alarm);
        stream->media_alarm = NULL;
    }

    a2dp_sink_flush_packet_queue();
    stream->underflow_ts = 0;
    stream->last_ts = 0;
    stream->block_ticks = 0;

out:
    event = (a2dp_audio_event_t*)zalloc(sizeof(a2dp_audio_event_t));
    if (!event) {
        BT_LOGE("%s: malloc event fail", __func__);
        return;
    }

    event->type = A2DP_AUDIO_EVENT_STOPPED;
    do_in_service_loop(a2dp_sink_audio_handle_event, event);
}

void a2dp_sink_on_stopped(void)
{
    a2dp_sink_stream_t* stream = &sink_stream;

    BT_LOGD("%s", __func__);

    if (stream->state == STATE_OFF)
        return;

    a2dp_sink_stop_audio_req();
}

static void a2dp_sink_audio_stop(void)
{
    a2dp_audio_event_t* event;

    BT_LOGD("%s", __func__);

    event = (a2dp_audio_event_t*)zalloc(sizeof(a2dp_audio_event_t));
    if (!event) {
        BT_LOGE("%s: malloc event fail", __func__);
        return;
    }

    event->type = A2DP_AUDIO_EVENT_STOP;
    do_in_service_loop(a2dp_sink_audio_handle_event, event);
}

static void a2dp_sink_audio_start(void)
{
    a2dp_audio_event_t* event;

    BT_LOGD("%s", __func__);

    event = (a2dp_audio_event_t*)zalloc(sizeof(a2dp_audio_event_t));
    if (!event) {
        BT_LOGE("%s: malloc event fail", __func__);
        return;
    }

    event->type = A2DP_AUDIO_EVENT_START;
    do_in_service_loop(a2dp_sink_audio_handle_event, event);
}

void a2dp_sink_setup_codec(bt_address_t* bd_addr)
{
    sink_stream.stream_interface = get_stream_interface();
    if (!sink_stream.stream_interface)
        BT_LOGE("get_stream_interface fail");
}

static audio_control_callbacks_t audio_control_callbacks = {
    .size = sizeof(audio_control_callbacks_t),
    .start_cb = a2dp_sink_audio_start,
    .stop_cb = a2dp_sink_audio_stop,
};

void a2dp_sink_audio_open(void)
{
    a2dp_sink_stream_t* stream = &sink_stream;
    bt_audio_config_t* config = NULL;

    if (stream->state != STATE_OFF) {
        BT_LOGE("%s: stream is running, open failed", __func__);
        return;
    }

    stream->stream_interface = get_stream_interface();
    uv_mutex_init(&sink_stream.queue_lock);
    list_initialize(&sink_stream.packet_queue);

    config = a2dp_codec_create_media_config(A2DP_SINK_PROFILE_ID);
    if (!config) {
        BT_LOGE("%s: create media config failed", __func__);
        return;
    }

    audio_control_open(A2DP_SINK_PROFILE_ID, config, &audio_control_callbacks);
    a2dp_sink_codec_state_change();
    stream->opened = true;
    a2dp_codec_delete_media_config(config);
}

void a2dp_sink_audio_init(void)
{
    BT_LOGD("%s", __func__);
}

void a2dp_sink_audio_cleanup(void)
{
    BT_LOGD("%s", __func__);
    a2dp_sink_stop_audio_req();
    if (!sink_stream.opened) {
        return;
    }

    uv_mutex_destroy(&sink_stream.queue_lock);
    list_delete(&sink_stream.packet_queue);
    sink_stream.opened = false;
}
