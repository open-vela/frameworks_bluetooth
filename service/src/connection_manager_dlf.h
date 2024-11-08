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

#ifndef __CONNECTION_MANAGER_DLF_H__
#define __CONNECTION_MANAGER_DLF_H__

#include "bt_addr.h"
#include "bt_status.h"

void bt_cm_dlf_cleanup(void);

bt_status_t bt_cm_enable_dlf(bt_address_t* peer_addr);
bt_status_t bt_cm_disable_dlf(bt_address_t* peer_addr);

#endif /*__CONNECTION_MANAGER_DLF_H__*/
