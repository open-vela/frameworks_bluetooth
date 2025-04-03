/***********************************************************************
 *
 * Copyright 2025 XiaoMi All Rights Reserved.
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

#ifndef __VELA_MIBLE_MANAGE_H__
#define __VELA_MIBLE_MANAGE_H__

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void* vela_mible_svc_ins_get(void);
int vela_mible_manager_deinit(void);
int vela_mible_manager_init(void);
void vela_mible_register_disable_callback(int32_t (*callback)(bool status));
void vela_mible_unregister_disable_callback(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__VELA_MIBLE_MANAGE_H__
