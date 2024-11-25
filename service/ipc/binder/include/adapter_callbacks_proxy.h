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

#ifndef __ADAPTER_CALLBACKS_PROXY_H__
#define __ADAPTER_CALLBACKS_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <android/binder_manager.h>
// #include <android/binder_auto_utils.h>
const adapter_callbacks_t* BpBtAdapterCallbacks_getStatic(void);

#ifdef __cplusplus
}
#endif
#endif /* __ADAPTER_CALLBACKS_PROXY_H__ */