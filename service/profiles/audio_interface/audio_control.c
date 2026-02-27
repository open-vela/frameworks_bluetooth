
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
#define LOG_TAG "audio_control"
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "service_loop.h"

#include "a2dp_codec.h"
#include "audio_codec.h"
#include "audio_control.h"
#include "bt_utils.h"
#include "utils/log.h"

#ifdef CONFIG_AUDIOUTILS_TINYCOMPRESS
#include "include/audio_transport.h"
#include "nuttx/audio/audio.h"
#include "poll.h"
#include "sound/compress_params.h"
#include "tinycompress/tinycompress.h"

#define A2DP_MEDIA_FRAGMENTS 4

#define AUDIO_CTRL_MSG_NONE 0
#define AUDIO_CTRL_MSG_START 2
#define AUDIO_CTRL_MSG_STOP 3
#define AUDIO_CTRL_MSG_PAUSE 4
#define AUDIO_CTRL_MSG_RESUME 5
// TODO:
// In the short term, Media using AUDIO_MSG_IOERR to notify Bluetooth compress that it has terminated is acceptable.
// However, in the long term, using AUDIO_MSG_IOERR as an error code to represent a correct termination process is not entirely as expected.
// A unique message should be used to respond to "unavailable".
#define AUDIO_CTRL_MSG_IOERR 13

static const char* audio_transport_device[] = {
    CONFIG_BLUETOOTH_A2DP_SOURCE_DEVICE,
    CONFIG_BLUETOOTH_A2DP_SINK_DEVICE
};

typedef struct {
    uint8_t profile_id;
    service_poll_t* poll;
    struct compress* cps;
    uint8_t cmd_flag;
    audio_control_callbacks_t* callbacks;
    bool is_stopping;
} bt_audio_t;

static bt_list_t* g_audio_list = NULL;

static bool audio_control_cmp_compress(void* audio, void* cps)
{
    return ((bt_audio_t*)audio)->cps == (struct compress*)cps;
}

static bool audio_control_cmp_profile(void* audio, void* profile_id)
{
    return ((bt_audio_t*)audio)->profile_id == *(uint8_t*)profile_id;
}

static bt_audio_t* audio_control_get_audio_by_profile_id(uint8_t profile_id)
{
    if (!g_audio_list) {
        return NULL;
    }

    return bt_list_find(g_audio_list, audio_control_cmp_profile, &profile_id);
}

static bt_audio_t* audio_control_get_audio_by_compress(void* compress)
{
    if (!g_audio_list) {
        return NULL;
    }

    return bt_list_find(g_audio_list, audio_control_cmp_compress, compress);
}

static char* audio_ctrl_msg_dump(int event)
{
    switch (event) {
        CASE_RETURN_STR(AUDIO_CTRL_MSG_START);
        CASE_RETURN_STR(AUDIO_CTRL_MSG_STOP);
        CASE_RETURN_STR(AUDIO_CTRL_MSG_PAUSE);
        CASE_RETURN_STR(AUDIO_CTRL_MSG_RESUME);
        CASE_RETURN_STR(AUDIO_CTRL_MSG_IOERR);
    default:
        return "UNKNOWN_EVENT";
        break;
    }
}

static void audio_ctrl_callback(FAR void* cookie, int event, const FAR void* extra)
{
    BT_LOGD("%s: event %s", __func__, audio_ctrl_msg_dump(event));
    bt_audio_t* audio = (bt_audio_t*)cookie;

    if (!audio || audio != audio_control_get_audio_by_compress(audio->cps)) {
        BT_LOGE("audio is NULL or not match");
        return;
    }

    if (!audio->callbacks) {
        BT_LOGE("callbacks is NULL");
        return;
    }

    if (audio->cmd_flag != AUDIO_CTRL_MSG_NONE) {
        BT_LOGE("a2dp is excuting %d", audio->cmd_flag);
        return;
    }

    switch (event) {
    case AUDIO_CTRL_MSG_START:
        audio->cmd_flag = AUDIO_CTRL_MSG_START;
        audio->callbacks->start_cb();
        break;
    case AUDIO_CTRL_MSG_RESUME:
        audio->cmd_flag = AUDIO_CTRL_MSG_RESUME;
        audio->callbacks->start_cb();
        break;
    case AUDIO_CTRL_MSG_STOP:
        audio->cmd_flag = AUDIO_CTRL_MSG_STOP;
        audio->callbacks->stop_cb();
        break;
    case AUDIO_CTRL_MSG_IOERR:
        audio->cmd_flag = AUDIO_CTRL_MSG_IOERR;
        audio->callbacks->stop_cb();
        break;
    case AUDIO_CTRL_MSG_PAUSE:
        audio->cmd_flag = AUDIO_CTRL_MSG_PAUSE;
        audio->callbacks->stop_cb();
        break;
    default:
        break;
    }
}

static uint8_t audio_control_codec_id_to_media(uint8_t codec_id)
{
    uint8_t ret = 0;

    switch (codec_id) {
    case BTS_A2DP_TYPE_SBC:
        ret = AUDIO_FMT_SBC;
        break;
    case BTS_A2DP_TYPE_MPEG2_4_AAC:
        ret = AUDIO_FMT_AAC;
        break;
    default:
        ret = AUDIO_FMT_UNDEF;
    }

    BT_LOGD("media codec id:%d", ret);
    return ret;
}

static void audio_control_dump_config(const struct compr_config* config)
{
    if (!config) {
        BT_LOGE("%s, config is NULL", __func__);
        return;
    }

    BT_LOGD("%s, fragment size: %" PRId32, __func__, config->fragment_size);
    BT_LOGD("%s, fragments: %" PRId32, __func__, config->fragments);
    BT_LOGD("%s, codec: %" PRId32, __func__, config->codec->id);
    BT_LOGD("%s, ch_in: %" PRId32, __func__, config->codec->ch_in);
    BT_LOGD("%s, sample_rate: %" PRId32, __func__, config->codec->sample_rate);
    BT_LOGD("%s, bit_rate: %" PRId32, __func__, config->codec->bit_rate);
    BT_LOGD("%s, channel_mode: %" PRId32, __func__, config->codec->ch_mode);
    BT_LOGD("%s, blocks: %d", __func__, config->codec->options.sbc.blocks);
    BT_LOGD("%s, subbands: %d", __func__, config->codec->options.sbc.subbands);
    BT_LOGD("%s, alloc_method: %d", __func__, config->codec->options.sbc.alloc_method);
    BT_LOGD("%s, bitpool: %d", __func__, config->codec->options.sbc.bitpool);
    BT_LOGD("%s, format: %" PRId32 "", __func__, config->codec->format);
    BT_LOGD("%s, pcm_format: %" PRId32 "", __func__, config->codec->pcm_format);
}

static struct compr_config* audio_control_media_config_create(const bt_audio_config_t* bt_config)
{
    struct compr_config* config = (struct compr_config*)zalloc(sizeof(struct compr_config));

    if (!config) {
        BT_LOGE("%s, error config", __func__);
        return NULL;
    }

    config->fragment_size = bt_config->fragment_size;
    config->fragments = bt_config->fragments;
    config->codec = (struct snd_codec*)zalloc(sizeof(struct snd_codec));
    config->codec->id = audio_control_codec_id_to_media(bt_config->codec->id);
    config->codec->sample_rate = bt_config->codec->sample_rate;
    config->codec->bit_rate = bt_config->codec->bit_rate;
    config->codec->ch_in = bt_config->codec->ch_in;
    config->codec->format = bt_config->codec->format;
    config->codec->ch_mode = bt_config->codec->ch_mode;
    config->codec->pcm_format = bt_config->codec->pcm_format;
    memcpy(&config->codec->options, &bt_config->codec->options, sizeof(config->codec->options));

    audio_control_dump_config(config);
    return config;
}

static void audio_control_media_config_destory(struct compr_config* config)
{
    if (!config) {
        return;
    }

    if (config->codec != NULL) {
        free(config->codec);
        config->codec = NULL;
    }

    free(config);
    config = NULL;
}

static void audio_handle_poll(service_poll_t* poll, int revent, void* cps)
{
    if (revent & POLL_ERROR || revent & POLL_DISCONNECT) {
        service_loop_remove_poll(poll);
    } else if (revent & POLL_READABLE) {
        if (!cps) {
            return;
        }

        bt_audio_t* audio = audio_control_get_audio_by_compress(cps);

        if (!audio || !audio->cps) {
            BT_LOGE("%s, audio is NULL", __func__);
            return;
        }

        audio_transport_poll_available(cps);
    }
}

#endif /* CONFIG_AUDIOUTILS_TINYCOMPRESS */

void audio_control_open(uint8_t profile_id, const bt_audio_config_t* bt_config, const void* cb)
{
#ifdef CONFIG_AUDIOUTILS_TINYCOMPRESS
    struct compress* cps = NULL;
    struct compr_config* config = NULL;
    struct pollfd poll_fd;
    uint32_t flags = 0;
    service_poll_t* poll = NULL;
    bt_audio_t* audio;

    audio = audio_control_get_audio_by_profile_id(profile_id);

    if (audio) {
        BT_LOGE("%s, stream has opened", __func__);
        return;
    }

    switch (profile_id) {
    case A2DP_SINK_PROFILE_ID:
        flags = COMPRESS_IN;
        break;
    case A2DP_SOURCE_PROFILE_ID:
        flags = COMPRESS_OUT;
        break;
    default:
        BT_LOGE("%s, error profile_id: %d", __func__, profile_id);
        return;
    }

    config = audio_control_media_config_create(bt_config);
    if (!config) {
        BT_LOGE("%s, config is null", __func__);
        return;
    }

    cps = audio_transport_open(audio_transport_device[profile_id], flags, config);

    audio_control_media_config_destory(config);

    if (!cps) {
        BT_LOGE("%s: open compress failed", __func__);
        return;
    }

    BT_LOGD("%s: open compress success for %s\n", __func__, audio_transport_device[profile_id]);

    audio = zalloc(sizeof(bt_audio_t));
    if (!audio) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    audio->cps = cps;
    audio->callbacks = (audio_control_callbacks_t*)cb;
    audio_transport_nonblock(cps, 1);
    audio_transport_set_event(cps, audio, audio_ctrl_callback);
    poll_fd.fd = audio_transport_get_file_descriptor(cps);
    poll_fd.events = POLLIN;

    poll = service_loop_poll_fd(poll_fd.fd, POLL_READABLE, audio_handle_poll, cps);

    if (!poll) {
        BT_LOGE("%s: add poll failed", __func__);
        free(audio);
        return;
    }

    audio->poll = poll;
    audio->profile_id = profile_id;

    if (!g_audio_list) {
        g_audio_list = bt_list_new(NULL);
    }

    bt_list_add_tail(g_audio_list, audio);

    return;
#endif /* CONFIG_AUDIOUTILS_TINYCOMPRESS */
}

int audio_control_read(uint8_t profile_id, void* buf, unsigned int size)
{
#ifdef CONFIG_AUDIOUTILS_TINYCOMPRESS
    bt_audio_t* audio = audio_control_get_audio_by_profile_id(profile_id);
    int ret = -1;
    if (!audio) {
        return ret;
    }

    if (!audio->cps) {
        BT_LOGE("compress is NULL");
        return ret;
    }

    ret = audio_transport_read(audio->cps, buf, size);
    return ret;
#else
    return -1;
#endif /* CONFIG_AUDIOUTILS_TINYCOMPRESS */
}

int audio_control_write(uint8_t profile_id, const void* buf, unsigned int size)
{
#ifdef CONFIG_AUDIOUTILS_TINYCOMPRESS
    bt_audio_t* audio = audio_control_get_audio_by_profile_id(profile_id);
    int ret = -1;

    if (!audio) {
        return ret;
    }

    if (!audio->cps) {
        BT_LOGE("compress is NULL");
        return ret;
    }

    ret = audio_transport_write(audio->cps, buf, size);
    return ret;
#else
    return -1;
#endif /* CONFIG_AUDIOUTILS_TINYCOMPRESS */
}

void audio_control_cleanup(uint8_t profile_id)
{
#ifdef CONFIG_AUDIOUTILS_TINYCOMPRESS
    BT_LOGD("%s cleanup profile %d", __func__, profile_id);
    bt_audio_t* audio = audio_control_get_audio_by_profile_id(profile_id);

    if (!audio) {
        return;
    }

    BT_LOGD("%s, remove compress %p, poll %p", __func__, audio->cps, audio->poll);

    audio->cps = NULL;
    service_loop_remove_poll(audio->poll);
    bt_list_remove(g_audio_list, audio);
#endif /* CONFIG_AUDIOUTILS_TINYCOMPRESS */
}

void audio_control_start(uint8_t profile_id, bool started)
{
#ifdef CONFIG_AUDIOUTILS_TINYCOMPRESS
    BT_LOGD("%s profile %d", __func__, profile_id);
    bt_audio_t* audio = audio_control_get_audio_by_profile_id(profile_id);

    if (!audio) {
        BT_LOGD("%s, stream has not open", __func__);
        return;
    }

    if (!audio->cps) {
        BT_LOGE("compress is NULL");
        return;
    }

    if (!started) {
        BT_LOGD("%s, start fail", __func__);
        audio->cmd_flag = AUDIO_CTRL_MSG_NONE;
        return;
    }

    if (audio->cmd_flag == AUDIO_CTRL_MSG_START) {
        BT_LOGD("%s, start audio", __func__);
        audio_transport_start(audio->cps);
        audio->cmd_flag = AUDIO_CTRL_MSG_NONE;
    } else if (audio->cmd_flag == AUDIO_CTRL_MSG_RESUME) {
        BT_LOGD("%s, resume audio", __func__);
        audio_transport_resume(audio->cps);
        audio->cmd_flag = AUDIO_CTRL_MSG_NONE;
    } else {
        BT_LOGD("not in start or resume state, abort it");
    }

    audio->cmd_flag = AUDIO_CTRL_MSG_NONE;
#endif /* CONFIG_AUDIOUTILS_TINYCOMPRESS */
}

void audio_control_stop(uint8_t profile_id)
{
#ifdef CONFIG_AUDIOUTILS_TINYCOMPRESS
    BT_LOGD("%s profile %d", __func__, profile_id);
    bt_audio_t* audio = audio_control_get_audio_by_profile_id(profile_id);

    if (!audio) {
        BT_LOGD("%s, stream has not open", __func__);
        return;
    }

    if (!audio->cps) {
        BT_LOGE("compress is NULL");
        return;
    }

    if (audio->cmd_flag == AUDIO_CTRL_MSG_STOP) {
        BT_LOGD("%s, stop audio", __func__);
        service_loop_remove_poll(audio->poll);
        audio_transport_close(audio->cps);
        audio->cps = NULL;
        bt_list_remove(g_audio_list, audio);
        BT_LOGD("%s, stopped audio", __func__);
    } else if (audio->cmd_flag == AUDIO_CTRL_MSG_IOERR) {
        BT_LOGD("%s, reset audio", __func__);
        service_loop_remove_poll(audio->poll);
        audio->cps = NULL;
        bt_list_remove(g_audio_list, audio);
    } else if (audio->cmd_flag == AUDIO_CTRL_MSG_PAUSE) {
        BT_LOGD("%s, pause audio", __func__);
        audio_transport_pause(audio->cps);
    } else {
        BT_LOGD("not in stop or pause state, abort it");
    }

    audio->cmd_flag = AUDIO_CTRL_MSG_NONE;
#endif /* CONFIG_AUDIOUTILS_TINYCOMPRESS */
}

void audio_control_reset(uint8_t profile_id)
{
#ifdef CONFIG_AUDIOUTILS_TINYCOMPRESS
    BT_LOGD("%s profile %d", __func__, profile_id);
    bt_audio_t* audio = audio_control_get_audio_by_profile_id(profile_id);
    int ret = 0;
    uint8_t buf[1024];

    if (!audio) {
        BT_LOGD("%s, stream has not open", __func__);
        return;
    }

    if (!audio->cps) {
        BT_LOGE("compress is NULL");
        return;
    }

    do {
        ret = audio_control_read(A2DP_SOURCE_PROFILE_ID, buf, 1024);
    } while (ret > 0);

    if (audio->cmd_flag == AUDIO_CTRL_MSG_START) {
        BT_LOGD("%s, start fail", __func__);
        audio_transport_reset(audio->cps);
    } else {
        BT_LOGD("not in start state, abort it");
    }

    audio->cmd_flag = AUDIO_CTRL_MSG_NONE;
#endif /* CONFIG_AUDIOUTILS_TINYCOMPRESS */
}