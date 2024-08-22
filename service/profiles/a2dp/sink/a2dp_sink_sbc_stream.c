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

#include "a2dp_sink_audio.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "sink_sbc"
#include "utils/log.h"

#define LOAS_HDRSIZE 3

#define LATM_HEADER 0x56E0

static a2dp_sink_packet_t* sink_sbc_repackage(uint8_t* data, uint16_t length)
{
    a2dp_sink_packet_t* packet = NULL;

    /* pack aac loas header */
    packet = malloc(sizeof(a2dp_sink_packet_t) + length + LOAS_HDRSIZE);
    if (packet) {
        packet->data[0] = (LATM_HEADER >> 8) & 0xFF;
        packet->data[1] = (LATM_HEADER & 0xE0) | ((length >> 8) & 0x1F);
        packet->data[2] = length & 0xFF;
        packet->length = length + LOAS_HDRSIZE;
        memcpy(packet->data + LOAS_HDRSIZE, data, length);
    }

    return packet;
}

static void sink_sbc_packet_send_done(a2dp_sink_packet_t* packet)
{
    free(packet);
}

static const a2dp_sink_stream_interface_t a2dp_sink_stream_sbc = {
    sink_sbc_repackage,
    sink_sbc_packet_send_done,
};

const a2dp_sink_stream_interface_t* get_a2dp_sink_sbc_stream_interface(void)
{
    return &a2dp_sink_stream_sbc;
}
