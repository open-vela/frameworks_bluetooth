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

#ifndef __BLUETOOTH_IPC_H__
#define __BLUETOOTH_IPC_H__

#include "bt_status.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef CONFIG_BLUETOOTH_FRAMEWORK_LOCAL
#define bluetooth_ipc_add_services()
#define bluetooth_ipc_join_thread_pool()
#define bluetooth_ipc_join_service_loop()
#else
bt_status_t bluetooth_ipc_add_services(void);
void bluetooth_ipc_join_thread_pool(void);
void bluetooth_ipc_join_service_loop(void);
#endif

#endif /* __BLUETOOTH_IPC_H__ */
