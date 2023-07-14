/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#ifndef __LEA_VMICPS_EVENT_H__
#define __LEA_VMICPS_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_addr.h"
#include "bt_lea_vmicps.h"
#include <stdint.h>

typedef struct {
    uint8_t volume;
    uint8_t mute;
} bts_vmicp_vol_state_s;

typedef enum {
    STACK_EVENT_VCS_VOLUME_STATE = 0,
    STACK_EVENT_VCS_VOLUME_FLAGS,
    STACK_EVENT_MICS_MUTE_STATE
} lea_vmicps_event_t;

typedef struct {
    lea_vmicps_event_t event;
    union {
        bts_vmicp_vol_state_s vol_state;
        uint8_t vol_flags;
        uint8_t mic_mute_state;
    } data;
} lea_vmicps_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
lea_vmicps_msg_t *lea_vmicps_msg_new(lea_vmicps_event_t event);

void lea_vmicps_msg_destory(lea_vmicps_msg_t *msg);

#endif /* __LEA_VMICPS_EVENT_H__ */