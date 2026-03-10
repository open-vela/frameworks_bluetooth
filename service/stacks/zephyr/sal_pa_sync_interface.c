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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/direction.h>

#include "sal_pa_sync_interface.h"

#include "bt_list.h"
#include "pa_sync_event.h"
#include "pa_sync_service.h"
#include "sal_interface.h"
#include "service_loop.h"

#include "utils/log.h"

typedef void (*sal_func_t)(const void* args);

typedef struct {
    bt_le_address_t addr;
    bt_controller_id_t id;
    uint8_t sid;
    sal_func_t func;
    void* context;
} sal_pa_sync_req_t;

typedef struct {
    bt_le_address_t addr;
    bt_controller_id_t id;
    uint8_t sid;
    struct bt_le_per_adv_sync* sync; /**< zephyr sync object */
    struct bt_le_per_adv_sync_synced_info* info; /**< zephyr sync info */
} sal_pa_sync_device_t;

typedef struct {
    bt_list_t* sync_list;
} sal_pa_sync_info_t;

static sal_pa_sync_info_t* g_sal_pa_sync_info = NULL;

static sal_pa_sync_device_t* device_new(void)
{
    sal_pa_sync_device_t* device = zalloc(sizeof(sal_pa_sync_device_t));

    if (!device) {
        BT_LOGE("%s, malloc failed", __func__);
        return NULL;
    }

    return device;
}

static void device_delete(sal_pa_sync_device_t* device)
{
    BT_LOGD("%s", __func__);

    if (!device)
        return;

    pa_sync_on_terminated(device->id, &device->addr, device->sid);

    free(device->info);
    free(device);
}

static bool sync_cmp(void* data, void* context)
{
    sal_pa_sync_device_t* device = (sal_pa_sync_device_t*)data;
    struct bt_le_per_adv_sync* sync = (struct bt_le_per_adv_sync*)context;

    if (!device)
        return false;

    return device->sync == sync;
}

static sal_pa_sync_device_t* find_device_by_sync(struct bt_le_per_adv_sync* sync)
{
    if (!g_sal_pa_sync_info || !g_sal_pa_sync_info->sync_list || !sync)
        return NULL;

    return (sal_pa_sync_device_t*)bt_list_find(g_sal_pa_sync_info->sync_list, sync_cmp, sync);
}

static bool req_cmp(void* data, void* context)
{
    const sal_pa_sync_device_t* device = (const sal_pa_sync_device_t*)data;
    const sal_pa_sync_req_t* req = (const sal_pa_sync_req_t*)context;

    if (!device || !req)
        return false;

    if (memcmp(&device->addr, &req->addr, sizeof(bt_le_address_t)))
        return false;

    if (device->id != req->id || device->sid != req->sid)
        return false;

    return true;
}

static sal_pa_sync_device_t* find_device_by_req(const sal_pa_sync_req_t* req)
{
    if (!g_sal_pa_sync_info || !g_sal_pa_sync_info->sync_list || !req)
        return NULL;

    return (sal_pa_sync_device_t*)bt_list_find(g_sal_pa_sync_info->sync_list, req_cmp, (void*)req);
}

static void sync_removed(void* data)
{
    sal_pa_sync_device_t* device = (sal_pa_sync_device_t*)data;

    BT_LOGD("%s", __func__);

    device_delete(device);
}

static sal_pa_sync_req_t* sal_pa_sync_req(bt_controller_id_t id, const bt_le_address_t* addr,
    uint8_t sid, sal_func_t func, void* context)
{
    sal_pa_sync_req_t* req = zalloc(sizeof(sal_pa_sync_req_t));

    if (!req) {
        BT_LOGE("%s, req malloc fail", __func__);
        return NULL;
    }

    memcpy(&req->addr, addr, sizeof(bt_le_address_t));
    req->id = id;
    req->sid = sid;
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

static void on_synced(struct bt_le_per_adv_sync* sync, struct bt_le_per_adv_sync_synced_info* info)
{
    sal_pa_sync_device_t* device;

    BT_LOGD("%s", __func__);

    device = find_device_by_sync(sync);
    if (!device) {
        BT_LOGE("%s, device not found", __func__);
        return; /**< FIXME: consider periodic advertising via PAST */
    }

    if (!device->info)
        device->info = zalloc(sizeof(struct bt_le_per_adv_sync_synced_info));

    if (!device->info) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    memcpy(device->info, info, sizeof(struct bt_le_per_adv_sync_synced_info));

    pa_sync_on_established(device->id, &device->addr, device->sid);
}

static void on_term(struct bt_le_per_adv_sync* sync,
    const struct bt_le_per_adv_sync_term_info* info)
{
    sal_pa_sync_device_t* device;

    BT_LOGD("%s", __func__);

    device = find_device_by_sync(sync);
    if (!device) {
        BT_LOGE("%s, device not found", __func__);
        return;
    }

    bt_list_remove(g_sal_pa_sync_info->sync_list, device);
}

static uint8_t sal_cte_type_zephyr_to_sal(uint8_t z_cte)
{
    switch (z_cte) {
    case BT_DF_CTE_TYPE_NONE:
        return BT_LE_PA_SYNC_EVENT_CTE_TYPE_NONE;
    case BT_DF_CTE_TYPE_AOA:
        return BT_LE_PA_SYNC_EVENT_CTE_TYPE_AOA;
    case BT_DF_CTE_TYPE_AOD_1US:
        return BT_LE_PA_SYNC_EVENT_CTE_TYPE_AOD_1US;
    case BT_DF_CTE_TYPE_AOD_2US:
        return BT_LE_PA_SYNC_EVENT_CTE_TYPE_AOD_2US;
    default:
        break;
    }

    return BT_LE_PA_SYNC_EVENT_CTE_TYPE_NONE; /**< unrecognized */
}

static void on_recv(struct bt_le_per_adv_sync* sync,
    const struct bt_le_per_adv_sync_recv_info* info, struct net_buf_simple* buf)
{
    sal_pa_sync_device_t* device;
    uint16_t periodic_event_counter = 0;
    uint8_t subevent = BT_PA_SYNC_SUBEVENT_NONE;

    device = find_device_by_sync(sync);
    if (!device || !device->info) {
        BT_LOGE("%s, device not found or info not allocated", __func__);
        return;
    }

    /** Update info of periodic advertising */
    memcpy(device->info, info, sizeof(struct bt_le_per_adv_sync_synced_info));

#ifdef CONFIG_BT_PER_ADV_SYNC_RSP
    periodic_event_counter = info->periodic_event_counter;
    subevent = info->subevent;
#endif
    pa_sync_on_received(device->id, &device->addr, device->sid, info->tx_power, info->rssi,
        sal_cte_type_zephyr_to_sal(info->cte_type), periodic_event_counter, subevent,
        BT_LE_PA_SYNC_EVENT_DATA_COMPLETE, buf->len, buf->data);
}

static void sal_create_sync_param_sal_to_zephyr(struct bt_le_per_adv_sync_param* z_param,
    const bt_sal_pa_sync_param_t* params)
{
    memcpy(z_param->addr.a.val, params->addr.addr, BT_ADDR_SIZE);
    z_param->addr.type = params->addr.addr_type;
    z_param->sid = params->sid;
    z_param->skip = params->skip;
    z_param->timeout = params->timeout;
    if (params->options & BT_SAL_PA_SYNC_OPTION_USE_LIST)
        z_param->options |= BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST;
    if (params->options & BT_SAL_PA_SYNC_OPTION_REPORTING_DISABLED)
        z_param->options |= BT_LE_PER_ADV_SYNC_OPT_REPORTING_INITIALLY_DISABLED;
    if (params->options & BT_SAL_PA_SYNC_OPTION_FILTER_ENABLED)
        z_param->options |= BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
    if (params->cte & BT_SAL_PA_SYNC_CTE_TYPE_NO_AOA)
        z_param->options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOA;
    if (params->cte & BT_SAL_PA_SYNC_CTE_TYPE_NO_AOD_1US)
        z_param->options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_1US;
    if (params->cte & BT_SAL_PA_SYNC_CTE_TYPE_NO_AOD_2US)
        z_param->options |= BT_LE_PER_ADV_SYNC_OPT_DONT_SYNC_AOD_2US;
    if (params->cte & BT_SAL_PA_SYNC_CTE_TYPE_CTE_ONLY)
        z_param->options |= BT_LE_PER_ADV_SYNC_OPT_SYNC_ONLY_CONST_TONE_EXT;
}

/** TODO: Add const */
static struct bt_le_per_adv_sync_cb sal_pa_sync_cbs = {
    .synced = on_synced,
    .term = on_term,
    .recv = on_recv,
    .state_changed = NULL,
    .biginfo = NULL,
    .cte_report_cb = NULL,
};

static void create_sync(const void* data)
{
    const sal_pa_sync_req_t* req = (const sal_pa_sync_req_t*)data;
    struct bt_le_per_adv_sync_param* z_param = (struct bt_le_per_adv_sync_param*)req->context;
    sal_pa_sync_device_t* device = NULL;
    int err;

    BT_LOGD("%s", __func__);

    device = device_new();
    if (!device) {
        pa_sync_on_terminated(req->id, &req->addr, req->sid);
        free(z_param);
        return;
    }

    memcpy(&device->addr, &req->addr, sizeof(bt_le_address_t));
    device->id = req->id;
    device->sid = req->sid;

    err = bt_le_per_adv_sync_cb_register(&sal_pa_sync_cbs);
    if (err != 0 && err != -EEXIST)
        goto error;

    err = bt_le_per_adv_sync_create(z_param, &device->sync);
    if (err) {
        BT_LOGE("failed to create sync, err = %d", err);
        goto error;
    }

    if (!device->sync) {
        BT_LOGE("sync not generated");
        goto error;
    }

    bt_list_add_tail(g_sal_pa_sync_info->sync_list, device);

    free(z_param);
    return;

error:
    device_delete(device);
    free(z_param);
    return;
}

static void terminate_sync(const void* data)
{
    const sal_pa_sync_req_t* req = (const sal_pa_sync_req_t*)data;
    sal_pa_sync_device_t* device;
    int err;

    device = find_device_by_req(req);
    if (!device) {
        pa_sync_on_terminated(req->id, &req->addr, req->sid);
        return;
    }

    err = bt_le_per_adv_sync_delete(device->sync);
    if (err) {
        BT_LOGE("failed to terminate sync, err = %d", err);
        device_delete(device);
        return;
    }
}

bt_status_t bt_sal_pa_sync_init(void)
{
    g_sal_pa_sync_info = zalloc(sizeof(sal_pa_sync_info_t));
    if (!g_sal_pa_sync_info)
        return BT_STATUS_NOMEM;

    g_sal_pa_sync_info->sync_list = bt_list_new(sync_removed);
    if (g_sal_pa_sync_info->sync_list == NULL)
        goto error;

    return BT_STATUS_SUCCESS;

error:
    bt_list_free(g_sal_pa_sync_info->sync_list);
    free(g_sal_pa_sync_info);
    return BT_STATUS_FAIL;
}

bt_status_t bt_sal_pa_sync_cleanup(void)
{
    if (!g_sal_pa_sync_info)
        return BT_STATUS_DONE;

    /* TODO: add unregisteration for stack callbacks */
    bt_list_free(g_sal_pa_sync_info->sync_list);
    free(g_sal_pa_sync_info);
    g_sal_pa_sync_info = NULL;

    return BT_STATUS_SUCCESS;
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
    req = sal_pa_sync_req(id, &params->addr, params->sid, create_sync, z_param);
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

bt_status_t bt_sal_pa_terminate_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr)
{
    bt_status_t status;
    sal_pa_sync_req_t* req;

    BT_LOGD("%s", __func__);

    req = sal_pa_sync_req(id, addr, sid, terminate_sync, NULL);
    if (!req)
        return BT_STATUS_NOMEM;

    status = sal_send_req(req);
    if (status != BT_STATUS_SUCCESS)
        free(req);

    return status;
}
