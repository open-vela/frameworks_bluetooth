/***********************************************************************
 *
 * Copyright 2024 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#include <nuttx/list.h>
#include <pthread.h>
#include <semaphore.h>
#include <uv.h>

#include "vela_mible_manage.h"
#include "vela_mible_port.h"
#include "bt_adapter.h"
#include "utils/log.h"

typedef struct {
    bt_instance_t* vela_le_svc_ins;
    void* adpt_ck;
    uv_async_t vela_le_async;
    bool vela_async_hdl_cls;
    void* adapter_cookie;
    int32_t (*vela_le_disable_callback)(bool status);
} vela_mible_manager_t;

static vela_mible_manager_t vela_mible_manager = { 0 };

static void adapter_state_changed_callback(void* cookie, bt_adapter_state_t state)
{
    BT_LOGI("Context:%p, Adapter state changed: %d", cookie, state);
    if (state == BT_ADAPTER_STATE_ON) {
        // This function is called only when adapter first time enable or re-start
        vela_mible_gap_init();
    } else if (state == BT_ADAPTER_STATE_OFF) {
        vela_mible_gap_deinit();
    }
}

const static adapter_callbacks_t adapter_cbs = {
    .on_adapter_state_changed = adapter_state_changed_callback,
};

// Async close callback
static void async_close_callback(uv_handle_t* handle)
{
    vela_mible_manager.vela_async_hdl_cls = false;
    // Release band shouldn't do it
    uv_stop(uv_default_loop());
}

static void vela_mible_async_callback(uv_async_t* handle)
{
    if (!bt_adapter_unregister_callback(vela_mible_manager.vela_le_svc_ins, vela_mible_manager.adpt_ck)) {
        BT_LOGE("bt adapter unregister callback failed.");
    }
    vela_mible_gap_deinit();
    bluetooth_delete_instance(vela_mible_manager.vela_le_svc_ins);
    vela_mible_manager.vela_le_svc_ins = NULL;
    vela_mible_manager.adpt_ck = NULL;
    vela_mible_manager.vela_async_hdl_cls = true;
    uv_close((uv_handle_t*)handle, async_close_callback);
}

int vela_mible_manager_init(void)
{
    if (vela_mible_manager.vela_le_svc_ins || vela_mible_manager.vela_async_hdl_cls) {
        BT_LOGE("vela_le_svc_ins %p or vela_async_hdl_cls %d is not reset",
            vela_mible_manager.vela_le_svc_ins, vela_mible_manager.vela_async_hdl_cls);
        goto fail;
    }

    vela_mible_manager.vela_le_svc_ins = bluetooth_create_instance();
    if (!vela_mible_manager.vela_le_svc_ins) {
        BT_LOGE("create bt instance failed");
        goto fail;
    }

    uv_async_init(uv_default_loop(), &vela_mible_manager.vela_le_async, vela_mible_async_callback);
    bluetooth_set_external_uv(vela_mible_manager.vela_le_svc_ins, uv_default_loop());

    vela_mible_manager.adpt_ck = bt_adapter_register_callback(vela_mible_manager.vela_le_svc_ins,
        &adapter_cbs);
    if (!vela_mible_manager.adpt_ck) {
        BT_LOGE("bt adapter register callback failed");
        bluetooth_delete_instance(vela_mible_manager.vela_le_svc_ins);
        vela_mible_manager.vela_le_svc_ins = NULL;
        goto fail;
    }

    bt_adapter_state_t state = bt_adapter_get_state(vela_mible_manager.vela_le_svc_ins);
    if (state == BT_ADAPTER_STATE_ON) {
        BT_LOGI("BT adapter state is %d, already enabled, registering GAP callback function.",
            state);
        vela_mible_gap_init();
    } else {
        bt_adapter_enable(vela_mible_manager.vela_le_svc_ins);
        BT_LOGW("BT adapter state is %d, enable adapter by iccoa", state);
    }

    return 0;

fail:
    return -1;
}

int vela_mible_manager_deinit(void)
{
    if (!vela_mible_manager.vela_le_svc_ins) {
        BT_LOGE("bt instance NULL");
        return -1;
    }

    vela_mible_manager.vela_le_svc_ins->external_loop = NULL;

    if (!bt_adapter_unregister_callback(vela_mible_manager.vela_le_svc_ins, vela_mible_manager.adapter_cookie)) {
        BT_LOGE("bt adapter unregister callback failed");
    }

    free(vela_mible_manager.vela_le_svc_ins->external_async);

    bluetooth_delete_instance(vela_mible_manager.vela_le_svc_ins);
    vela_mible_manager.vela_le_svc_ins = NULL;
    vela_mible_manager.adapter_cookie = NULL;

    return 0;
}

void* vela_mible_svc_ins_get(void)
{
    if (!vela_mible_manager.vela_le_svc_ins) {
        BT_LOGE("bt instance NULL");
        return NULL;
    }
    return vela_mible_manager.vela_le_svc_ins;
}

void vela_mible_register_disable_callback(int32_t (*callback)(bool status))
{
    vela_mible_manager.vela_le_disable_callback = callback;
}

void vela_mible_unregister_disable_callback(void)
{
    vela_mible_manager.vela_le_disable_callback = NULL;
}