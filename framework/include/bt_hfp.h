/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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

#ifndef __BT_HFP_H__
#define __BT_HFP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include <stddef.h>

#define HFP_PHONENUM_DIGITS_MAX 32
#define HFP_NAME_DIGITS_MAX 64

typedef enum {
    HFP_AUDIO_STATE_DISCONNECTED,
    HFP_AUDIO_STATE_CONNECTING,
    HFP_AUDIO_STATE_CONNECTED,
    HFP_AUDIO_STATE_DISCONNECTING,
} hfp_audio_state_t;

typedef enum {
    HFP_HF_VR_STATE_STOPPED = 0,
    HFP_HF_VR_STATE_STARTED
} hfp_hf_vr_state_t;

typedef enum {
    HFP_VOLUME_TYPE_SPK = 0,
    HFP_VOLUME_TYPE_MIC
} hfp_volume_type_t;

typedef enum {
    HFP_HF_CALL_CONTROL_CHLD_0, /* Releases all held calls or sets User Determined User Busy (UDUB) for a waiting call */
    HFP_HF_CALL_CONTROL_CHLD_1, /* Releases all active calls (if any exist) and accepts the other (held or waiting) call */
    HFP_HF_CALL_CONTROL_CHLD_2, /* Places all active calls (if any exist) on hold and accepts the other (held or waiting) call */
    HFP_HF_CALL_CONTROL_CHLD_3, /* Adds a held call to the conversation */
    HFP_HF_CALL_CONTROL_CHLD_4 /* Connects the two calls and disconnects the subscriber from both calls (Explicit Call Transfer).
                                  Support for this value and its associated functionality is optional for the HF */
} hfp_call_control_t;

typedef enum {
    HFP_HF_CALL_ACCEPT_NONE,
    HFP_HF_CALL_ACCEPT_RELEASE,
    HFP_HF_CALL_ACCEPT_HOLD,
} hfp_call_accept_t;

typedef enum {
    HFP_CALL_DIRECTION_OUTGOING = 0,
    HFP_CALL_DIRECTION_INCOMING
} hfp_call_direction_t;

typedef enum {
    HFP_CALL_MPTY_TYPE_SINGLE = 0,
    HFP_CALL_MPTY_TYPE_MULTI
} hfp_call_mpty_type_t;

typedef enum {
    HFP_CALL_MODE_VOICE = 0,
    HFP_CALL_MODE_DATA,
    HFP_CALL_MODE_FAX
} hfp_call_mode_t;

typedef enum {
    HFP_CALL_ADDRTYPE_UNKNOWN = 0x81,
    HFP_CALL_ADDRTYPE_INTERNATIONAL = 0x91,
    HFP_CALL_ADDRTYPE_NATIONAL = 0xA1,
} hfp_call_addrtype_t;

#ifdef __cplusplus
}
#endif

#endif /* __BT_HFP_H__ */
