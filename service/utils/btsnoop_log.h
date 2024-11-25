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

#include "bt_status.h"
#include <stdint.h>

int btsnoop_create_new_file(void);
void btsnoop_close_file(void);
bt_status_t btsnoop_log_open(void);
void btsnoop_log_capture(uint8_t is_recieve, uint8_t* hci_pkt, uint32_t hci_pkt_size);
void btsnoop_log_close(void);

#endif //__BT_SNOOP_LOG_H__