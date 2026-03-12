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
#ifndef __SCAN_DEBUG_H__
#define __SCAN_DEBUG_H__

#include "scan_manager_internal.h"

#ifdef CONFIG_BLUETOOTH_LE_SCANNER_DEBUG

void scan_dump_params(const ble_scan_params_t* params);
void scan_dump_scanners(void);
void scan_update_statistics(scanner_t* scanner);
void scan_debug_timer_start(void);
void scan_debug_timer_stop(void);

#else

#define scan_dump_params(params)
#define scan_dump_scanners()
#define scan_update_statistics(scanner)
#define scan_debug_timer_start()
#define scan_debug_timer_stop()

#endif /* CONFIG_BLUETOOTH_LE_SCANNER_DEBUG */

#endif /* __SCAN_DEBUG_H__ */
