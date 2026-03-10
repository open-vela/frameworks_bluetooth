/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#include <string.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>

#ifdef CONFIG_BLUETOOTH_BLE_SCAN
#include "sal_interface.h"
#include "sal_le_scan_interface.h"
#include "service_loop.h"

#include "utils/log.h"

#define STACK_CALL(func) zblue_##func

typedef void (*sal_func_t)(void* args);

typedef struct {
    bt_controller_id_t id;
    sal_func_t func;
} sal_scan_req_t;

static struct bt_le_scan_param scan_param;

static sal_scan_req_t* sal_scan_req(bt_controller_id_t id, sal_func_t func)
{
    sal_scan_req_t* req = calloc(1, sizeof(sal_scan_req_t));

    if (!req) {
        BT_LOGE("%s, req malloc fail", __func__);
        return NULL;
    }

    req->id = id;
    req->func = func;

    return req;
}

static void sal_invoke_async(service_work_t* work, void* userdata)
{
    sal_scan_req_t* req = userdata;

    SAL_ASSERT(req);
    req->func(req);
    free(userdata);
}

static bt_status_t sal_send_req(sal_scan_req_t* req)
{
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work fail", __func__);
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static ble_adv_type_t parse_adv_type(const struct bt_le_scan_recv_info* info)
{
    switch (info->adv_type) {
    case BT_GAP_ADV_TYPE_ADV_IND:
        return BT_LE_ADV_IND;
    case BT_GAP_ADV_TYPE_ADV_DIRECT_IND:
        return BT_LE_ADV_DIRECT_IND;
    case BT_GAP_ADV_TYPE_ADV_SCAN_IND:
        return BT_LE_ADV_SCAN_IND;
    case BT_GAP_ADV_TYPE_ADV_NONCONN_IND:
        return BT_LE_ADV_NONCONN_IND;
    case BT_GAP_ADV_TYPE_SCAN_RSP:
        return BT_LE_SCAN_RSP;
    case BT_GAP_ADV_TYPE_EXT_ADV:
        break; /**< Determined via `info->adv_props` */
    default:
        break; /**< Unrecognized */
    }

    if (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE)
        return BT_LE_EXT_SCAN_RSP;
    if (info->adv_props & BT_GAP_ADV_PROP_DIRECTED)
        return BT_LE_EXT_ADV_DIRECT_IND;
    if (!(info->adv_props & (BT_GAP_ADV_PROP_CONNECTABLE | BT_GAP_ADV_PROP_SCANNABLE)))
        return BT_LE_EXT_ADV_NONCONN_IND;
    if (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE)
        return BT_LE_EXT_ADV_SCAN_IND;
    if (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE)
        return BT_LE_EXT_ADV_IND;

    return BT_LE_EXT_ADV_IND; /**< Unknown */
}

static void scan_recv_cb(const struct bt_le_scan_recv_info* info, struct net_buf_simple* ad)
{
    ble_scan_result_t result_info = { 0 };

    result_info.dev_type = BT_DEVICE_DEVTYPE_BLE;
    result_info.adv_type = parse_adv_type(info);
    result_info.length = ad->len;
    result_info.rssi = info->rssi;
    result_info.tx_power = info->tx_power;
    result_info.sid = info->sid;
    result_info.interval = info->interval;
    result_info.addr_type = info->addr->type;
    memcpy(&result_info.addr, &info->addr->a, sizeof(result_info.addr));

    scan_on_result_data_update(&result_info, ad->data);
}

static struct bt_le_scan_cb scan_cbs = {
    .recv = scan_recv_cb,
    .timeout = NULL,
};

static void STACK_CALL(start_scan)(void* args)
{
    int err;

    err = bt_le_scan_cb_register(&scan_cbs);
    if (err != 0 && err != -EEXIST) {
        BT_LOGE("%s, register failed, ret = %d", __func__, err);
        return;
    }

    err = bt_le_scan_start(&scan_param, NULL);
    if (err) {
        BT_LOGE("%s, failed, ret = %d", __func__, err);
        bt_le_scan_cb_unregister(&scan_cbs);
    }
}

static void STACK_CALL(stop_scan)(void* args)
{
    bt_le_scan_cb_unregister(&scan_cbs);
    SAL_CHECK(bt_le_scan_stop(), 0);
}

bt_status_t bt_sal_le_set_scan_parameters(bt_controller_id_t id, ble_scan_params_t* params)
{
    memset(&scan_param, 0, sizeof(scan_param));
    scan_param.type = (uint8_t)params->scan_type;
    scan_param.interval = params->scan_interval;
    scan_param.window = params->scan_window;

    if (params->filter_type == BT_LE_SCAN_POLICY_ONLY_WHITE_LIST ||
        params->filter_type == BT_LE_SCAN_POLICY_ONLY_WHITE_LIST_AND_RPA) {
        scan_param.options |= BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_le_start_scan(bt_controller_id_t id)
{
    sal_scan_req_t* req;

    req = sal_scan_req(id, STACK_CALL(start_scan));
    if (!req) {
        BT_LOGE("%s, sal req fail", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

bt_status_t bt_sal_le_stop_scan(bt_controller_id_t id)
{
    sal_scan_req_t* req;

    req = sal_scan_req(id, STACK_CALL(stop_scan));
    if (!req) {
        BT_LOGE("%s, sal req fail", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}
#endif /* CONFIG_BLUETOOTH_BLE_SCAN */
