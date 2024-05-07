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
#ifndef __LEA_VMICS_MEDIA_CONTROL_H__
#define __LEA_VMICS_MEDIA_CONTROL_H__

#include <stdint.h>

/****************************************************************************
 * Public Function
 ****************************************************************************/

// server interface
void lea_vcs_vol_state_request(void* volume_session, uint8_t volume, uint8_t mute);
void lea_vcs_vol_flags_request(uint8_t flags);
void lea_mics_mic_mute_request(uint8_t mute);

uint8_t lea_vcs_get_volume(void* volume_session);
uint8_t lea_vcs_get_mute(void);

#endif /* __LEA_VMICS_MEDIA_CONTROL_H__ */