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
#define LOG_TAG "pan_api"

#include <stdint.h>

#include "bluetooth.h"
#include "bt_pan.h"
#include "bt_profile.h"

#include "pan_callbacks_stub.h"
#include "pan_proxy.h"
#include "pan_stub.h"

#include "utils/log.h"

void* bt_pan_register_callbacks(bt_instance_t* ins, const pan_callbacks_t* callbacks)
{
    BpBtPan* pan = (BpBtPan*)bluetooth_get_proxy(ins, PROFILE_PANU);

    IBtPanCallbacks* cbks = BtPanCallbacks_new(callbacks);
    AIBinder* binder = BtPanCallbacks_getBinder(cbks);
    if (!binder) {
        BtPanCallbacks_delete(cbks);
        return NULL;
    }

    void* remote_cbks = BpBtPan_registerCallback(pan, binder);
    if (!remote_cbks) {
        BtPanCallbacks_delete(cbks);
        return NULL;
    }
    cbks->cookie = remote_cbks;

    return cbks;
}

bool bt_pan_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    IBtPanCallbacks* cbks = cookie;
    BpBtPan* pan = (BpBtPan*)bluetooth_get_proxy(ins, PROFILE_PANU);

    bool ret = BpBtPan_unRegisterCallback(pan, cbks->cookie);
    if (ret)
        BtPanCallbacks_delete(cbks);

    return ret;
}

bt_status_t bt_pan_connect(bt_instance_t* ins, bt_address_t* addr, uint8_t dst_role, uint8_t src_role)
{
    BpBtPan* pan = (BpBtPan*)bluetooth_get_proxy(ins, PROFILE_PANU);

    return BpBtPan_connect(pan, addr, dst_role, src_role);
}

bt_status_t bt_pan_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtPan* pan = (BpBtPan*)bluetooth_get_proxy(ins, PROFILE_PANU);

    return BpBtPan_disconnect(pan, addr);
}