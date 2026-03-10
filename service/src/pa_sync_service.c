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
#include "bt_utils.h"
#include "pa_sync_event.h"
#include "sal_pa_sync_interface.h"
#include "service_loop.h"

#include "utils/log.h"

#define BT_PA_SYNC_DEFAULT_SKIP (1)
#define BT_PA_SYNC_DEFAULT_TIMEOUT_MS (5000)
#define BT_PA_SYNC_DEFAULT_DEFAULT_PARAM {           \
    .skip = (BT_PA_SYNC_DEFAULT_SKIP),               \
    .timeout = (BT_PA_SYNC_DEFAULT_TIMEOUT_MS) / 10, \
    .filter = false,                                 \
    .no_report = false,                              \
}

typedef struct pa_sync_device {
    bt_le_address_t addr;
    uint8_t sid;
    const bt_pa_sync_callbacks_t* cbs;
    const void* context;
} pa_sync_device_t;

typedef void (*func_for_each_t)(const pa_sync_device_t* device, const void* data);

typedef struct pa_sync_for_each {
    func_for_each_t func;
    const bt_le_address_t* addr;
    uint8_t sid;
    const void* data;
} pa_sync_for_each_t;

typedef struct pa_sync_info {
    bt_list_t* sync_list;
} pa_sync_info_t;

static pa_sync_info_t* g_pa_sync_info = NULL;
static void pa_sync_process_message(void* data);

static void func_for_device(void* data, void* context)
{
    const pa_sync_device_t* device = (const pa_sync_device_t*)data;
    const pa_sync_for_each_t* iter = (const pa_sync_for_each_t*)context;

    if (memcmp(device->addr.addr, iter->addr->addr, BT_ADDR_LENGTH))
        return;

    if (device->addr.addr_type != iter->addr->addr_type)
        return;

    if (device->sid != iter->sid)
        return;

    iter->func(device, iter->data);
}

/** Handles the scenario where multiple applications watch the same device */
static void callback_for_each_device(pa_sync_for_each_t* iter)
{
    if (!g_pa_sync_info || !g_pa_sync_info->sync_list || !iter->func || !iter->addr)
        return;

    bt_list_foreach(g_pa_sync_info->sync_list, func_for_device, iter);
}

static bt_status_t pa_sync_send_message(pa_sync_event_t* msg)
{
    assert(msg);

    do_in_service_loop(pa_sync_process_message, msg);

    return BT_STATUS_SUCCESS;
}

static void sync_established_callback(const bt_pa_sync_callbacks_t* cbs,
    const bt_le_address_t* addr, uint8_t sid, const void* context)
{
    BT_LOGD("%s", __func__);

    if (!cbs || !cbs->on_sync_established)
        return;

    cbs->on_sync_established(addr, sid, (void*)context);
}

static void sync_terminated_callback(const bt_pa_sync_callbacks_t* cbs, const bt_le_address_t* addr,
    uint8_t sid, const void* context)
{
    BT_LOGD("%s", __func__);

    if (!cbs || !cbs->on_sync_terminated)
        return;

    cbs->on_sync_terminated(addr, sid, (void*)context);
}

static void sync_report_callback(const bt_pa_sync_callbacks_t* cbs, const bt_le_address_t* addr,
    uint8_t sid, const bt_pa_sync_report_t* report, const void* context)
{
    if (!cbs || !cbs->on_sync_report)
        return;

    cbs->on_sync_report(addr, sid, report, (void*)context);
}

static void create_sync(const pa_sync_event_t* msg)
{
    pa_sync_event_create_sync_t* params = (pa_sync_event_create_sync_t*)msg->data;
    pa_sync_device_t* device = NULL;
    bt_sal_pa_sync_param_t sal_params = { 0 };
    bt_status_t status;

    BT_ADDR_LOG("create sync to [%s], addr_type:%d, sid:%d, skip:%d, timeout:%dms, filter:%c, "
                "report:%c",
        (bt_address_t*)msg->addr.addr, msg->addr.addr_type, msg->sid, params->params.skip,
        params->params.timeout * 10, params->params.filter ? 'y' : 'n',
        params->params.no_report ? 'n' : 'y');

    device = zalloc(sizeof(pa_sync_device_t));
    if (device == NULL) {
        BT_LOGE("%s, malloc failed", __func__);
        goto error;
    }

    memcpy(&sal_params.addr, &msg->addr, sizeof(bt_le_address_t));
    sal_params.sid = msg->sid;
    sal_params.skip = params->params.skip;
    sal_params.timeout = params->params.timeout;
    sal_params.options &= ~BT_SAL_PA_SYNC_OPTION_USE_LIST; /**< not supported */
    sal_params.options |= params->params.no_report ? BT_SAL_PA_SYNC_OPTION_REPORTING_DISABLED : 0;
    sal_params.options |= params->params.filter ? BT_SAL_PA_SYNC_OPTION_FILTER_ENABLED : 0;
    sal_params.cte = 0; /**< nothing specified */
    status = bt_sal_pa_create_sync(PRIMARY_ADAPTER, &sal_params);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, failed to create sync, status = %d", __func__, status);
        goto error;
    }

    /** sync created, add this device into list */
    memcpy(&device->addr, &msg->addr, sizeof(bt_le_address_t));
    device->sid = msg->sid;
    device->cbs = params->cbs;
    device->context = params->context;
    bt_list_add_tail(g_pa_sync_info->sync_list, device);
    return;

error:
    sync_terminated_callback(params->cbs, &msg->addr, msg->sid, params->context);
    free(device);
}

static void terminate_sync(const pa_sync_event_t* msg)
{
    /** Do something */
}

static void process_sync_established(const pa_sync_device_t* device, const void* data)
{
    UNUSED(data);

    sync_established_callback(device->cbs, &device->addr, device->sid, device->context);
}

static void sync_established(const pa_sync_event_t* msg)
{
    pa_sync_for_each_t iter = { 0 };

    iter.func = process_sync_established;
    iter.addr = &msg->addr;
    iter.sid = msg->sid;

    callback_for_each_device(&iter);
}

static void sync_removed(void* data)
{
    pa_sync_device_t* device = (pa_sync_device_t*)data;

    BT_LOGD("%s", __func__);

    sync_terminated_callback(device->cbs, &device->addr, device->sid, device->context);

    free(device);
}

static void process_sync_terminated(const pa_sync_device_t* device, const void* data)
{
    UNUSED(data);

    bt_list_remove(g_pa_sync_info->sync_list, (void*)device);
}

static void sync_terminated(const pa_sync_event_t* msg)
{
    pa_sync_for_each_t iter = { 0 };

    iter.func = process_sync_terminated;
    iter.addr = &msg->addr;
    iter.sid = msg->sid;

    callback_for_each_device(&iter);
}

static void report_service_to_app(bt_pa_sync_report_t* out, const pa_sync_event_report_data_t* in)
{
    out->tx_power = in->tx_power;
    out->rssi = in->rssi;
    out->cnt = in->cnt;
    out->subevent = in->subevent;
    out->adv_data_len = in->adv_data_len;
    if (out->adv_data_len)
        out->data = &in->adv_data[0];
    else
        out->data = NULL;
}

static void process_sync_report(const pa_sync_device_t* device, const void* data)
{
    const pa_sync_event_report_data_t* report_in = (const pa_sync_event_report_data_t*)data;
    bt_pa_sync_report_t report_out = { 0 };

    report_service_to_app(&report_out, report_in);

    sync_report_callback(device->cbs, &device->addr, device->sid, &report_out, device->context);
}

static void sync_report(const pa_sync_event_t* msg)
{
    const pa_sync_event_report_data_t* report = (const pa_sync_event_report_data_t*)msg->data;
    pa_sync_for_each_t iter = { 0 };

    if (report->tx_power > +20 && report->tx_power != BT_POWER_UNAVAILABLE) {
        BT_LOGE("%s, invalid tx_power(%d)", __func__, report->tx_power);
        return;
    }

    if (report->rssi > +20 && report->rssi != BT_POWER_UNAVAILABLE) {
        BT_LOGE("%s, invalid rssi(%d)", __func__, report->rssi);
        return;
    }

    if (report->adv_data_len > BT_PA_SYNC_DATA_LEN_MAX) {
        BT_LOGE("%s, length(%d) exceeds limit(%d)", __func__, report->adv_data_len,
            BT_PA_SYNC_DATA_LEN_MAX);
        return;
    }

    if (report->status != BT_LE_PA_SYNC_EVENT_DATA_COMPLETE) {
        /** TODO: reassemble segmented data */
        BT_LOGW("%s, segmented data not supported", __func__);
        return;
    }

    iter.func = process_sync_report;
    iter.addr = &msg->addr;
    iter.sid = msg->sid;
    iter.data = report;

    callback_for_each_device(&iter);
}

static const char* pa_sync_event_to_string(pa_sync_event_type_t event)
{
    switch (event) {
        CASE_RETURN_STR(CREATE_SYNC)
        CASE_RETURN_STR(TERMINATE_SYNC)
        CASE_RETURN_STR(SYNC_ESTABLISHED)
        CASE_RETURN_STR(SYNC_TERMINATED)
        CASE_RETURN_STR(SYNC_REPORT)
        DEFAULT_BREAK();
    }

    return "Unknown";
}

static void pa_sync_process_message(void* data)
{
    pa_sync_event_t* msg = (pa_sync_event_t*)data;

    if (msg->event != SYNC_REPORT) { /**< avoid spam logs */
        BT_LOGD("%s, event = %s(%d)", __func__, pa_sync_event_to_string(msg->event), msg->event);
    }

    switch (msg->event) {
    case CREATE_SYNC:
        create_sync(msg);
        break;
    case TERMINATE_SYNC:
        terminate_sync(msg);
        break;
    case SYNC_ESTABLISHED:
        sync_established(msg);
        break;
    case SYNC_TERMINATED:
        sync_terminated(msg);
        break;
    case SYNC_REPORT:
        sync_report(msg);
        break;
    default:
        break;
    }

    free(msg);
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

    if (bt_sal_pa_sync_init() != BT_STATUS_SUCCESS)
        goto error;

    return BT_STATUS_SUCCESS;

error:
    bt_sal_pa_sync_cleanup();
    bt_list_free(g_pa_sync_info->sync_list);
    free(g_pa_sync_info);
    return BT_STATUS_FAIL;
}

bt_status_t pa_sync_cleanup(void)
{
    BT_LOGD("%s", __func__);

    if (!g_pa_sync_info)
        return BT_STATUS_DONE;

    bt_sal_pa_sync_cleanup();
    bt_list_free(g_pa_sync_info->sync_list);
    free(g_pa_sync_info);
    g_pa_sync_info = NULL;

    return BT_STATUS_SUCCESS;
}

bt_status_t pa_sync_create(const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_create_param_t* params, const bt_pa_sync_callbacks_t* cbs,
    const void* context)
{
    pa_sync_event_t* msg;
    pa_sync_event_create_sync_t* param;
    bt_pa_sync_create_param_t default_params = BT_PA_SYNC_DEFAULT_DEFAULT_PARAM;

    BT_LOGD("%s", __func__);

    if (!cbs) {
        BT_LOGE("cbs is null");
        return BT_STATUS_PARM_INVALID;
    }

    if (sid > BLE_SCAN_SID_MAX) {
        BT_LOGE("invalid sid 0x%x", sid);
        return BT_STATUS_PARM_INVALID;
    }

    if (!params)
        params = &default_params;

    if (params->skip > BT_PA_SYNC_SKIP_MAX) {
        BT_LOGE("invalid skip 0x%04x", params->skip);
        return BT_STATUS_PARM_INVALID;
    }

    if (params->timeout < BT_PA_SYNC_TIMEOUT_MIN || params->timeout > BT_PA_SYNC_TIMEOUT_MAX) {
        BT_LOGE("invalid timeout 0x%04x", params->timeout);
        return BT_STATUS_PARM_INVALID;
    }

    msg = zalloc(sizeof(pa_sync_event_t) + sizeof(pa_sync_event_create_sync_t));
    if (!msg) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    param = (pa_sync_event_create_sync_t*)msg->data;
    memcpy(&msg->addr, addr, sizeof(bt_le_address_t));
    msg->event = CREATE_SYNC;
    msg->id = PRIMARY_ADAPTER;
    msg->sid = sid;
    param->cbs = cbs;
    param->context = context;
    memcpy(&param->params, params, sizeof(bt_pa_sync_create_param_t));

    if (pa_sync_send_message(msg) != BT_STATUS_SUCCESS) {
        BT_LOGE("message send failed");
        free(msg);
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t pa_sync_terminate(const bt_le_address_t* addr, uint8_t sid)
{
    pa_sync_event_t* msg;

    BT_LOGD("%s", __func__);

    msg = zalloc(sizeof(pa_sync_event_t));
    if (!msg) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    memcpy(&msg->addr, addr, sizeof(bt_le_address_t));
    msg->event = TERMINATE_SYNC;
    msg->id = PRIMARY_ADAPTER;
    msg->sid = sid;

    if (pa_sync_send_message(msg) != BT_STATUS_SUCCESS) {
        BT_LOGE("message send failed");
        free(msg);
    }

    return BT_STATUS_SUCCESS;
}

void pa_sync_on_established(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid)
{
    pa_sync_event_t* msg = zalloc(sizeof(pa_sync_event_t));
    if (!msg) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    msg->event = SYNC_ESTABLISHED;
    msg->id = id;
    msg->sid = sid;
    memcpy(&msg->addr, addr, sizeof(bt_le_address_t));

    if (pa_sync_send_message(msg) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, message send failed", __func__);
        free(msg);
    }
}

void pa_sync_on_terminated(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid)
{
    pa_sync_event_t* msg = zalloc(sizeof(pa_sync_event_t));
    if (!msg) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    msg->event = SYNC_TERMINATED;
    msg->id = id;
    msg->sid = sid;
    memcpy(&msg->addr, addr, sizeof(bt_le_address_t));

    if (pa_sync_send_message(msg) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, message send failed", __func__);
        free(msg);
    }
}

void pa_sync_on_received(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid,
    int tx_power, int rssi, uint8_t cte, uint16_t cnt, uint8_t subevent, uint8_t status,
    uint8_t adv_data_len, const uint8_t* adv_data)
{
    pa_sync_event_report_data_t* report;
    pa_sync_event_t* msg;
    uint8_t* data;

    msg = zalloc(sizeof(pa_sync_event_t) + sizeof(pa_sync_event_report_data_t) + adv_data_len);
    if (!msg) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    report = (pa_sync_event_report_data_t*)msg->data;
    data = report->adv_data;

    /** Part 1, general message data */
    msg->event = SYNC_REPORT;
    msg->id = id;
    msg->sid = sid;
    memcpy(&msg->addr, addr, sizeof(bt_le_address_t));

    /** Part 2, periodic advertising report info */
    report->tx_power = tx_power;
    report->rssi = rssi;
    report->cte = cte;
    report->cnt = cnt;
    report->subevent = subevent;
    report->status = status;
    report->adv_data_len = adv_data_len;

    /** Part 3, adv data */
    memcpy(data, adv_data, adv_data_len);

    if (pa_sync_send_message(msg) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, message send failed", __func__);
        free(msg);
    }
}
