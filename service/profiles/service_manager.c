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

#include "adapter_internel.h"
#include "bt_profile.h"
#include "service_manager.h"

#define LOG_TAG "service_manager"
#include "utils/log.h"

enum profile_service_state {
    TURN_OFF,
    TURNING_ON,
    TURN_ON,
    TURNING_OFF,
};

struct service_state_map {
    profile_service_t* service;
    uint8_t state;
};

static struct service_state_map service_slots[PROFILE_MAX] = { 0 };
// static profile_service_t *service_slots[PROFILE_MAX];

static bool check_is_all_startup(uint8_t transport)
{
    for (int i = 0; i < PROFILE_MAX; i++) {
        profile_service_t* profile = service_slots[i].service;
        int state;

        if (!profile) {
            // check unregistered profile service
            continue;
        }

        state = profile->get_state ? profile->get_state() : 1;
        if ((profile->transport == transport) && profile->auto_start && (service_slots[i].state != TURN_ON) && state) {
            return false;
        }
    }

    return true;
}

static bool check_is_all_shutdown(uint8_t transport)
{
    for (int i = 0; i < PROFILE_MAX; i++) {
        profile_service_t* profile = service_slots[i].service;
        int state;

        if (!profile) {
            // check unregistered profile service
            continue;
        }

        state = profile->get_state ? profile->get_state() : 1;
        if ((profile->transport == transport) && (service_slots[i].state != TURN_OFF) && state) {
            return false;
        }
    }

    return true;
}

static void service_on_startup(enum profile_id id, bool ret)
{
    assert(service_slots[id].service);

    profile_service_t* profile = service_slots[id].service;
    BT_LOGD("%s {%s} start ret:%d", __func__, profile->name, ret);

    if (ret)
        service_slots[id].state = TURN_ON;
    else {
        /* notify startup fail??? */
        assert(0);
    }

    if (check_is_all_startup(profile->transport)) {
        BT_LOGD("%s all profiles is startup", __func__);
        adapter_on_profile_services_startup(profile->transport, true);
    }
}

static void service_on_shutdown(enum profile_id id, bool ret)
{
    assert(service_slots[id].service);

    profile_service_t* profile = service_slots[id].service;
    BT_LOGD("%s {%s} shutdown ret:%d", __func__, profile->name, ret);

    if (ret)
        service_slots[id].state = TURN_OFF;
    else {
        /* notify shutdown fail??? */
        assert(0);
    }

    if (check_is_all_shutdown(profile->transport)) {
        BT_LOGD("%s all profile is shutdown", __func__);
        adapter_on_profile_services_shutdown(profile->transport, true);
    }
}

void register_service(const profile_service_t* service)
{
    if (!service_slots[service->id].service) {
        service_slots[service->id].service = (profile_service_t*)service;
        service_slots[service->id].state = TURN_OFF;
        BT_LOGD("%s service register success", service->name);
    } else
        BT_LOGW("%s service had registered", service->name);
}

int service_manager_init(void)
{
    for (int i = 0; i < PROFILE_MAX; i++) {
        profile_service_t* profile = service_slots[i].service;
        if (profile && profile->init)
            profile->init();
    }

    return 0;
}

int service_manager_startup(uint8_t transport)
{
    if (check_is_all_startup(transport)) {
        BT_LOGD("%s all profile is startup", __func__);
        adapter_on_profile_services_startup(transport, true);
    } else {
        for (int i = 0; i < PROFILE_MAX; i++) {
            profile_service_t* profile = service_slots[i].service;
            if (profile && profile->startup && profile->auto_start && profile->transport == transport) {
                service_slots[i].state = TURNING_ON;
                profile->startup(service_on_startup);
            }
        }
    }

    return 0;
}

int service_manager_processmsg(profile_msg_t* msg)
{
    for (int i = 0; i < PROFILE_MAX; i++) {
        profile_service_t* profile = service_slots[i].service;
        if (profile && profile->process_msg)
            profile->process_msg(msg);
    }

    return 0;
}

int service_manager_shutdown(uint8_t transport)
{
    if (check_is_all_shutdown(transport)) {
        BT_LOGD("%s all profile is shutdown", __func__);
        adapter_on_profile_services_shutdown(transport, true);
    } else {
        for (int i = 0; i < PROFILE_MAX; i++) {
            profile_service_t* profile = service_slots[i].service;
            if (profile && profile->shutdown && profile->transport == transport)
                profile->shutdown(service_on_shutdown);
        }
    }

    return 0;
}

const void* service_manager_get_profile(enum profile_id id)
{
    assert(id < PROFILE_MAX);
    profile_service_t* profile = service_slots[id].service;
    if (!profile || !profile->get_profile_interface) {
        BT_LOGE("%s profile-id:%d is not found, profile:%p\n", __func__, id, profile);
        assert(0);
    }

    return profile->get_profile_interface();
}

bt_status_t service_manager_control(enum profile_id id, control_cmd_t cmd)
{
    profile_service_t* profile = service_slots[id].service;

    switch (cmd) {
    case CONTROL_CMD_START:
        if (profile && profile->startup)
            return profile->startup(NULL);
        else {
            BT_LOGE("startup not implemented");
            return BT_STATUS_NOT_SUPPORTED;
        }
        break;
    case CONTROL_CMD_STOP:
        if (profile && profile->shutdown)
            return profile->shutdown(NULL);
        else {
            BT_LOGE("shutdown not implemented");
            return BT_STATUS_NOT_SUPPORTED;
        }
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
        profile_service_t* profile = service_slots[i].service;
        if (!profile)
            continue;

        if (profile->cleanup) {
            profile->cleanup();
        } else {
            BT_LOGE("%s profile cleanup method is NULL", profile->name);
        }
        service_slots[i].service = NULL;
    }

    return 0;
}
