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

#include "bluetooth_ipc.h"
#include "service_loop.h"
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_BINDER_IPC
#include "adapter_stub.h"
#include "bluetooth_stub.h"
#include "gattc_stub.h"
#include "gatts_stub.h"
#include "hfp_ag_stub.h"
#include "hfp_hf_stub.h"
#include "hid_device_stub.h"
#include "pan_stub.h"
#include "spp_stub.h"
#endif
#include "utils/log.h"

#include "bt_socket.h"

#ifdef CONFIG_BLUETOOTH_FRAMEWORK_BINDER_IPC
static IBtManager binderManager = { 0 };
static IBtAdapter binderAdapter = { 0 };

#ifdef CONFIG_BLUETOOTH_HFP_AG
static IBtHfpAg binderHfpAg = { 0 };
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
static IBtHfpHf binderHfpHf = { 0 };
#endif
#ifdef CONFIG_BLUETOOTH_SPP
static IBtSpp binderSpp = { 0 };
#endif
#ifdef CONFIG_BLUETOOTH_HID_DEVICE
static IBtHidd binderHidd = { 0 };
#endif
#ifdef CONFIG_BLUETOOTH_PAN
static IBtPan binderPan = { 0 };
#endif
#ifdef CONFIG_BLUETOOTH_GATT
static IBtGattClient binderGattc = { 0 };
static IBtGattServer binderGatts = { 0 };
#endif

static void ipc_pollin_process(service_poll_t* poll, int revent, void* userdata)
{
    Bluetooth_handlePolledCommands();
}

static int add_ipc_poll_setup(void* data)
{
    binder_status_t stat;
    service_poll_t* poll;
    int fd = -1;

    stat = Bluetooth_setupPolling(&fd);
    if (stat != STATUS_OK || fd <= 0) {
        BT_LOGD("%s :%d", __func__, stat);
        return fd;
    }

    poll = service_loop_poll_fd(fd, POLL_READABLE, ipc_pollin_process, NULL);
    if (poll == NULL) {
        BT_LOGD("%s setup poll failed", __func__);
        return -1;
    }

    return 0;
}

bt_status_t bluetooth_ipc_add_services(void)
{
    binder_status_t stat = BtManager_addService(&binderManager, MANAGER_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add Manager Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }

    stat = BtAdapter_addService(&binderAdapter, ADAPTER_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add Adapter Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }

#ifdef CONFIG_BLUETOOTH_HFP_HF
    stat = BtHfpHf_addService(&binderHfpHf, HFP_HF_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add HfpHf Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }
#endif

#ifdef CONFIG_BLUETOOTH_HFP_AG
    stat = BtHfpAg_addService(&binderHfpAg, HFP_AG_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add HfpAg Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }
#endif

#ifdef CONFIG_BLUETOOTH_SPP
    stat = BtSpp_addService(&binderSpp, SPP_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add Spp Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }
#endif

#ifdef CONFIG_BLUETOOTH_HID_DEVICE
    stat = BtHidd_addService(&binderHidd, HID_DEVICE_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add HidDevice Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }
#endif

#ifdef CONFIG_BLUETOOTH_PAN
    stat = BtPan_addService(&binderPan, PAN_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add Pan Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }
#endif

#ifdef CONFIG_BLUETOOTH_GATT
    stat = BtGattClient_addService(&binderGattc, GATT_CLIENT_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add Gattc Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }
    stat = BtGattServer_addService(&binderGatts, GATT_SERVER_BINDER_INSTANCE);
    if (stat != STATUS_OK) {
        BT_LOGD("Add Gatts Service Failed:%d", stat);
        return BT_STATUS_IPC_ERROR;
    }
#endif

    return BT_STATUS_SUCCESS;
}

void bluetooth_ipc_join_thread_pool(void)
{
    Bluetooth_joinThreadPool();
}

void bluetooth_ipc_join_service_loop(void)
{
    add_init_process(add_ipc_poll_setup);
}

#else

static int add_ipc_server_setup(void* data)
{
    int ret = bt_socket_server_init("bluetooth", CONFIG_BLUETOOTH_SOCKET_PORT);
    if (ret < 0) {
        BT_LOGE("%s error: %d", __func__, ret);
        return ret;
    }

    return 0;
}

bt_status_t bluetooth_ipc_add_services(void)
{
    add_init_process(add_ipc_server_setup);
    return BT_STATUS_SUCCESS;
}

void bluetooth_ipc_join_thread_pool(void)
{
}

void bluetooth_ipc_join_service_loop(void)
{
}
#endif
