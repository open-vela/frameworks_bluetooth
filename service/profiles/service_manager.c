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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bt_profile.h"
#include "service_manager.h"

#define LOG_TAG "service_manager"
#include "utils/log.h"
static profile_service_t *service_slots[PROFILE_MAX];

void register_service(const profile_service_t *service)
{
    if (!service_slots[service->id]) {
        service_slots[service->id] = (profile_service_t *)service;
        BT_LOGW("%s service register success", service->name);
    } else
        BT_LOGW("%s service had registered", service->name);
}

int service_manager_init(void)
{
    for (int i = 0; i < PROFILE_MAX; i++) {
        if (service_slots[i] && service_slots[i]->init)
            service_slots[i]->init();
    }

    return 0;
}

int service_manager_startup(uint8_t transport)
{
    for (int i = 0; i < PROFILE_MAX; i++) {
        profile_service_t *profile = service_slots[i];
        if (profile && profile->startup && profile->auto_start &&
            profile->transport == transport)
            profile->startup(NULL);
    }

    return 0;
}

int service_manager_shutdown(uint8_t transport)
{
    for (int i = 0; i < PROFILE_MAX; i++) {
        profile_service_t *profile = service_slots[i];
        if (profile && profile->shutdown && profile->transport == transport)
            profile->shutdown(NULL);
    }

    return 0;
}

const void *service_manager_get_profile(enum profile_id id)
{
    assert(id < PROFILE_MAX && service_slots[id] && service_slots[id]->get_profile_interface);

    return service_slots[id]->get_profile_interface();
}

bt_status_t service_manager_control(enum profile_id id, control_cmd_t cmd)
{
    switch (cmd) {
    case CONTROL_CMD_START:
        if (service_slots[id] && service_slots[id]->startup)
            return service_slots[id]->startup(NULL);
        break;
    case CONTROL_CMD_STOP:
        if (service_slots[id] && service_slots[id]->shutdown)
            return service_slots[id]->shutdown(NULL);
        break;
    case CONTROL_CMD_DUMP:
        break;
    default:
        break;
    }

    return BT_STATUS_SUCCESS;
}

int service_manager_cleanup(void)
{
    for (int i = 0; i < PROFILE_MAX; i++) {
        if (service_slots[i] && service_slots[i]->cleanup) {
            service_slots[i]->cleanup();
        }
        service_slots[i] = NULL;
    }

    return 0;
}
