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
<<<<<<< HEAD
#include "bt_internal.h"
#include "manager_service.h"
=======
>>>>>>> bluetooth framework re-implement base

/*

*/
<<<<<<< HEAD
bt_instance_t *BTSYMBOLS(bluetooth_create_instance)(void)
=======
bt_instance_t *bluetooth_create_instance(void)
>>>>>>> bluetooth framework re-implement base
{
    uint32_t app_id;
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_LOCAL
    service_loop_init();
    bt_service_init();
<<<<<<< HEAD
    service_loop_run(true, "bt_service");
#endif
    bt_instance_t *ins = zalloc(sizeof(bt_instance_t));
    if (!ins) {
        return NULL;
    }

    bt_status_t status = manager_create_instance((uint32_t)ins, BLUETOOTH_SYSTEM, "local", getpid(), 0, &app_id);
=======
    service_loop_run(true);
#endif
    bt_instance_t *ins = malloc(sizeof(bt_instance_t));
    pid_t pid = getpid();

    bt_status_t status = manager_create_instance((uint32_t)ins, BLUETOOTH_SYSTEM, "local", pid, 0, &app_id);
>>>>>>> bluetooth framework re-implement base
    if (status != BT_STATUS_SUCCESS) {
        free(ins);
        return NULL;
    }

    ins->app_id = app_id;

    return ins;
}

<<<<<<< HEAD
bt_instance_t *BTSYMBOLS(bluetooth_get_instance)(void)
{
    bt_status_t status;
    uint32_t handle;

    status = manager_get_instance("local", getpid(), &handle);
    if (status == BT_STATUS_SUCCESS && handle)
        return (bt_instance_t *)handle;
    else
        return BTSYMBOLS(bluetooth_create_instance)();
}

void *BTSYMBOLS(bluetooth_get_proxy)(bt_instance_t *ins, enum profile_id id)
=======
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
>>>>>>> bluetooth framework re-implement base
{
    switch (id) {
    case PROFILE_HFP_HF:
        /* for binder ipc*/
<<<<<<< HEAD
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_BINDER_IPC
=======
>>>>>>> bluetooth framework re-implement base
        if (!ins->hfp_hf_proxy) {
            ins->hfp_hf_proxy = NULL;
        }
        return ins->hfp_hf_proxy;
<<<<<<< HEAD
#endif
=======
>>>>>>> bluetooth framework re-implement base

    default:
        break;
    }
    return NULL;
}

<<<<<<< HEAD
void BTSYMBOLS(bluetooth_delete_instance)(bt_instance_t *ins)
=======
void bluetooth_delete_instance(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    manager_delete_instance(ins->app_id);
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_LOCAL
    bt_service_cleanup();
    service_loop_exit();
#endif
    free(ins);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bluetooth_start_service)(bt_instance_t *ins, enum profile_id id)
=======
bt_status_t bluetooth_start_service(bt_instance_t *ins, enum profile_id id)
>>>>>>> bluetooth framework re-implement base
{
    return manager_start_service(ins->app_id, id);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bluetooth_stop_service)(bt_instance_t *ins, enum profile_id id)
{
    return manager_stop_service(ins->app_id, id);
}

#include "uv.h"
bool BTSYMBOLS(bluetooth_set_external_uv)(bt_instance_t *ins, uv_loop_t *ext_loop)
{
    return false;
=======
bt_status_t bluetooth_stop_service(bt_instance_t *ins, enum profile_id id)
{
    return manager_stop_service(ins->app_id, id);
>>>>>>> bluetooth framework re-implement base
}