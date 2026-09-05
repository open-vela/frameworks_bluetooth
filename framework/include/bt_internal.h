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

#include "bt_config.h"

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

#if defined(CONFIG_CPU_BIT64) // CONFIG_CPU_BIT64
#define PTR uint64_t
#define PRTx PRIx64
#if !defined(INT2PTR)
#define INT2PTR(pt) (pt)(uint64_t) // For example, INT2PTR(pt)(int): (int)=>uint64_t=>(pt)/pointer type
#endif
#if !defined(PTR2INT)
#define PTR2INT(it) (it)(uint64_t) // For example, PTR2INT(it)(pointer): (pointer)=>uint64_t=>(it)/int type
#endif
#else // CONFIG_CPU_BIT32 and others
#define PTR uint32_t
#define PRTx PRIx32
#if !defined(INT2PTR)
#define INT2PTR(pt) (pt)(uint32_t) // For example, INT2PTR(pt)(int): (int)=>uint32_t=>(pt)/pointer type
#endif
#if !defined(PTR2INT)
#define PTR2INT(it) (it)(uint32_t) // For example, PTR2INT(it)(pointer): (pointer)=>uint32_t=>(it)/int type
#endif
#endif // End of else

#ifdef __cplusplus
}
#endif

#endif /* _BT_INTERNAL_H__ */
