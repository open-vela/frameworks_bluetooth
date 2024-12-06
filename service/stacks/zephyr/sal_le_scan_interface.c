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

#include <bluetooth/addr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/uuid.h>
#include <string.h>

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

typedef struct {
    char value[256];
    uint8_t length;
} le_eir_data_t;

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
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bool zblue_on_eir_found(struct bt_data* data, void* user_data)
{
    le_eir_data_t* eir = user_data;

    eir->value[eir->length++] = data->data_len + 1;
    eir->value[eir->length++] = data->type;
    memcpy(&eir->value[eir->length], data->data, data->data_len);
    eir->length += data->data_len;
    return true;
}

static void zblue_on_device_found(const bt_addr_le_t* addr, int8_t rssi, uint8_t type, struct net_buf_simple* ad)
{
    ble_scan_result_t result_info = { 0 };
    le_eir_data_t eir = { 0 };

    bt_data_parse(ad, zblue_on_eir_found, &eir);

    result_info.length = eir.length;
    result_info.adv_type = type;
    result_info.rssi = rssi;
    result_info.dev_type = BT_DEVICE_DEVTYPE_BLE;
    result_info.addr_type = addr->type;
    memcpy(&result_info.addr, &addr->a, sizeof(result_info.addr));

    scan_on_result_data_update(&result_info, eir.value);
}

static void STACK_CALL(start_scan)(void* args)
{
    SAL_CHECK(bt_le_scan_start(&scan_param, zblue_on_device_found), 0);
}

static void STACK_CALL(stop_scan)(void* args)
{
    SAL_CHECK(bt_le_scan_stop(), 0);
}

bt_status_t bt_sal_le_set_scan_parameters(bt_controller_id_t id, ble_scan_params_t* params)
{
    memset(&scan_param, 0, sizeof(scan_param));
    scan_param.type = params->scan_type;
    scan_param.interval = params->scan_interval;
    scan_param.window = params->scan_window;

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
