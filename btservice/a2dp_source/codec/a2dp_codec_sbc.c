/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
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
#include <stdlib.h>
#include <string.h>

#include "a2dp_codec_sbc.h"
#include "sbc_encoder.h"
static sbc_param_t* sbc_param;

#define LOG_TAG "a2dp_codec_sbc"
#include "log.h"

void a2dp_codec_sbc_init(void)
{
    const sbc_param_t *defaultparam;
    sbc_param = malloc(sizeof(sbc_param_t));
    defaultparam = sbc_encoder_param_get();
    memcpy(sbc_param, defaultparam, sizeof(sbc_param_t));
}

uint16_t a2dp_codec_sample_frequency(uint16_t sample_frequency)
{
    uint16_t sampling_freq;

    if (sample_frequency == SBC_SF_16000)
        sampling_freq = 16000;
    else if (sample_frequency == SBC_SF_32000)
        sampling_freq = 32000;
    else if (sample_frequency == SBC_SF_44100)
        sampling_freq = 44100;
    else
        sampling_freq = 48000;

    return sampling_freq;
}

uint32_t a2dp_codec_sbc_frame_length(void)
{
    uint32_t frame_len;
#if 1
  if (sbc_param->s16ChannelMode == SBC_JOINT_STEREO ||
      sbc_param->s16ChannelMode == SBC_STEREO) {
    frame_len = 4 +
                (4 * sbc_param->s16NumOfSubBands *
                sbc_param->s16NumOfChannels) /
                8 +
                (((sbc_param->s16ChannelMode - 2) *
                sbc_param->s16NumOfSubBands) +
                (sbc_param->s16NumOfBlocks * sbc_param->s16BitPool)) /
                8;
  } else {
    frame_len = 4 +
                ((4 * sbc_param->s16NumOfSubBands *
                sbc_param->s16NumOfChannels) /
                8) +
                ((sbc_param->s16NumOfBlocks *
                sbc_param->s16NumOfChannels *
                sbc_param->s16BitPool) /
                8);
  }
#else
  frame_len = 4 + (4 * sbc_param->s16NumOfSubBands * sbc_param->s16NumOfChannels) / 8
               + ((sbc_param->s16NumOfBlocks * sbc_param->s16BitPool * (1 + (sbc_param->s16ChannelMode == SBC_DUAL))
               + (sbc_param->s16ChannelMode == SBC_JOINT_STEREO) * sbc_param->s16NumOfSubBands) + 7) / 8;
#endif
    BT_LOGD("%s :frame_len:%d", __func__, frame_len);

    return frame_len;
}

uint32_t a2dp_codec_sbc_bit_rate(uint32_t frame_len)
{
    uint16_t sampling_freq;
    uint32_t bit_rate;

    sampling_freq = a2dp_codec_sample_frequency(sbc_param->s16SamplingFreq);
    bit_rate = (8 * frame_len * sampling_freq) / (sbc_param->s16NumOfSubBands * sbc_param->s16NumOfBlocks);
    BT_LOGD("%s, birtate: %d", __func__, bit_rate);

    return bit_rate;
}
