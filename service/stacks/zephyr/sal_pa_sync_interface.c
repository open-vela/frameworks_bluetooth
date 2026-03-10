/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#define LOG_TAG "sal_pa_sync"

#include "sal_interface.h"
#include "sal_pa_sync_interface.h"
#include "service_loop.h"

#include "utils/log.h"

typedef void (*sal_func_t)(const void* args);

typedef struct {
    bt_le_address_t addr;
    bt_controller_id_t id;
    sal_func_t func;
    void* context;
} sal_pa_sync_req_t;

static sal_pa_sync_req_t* sal_pa_sync_req(bt_controller_id_t id, const bt_le_address_t* addr,
    sal_func_t func, void* context)
{
    sal_pa_sync_req_t* req = zalloc(sizeof(sal_pa_sync_req_t));

    if (!req) {
        BT_LOGE("%s, req malloc fail", __func__);
        return NULL;
    }

    memcpy(&req->addr, addr, sizeof(bt_le_address_t));
    req->id = id;
    req->func = func;
    req->context = context;

    return req;
}

static void sal_invoke_async(service_work_t* work, void* data)
{
    sal_pa_sync_req_t* req = data;

    SAL_ASSERT(req);
    req->func(req);

    free(data);
}

static bt_status_t sal_send_req(sal_pa_sync_req_t* req)
{
    if (!req)
        return BT_STATUS_PARM_INVALID;

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void sal_create_sync_param_sal_to_zephyr(struct bt_le_per_adv_sync_param* z_param,
    const bt_sal_pa_sync_param_t* params)
{
}

static void create_sync(const void* data)
{
    const sal_pa_sync_req_t* req = (const sal_pa_sync_req_t*)data;
    struct bt_le_per_adv_sync_param* z_param = (struct bt_le_per_adv_sync_param*)req->context;

    free(z_param);
}

bt_status_t bt_sal_pa_create_sync(bt_controller_id_t id, const bt_sal_pa_sync_param_t* params)
{
    sal_pa_sync_req_t* req = NULL;
    struct bt_le_per_adv_sync_param* z_param = NULL;

    BT_LOGD("%s", __func__);

    z_param = zalloc(sizeof(struct bt_le_per_adv_sync_param));
    if (!z_param)
        return BT_STATUS_NOMEM;

    sal_create_sync_param_sal_to_zephyr(z_param, params);
    req = sal_pa_sync_req(id, &params->addr, create_sync, z_param);
    if (!req)
        goto error;

    if (sal_send_req(req) != BT_STATUS_SUCCESS)
        goto error;

    return BT_STATUS_SUCCESS;

error:
    free(z_param);
    free(req);

    return BT_STATUS_FAIL;
}
