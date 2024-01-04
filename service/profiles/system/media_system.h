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

#ifndef __MEDIA_SYSTEM_H__
#define __MEDIA_SYSTEM_H__

bt_status_t bt_media_set_a2dp_available(void);
bt_status_t bt_media_set_a2dp_unavailable(void);
bt_status_t bt_media_set_hfp_samplerate(uint16_t samplerate);
bt_status_t bt_media_get_voice_call_volume(uint16_t *volume);
bt_status_t bt_media_set_voice_call_volume(uint16_t volume);
bt_status_t bt_media_set_sco_available(void);
bt_status_t bt_media_set_sco_unavailable(void);
bt_status_t bt_media_set_a2dp_offloading(bool enable);
bt_status_t bt_media_set_hfp_offloading(bool enable);

#endif /* __MEDIA_SYSTEM_H__ */