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
#define LOG_TAG "spp_api"

#include <stdint.h>

#include "bluetooth.h"
#include "bt_profile.h"
#include "bt_spp.h"

#include "spp_callbacks_stub.h"
#include "spp_proxy.h"
#include "spp_stub.h"

#include "utils/log.h"

void* bt_spp_register_app(bt_instance_t* ins, const spp_callbacks_t* callbacks)
{
    BpBtSpp* spp = (BpBtSpp*)bluetooth_get_proxy(ins, PROFILE_SPP);

    IBtSppCallbacks* cbks = BtSppCallbacks_new(callbacks);
    AIBinder* binder = BtSppCallbacks_getBinder(cbks);
    if (!binder) {
        BtSppCallbacks_delete(cbks);
        return NULL;
    }

    void* handle = BpBtSpp_registerApp(spp, binder);
    if (!handle) {
        BtSppCallbacks_delete(cbks);
        return NULL;
    }
    cbks->cookie = handle;

    return cbks;
}

bt_status_t bt_spp_unregister_app(bt_instance_t* ins, void* handle)
{
    IBtSppCallbacks* cbks = handle;
    BpBtSpp* spp = (BpBtSpp*)bluetooth_get_proxy(ins, PROFILE_SPP);

    bt_status_t status = BpBtSpp_unRegisterApp(spp, cbks->cookie);
    if (status == BT_STATUS_SUCCESS)
        BtSppCallbacks_delete(cbks);

    return status;
}

bt_status_t bt_spp_server_start(bt_instance_t* ins, void* handle, uint16_t scn, bt_uuid_t* uuid, uint8_t max_connection)
{
    IBtSppCallbacks* cbks = handle;
    BpBtSpp* spp = (BpBtSpp*)bluetooth_get_proxy(ins, PROFILE_SPP);

    return BpBtSpp_serverStart(spp, cbks->cookie, scn, uuid, max_connection);
}

bt_status_t bt_spp_server_stop(bt_instance_t* ins, void* handle, uint16_t scn)
{
    IBtSppCallbacks* cbks = handle;
    BpBtSpp* spp = (BpBtSpp*)bluetooth_get_proxy(ins, PROFILE_SPP);

    return BpBtSpp_serverStop(spp, cbks->cookie, scn);
}

bt_status_t bt_spp_connect(bt_instance_t* ins, void* handle, bt_address_t* addr, int16_t scn, bt_uuid_t* uuid, uint16_t* port)
{
    IBtSppCallbacks* cbks = handle;
    BpBtSpp* spp = (BpBtSpp*)bluetooth_get_proxy(ins, PROFILE_SPP);

    return BpBtSpp_connect(spp, cbks->cookie, addr, scn, uuid, port);
}

bt_status_t bt_spp_disconnect(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t port)
{
    IBtSppCallbacks* cbks = handle;
    BpBtSpp* spp = (BpBtSpp*)bluetooth_get_proxy(ins, PROFILE_SPP);

    return BpBtSpp_disconnect(spp, cbks->cookie, addr, port);
}