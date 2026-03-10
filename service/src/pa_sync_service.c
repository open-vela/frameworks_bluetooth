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
#define LOG_TAG "pa_sync"

#include "pa_sync_service.h"

#include "bluetooth.h"
#include "bt_list.h"
#include "pa_sync_event.h"
#include "sal_pa_sync_interface.h"
#include "service_loop.h"

#include "utils/log.h"

typedef struct pa_sync_device {
    bt_le_address_t addr;
    uint8_t sid;
    const bt_pa_sync_callbacks_t* cbs;
    const void* context;
} pa_sync_device_t;

typedef struct pa_sync_info {
    bt_list_t* sync_list;
} pa_sync_info_t;

static pa_sync_info_t* g_pa_sync_info = NULL;

static void sync_terminated_callback(const bt_pa_sync_callbacks_t* cbs, const bt_le_address_t* addr,
    uint8_t sid, const void* context)
{
    BT_LOGD("%s", __func__);

    if (!cbs || !cbs->on_sync_terminated)
        return;

    cbs->on_sync_terminated(addr, sid, (void*)context);
}

static void create_sync(void* data)
{
    pa_sync_event_create_sync_t* msg = (pa_sync_event_create_sync_t*)data;
    bt_sal_pa_sync_param_t params = { 0 };
    pa_sync_device_t* device;

    BT_LOGD("%s", __func__);

    device = zalloc(sizeof(pa_sync_device_t));
    if (device == NULL)
        goto error;

    memcpy(&params.addr, &msg->addr, sizeof(bt_le_address_t));
    params.sid = msg->sid;
    params.skip = msg->params.skip;
    params.timeout = msg->params.timeout;
    params.options &= ~BT_SAL_PA_SYNC_OPTION_USE_LIST; /**< periodic advertiser list unsupported */
    params.options |= msg->params.no_report ? BT_SAL_PA_SYNC_OPTION_REPORTING_DISABLED : 0;
    params.options |= msg->params.duplicate_filter ? BT_SAL_PA_SYNC_OPTION_FILTER_ENABLED : 0;
    params.cte = 0; /**< nothing specified */
    if (bt_sal_pa_create_sync(PRIMARY_ADAPTER, &params) != BT_STATUS_SUCCESS)
        goto error;

    /** sync created, add this device into list */
    memcpy(&device->addr, &msg->addr, sizeof(bt_le_address_t));
    device->sid = msg->sid;
    device->cbs = msg->cbs;
    device->context = msg->context;
    bt_list_add_tail(g_pa_sync_info->sync_list, device);

    free(msg);
    return;

error:
    sync_terminated_callback(msg->cbs, &msg->addr, msg->sid, msg->context);
    free(msg);
}

static void sync_removed(void* data)
{
    pa_sync_device_t* device = (pa_sync_device_t*)data;

    BT_LOGD("%s", __func__);

    sync_terminated_callback(device->cbs, &device->addr, device->sid, device->context);

    free(device);
}

bt_status_t pa_sync_init(void)
{
    BT_LOGD("%s", __func__);

    if (g_pa_sync_info)
        return BT_STATUS_BUSY;

    g_pa_sync_info = zalloc(sizeof(pa_sync_info_t));
    if (!g_pa_sync_info)
        return BT_STATUS_NOMEM;

    g_pa_sync_info->sync_list = bt_list_new(sync_removed);
    if (g_pa_sync_info->sync_list == NULL)
        goto error;

    return BT_STATUS_SUCCESS;

error:
    bt_list_free(g_pa_sync_info->sync_list);
    free(g_pa_sync_info);
    return BT_STATUS_FAIL;
}

bt_status_t pa_sync_cleanup(void)
{
    BT_LOGD("%s", __func__);

    if (!g_pa_sync_info)
        return BT_STATUS_DONE;

    bt_list_free(g_pa_sync_info->sync_list);
    free(g_pa_sync_info);
    g_pa_sync_info = NULL;

    return BT_STATUS_SUCCESS;
}

bt_status_t pa_sync_create(const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_create_param_t* params, const bt_pa_sync_callbacks_t* cbs,
    const void* context)
{
    pa_sync_event_create_sync_t msg = {
        .params = BT_PA_SYNC_DEFAULT_DEFAULT_PARAM,
    };

    BT_LOGD("%s", __func__);

    if (!cbs || sid > BLE_SCAN_SID_MAX)
        return BT_STATUS_PARM_INVALID;

    memcpy(&msg.addr, addr, sizeof(bt_le_address_t));
    msg.sid = sid;
    msg.cbs = cbs;
    msg.context = context;
    if (params) {
        if (params->skip > BT_PA_SYNC_SKIP_MAX)
            return BT_STATUS_PARM_INVALID;
        if (params->timeout > BT_PA_SYNC_TIMEOUT_MAX)
            return BT_STATUS_PARM_INVALID;

        memcpy(&msg.params, params, sizeof(bt_pa_sync_create_param_t));
    }

    do_in_service_loop(create_sync, &msg);

    return BT_STATUS_SUCCESS;
}

bt_status_t pa_sync_terminate(void)
{
    BT_LOGD("%s", __func__);

    return BT_STATUS_UNSUPPORTED;
}
