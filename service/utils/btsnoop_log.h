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

#ifndef __BT_SNOOP_LOG_H__
#define __BT_SNOOP_LOG_H__

#include "bt_list.h"
#include "bt_status.h"
#include "bt_trace.h"

#include <stdint.h>

#define CONFIG_BLUETOOTH_SNOOP_LOG_DEFAULT_PATH "/data/misc/bt/snoop"
#define SNOOP_PATH_MAX_LEN 255

void btsnoop_log_capture(uint8_t is_recieve, uint8_t* hci_pkt, uint32_t hci_pkt_size);
int btsnoop_log_init(char* path);
void btsnoop_log_uninit(void);
int btsnoop_log_enable(void);
void btsnoop_log_disable(void);
int btsnoop_set_filter(btsnoop_filter_flag_t filter_flag);
int btsnoop_remove_filter(btsnoop_filter_flag_t filter_flag);

#endif //__BT_SNOOP_LOG_H__