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

#ifndef __MOCK_SERVICE_MANAGER_H__
#define __MOCK_SERVICE_MANAGER_H__

/* "mock_" prefix: full module replacement (replaces service_manager).
 * "cm_" prefix: cmocka mock wrappers for individual API functions. */

#include "bt_profile.h"

void mock_service_manager_register(enum profile_id id, void* iface);
void mock_service_manager_reset(void);

#endif /* __MOCK_SERVICE_MANAGER_H__ */
