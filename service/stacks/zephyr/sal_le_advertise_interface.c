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
#define LOG_TAG "adver"

#include "include/sal_le_advertise_interface.h"

#include "advertising.h"
#include "sal_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#ifndef CONFIG_BT_EXT_ADV_MAX_ADV_SET
#define CONFIG_BT_EXT_ADV_MAX_ADV_SET 3
#endif

#ifndef CONFIG_BT_EXT_ADV_MAX_ADV_SEGMENT
#define CONFIG_BT_EXT_ADV_MAX_ADV_SEGMENT 5
#endif

#ifdef CONFIG_BLUETOOTH_BLE_ADV
#define STACK_CALL(func) zblue_##func

typedef void (*sal_func_t)(void* args);

typedef union {
    struct {
        struct bt_le_adv_param param;
        uint8_t* adv_data;
        uint16_t adv_len;
        uint8_t* scan_rsp_data;
        uint16_t scan_rsp_len;
    } start_adv;
} sal_adapter_args_t;

typedef struct {
    bt_controller_id_t id;
    uint8_t adv_id;
    sal_func_t func;
    sal_adapter_args_t adpt;
} sal_adapter_req_t;

struct bt_le_adv_set;

static void ext_adv_sent(struct bt_le_ext_adv* adv, struct bt_le_ext_adv_sent_info* info);
static void ext_adv_connected(struct bt_le_ext_adv* adv, struct bt_le_ext_adv_connected_info* info);
static bt_status_t zblue_le_ext_delete(struct bt_le_adv_set* adv);

struct bt_le_adv_set {
    struct bt_le_ext_adv* adv;
    uint8_t adv_id;
};

static struct bt_le_adv_set* g_adv_sets[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
static struct bt_le_ext_adv_cb g_adv_cb = {
    .sent = ext_adv_sent,
    .connected = ext_adv_connected,
};

static void ext_adv_sent(struct bt_le_ext_adv* adv, struct bt_le_ext_adv_sent_info* info)
{
    BT_LOGD("%s ", __func__);
}

static void ext_adv_connected(struct bt_le_ext_adv* adv, struct bt_le_ext_adv_connected_info* info)
{
    int index;

    BT_LOGD("%s ", __func__);

    index = bt_le_ext_adv_get_index(adv);
    if (!g_adv_sets[index]) {
        BT_LOGE("%s, adv set index:%d null", __func__, index);
        return;
    }

    advertising_on_state_changed(g_adv_sets[index]->adv_id, LE_ADVERTISING_STOPPED);
    zblue_le_ext_delete(g_adv_sets[index]);
}

static bt_status_t zblue_le_ext_convert_param(ble_adv_params_t* params, struct bt_le_adv_param* param)
{
    static bt_addr_le_t addr;

    switch (params->adv_type) {
    case BT_LE_ADV_IND:
    case BT_LE_ADV_DIRECT_IND:
    case BT_LE_ADV_SCAN_IND:
        param->options |= BT_LE_ADV_OPT_SCANNABLE;
        param->options |= BT_LE_ADV_OPT_CONN;
        break;
    case BT_LE_ADV_NONCONN_IND:
        param->options |= BT_LE_ADV_OPT_EXT_ADV;
        break;
    case BT_LE_SCAN_RSP:
        param->options |= BT_LE_ADV_OPT_CONN;
        param->options |= BT_LE_ADV_OPT_SCANNABLE;
        param->options |= BT_LE_ADV_OPT_EXT_ADV;
        break;
    case BT_LE_LEGACY_ADV_IND:
    case BT_LE_LEGACY_ADV_DIRECT_IND:
    case BT_LE_LEGACY_ADV_SCAN_IND:
        param->options |= BT_LE_ADV_OPT_CONN;
        break;
    case BT_LE_LEGACY_ADV_NONCONN_IND:
        break;
    case BT_LE_LEGACY_SCAN_RSP:
        param->options |= BT_LE_ADV_OPT_SCANNABLE;
        break;
    default:
        BT_LOGE("%s, le ext adv convert fail, invalid adv_type:%d", __func__, params->adv_type);
        return BT_STATUS_PARM_INVALID;
    }

    param->interval_min = params->interval;
    param->interval_max = params->interval;

    if (params->adv_type == BT_LE_ADV_DIRECT_IND) {
        addr.type = params->peer_addr_type;
        memcpy(&addr.a, &params->peer_addr, sizeof(bt_address_t));
        param->peer = &addr;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t zblue_le_ext_create(struct bt_le_adv_param* param, struct bt_le_ext_adv** adv, uint8_t adv_id)
{
    int ret;
    uint8_t index;
    struct bt_le_adv_set* adv_set;

    ret = bt_le_ext_adv_create(param, &g_adv_cb, adv);
    if (ret) {
        BT_LOGE("%s, le ext adv create fail, err:%d", __func__, ret);
        return BT_STATUS_FAIL;
    }

    index = bt_le_ext_adv_get_index(*adv);
    adv_set = malloc(sizeof(*adv_set));
    if (!adv_set) {
        BT_LOGE("%s, malloc fail", __func__);
        bt_le_ext_adv_delete(*adv);
        return BT_STATUS_NOMEM;
    }

    adv_set->adv = *adv;
    adv_set->adv_id = adv_id;
    g_adv_sets[index] = adv_set;

    return BT_STATUS_SUCCESS;
}

static bt_status_t zblue_le_ext_delete(struct bt_le_adv_set* adv_set)
{
    int ret;
    uint8_t index;

    if (!adv_set) {
        BT_LOGE("%s, adv set null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    ret = bt_le_ext_adv_delete(adv_set->adv);
    if (ret) {
        BT_LOGE("%s, le ext adv delet fail, err:%d", __func__, ret);
        return BT_STATUS_FAIL;
    }

    index = bt_le_ext_adv_get_index(adv_set->adv);
    free(adv_set);
    g_adv_sets[index] = NULL;

    return BT_STATUS_SUCCESS;
}

static struct bt_le_adv_set* zblue_le_ext_find_adv(uint8_t adv_id)
{
    size_t index;

    for (index = 0; index < ARRAY_SIZE(g_adv_sets); index++) {
        if (!g_adv_sets[index]) {
            continue;
        }

        if (g_adv_sets[index]->adv_id == adv_id) {
            return g_adv_sets[index];
        }
    }

    return NULL;
}

static bt_status_t zblue_le_ext_adv_set_data(struct bt_le_ext_adv* adv, uint8_t* adv_data, uint16_t adv_len, uint8_t* scan_rsp_data, uint16_t scan_rsp_len)
{
    size_t index;
    struct bt_data ad[CONFIG_BT_EXT_ADV_MAX_ADV_SEGMENT] = { 0 };
    struct bt_data sd[CONFIG_BT_EXT_ADV_MAX_ADV_SEGMENT] = { 0 };
    size_t ad_size = 0;
    size_t sd_size = 0;
    int ret;

    for (index = 0; index < adv_len;) {
        ad[ad_size].data_len = adv_data[index] - 1;
        ad[ad_size].type = adv_data[index + 1];
        ad[ad_size].data = &adv_data[index + 2];
        index += ad[ad_size].data_len + 2;
        ad_size++;
    }

    for (index = 0; index < scan_rsp_len;) {
        sd[sd_size].data_len = scan_rsp_data[index] - 1;
        sd[sd_size].type = scan_rsp_data[index + 1];
        sd[sd_size].data = &scan_rsp_data[index + 2];
        index += sd[sd_size].data_len + 2;
        sd_size++;
    }

    ret = bt_le_ext_adv_set_data(adv, ad_size > 0 ? ad : NULL, ad_size,
        sd_size > 0 ? sd : NULL, sd_size);
    if (ret) {
        BT_LOGE("%s, le ext adv set data fail, err:%d", __func__, ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static sal_adapter_req_t* sal_adapter_req(bt_controller_id_t id, uint8_t adv_id, sal_func_t func)
{
    sal_adapter_req_t* req = calloc(sizeof(sal_adapter_req_t), 1);

    if (req) {
        req->id = id;
        req->adv_id = adv_id;
        req->func = func;
    }

    return req;
}

static void sal_invoke_async(service_work_t* work, void* userdata)
{
    sal_adapter_req_t* req = userdata;

    SAL_ASSERT(req);
    req->func(req);
    free(userdata);
}

static bt_status_t sal_send_req(sal_adapter_req_t* req)
{
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(start_adv)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_le_ext_adv* adv;
    int ret;

    ret = zblue_le_ext_create(&req->adpt.start_adv.param, &adv, req->adv_id);
    if (ret) {
        BT_LOGE("%s, zblue le ext adv create fail, err:%d", __func__, ret);
        ret = BT_STATUS_FAIL;
        goto done;
    }

    ret = zblue_le_ext_adv_set_data(adv, req->adpt.start_adv.adv_data, req->adpt.start_adv.adv_len,
        req->adpt.start_adv.scan_rsp_data, req->adpt.start_adv.scan_rsp_len);
    if (ret) {
        BT_LOGE("%s, le ext adv set fail, err:%d", __func__, ret);
        ret = BT_STATUS_FAIL;
        goto done;
    }

    ret = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (ret) {
        BT_LOGE("%s, le ext adv start fail, err:%d", __func__, ret);
        ret = BT_STATUS_FAIL;
        goto done;
    }

    advertising_on_state_changed(req->adv_id, LE_ADVERTISING_STARTED);
    ret = BT_STATUS_SUCCESS;

done:
    free(req->adpt.start_adv.adv_data);
    free(req->adpt.start_adv.scan_rsp_data);
}

bt_status_t bt_sal_le_start_adv(bt_controller_id_t id, uint8_t adv_id, ble_adv_params_t* params, uint8_t* adv_data, uint16_t adv_len, uint8_t* scan_rsp_data, uint16_t scan_rsp_len)
{
    sal_adapter_req_t* req;
    int ret;

    req = sal_adapter_req(id, adv_id, STACK_CALL(start_adv));
    if (!req) {
        BT_LOGE("%s, req null", __func__)
        return BT_STATUS_NOMEM;
    }

    ret = zblue_le_ext_convert_param(params, &req->adpt.start_adv.param);
    if (ret) {
        BT_LOGE("%s, le ext adv convert fail, err:%d", __func__, ret);
        ret = BT_STATUS_PARM_INVALID;
        goto error;
    }

    req->adpt.start_adv.adv_data = malloc(adv_len);
    if (!req->adpt.start_adv.adv_data) {
        BT_LOGE("%s, malloc fail", __func__);
        ret = BT_STATUS_NOMEM;
        goto error;
    }

    memcpy(req->adpt.start_adv.adv_data, adv_data, adv_len);
    req->adpt.start_adv.adv_len = adv_len;

    req->adpt.start_adv.scan_rsp_data = malloc(scan_rsp_len);
    if (!req->adpt.start_adv.scan_rsp_data) {
        BT_LOGE("%s, malloc fail", __func__);
        ret = BT_STATUS_NOMEM;
        goto error;
    }

    memcpy(req->adpt.start_adv.scan_rsp_data, scan_rsp_data, scan_rsp_len);
    req->adpt.start_adv.scan_rsp_len = scan_rsp_len;

    return sal_send_req(req);

error:
    free(req);
    return ret;
};

static void STACK_CALL(stop_adv)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_le_adv_set* adv_set;
    int ret;

    adv_set = zblue_le_ext_find_adv(req->adv_id);
    if (!adv_set) {
        BT_LOGE("%s, le ext adv_set find fail", __func__);
        return;
    }

    ret = bt_le_ext_adv_stop(adv_set->adv);
    if (ret) {
        BT_LOGE("%s, le ext adv stop fail", __func__);
        return;
    }

    ret = zblue_le_ext_delete(adv_set);
    if (ret) {
        BT_LOGE("%s, le ext adv stop fail", __func__);
        return;
    }

    advertising_on_state_changed(req->adv_id, LE_ADVERTISING_STOPPED);
}

bt_status_t bt_sal_le_stop_adv(bt_controller_id_t id, uint8_t adv_id)
{
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, adv_id, STACK_CALL(stop_adv));
    if (!req) {
        BT_LOGE("%s, req null", __func__)
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}
#endif /*CONFIG_BLUETOOTH_BLE_ADV*/