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
#include "bt_trace.h"
#include "bluetooth.h"
#include "bt_internal.h"
#include "utils/btsnoop_log.h"
#include "utils/log.h"

void BTSYMBOLS(bluetooth_enable_btsnoop_log)(bt_instance_t* ins)
{
    bt_log_module_enable(LOG_ID_SNOOP, false);
}

void BTSYMBOLS(bluetooth_disable_btsnoop_log)(bt_instance_t* ins)
{
    bt_log_module_disable(LOG_ID_SNOOP, false);
}

void BTSYMBOLS(bluetooth_set_btsnoop_filter)(bt_instance_t* ins, btsnoop_filter_flag_t filter_flag)
{
    btsnoop_set_filter(filter_flag);
}

void BTSYMBOLS(bluetooth_remove_btsnoop_filter)(bt_instance_t* ins, btsnoop_filter_flag_t filter_flag)
{
    btsnoop_remove_filter(filter_flag);
}
