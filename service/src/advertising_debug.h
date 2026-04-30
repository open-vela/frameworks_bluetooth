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
#ifndef __ADVERTISING_DEBUG_H__
#define __ADVERTISING_DEBUG_H__

#include <nuttx/config.h>

#include "advertising_internal.h"

#ifdef CONFIG_BLUETOOTH_LE_ADVERTISER_DEBUG

void adv_dump_info(uint8_t adv_id, const advertising_info_t* adv_info);
void adv_dump_advertiser(void);

#else

#define adv_dump_info(adv_id, adv_info)
#define adv_dump_advertiser()

#endif /* CONFIG_BLUETOOTH_LE_ADVERTISER_DEBUG */

#endif /* __ADVERTISING_DEBUG_H__ */
