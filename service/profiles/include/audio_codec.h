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
#ifndef __AUDIO_CODEC_H__
#define __AUDIO_CODEC_H__

#define BT_AUDIO_FRAGMENTS 4

/* Audio Sub-Format Types */
#define BT_AUDIO_STREAMFORMAT_SBC_PACKED 0x01 /* SBC data with MTU packed */
#define BT_AUDIO_STREAMFORMAT_AAC_LATM 0x08
#define BT_AUDIO_SUBFMT_PCM_U8 0x06
#define BT_AUDIO_SUBFMT_PCM_S16_LE 0x0a
typedef struct {
    uint32_t super_block_align;
} bt_audio_enc_wma_t;

typedef struct {
    uint32_t quality;
    uint32_t managed;
    uint32_t max_bit_rate;
    uint32_t min_bit_rate;
    uint32_t downmix;
} bt_audio_enc_vorbis_t;

typedef struct {
    uint32_t num;
    uint32_t gain;
} bt_audio_enc_flac_t;

typedef struct {
    uint32_t bw;
    int32_t reserved[15];
} bt_audio_enc_generic_t;

typedef struct {
    uint16_t sample_size;
    uint16_t min_blk_size;
    uint16_t max_blk_size;
    uint16_t min_frame_size;
    uint16_t max_frame_size;
    uint16_t reserved;
} bt_audio_dec_flac_t;

typedef struct {
    uint32_t encoder_option;
    uint32_t adv_encoder_option;
    uint32_t adv_encoder_option2;
    uint32_t reserved;
} bt_audio_dec_wma_t;

typedef struct {
    uint8_t blocks;
    uint8_t subbands;
    uint8_t alloc_method;
    uint8_t bitpool;
} bt_audio_enc_sbc_t;

typedef struct {
    uint32_t frame_length;
    uint8_t compatible_version;
    uint8_t pb;
    uint8_t mb;
    uint8_t kb;
    uint32_t max_run;
    uint32_t max_frame_bytes;
} bt_audio_dec_alac_t;

typedef struct {
    uint32_t quant_bits;
    uint32_t start_region;
    uint32_t num_regions;
} bt_audio_enc_real_t;

typedef struct {
    uint16_t compatible_version;
    uint16_t compression_level;
    uint32_t format_flags;
    uint32_t blocks_per_frame;
    uint32_t final_frame_blocks;
    uint32_t total_frames;
    uint32_t seek_table_present;
} bt_audio_dec_ape_t;

typedef struct {
    uint32_t id;
    uint32_t ch_in;
    uint32_t ch_out;
    uint32_t sample_rate;
    uint32_t bit_rate;
    uint32_t rate_control;
    uint32_t profile;
    uint32_t level;
    uint32_t ch_mode;
    uint32_t format;
    uint32_t align;
    union {
        bt_audio_enc_real_t real;
        bt_audio_dec_alac_t alac_d;
        bt_audio_dec_ape_t ape_d;
        bt_audio_enc_wma_t wma;
        bt_audio_enc_sbc_t sbc;
        bt_audio_enc_vorbis_t vorbis;
        bt_audio_enc_flac_t flac;
        bt_audio_enc_generic_t generic;
        bt_audio_dec_flac_t flac_d;
        bt_audio_dec_wma_t wma_d;
    } options;

    uint32_t pcm_format;
    uint32_t reserved[2];
} bt_audio_codec_t;

typedef struct {
    uint32_t fragment_size;
    uint32_t fragments;
    bt_audio_codec_t* codec;
} bt_audio_config_t;
#endif
