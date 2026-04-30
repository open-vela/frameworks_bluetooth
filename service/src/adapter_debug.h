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
#ifndef __ADAPTER_DEBUG_H__
#define __ADAPTER_DEBUG_H__

#include <nuttx/config.h>

#ifdef CONFIG_BLUETOOTH_ADAPTER_DEBUG

void adapter_dump_whitelist(void);

#else

#define adapter_dump_whitelist()

#endif /* CONFIG_BLUETOOTH_ADAPTER_DEBUG */

#endif /* __ADAPTER_DEBUG_H__ */
