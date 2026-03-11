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
#ifndef __GATTC_DEBUG_H__
#define __GATTC_DEBUG_H__

#include "gattc_internal.h"

#ifdef CONFIG_BLUETOOTH_GATT_CLIENT_DEBUG

void gattc_log(const bt_address_t* addr, const char* msg);
void gattc_log_state(const bt_address_t* addr, const char* msg, profile_connection_state_t state);
void gattc_log_status(const bt_address_t* addr, const char* msg, gatt_status_t status);
void gattc_dump_services(const gattc_connection_t* connection);

#else

#define gattc_log(addr, msg)
#define gattc_log_state(addr, msg, state)
#define gattc_log_status(addr, msg, status)
#define gattc_dump_services(connection)

#endif /* CONFIG_BLUETOOTH_GATT_CLIENT_DEBUG */

#endif /* __GATTC_DEBUG_H__ */
