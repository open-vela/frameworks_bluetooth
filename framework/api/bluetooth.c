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
#include "manager_service.h"
#include "service_loop.h"
#endif
#include "bluetooth.h"
#include "bt_internal.h"
#include "manager_service.h"

/*

*/
bt_instance_t* BTSYMBOLS(bluetooth_create_instance)(void)
{
    uint32_t app_id;
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_LOCAL
    service_loop_init();
    bt_service_init();
    service_loop_run(true, "bt_service");
#endif
    bt_instance_t* ins = zalloc(sizeof(bt_instance_t));
    if (!ins) {
        return NULL;
    }

    bt_status_t status = manager_create_instance(PTR2INT(uint64_t) ins, BLUETOOTH_SYSTEM, "local", getpid(), 0, &app_id);
    if (status != BT_STATUS_SUCCESS) {
        free(ins);
        return NULL;
    }

    ins->app_id = app_id;

    return ins;
}

bt_instance_t* BTSYMBOLS(bluetooth_get_instance)(void)
{
    bt_status_t status;
    uint64_t handle;

    status = manager_get_instance("local", getpid(), &handle);
    if (status == BT_STATUS_SUCCESS && handle)
        return INT2PTR(bt_instance_t*) handle;
    else
        return BTSYMBOLS(bluetooth_create_instance)();
}

void* BTSYMBOLS(bluetooth_get_proxy)(bt_instance_t* ins, enum profile_id id)
{
    switch (id) {
    case PROFILE_HFP_HF:
        /* for binder ipc*/
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_BINDER_IPC
        if (!ins->hfp_hf_proxy) {
            ins->hfp_hf_proxy = NULL;
        }
        return ins->hfp_hf_proxy;
#endif

    default:
        break;
    }
    return NULL;
}

void BTSYMBOLS(bluetooth_delete_instance)(bt_instance_t* ins)
{
    manager_delete_instance(ins->app_id);
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_LOCAL
    bt_service_cleanup();
    service_loop_exit();
#endif
    free(ins);
}

bt_status_t BTSYMBOLS(bluetooth_start_service)(bt_instance_t* ins, enum profile_id id)
{
    return manager_start_service(ins->app_id, id);
}

bt_status_t BTSYMBOLS(bluetooth_stop_service)(bt_instance_t* ins, enum profile_id id)
{
    return manager_stop_service(ins->app_id, id);
}

#include "uv.h"
bool BTSYMBOLS(bluetooth_set_external_uv)(bt_instance_t* ins, uv_loop_t* ext_loop)
{
    return false;
}