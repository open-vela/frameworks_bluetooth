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

#ifdef CONFIG_BLUETOOTH_FRAMEWORK_BINDER_IPC
#include "adapter_proxy.h"
#include "adapter_stub.h"
#include "bluetooth_proxy.h"
#include "gattc_proxy.h"
#include "gatts_proxy.h"
#include "hfp_ag_proxy.h"
#include "hfp_hf_proxy.h"
#include "hid_device_proxy.h"
#include "pan_proxy.h"
#include "spp_proxy.h"
#endif
#include "bluetooth.h"

bt_instance_t* bluetooth_create_instance(void)
{
    AIBinder* binder;
    char name[64] = { 0 };

    bt_instance_t* ins = zalloc(sizeof(bt_instance_t));
    if (!ins) {
        return NULL;
    }

    binder = BtManager_getService((BpBtManager**)&ins->manager_proxy, MANAGER_BINDER_INSTANCE);
    if (!binder)
        goto bail;

    gethostname(name, 64);
    bt_status_t status = BpBtManager_createInstance(binder, (uint32_t)ins, BLUETOOTH_SYSTEM, name, &ins->app_id);
    if (status != BT_STATUS_SUCCESS)
        goto bail;

    BtAdapter_getService((BpBtAdapter**)&ins->adapter_proxy, ADAPTER_BINDER_INSTANCE);
    Bluetooth_startThreadPool();

    return ins;

bail:
    free(ins);
    return NULL;
}

bt_instance_t* bluetooth_get_instance(void)
{
    uint32_t handle;
    char name[64] = { 0 };
    AIBinder* binder;
    BpBtManager* proxy = NULL;

    /* find instance from bluetooth manager by hostname,
        if not found, create new instance for this hostname
    */

    binder = BtManager_getService(&proxy, MANAGER_BINDER_INSTANCE);
    if (!binder)
        return NULL;

    gethostname(name, 64);
    bt_status_t status = BpBtManager_getInstance(binder, name, &handle);
    BpBtManager_delete(proxy);
    if (status == BT_STATUS_SUCCESS && handle)
        return (bt_instance_t*)handle;
    else
        return bluetooth_create_instance();
}

void* bluetooth_get_proxy(bt_instance_t* ins, enum profile_id id)
{
    switch (id) {
    case PROFILE_HFP_HF:
        /* for binder ipc*/
        if (!ins->hfp_hf_proxy) {
            ins->hfp_hf_proxy = BpBtHfpHf_new(HFP_HF_BINDER_INSTANCE);
        }
        return ins->hfp_hf_proxy;
    case PROFILE_HFP_AG: {
        if (!ins->hfp_ag_proxy) {
            ins->hfp_ag_proxy = BpBtHfpAg_new(HFP_AG_BINDER_INSTANCE);
        }
        return ins->hfp_ag_proxy;
    }
    case PROFILE_SPP: {
        if (!ins->spp_proxy) {
            ins->spp_proxy = BpBtSpp_new(SPP_BINDER_INSTANCE);
        }
        return ins->spp_proxy;
    }
    case PROFILE_HID_DEV: {
        if (!ins->hidd_proxy) {
            ins->hidd_proxy = BpBtHidd_new(HID_DEVICE_BINDER_INSTANCE);
        }
        return ins->hidd_proxy;
    }
    case PROFILE_PANU: {
        if (!ins->pan_proxy) {
            ins->pan_proxy = BpBtPan_new(PAN_BINDER_INSTANCE);
        }
        return ins->pan_proxy;
    }
    case PROFILE_GATTC: {
        if (!ins->gattc_proxy) {
            ins->gattc_proxy = BpBtGattClient_new(GATT_CLIENT_BINDER_INSTANCE);
        }
        return ins->gattc_proxy;
    }
    case PROFILE_GATTS: {
        if (!ins->gatts_proxy) {
            ins->gatts_proxy = BpBtGattServer_new(GATT_SERVER_BINDER_INSTANCE);
        }
        return ins->gatts_proxy;
    }
    default:
        break;
    }
    return NULL;
}

void bluetooth_delete_instance(bt_instance_t* ins)
{
    if (ins->hfp_hf_proxy)
        BpBtHfpHf_delete(ins->hfp_hf_proxy);

    if (ins->hfp_ag_proxy)
        BpBtHfpAg_delete(ins->hfp_ag_proxy);

    if (ins->spp_proxy)
        BpBtSpp_delete(ins->spp_proxy);

    if (ins->pan_proxy)
        BpBtPan_delete(ins->pan_proxy);

    if (ins->gattc_proxy)
        BpBtGattClient_delete(ins->gattc_proxy);

    if (ins->gatts_proxy)
        BpBtGattServer_delete(ins->gatts_proxy);

    BpBtAdapter_delete(ins->adapter_proxy);
    BpBtManager_deleteInstance(ins->manager_proxy, ins->app_id);
    BpBtManager_delete(ins->manager_proxy);
    free(ins);
}

bt_status_t bluetooth_start_service(bt_instance_t* ins, enum profile_id id)
{
    return BpBtManager_startService(ins->manager_proxy, ins->app_id, id);
}

bt_status_t bluetooth_stop_service(bt_instance_t* ins, enum profile_id id)
{
    return BpBtManager_stopService(ins->manager_proxy, ins->app_id, id);
}

#include "uv.h"
bool bluetooth_set_external_uv(bt_instance_t* ins, uv_loop_t* ext_loop)
{
    return false;
}