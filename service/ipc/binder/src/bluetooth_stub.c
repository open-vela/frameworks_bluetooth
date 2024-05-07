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

#include <stdint.h>
#include <string.h>
#include <uchar.h>

#include <android/binder_ibinder_platform.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "binder_utils.h"
#include "bluetooth.h"
#include "bluetooth_proxy.h"
#include "bluetooth_stub.h"
#include "manager_service.h"
#include "utils/log.h"

#define BT_MANAGER_DESC "BluetoothManager"
static const AIBinder_Class* kIBtManager_Class = NULL;

static void* IBtManager_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtManager_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtManager_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* reply)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;

    switch (code) {
    case IBLUETOOTH_CREATE_INSTANCE: {
        uid_t uid = AIBinder_getCallingUid();
        pid_t pid = AIBinder_getCallingPid();
        char* hostName = NULL;
        uint32_t handle, type, appId = 0xFF;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &hostName,
            AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        bt_status_t status = manager_create_instance(handle, type, hostName, (pid_t)pid, uid, &appId);
        free(hostName);

        stat = AParcel_writeUint32(reply, appId);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, status);
        if (stat != STATUS_OK)
            return stat;
        break;
    }
    case IBLUETOOTH_DELETE_INSTANCE: {
        uint32_t appId = 0xFF;

        stat = AParcel_readUint32(in, &appId);
        if (stat != STATUS_OK)
            return stat;
        bt_status_t status = manager_delete_instance(appId);

        stat = AParcel_writeUint32(reply, status);
        if (stat != STATUS_OK)
            return stat;

        break;
    }
    case IBLUETOOTH_GET_INSTANCE: {
        uid_t uid = AIBinder_getCallingUid();
        pid_t pid = AIBinder_getCallingPid();
        char* hostName = NULL;
        uint32_t handle;

        stat = AParcel_readString(in, &hostName,
            AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        BT_LOGD("UID:%d, PID:%d, hostname:%s", uid, pid, hostName);
        bt_status_t status = manager_get_instance(hostName, (pid_t)pid, &handle);
        free(hostName);

        stat = AParcel_writeUint32(reply, handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, status);
        if (stat != STATUS_OK)
            return stat;
        break;
    }
    case IBLUETOOTH_START_SERVICE: {
        uint32_t profileId, appId = 0xFF;

        stat = AParcel_readUint32(in, &appId);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &profileId);
        if (stat != STATUS_OK)
            return stat;

        bt_status_t status = manager_start_service(appId, profileId);

        stat = AParcel_writeUint32(reply, status);
        if (stat != STATUS_OK)
            return stat;

        break;
    }
    case IBLUETOOTH_STOP_SERVICE: {
        uint32_t profileId, appId = 0xFF;

        stat = AParcel_readUint32(in, &appId);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &profileId);
        if (stat != STATUS_OK)
            return stat;

        bt_status_t status = manager_stop_service(appId, profileId);

        stat = AParcel_writeUint32(reply, status);
        if (stat != STATUS_OK)
            return stat;
        break;
    }
    default:
        break;
    }

    return stat;
}

static const AIBinder_Class* BtManager_getClass(void)
{
    if (!kIBtManager_Class) {
        kIBtManager_Class = AIBinder_Class_define(BT_MANAGER_DESC, IBtManager_Class_onCreate,
            IBtManager_Class_onDestroy, IBtManager_Class_onTransact);
    }

    return kIBtManager_Class;
}

static AIBinder* BtManager_getBinder(IBtManager* manager)
{
    AIBinder* binder = NULL;

    if (manager->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(manager->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(manager->clazz, (void*)manager);
        if (manager->WeakBinder != NULL) {
            AIBinder_Weak_delete(manager->WeakBinder);
        }

        manager->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtManager_addService(IBtManager* manager, const char* instance)
{
    manager->clazz = (AIBinder_Class*)BtManager_getClass();
    AIBinder* binder = BtManager_getBinder(manager);
    manager->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtManager* BpBtManager_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtManager* bpBinder = NULL;

    clazz = (AIBinder_Class*)BtManager_getClass();
    binder = AServiceManager_getService(instance);
    if (!binder)
        return NULL;

    if (!AIBinder_associateClass(binder, clazz))
        goto bail;

    if (!AIBinder_isRemote(binder))
        goto bail;

    /* linktoDeath ? */

    bpBinder = malloc(sizeof(*bpBinder));
    if (!bpBinder)
        goto bail;

    bpBinder->binder = binder;
    bpBinder->clazz = clazz;

    return bpBinder;

bail:
    AIBinder_decStrong(binder);
    return NULL;
}

void BpBtManager_delete(BpBtManager* bpManager)
{
    AIBinder_decStrong(bpManager->binder);
    free(bpManager);
}

AIBinder* BtManager_getService(BpBtManager** bpManager, const char* instance)
{
    BpBtManager* bpBinder = *bpManager;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtManager_new(instance);
    if (!bpBinder)
        return NULL;

    *bpManager = bpBinder;

    return bpBinder->binder;
}

void Bluetooth_joinThreadPool(void)
{
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    ABinderProcess_joinThreadPool();
}

void Bluetooth_startThreadPool(void)
{
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    ABinderProcess_startThreadPool();
}

binder_status_t Bluetooth_setupPolling(int* fd)
{
    return ABinderProcess_setupPolling(fd);
}

binder_status_t Bluetooth_handlePolledCommands(void)
{
    return ABinderProcess_handlePolledCommands();
}
