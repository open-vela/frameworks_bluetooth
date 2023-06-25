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
#include <stdlib.h>
#include <unistd.h>

#ifdef CONFIG_BLUETOOTH_FRAMEWORK_LOCAL
#include "btservice.h"
#include "service_loop.h"
#include "manager_service.h"
#endif
#include "bluetooth.h"

/*

*/
bt_instance_t *bluetooth_create_instance(void)
{
    uint32_t app_id;
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_LOCAL
    service_loop_init();
    bt_service_init();
    service_loop_run(true);
#endif
    bt_instance_t *ins = malloc(sizeof(bt_instance_t));
    if (!ins) {
        return NULL;
    }

    pid_t pid = getpid();

    bt_status_t status = manager_create_instance((uint32_t)ins, BLUETOOTH_SYSTEM, "local", pid, 0, &app_id);
    if (status != BT_STATUS_SUCCESS) {
        free(ins);
        return NULL;
    }

    ins->app_id = app_id;

    return ins;
}

bt_instance_t *bluetooth_get_instance(void)
{
    uint32_t handle = 0;
    pid_t pid = getpid();

    handle = manager_get_instance("local", pid, &handle);
    if (handle)
        return (bt_instance_t *)handle;
    else
        return bluetooth_create_instance();
}

void *bluetooth_get_proxy(bt_instance_t *ins, enum profile_id id)
{
    switch (id) {
    case PROFILE_HFP_HF:
        /* for binder ipc*/
        if (!ins->hfp_hf_proxy) {
            ins->hfp_hf_proxy = NULL;
        }
        return ins->hfp_hf_proxy;

    default:
        break;
    }
    return NULL;
}

void bluetooth_delete_instance(bt_instance_t *ins)
{
    manager_delete_instance(ins->app_id);
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_LOCAL
    bt_service_cleanup();
    service_loop_exit();
#endif
    free(ins);
}

bt_status_t bluetooth_start_service(bt_instance_t *ins, enum profile_id id)
{
    return manager_start_service(ins->app_id, id);
}

bt_status_t bluetooth_stop_service(bt_instance_t *ins, enum profile_id id)
{
    return manager_stop_service(ins->app_id, id);
}