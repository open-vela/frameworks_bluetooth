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
#ifndef __MANAGER_SERVICE_H__
#define __MANAGER_SERVICE_H__

#include <stdint.h>
#include <unistd.h>

#include "bt_profile.h"
#include "bt_status.h"

void manager_init(void);
void manager_cleanup(void);
bt_status_t manager_create_instance(uint64_t handle, uint32_t type,
    const char* name, pid_t pid, uid_t uid,
    uint32_t* app_id);
bt_status_t manager_get_instance(const char* name, pid_t pid, uint64_t* handle);
bt_status_t manager_delete_instance(uint32_t app_id);
bt_status_t manager_start_service(uint32_t app_id, enum profile_id profile);
bt_status_t manager_stop_service(uint32_t app_id, enum profile_id profile);

#endif /* __MANAGER_SERVICE_H__ */
