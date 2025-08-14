/****************************************************************************
 *
 *   Copyright (C) 2025 Xiaomi InC. All rights reserved.
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
#define LOG_TAG "hfp_hf_stream"
#include <stdint.h>
#include <stdlib.h>

#include <nuttx/circbuf.h>

#include "audio_codec.h"
#include "audio_control.h"
#include "bt_utils.h"
#include "hfp_define.h"
#include "hfp_hf_service.h"
#include "service_loop.h"
#include "utils.h"
#include "utils/log.h"
typedef struct {
    bt_address_t addr;
    bool offloading;
} hfp_hf_stream_t;

static hfp_hf_stream_t hfp_hf_stream = { 0 };

void hfp_hf_audio_init() { }

void hfp_hf_on_stopped()
{
    BT_LOGD("%s", __func__);
    hfp_hf_stream_t* stream = &hfp_hf_stream;

    if (!stream->offloading) {
        return;
    }

    audio_control_stop(HFP_HF_PROFILE_ID);
}

void hfp_hf_on_started(void)
{
    BT_LOGD("%s", __func__);
    hfp_hf_stream_t* stream = &hfp_hf_stream;

    if (!stream->offloading) {
        return;
    }

    audio_control_start(HFP_HF_PROFILE_ID, true);
}

static bt_audio_config_t* hfp_hf_media_config_create(uint8_t codec)
{
    bt_audio_config_t* config;

    config = (bt_audio_config_t*)zalloc(sizeof(bt_audio_config_t));

    if (!config) {
        BT_LOGE("%s: allocate memory failed", __func__);
        return NULL;
    }

    config->codec = (bt_audio_codec_t*)zalloc(sizeof(bt_audio_codec_t));
    if (!config->codec) {
        BT_LOGE("%s: allocate memory failed", __func__);
        free(config);
        return NULL;
    }

    config->codec->id = codec;
    config->fragments = 0;
    config->fragment_size = 0;
    if (codec == HFP_CODEC_MSBC) {
        config->codec->sample_rate = 8000;
        config->codec->format = 0; // TODO:add format
    } else if (codec == HFP_CODEC_CVSD) {
        config->codec->sample_rate = 16000;
        config->codec->format = 0; // TODO:add format
    } else {
        // TODO: other codec
    }

    return config;
}

static void hfp_hf_media_config_destroy(bt_audio_config_t* config)
{
    if (!config) {
        return;
    }

    if (config->codec) {
        free(config->codec);
        config->codec = NULL;
    }

    free(config);
    config = NULL;
}

static void hfp_hf_audio_start(void)
{
    BT_LOGD("%s", __func__);
    hfp_hf_on_sco_start();
}

static void hfp_hf_audio_stop(void)
{
    BT_LOGD("%s", __func__);
    if (!hfp_hf_on_sco_stop()) {
        hfp_hf_on_stopped();
    }
}

static audio_control_callbacks_t audio_control_callbacks = {
    .size = sizeof(audio_control_callbacks_t),
    .start_cb = hfp_hf_audio_start,
    .stop_cb = hfp_hf_audio_stop,
};

void hfp_hf_audio_open(uint8_t codec, bool offloading, bt_address_t* bd_addr)
{
    hfp_hf_stream_t* stream = &hfp_hf_stream;

    stream->offloading = offloading;
    if (!stream->offloading) {
        return;
    }

    bt_audio_config_t* config;

    config = hfp_hf_media_config_create(codec);

    if (!config) {
        BT_LOGE("%s: create media config failed", __func__);
        return;
    }

    audio_control_open(HFP_HF_PROFILE_ID, config, &audio_control_callbacks);

    hfp_hf_media_config_destroy(config);
}

void hfp_hf_audio_cleanup(void)
{
    hfp_hf_stream_t* stream = &hfp_hf_stream;

    if (!stream->offloading) {
        return;
    }

    hfp_hf_on_stopped();
    memset(&hfp_hf_stream, 0, sizeof(hfp_hf_stream));
}
