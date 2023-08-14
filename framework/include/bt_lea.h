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
#ifndef __BT_LEA__H__
#define __BT_LEA__H__

#include <stddef.h>

#include "bt_addr.h"
#include "bt_device.h"

typedef enum {
    LEA_AUDIO_STATE_DISCONNECTED,
    LEA_AUDIO_STATE_CONNECTING,
    LEA_AUDIO_STATE_CONNECTED,
    LEA_AUDIO_STATE_DISCONNECTING,
} lea_audio_state_t;

#endif