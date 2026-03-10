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
#ifndef __BT_AUDIO_NUMBERS_H__
#define __BT_AUDIO_NUMBERS_H__
#include <stdint.h>

/** Coding Format and Codec ID */
#define BT_CODEC_ID_ULAW_LOG 0x00 /** µ-law log */
#define BT_CODEC_ID_ALAW_LOG 0x01 /** A-law log */
#define BT_CODEC_ID_CVSD 0x02 /** Continuous Variable Slope Delta Modulation */
#define BT_CODEC_ID_TRANSPARENT 0x03 /** No transcoding or resampling is performed */
#define BT_CODEC_ID_LINEAR_PCM 0x04 /** Linear PCM */
#define BT_CODEC_ID_MSBC 0x05 /** Modified Sub-Band Codec */
#define BT_CODEC_ID_LC3 0x06 /** Low Complexity Communication Codec */
#define BT_CODEC_ID_G729A 0x07 /** G.729A */
#define BT_CODEC_ID_VENDOR 0xFF /** Vendor-specific codec */

/** Codec Specific Configuration LTV values */

/** Sampling frequency */
#define BT_CODEC_CONFIG_FREQUENCY_LEN 2
#define BT_CODEC_CONFIG_FREQUENCY_TYPE 1
#define BT_CODEC_CONFIG_FREQUENCY_8000 0x01 /** 8000 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_11025 0x02 /** 11025 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_16000 0x03 /** 16000 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_22050 0x04 /** 22050 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_24000 0x05 /** 24000 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_32000 0x06 /** 32000 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_44100 0x07 /** 44100 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_48000 0x08 /** 48000 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_88200 0x09 /** 88200 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_96000 0x0A /** 96000 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_176400 0x0B /** 176400 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_192000 0x0C /** 192000 Hz */
#define BT_CODEC_CONFIG_FREQUENCY_384000 0x0D /** 384000 Hz */

/** Frame duration */
#define BT_CODEC_CONFIG_DURATION_LEN 2
#define BT_CODEC_CONFIG_DURATION_TYPE 2
#define BT_CODEC_CONFIG_DURATION_7_5_MS 0x00 /** Use 7.5 ms codec frames */
#define BT_CODEC_CONFIG_DURATION_10_MS 0x01 /** Use 10 ms codec frames */

/** Bitfield of audio location values */
#define BT_CODEC_CONFIG_ALLOCATION_LEN 5
#define BT_CODEC_CONFIG_ALLOCATION_TYPE 3
#define BT_CODEC_CONFIG_ALLOCATION_MONO 0x00000000 /** Mono Audio (no specified Audio Location) */
#define BT_CODEC_CONFIG_ALLOCATION_FRONT_LEFT 0x00000001 /** Front Left */
#define BT_CODEC_CONFIG_ALLOCATION_FRONT_RIGHT 0x00000002 /** Front Right */
#define BT_CODEC_CONFIG_ALLOCATION_FRONT_CENTER 0x00000004 /** Front Center */
#define BT_CODEC_CONFIG_ALLOCATION_LOW_FREQ_1 0x00000008 /** Low Frequency Effects 1 */
#define BT_CODEC_CONFIG_ALLOCATION_BACK_LEFT 0x00000010 /** Back Left */
#define BT_CODEC_CONFIG_ALLOCATION_BACK_RIGHT 0x00000020 /** Back Right */
#define BT_CODEC_CONFIG_ALLOCATION_FRONT_LEFT_CENTER 0x00000040 /** Front Left of Center */
#define BT_CODEC_CONFIG_ALLOCATION_FRONT_RIGHT_CENTER 0x00000080 /** Front Right of Center */
#define BT_CODEC_CONFIG_ALLOCATION_BACK_CENTER 0x00000100 /** Back Center */
#define BT_CODEC_CONFIG_ALLOCATION_LOW_FREQ_2 0x00000200 /** Low Frequency Effects 2 */
#define BT_CODEC_CONFIG_ALLOCATION_SIDE_LEFT 0x00000400 /** Side Left */
#define BT_CODEC_CONFIG_ALLOCATION_SIDE_RIGHT 0x00000800 /** Side Right */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_FRONT_LEFT 0x00001000 /** Top Front Left */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_FRONT_RIGHT 0x00002000 /** Top Front Right */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_FRONT_CENTER 0x00004000 /** Top Front Center */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_CENTER 0x00008000 /** Top Center */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_BACK_LEFT 0x00010000 /** Top Back Left */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_BACK_RIGHT 0x00020000 /** Top Back Right */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_SIDE_LEFT 0x00040000 /** Top Side Left */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_SIDE_RIGHT 0x00080000 /** Top Side Right */
#define BT_CODEC_CONFIG_ALLOCATION_TOP_BACK_CENTER 0x00100000 /** Top Back Center */
#define BT_CODEC_CONFIG_ALLOCATION_BOTTOM_FRONT_CENTER 0x00200000 /** Bottom Front Center */
#define BT_CODEC_CONFIG_ALLOCATION_BOTTOM_FRONT_LEFT 0x00400000 /** Bottom Front Left */
#define BT_CODEC_CONFIG_ALLOCATION_BOTTOM_FRONT_RIGHT 0x00800000 /** Bottom Front Right */
#define BT_CODEC_CONFIG_ALLOCATION_FRONT_LEFT_WIDE 0x01000000 /** Front Left Wide */
#define BT_CODEC_CONFIG_ALLOCATION_FRONT_RIGHT_WIDE 0x02000000 /** Front Right Wide */
#define BT_CODEC_CONFIG_ALLOCATION_LEFT_SURROUND 0x04000000 /** Left Surround */
#define BT_CODEC_CONFIG_ALLOCATION_RIGHT_SURROUND 0x08000000 /** Right Surround */

/** Number of octets used per codec */
#define BT_CODEC_CONFIG_OCTETS_PER_FRAME_LEN 3
#define BT_CODEC_CONFIG_OCTETS_PER_FRAME_TYPE 4

/** Number of blocks of codec frames per SDU */
#define BT_CODEC_CONFIG_BLOCKS_PER_SDU_LEN 2
#define BT_CODEC_CONFIG_BLOCKS_PER_SDU_TYPE 5

#define BT_CODEC_CONFIG_LEN_MAX ((1 + BT_CODEC_CONFIG_FREQUENCY_LEN)            \
    + (1 + BT_CODEC_CONFIG_DURATION_LEN) + (1 + BT_CODEC_CONFIG_ALLOCATION_LEN) \
    + (1 + BT_CODEC_CONFIG_OCTETS_PER_FRAME_LEN) + (1 + BT_CODEC_CONFIG_BLOCKS_PER_SDU_LEN))

/** Metadatas */

/** Streaming Audio Context */
#define BT_METADATA_STREAMING_AUDIO_CONTEXT_LEN 3
#define BT_METADATA_STREAMING_AUDIO_CONTEXT_TYPE 2
/** Identifies audio where the use case context does not match any other defined value, or where the
 *  context is unknown or cannot be determined. */
#define BT_METADATA_AUDIO_CONTEXT_UNSPECIFIED 0x0001
/** Conversation between humans, for example, in telephony or video calls, including traditional
 *  cellular as well as VoIP and Push-to-Talk */
#define BT_METADATA_AUDIO_CONTEXT_CONVERSATIONAL 0x0002
/** Media, for example, music playback, radio, podcast or movie soundtrack, or TV audio */
#define BT_METADATA_AUDIO_CONTEXT_MEDIA 0x0004
/** Audio associated with video gaming, for example, gaming media; gaming effects; music and in-game
 *  voice chat between participants; or a mix of all the above */
#define BT_METADATA_AUDIO_CONTEXT_GAME 0x0008
/** Instructional audio, for example, in navigation, announcements, or user guidance */
#define BT_METADATA_AUDIO_CONTEXT_INSTRUCTIONAL 0x0010
/** Man-machine communication, for example, with voice recognition or virtual assistants */
#define BT_METADATA_AUDIO_CONTEXT_VOICE_ASSISTANTS 0x0020
/** Live audio, for example, from a microphone where audio is perceived both through a direct
 *  acoustic path and through an LE Audio Stream */
#define BT_METADATA_AUDIO_CONTEXT_LIVE 0x0040
/** Sound effects, including keyboard and touch feedback; menu and user interface sounds; and other
 *  system sounds */
#define BT_METADATA_AUDIO_CONTEXT_SOUND_EFFECTS 0x0080
/** Notification and reminder sounds; attention-seeking audio, for example, in beeps signaling the
 *  arrival of a message */
#define BT_METADATA_AUDIO_CONTEXT_NOTIFICATIONS 0x0100
/** Alerts the user to an incoming call, for example, an incoming telephony or video call, including
 *  traditional cellular as well as VoIP and Push-to-Talk */
#define BT_METADATA_AUDIO_CONTEXT_RINGTONE 0x0200
/** Alarms and timers; immediate alerts, for example, in a critical battery alarm, timer expiry,
 *  alarm clock, toaster, cooker, kettle, microwave, etc. */
#define BT_METADATA_AUDIO_CONTEXT_ALERTS 0x0400
/** Emergency alarm sounds, for example, fire alarms or other urgent alerts */
#define BT_METADATA_AUDIO_CONTEXT_EMERGENCY_ALARM 0x0800

#define BT_METADATA_LANGUAGE_LEN 4
#define BT_METADATA_LANGUAGE_TYPE 4

/** Size of language codes as defined in ISO 639-3 */
#define BT_METADATA_ISO_639_3_SIZE 3

#endif /* __BT_AUDIO_NUMBERS_H__ */
