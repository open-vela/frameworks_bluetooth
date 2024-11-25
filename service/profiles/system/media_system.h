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

#define INVALID_MEDIA_VOLUME (-1)

typedef void (*bt_media_voice_volume_change_callback_t)(void* context, int volume);

int bt_media_get_music_volume_range();
int bt_media_volume_avrcp_to_media(uint8_t volume);
uint8_t bt_media_volume_media_to_avrcp(int volume);
int bt_media_volume_hfp_to_media(uint8_t hfp_volume);
uint8_t bt_media_volume_media_to_hfp(int media_volume);
void bt_media_remove_listener(void* handle);
bt_status_t bt_media_set_a2dp_available(void);
bt_status_t bt_media_set_a2dp_unavailable(void);
bt_status_t bt_media_set_hfp_samplerate(uint16_t samplerate);
void* bt_media_listen_voice_call_volume_change(bt_media_voice_volume_change_callback_t cb, void* context);
bt_status_t bt_media_get_voice_call_volume(int* volume);
bt_status_t bt_media_set_voice_call_volume(int volume);
void* bt_media_listen_music_volume_change(bt_media_voice_volume_change_callback_t cb, void* context);
bt_status_t bt_media_get_music_volume(int* volume);
bt_status_t bt_media_set_music_volume(int volume);
bt_status_t bt_media_set_sco_available(void);
bt_status_t bt_media_set_sco_unavailable(void);
bt_status_t bt_media_set_a2dp_offloading(bool enable);
bt_status_t bt_media_set_hfp_offloading(bool enable);
bt_status_t bt_media_set_lea_offloading(bool enable);
bt_status_t bt_media_set_lea_available(void);
bt_status_t bt_media_set_lea_unavailable(void);
bt_status_t bt_media_set_anc_enable(bool enable);

#endif /* __MEDIA_SYSTEM_H__ */