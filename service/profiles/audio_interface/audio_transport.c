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
#define LOG_TAG "audio_transport"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "bt_utils.h"
#include "utils/log.h"

#include "include/audio_transport.h"

#include "tinycompress/tinycompress.h"

struct compress* audio_transport_open(const char* name, uint32_t flags, struct compr_config* config)
{

    struct compress* cps = NULL;
    BT_LOGE("compress open %s", name);
    cps = compress_open_by_name(name, flags, config);

    if (cps == NULL) {
        BT_LOGE("compress open failed");
    }

    return cps;
}

void audio_transport_nonblock(struct compress* cps, int nonblock)
{
    compress_nonblock(cps, nonblock);
}

void audio_transport_set_event(struct compress* cps, void* cookie, void* callback)
{
    compress_set_event_callback(cps, callback, cookie);
}

int audio_transport_get_file_descriptor(struct compress* cps)
{
    return compress_get_file_descriptor(cps);
}

void audio_transport_poll_available(struct compress* cps)
{
    compress_poll_available(cps);
}

void audio_transport_start(struct compress* cps)
{
    compress_start(cps);
}

void audio_transport_resume(struct compress* cps)
{
    compress_resume(cps);
}

void audio_transport_stop(struct compress* cps)
{
    compress_stop(cps);
}

void audio_transport_close(struct compress* cps)
{
    compress_close(cps);
}

void audio_transport_pause(struct compress* cps)
{
    compress_pause(cps);
}

int audio_transport_read(struct compress* cps, void* buf, unsigned int size)
{
    return compress_read(cps, buf, size);
}

int audio_transport_write(struct compress* cps, const void* data, uint16_t len)
{
    return compress_write(cps, data, len);
}

void audio_transport_reset(struct compress* cps)
{
    compress_reset(cps);
}