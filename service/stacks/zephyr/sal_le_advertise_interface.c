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
        struct bt_le_ext_adv_start_param ext_param;
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

static bt_status_t parse_bt_adv_data(uint8_t* raw, uint16_t raw_len,
    struct bt_data* out, size_t out_max, size_t* out_size)
{
    size_t index;
    uint8_t item_len;

    if (!out || !out_size || out_max == 0) {
        return BT_STATUS_PARM_INVALID;
    }

    *out_size = 0;

    if (!raw || raw_len == 0) {
        return BT_STATUS_SUCCESS;
    }

    for (index = 0; index < raw_len;) {
        item_len = raw[index];
        if (item_len == 0) {
            break;
        }

        if (item_len < 2) {
            BT_LOGE("%s, invalid adv data item len:%d", __func__, item_len);
            return BT_STATUS_PARM_INVALID;
        }

        if (index + 1 + item_len > raw_len) {
            BT_LOGE("%s, adv data overflow: raw_len=%d idx=%d item_len=%d", __func__,
                raw_len, index, item_len);
            return BT_STATUS_PARM_INVALID;
        }

        if (*out_size >= out_max) {
            BT_LOGE("too many segments: out_max=%d idx=%d", out_max, index);
            return BT_STATUS_PARM_INVALID;
        }

        out[*out_size].data_len = item_len - 1;
        out[*out_size].type = raw[index + 1];
        out[*out_size].data = &raw[index + 2];
        index += out[*out_size].data_len + 2;
        (*out_size)++;
    }

    return BT_STATUS_SUCCESS;
}

static void ext_adv_terminated_cb(struct bt_le_ext_adv* adv)
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

static void ext_adv_sent(struct bt_le_ext_adv* adv, struct bt_le_ext_adv_sent_info* info)
{
    BT_LOGD("%s ", __func__);

    ext_adv_terminated_cb(adv);
}

static void ext_adv_connected(struct bt_le_ext_adv* adv, struct bt_le_ext_adv_connected_info* info)
{
    BT_LOGD("%s ", __func__);

    ext_adv_terminated_cb(adv);
}

static bt_status_t zblue_le_ext_convert_param(ble_adv_params_t* params, struct bt_le_adv_param* param)
{
    static bt_addr_le_t addr;

    switch (params->adv_type) {
    case BT_LE_ADV_IND:
    case BT_LE_EXT_ADV_IND:
        param->options |= BT_LE_ADV_OPT_CONN;
        param->options |= BT_LE_ADV_OPT_EXT_ADV;
        param->options |= BT_LE_ADV_OPT_NO_2M;
        break;
    case BT_LE_ADV_SCAN_IND:
    case BT_LE_EXT_ADV_SCAN_IND:
        param->options |= BT_LE_ADV_OPT_SCANNABLE;
        param->options |= BT_LE_ADV_OPT_EXT_ADV;
        param->options |= BT_LE_ADV_OPT_NO_2M;
        break;
    case BT_LE_ADV_DIRECT_IND:
    case BT_LE_EXT_ADV_DIRECT_IND:
        param->options |= BT_LE_ADV_OPT_CONN;
        param->options |= BT_LE_ADV_OPT_EXT_ADV;
        param->options |= BT_LE_ADV_OPT_NO_2M;
        param->options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
        break;
    case BT_LE_SCAN_RSP:
    case BT_LE_EXT_SCAN_RSP:
    case BT_LE_ADV_NONCONN_IND:
    case BT_LE_EXT_ADV_NONCONN_IND:
        param->options |= BT_LE_ADV_OPT_EXT_ADV;
        param->options |= BT_LE_ADV_OPT_NO_2M;
        break;
    case BT_LE_LEGACY_ADV_IND:
        param->options |= BT_LE_ADV_OPT_CONN;
        param->options |= BT_LE_ADV_OPT_SCANNABLE;
        break;
    case BT_LE_LEGACY_ADV_DIRECT_IND:
        param->options |= BT_LE_ADV_OPT_CONN;
        break;
    case BT_LE_LEGACY_ADV_SCAN_IND:
        param->options |= BT_LE_ADV_OPT_SCANNABLE;
        break;
    case BT_LE_LEGACY_ADV_NONCONN_IND:
    case BT_LE_LEGACY_SCAN_RSP:
        break;
    default:
        BT_LOGE("%s, le ext adv convert fail, invalid adv_type:%d", __func__, params->adv_type);
        return BT_STATUS_PARM_INVALID;
    }

    switch (params->own_addr_type) {
    case BT_LE_ADDR_TYPE_PUBLIC:
        param->options |= BT_LE_ADV_OPT_USE_IDENTITY;
        break;
    }

    switch (params->channel_map) {
    case BT_LE_ADV_CHANNEL_37_ONLY:
        param->options |= BT_LE_ADV_OPT_DISABLE_CHAN_38 | BT_LE_ADV_OPT_DISABLE_CHAN_39;
        break;
    case BT_LE_ADV_CHANNEL_38_ONLY:
        param->options |= BT_LE_ADV_OPT_DISABLE_CHAN_37 | BT_LE_ADV_OPT_DISABLE_CHAN_39;
        break;
    case BT_LE_ADV_CHANNEL_39_ONLY:
        param->options |= BT_LE_ADV_OPT_DISABLE_CHAN_37 | BT_LE_ADV_OPT_DISABLE_CHAN_38;
        break;
    case BT_LE_ADV_CHANNEL_DEFAULT:
        break;
    default:
        BT_LOGE("%s, le ext adv convert fail, invalid channel_map:%d", __func__, params->channel_map);
        return BT_STATUS_PARM_INVALID;
    }

    switch (params->filter_policy) {
    case BT_LE_ADV_FILTER_WHITE_LIST_FOR_SCAN:
        param->options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
        break;
    case BT_LE_ADV_FILTER_WHITE_LIST_FOR_CONNECTION:
        param->options |= BT_LE_ADV_OPT_FILTER_CONN;
        break;
    case BT_LE_ADV_FILTER_WHITE_LIST_FOR_ALL:
        param->options |= BT_LE_ADV_OPT_FILTER_SCAN_REQ;
        param->options |= BT_LE_ADV_OPT_FILTER_CONN;
        break;
    case BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE:
    default:
        param->options |= BT_LE_ADV_OPT_NONE;
    }

    param->interval_min = params->interval;
    param->interval_max = params->interval;

    if (params->adv_type == BT_LE_ADV_DIRECT_IND
        || params->adv_type == BT_LE_EXT_ADV_DIRECT_IND
        || params->adv_type == BT_LE_LEGACY_ADV_DIRECT_IND) {
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
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(start_adv)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_le_ext_adv* adv;
    int ret;
    struct bt_data ad[CONFIG_BT_EXT_ADV_MAX_ADV_SEGMENT] = { 0 };
    struct bt_data sd[CONFIG_BT_EXT_ADV_MAX_ADV_SEGMENT] = { 0 };
    size_t ad_size = 0;
    size_t sd_size = 0;
    bool ext_supported = bt_le_ext_adv_is_supported();

    ret = parse_bt_adv_data(req->adpt.start_adv.adv_data, req->adpt.start_adv.adv_len,
        ad, ARRAY_SIZE(ad), &ad_size);
    if (ret) {
        BT_LOGE("%s, parse adv_data fail, err:%d", __func__, ret);
        goto done;
    }

    ret = parse_bt_adv_data(req->adpt.start_adv.scan_rsp_data, req->adpt.start_adv.scan_rsp_len,
        sd, ARRAY_SIZE(sd), &sd_size);
    if (ret) {
        BT_LOGE("%s, parse scan_rsp_data fail, ret:%d", __func__, ret);
        goto done;
    }

    if (ext_supported) {
        ret = zblue_le_ext_create(&req->adpt.start_adv.param, &adv, req->adv_id);
        if (ret) {
            BT_LOGE("%s, zblue le ext adv create fail, err:%d", __func__, ret);
            ret = BT_STATUS_FAIL;
            goto done;
        }

        ret = bt_le_ext_adv_set_data(adv, ad_size > 0 ? ad : NULL, ad_size,
            sd_size > 0 ? sd : NULL, sd_size);
        if (ret) {
            BT_LOGE("%s, le ext adv set fail, err:%d", __func__, ret);
            ret = BT_STATUS_FAIL;
            goto done;
        }

        ret = bt_le_ext_adv_start(adv, &req->adpt.start_adv.ext_param);
        if (ret) {
            BT_LOGE("%s, le ext adv start fail, err:%d", __func__, ret);
            ret = BT_STATUS_FAIL;
            goto done;
        }
    } else {
        ret = bt_le_adv_start(&req->adpt.start_adv.param, ad_size > 0 ? ad : NULL, ad_size,
            sd_size > 0 ? sd : NULL, sd_size);
        if (ret) {
            BT_LOGE("%s, legacy adv start fail, err:%d", __func__, ret);
            ret = BT_STATUS_FAIL;
            goto done;
        }
    }

    advertising_on_state_changed(req->adv_id, LE_ADVERTISING_STARTED);
    ret = BT_STATUS_SUCCESS;

done:
    if (req->adpt.start_adv.adv_data)
        free(req->adpt.start_adv.adv_data);
    if (req->adpt.start_adv.scan_rsp_data)
        free(req->adpt.start_adv.scan_rsp_data);
}

bt_status_t bt_sal_le_start_adv(bt_controller_id_t id, uint8_t adv_id, ble_adv_params_t* params, uint8_t* adv_data, uint16_t adv_len, uint8_t* scan_rsp_data, uint16_t scan_rsp_len)
{
    sal_adapter_req_t* req;
    int ret;
    bool ext_adv;
    bool ext_supported;

    req = sal_adapter_req(id, adv_id, STACK_CALL(start_adv));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    ret = zblue_le_ext_convert_param(params, &req->adpt.start_adv.param);
    if (ret) {
        BT_LOGE("%s, le ext adv convert fail, err:%d", __func__, ret);
        ret = BT_STATUS_PARM_INVALID;
        goto error;
    }

    ext_supported = bt_le_ext_adv_is_supported();
    ext_adv = (req->adpt.start_adv.param.options & BT_LE_ADV_OPT_EXT_ADV) ? true : false;

    if (ext_adv && !ext_supported) {
        BT_LOGE("%s, controller not support ext adv", __func__);
        ret = BT_STATUS_UNSUPPORTED;
        goto error;
    }

    if (((!(req->adpt.start_adv.param.options & BT_LE_ADV_OPT_SCANNABLE) && ext_adv)
            || !ext_adv)
        && adv_data && adv_len > 0) {
        req->adpt.start_adv.adv_data = malloc(adv_len);
        if (!req->adpt.start_adv.adv_data) {
            BT_LOGE("%s, malloc fail", __func__);
            ret = BT_STATUS_NOMEM;
            goto error;
        }

        memcpy(req->adpt.start_adv.adv_data, adv_data, adv_len);
        req->adpt.start_adv.adv_len = adv_len;
    } else {
        req->adpt.start_adv.adv_data = NULL;
        req->adpt.start_adv.adv_len = 0;
    }

    if ((((req->adpt.start_adv.param.options & BT_LE_ADV_OPT_SCANNABLE) && ext_adv)
            || !ext_adv)
        && scan_rsp_data && scan_rsp_len > 0) {
        req->adpt.start_adv.scan_rsp_data = malloc(scan_rsp_len);
        if (!req->adpt.start_adv.scan_rsp_data) {
            BT_LOGE("%s, malloc fail", __func__);
            ret = BT_STATUS_NOMEM;
            goto error;
        }

        memcpy(req->adpt.start_adv.scan_rsp_data, scan_rsp_data, scan_rsp_len);
        req->adpt.start_adv.scan_rsp_len = scan_rsp_len;
    } else {
        req->adpt.start_adv.scan_rsp_data = NULL;
        req->adpt.start_adv.scan_rsp_len = 0;
    }

    if (params->duration) {
        req->adpt.start_adv.ext_param.timeout = params->duration;
    }

    return sal_send_req(req);

error:
    if (req->adpt.start_adv.adv_data)
        free(req->adpt.start_adv.adv_data);
    if (req->adpt.start_adv.scan_rsp_data)
        free(req->adpt.start_adv.scan_rsp_data);
    free(req);
    return ret;
}

static void STACK_CALL(stop_adv)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_le_adv_set* adv_set;
    int ret;
    bool ext_supported;

    ext_supported = bt_le_ext_adv_is_supported();

    if (!ext_supported) {
        ret = bt_le_adv_stop();
        if (ret) {
            BT_LOGE("%s, legacy adv stop fail", __func__);
            return;
        } else {
            goto stopped;
        }
    }

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

stopped:
    advertising_on_state_changed(req->adv_id, LE_ADVERTISING_STOPPED);
}

bt_status_t bt_sal_le_stop_adv(bt_controller_id_t id, uint8_t adv_id)
{
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, adv_id, STACK_CALL(stop_adv));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}
#endif /*CONFIG_BLUETOOTH_BLE_ADV*/