/***********************************************************************
 *
 * Copyright 2026 XiaoMi All Rights Reserved.
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

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <uv.h>

#include "z_api.h"
#include "z_api_manager.h"
#include "bt_adapter.h"
#include "utils/log.h"

#include <debug.h>

/* z_api_manager: BT instance lifecycle management */

typedef struct {
    bt_instance_t* svc_ins;
    void* adpt_ck;
    uv_async_t async;
    bool async_hdl_cls;
    void* adapter_cookie;
    int32_t (*disable_callback)(bool status);
} z_api_manager_t;

static z_api_manager_t g_manager = { 0 };

static void adapter_state_changed_callback(void* cookie, bt_adapter_state_t state)
{
    _info("[z_api] adapter_state_changed: state=%d\n", state);
    /* Note: GAP init/deinit is now handled via dispatch from bt_enable/bt_disable,
     * not from the adapter callback, to avoid threading issues. */
}

const static adapter_callbacks_t adapter_cbs = {
    .on_adapter_state_changed = adapter_state_changed_callback,
};

static void async_close_callback(uv_handle_t* handle)
{
    g_manager.async_hdl_cls = false;
    uv_stop(uv_default_loop());
}

static void z_api_async_callback(uv_async_t* handle)
{
    if (!bt_adapter_unregister_callback(g_manager.svc_ins, g_manager.adpt_ck)) {
        _info("[z_api] async_cb: unregister callback failed\n");
    }
    /* Note: bt_gap_deinit is handled separately */
    bluetooth_delete_instance(g_manager.svc_ins);
    g_manager.svc_ins = NULL;
    g_manager.adpt_ck = NULL;
    g_manager.async_hdl_cls = true;
    uv_close((uv_handle_t*)handle, async_close_callback);
}

void* z_api(bt_svc_ins_get)(void)
{
    if (!g_manager.svc_ins) {
        _info("[z_api] z_bt_svc_ins_get: NULL\n");
        return NULL;
    }
    return g_manager.svc_ins;
}

int z_api(bt_manager_init)(void)
{
    _info("[z_api] >>> z_bt_manager_init: ENTRY (tid=%p)\n", (void*)pthread_self());

    if (g_manager.svc_ins || g_manager.async_hdl_cls) {
        _info("[z_api] >>> z_bt_manager_init: already init svc_ins=%p\n", g_manager.svc_ins);
        return -1;
    }

    g_manager.svc_ins = bluetooth_create_instance();
    if (!g_manager.svc_ins) {
        _info("[z_api] >>> z_bt_manager_init: create instance FAILED\n");
        return -1;
    }
    _info("[z_api] >>> z_bt_manager_init: instance=%p\n", g_manager.svc_ins);

    /* Use the bt_ipc_thread's uv_loop (set by main.c) */
    extern uv_loop_t *g_bt_loop;
    uv_loop_t *loop = g_bt_loop ? g_bt_loop : uv_default_loop();

    uv_async_init(loop, &g_manager.async, z_api_async_callback);
    bluetooth_set_external_uv(g_manager.svc_ins, loop);

    g_manager.adpt_ck = bt_adapter_register_callback(g_manager.svc_ins, &adapter_cbs);
    if (!g_manager.adpt_ck) {
        _info("[z_api] >>> z_bt_manager_init: register callback FAILED\n");
        bluetooth_delete_instance(g_manager.svc_ins);
        g_manager.svc_ins = NULL;
        return -1;
    }
    _info("[z_api] >>> z_bt_manager_init: callback registered\n");

    bt_adapter_state_t state = bt_adapter_get_state(g_manager.svc_ins);
    _info("[z_api] >>> z_bt_manager_init: adapter state=%d\n", state);

    if (state == BT_ADAPTER_STATE_ON) {
        _info("[z_api] >>> z_bt_manager_init: adapter already ON\n");
    } else {
        _info("[z_api] >>> z_bt_manager_init: calling bt_adapter_enable\n");
        bt_adapter_enable(g_manager.svc_ins);
        _info("[z_api] >>> z_bt_manager_init: bt_adapter_enable returned\n");
    }

    _info("[z_api] >>> z_bt_manager_init: done\n");
    return 0;
}

int z_api(bt_manager_deinit)(void)
{
    if (!g_manager.svc_ins) {
        BT_LOGE("bt instance NULL");
        return -1;
    }

    g_manager.svc_ins->external_loop = NULL;

    if (!bt_adapter_unregister_callback(g_manager.svc_ins, g_manager.adapter_cookie)) {
        BT_LOGE("bt adapter unregister callback failed");
    }

    free(g_manager.svc_ins->external_async);
    bluetooth_delete_instance(g_manager.svc_ins);
    g_manager.svc_ins = NULL;
    g_manager.adapter_cookie = NULL;

    return 0;
}

void z_api(bt_register_disable_callback)(int32_t (*callback)(bool status))
{
    g_manager.disable_callback = callback;
}

void z_api(bt_unregister_disable_callback)(void)
{
    g_manager.disable_callback = NULL;
}
