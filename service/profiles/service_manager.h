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
#ifndef _BT_SERVICE_MANAGER_H__
#define _BT_SERVICE_MANAGER_H__

#include "bt_profile.h"
#include "bt_status.h"
#include "bt_uuid.h"

typedef enum {
    SERVICE_DISABLED = 0,
    SERVICE_ENABLED
} service_state_t;

typedef enum control_cmd {
    CONTROL_CMD_START,
    CONTROL_CMD_STOP,
    CONTROL_CMD_DUMP
} control_cmd_t;

typedef void (*profile_on_startup_t)(void *context);
typedef void (*profile_on_shutdown_t)(void *context);
typedef struct profile_service {
    bool auto_start;
    const char *name;
    const enum profile_id id;
    uint8_t transport;
    bt_uuid_t uuid;
    bt_status_t (*init)(void);
    bt_status_t (*startup)(profile_on_startup_t cb);
    bt_status_t (*shutdown)(profile_on_shutdown_t cb);
    void (*process_msg)(void *msg);
    int (*get_state)(void);
    const void *(*get_profile_interface)(void);
    void (*cleanup)(void);
    int (*dump)(void);
} profile_service_t;

void register_service(const profile_service_t *service);
int service_manager_init(void);
int service_manager_startup(uint8_t transport);
int service_manager_shutdown(uint8_t transport);
const void *service_manager_get_profile(enum profile_id id);
bt_status_t service_manager_control(enum profile_id id, control_cmd_t cmd);
int service_manager_cleanup(void);

#endif /* _BT_SERVICE_MANAGER_H__ */