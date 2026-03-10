/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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

#include "bt_audio_numbers.h"

typedef struct {
    uint8_t u8;
    const char* str;
} u8_to_str_t;

typedef struct {
    uint16_t u16;
    const char* str;
} u16_to_str_t;

typedef struct {
    uint32_t u32;
    const char* str;
} u32_to_str_t;

static const u8_to_str_t codec_map[] = {
    { BT_CODEC_ID_ULAW_LOG, "µ-law" },
    { BT_CODEC_ID_ALAW_LOG, "A-law" },
    { BT_CODEC_ID_CVSD, "CVSD" },
    { BT_CODEC_ID_TRANSPARENT, "Trans" },
    { BT_CODEC_ID_LINEAR_PCM, "PCM" },
    { BT_CODEC_ID_MSBC, "mSBC" },
    { BT_CODEC_ID_LC3, "LC3" },
    { BT_CODEC_ID_G729A, "G.729A" },
    { BT_CODEC_ID_VENDOR, "Vendor" },
};

static const u8_to_str_t freq_map[] = {
    { BT_CODEC_CONFIG_FREQUENCY_8000, "8000" },
    { BT_CODEC_CONFIG_FREQUENCY_11025, "11025" },
    { BT_CODEC_CONFIG_FREQUENCY_16000, "16000" },
    { BT_CODEC_CONFIG_FREQUENCY_22050, "22050" },
    { BT_CODEC_CONFIG_FREQUENCY_24000, "24000" },
    { BT_CODEC_CONFIG_FREQUENCY_32000, "32000" },
    { BT_CODEC_CONFIG_FREQUENCY_44100, "44100" },
    { BT_CODEC_CONFIG_FREQUENCY_48000, "48000" },
    { BT_CODEC_CONFIG_FREQUENCY_88200, "88200" },
    { BT_CODEC_CONFIG_FREQUENCY_96000, "96000" },
    { BT_CODEC_CONFIG_FREQUENCY_176400, "176400" },
    { BT_CODEC_CONFIG_FREQUENCY_192000, "192000" },
    { BT_CODEC_CONFIG_FREQUENCY_384000, "384000" },
};

static const u8_to_str_t duration_map[] = {
    { BT_CODEC_CONFIG_DURATION_7_5_MS, "7.5" },
    { BT_CODEC_CONFIG_DURATION_10_MS, "10" },
};

static const u32_to_str_t allocation_map[] = {
    { BT_CODEC_CONFIG_ALLOCATION_MONO, "Mono" },
    { BT_CODEC_CONFIG_ALLOCATION_FRONT_LEFT, "Front Left" },
    { BT_CODEC_CONFIG_ALLOCATION_FRONT_RIGHT, "Front Right" },
    { BT_CODEC_CONFIG_ALLOCATION_FRONT_CENTER, "Front Center" },
    { BT_CODEC_CONFIG_ALLOCATION_LOW_FREQ_1, "Low Frequency Effects 1" },
    { BT_CODEC_CONFIG_ALLOCATION_BACK_LEFT, "Back Left" },
    { BT_CODEC_CONFIG_ALLOCATION_BACK_RIGHT, "Back Right" },
    { BT_CODEC_CONFIG_ALLOCATION_FRONT_LEFT_CENTER, "Front Left of Center" },
    { BT_CODEC_CONFIG_ALLOCATION_FRONT_RIGHT_CENTER, "Front Right of Center" },
    { BT_CODEC_CONFIG_ALLOCATION_BACK_CENTER, "Back Center" },
    { BT_CODEC_CONFIG_ALLOCATION_LOW_FREQ_2, "Low Frequency Effects 2" },
    { BT_CODEC_CONFIG_ALLOCATION_SIDE_LEFT, "Side Left" },
    { BT_CODEC_CONFIG_ALLOCATION_SIDE_RIGHT, "Side Right" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_FRONT_LEFT, "Top Front Left" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_FRONT_RIGHT, "Top Front Right" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_FRONT_CENTER, "Top Front Center" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_CENTER, "Top Center" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_BACK_LEFT, "Top Back Left" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_BACK_RIGHT, "Top Back Right" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_SIDE_LEFT, "Top Side Left" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_SIDE_RIGHT, "Top Side Right" },
    { BT_CODEC_CONFIG_ALLOCATION_TOP_BACK_CENTER, "Top Back Center" },
    { BT_CODEC_CONFIG_ALLOCATION_BOTTOM_FRONT_CENTER, "Bottom Front Center" },
    { BT_CODEC_CONFIG_ALLOCATION_BOTTOM_FRONT_LEFT, "Bottom Front Left" },
    { BT_CODEC_CONFIG_ALLOCATION_BOTTOM_FRONT_RIGHT, "Bottom Front Right" },
    { BT_CODEC_CONFIG_ALLOCATION_FRONT_LEFT_WIDE, "Front Left Wide" },
    { BT_CODEC_CONFIG_ALLOCATION_FRONT_RIGHT_WIDE, "Front Right Wide" },
    { BT_CODEC_CONFIG_ALLOCATION_LEFT_SURROUND, "Left Surround" },
    { BT_CODEC_CONFIG_ALLOCATION_RIGHT_SURROUND, "Right Surround" },
};

static const u16_to_str_t context_map[] = {
    { BT_METADATA_AUDIO_CONTEXT_UNSPECIFIED, "Unspecified" },
    { BT_METADATA_AUDIO_CONTEXT_CONVERSATIONAL, "Conversation" },
    { BT_METADATA_AUDIO_CONTEXT_MEDIA, "Media" },
    { BT_METADATA_AUDIO_CONTEXT_GAME, "Game" },
    { BT_METADATA_AUDIO_CONTEXT_INSTRUCTIONAL, "Instruction" },
    { BT_METADATA_AUDIO_CONTEXT_VOICE_ASSISTANTS, "Assistant" },
    { BT_METADATA_AUDIO_CONTEXT_LIVE, "Live" },
    { BT_METADATA_AUDIO_CONTEXT_SOUND_EFFECTS, "SoundEffect" },
    { BT_METADATA_AUDIO_CONTEXT_NOTIFICATIONS, "Notification" },
    { BT_METADATA_AUDIO_CONTEXT_RINGTONE, "Ring" },
    { BT_METADATA_AUDIO_CONTEXT_ALERTS, "Alert" },
    { BT_METADATA_AUDIO_CONTEXT_EMERGENCY_ALARM, "Emergency" },
};
