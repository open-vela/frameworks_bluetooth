/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef _BT_MEMORY_H_
#define _BT_MEMORY_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BLUETOOTH_DEBUG_MEMORY)
void* bt_malloc_hook(size_t size, const char* file, int line);
void* bt_calloc_hook(size_t n, size_t size, const char* file, int line);
void bt_free_hook(void* ptr);
void bt_report_leak(void);

#define bt_malloc(size) bt_malloc_hook(size, __FILE__, __LINE__)
#define bt_calloc(n, size) bt_calloc_hook(n, size, __FILE__, __LINE__)
#define bt_free(ptr) bt_free_hook(ptr)
#define bt_zalloc(size) bt_calloc(1, size)
#else
#define bt_malloc(size) malloc(size)
#define bt_calloc(n, size) calloc(n, size)
#define bt_free(ptr) free(ptr)
#define bt_zalloc(size) zalloc(size)
#endif // CONFIG_BLUETOOTH_DEBUG_MEMORY

#ifdef __cplusplus
}
#endif

#endif // _BT_MEMORY_H_