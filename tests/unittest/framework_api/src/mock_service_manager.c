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

#include "mock_service_manager.h"
#include "bt_profile.h"
#include <stddef.h>

static void* g_mock_profiles[PROFILE_MAX];

void mock_service_manager_register(enum profile_id id, void* iface)
{
    if (id < PROFILE_MAX) {
        g_mock_profiles[id] = iface;
    }
}

void mock_service_manager_reset(void)
{
    int i;
    for (i = 0; i < PROFILE_MAX; i++) {
        g_mock_profiles[i] = NULL;
    }
}

const void* service_manager_get_profile(enum profile_id id)
{
    if (id < PROFILE_MAX) {
        return g_mock_profiles[id];
    }
    return NULL;
}
