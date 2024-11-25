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

#ifndef __BT_GATTC_CALLBACKS_PROXY_H__
#define __BT_GATTC_CALLBACKS_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "bt_gattc.h"

#include <android/binder_manager.h>

const gattc_callbacks_t* BpBtGattClientCallbacks_getStatic(void);

#ifdef __cplusplus
}
#endif
#endif /* __BT_GATTC_CALLBACKS_PROXY_H__ */