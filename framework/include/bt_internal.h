/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef _BT_INTERNAL_H__
#define _BT_INTERNAL_H__

#include <nuttx/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BTSYMBOLS
#undef BTSYMBOLS
#endif

#ifdef CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC
#define BTSYMBOLS(s) server_##s
#else
#define BTSYMBOLS(s) s
#endif

#ifdef __cplusplus
}
#endif

#endif /* _BT_INTERNAL_H__ */
