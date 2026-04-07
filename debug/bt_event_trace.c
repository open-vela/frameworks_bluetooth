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
#include "bt_event_trace.h"

#include <stdbool.h>
#include <stdatomic.h>

atomic_bool g_bt_trace_enabled = false;

void bt_trace_init(void)
{
    atomic_store(&g_bt_trace_enabled, false);
}

void bt_trace_start(void)
{
    atomic_store(&g_bt_trace_enabled, true);
}

void bt_trace_stop(void)
{
    atomic_store(&g_bt_trace_enabled, false);
}
