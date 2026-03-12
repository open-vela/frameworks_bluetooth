/***********************************************************************
 *
 * Copyright 2026 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#ifndef __Z_API_MANAGER_H__
#define __Z_API_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "z_api.h"

/* Manager APIs */
void* z_api(bt_svc_ins_get)(void);
int z_api(bt_manager_init)(void);
int z_api(bt_manager_deinit)(void);
void z_api(bt_register_disable_callback)(int32_t (*callback)(bool status));
void z_api(bt_unregister_disable_callback)(void);

#ifdef __cplusplus
}
#endif

#endif /* __Z_API_MANAGER_H__ */
