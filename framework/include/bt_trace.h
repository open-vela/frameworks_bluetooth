/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#ifndef __BT_LOG_API_H__
#define __BT_LOG_API_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "bluetooth.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

typedef enum {
    BTSNOOP_FILTER_A2DP_AUDIO,
    BTSNOOP_FILTER_AVCTP_BROWSING,
    BTSNOOP_FILTER_ATT,
    BTSNOOP_FILTER_SPP,
    BTSNOOP_FILTER_MAX,
    BTSNOOP_FILTER_UNFILTER,
} btsnoop_filter_flag_t;

/**
 * @brief Enable bluetooth btsnoop log
 *
 * @param ins - bluetooth client instance.
 */
void BTSYMBOLS(bluetooth_enable_btsnoop_log)(bt_instance_t* ins);

/**
 * @brief Disable bluetooth btsnoop log
 *
 * @param ins - bluetooth client instance.
 */
void BTSYMBOLS(bluetooth_disable_btsnoop_log)(bt_instance_t* ins);

/**
 * @brief Set a filter flag in the btsnoop log
 *
 * @param ins - bluetooth client instance.
 * @param filter_flag - the flag bit for filtering specified data in the btsnoop log.
 */
void BTSYMBOLS(bluetooth_set_btsnoop_filter)(bt_instance_t* ins, btsnoop_filter_flag_t filter_flag);

/**
 * @brief Remove a filter flag in the btsnoop log
 *
 * @param ins - bluetooth client instance.
 * @param filter_flag - the flag bit for filtering specified data in the btsnoop log.
 */
void BTSYMBOLS(bluetooth_remove_btsnoop_filter)(bt_instance_t* ins, btsnoop_filter_flag_t filter_flag);

#ifdef __cplusplus
}
#endif
#endif /* __BT_LOG_API_H__ */
