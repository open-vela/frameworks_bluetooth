/*
 * Copyright (C) 2024 Xiaomi Corporation. All rights reserved.
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
 *
 */
#include "feature_exports.h"
#include "feature_log.h"
#include "system_bluetooth.h"
#include "system_bluetooth_bt.h"
#include "system_bluetooth_bt_a2dpsink.h"
#include "feature_bluetooth.h"
#include "bt_a2dp_sink.h"
#include "bt_adapter.h"
#include "bt_list.h"
#include "uv.h"

#define add_feature_callback(feature_callbacks, new_callbacks_type, handle)                          \
    {                                                                                                \
        uv_mutex_lock(&feature_callbacks.mutex);                                                     \
        new_callbacks_type *new_callback = (new_callbacks_type *)malloc(sizeof(new_callbacks_type)); \
        memset(new_callback, -1, sizeof(new_callbacks_type));                                        \
        new_callback->feature_ins = handle;                                                          \
        bt_list_add_tail(feature_callbacks.callbacks, new_callback);                                 \
        uv_mutex_unlock(&feature_callbacks.mutex);                                                   \
    }

#define set_feature_callback(feature_callbacks, callbacks_type, find_func, handle, callback_id, callback_type)      \
    {                                                                                                               \
        uv_mutex_lock(&feature_callbacks.mutex);                                                                    \
        callbacks_type *callbacks = (callbacks_type *)bt_list_find(feature_callbacks.callbacks, find_func, handle); \
        callbacks->callback_type = callback_id;                                                                     \
        uv_mutex_unlock(&feature_callbacks.mutex);                                                                  \
    };

#define get_feature_callback(feature_callbacks, callbacks_type, find_func, handle, callback_id, callback_type)      \
    {                                                                                                               \
        uv_mutex_lock(&feature_callbacks.mutex);                                                                    \
        callbacks_type *callbacks = (callbacks_type *)bt_list_find(feature_callbacks.callbacks, find_func, handle); \
        callback_id = callbacks->callback_type;                                                                     \
        uv_mutex_unlock(&feature_callbacks.mutex);                                                                  \
    };

static bool get_callback_bluetooth(void *data, void *feature_ins)
{
    feature_bluetooth_bluetooth_callbacks_t *callbacks = (feature_bluetooth_bluetooth_callbacks_t *)data;
    if (!callbacks) {
        return false;
    }

    return callbacks->feature_ins == feature_ins;
}

static bool get_callback_bluetooth_bt(void *data, void *feature_ins)
{
    feature_bluetooth_bluetooth_bt_callbacks_t *callbacks = (feature_bluetooth_bluetooth_bt_callbacks_t *)data;
    if (!callbacks) {
        return false;
    }

    return callbacks->feature_ins == feature_ins;
}

static bool get_callback_a2dp_sink(void *data, void *feature_ins)
{
    feature_bluetooth_a2dp_sink_callbacks_t *callbacks = (feature_bluetooth_a2dp_sink_callbacks_t *)data;
    if (!callbacks) {
        return false;
    }

    return callbacks->feature_ins == feature_ins;
}

static void free_feature_callback(feature_bluetooth_callbacks_t *callbacks, FeatureInstanceHandle handle, bt_list_find_cb find_func)
{
    void *data;

    if (!callbacks) {
        return;
    }

    uv_mutex_lock(&callbacks->mutex);
    data = bt_list_find(callbacks->callbacks, find_func, handle);
    if (data) {
        bt_list_remove(callbacks->callbacks, data);
    }

    uv_mutex_unlock(&callbacks->mutex);
}

static void on_adapter_state_changed_cb(void *cookie, bt_adapter_state_t state)
{
    bt_instance_t *bt_ins = (bt_instance_t *)cookie;
    feature_bluetooth_features_callbacks_t *features_callbacks;
    bt_list_t *callbacks;
    bt_list_node_t *node;

    FEATURE_LOG_INFO("adapter state change callback, state: %d", state);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    callbacks = features_callbacks->feature_bluetooth_callbacks.callbacks;
    if (!callbacks) {
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        return;
    }

    while (node) {
        feature_bluetooth_bluetooth_callbacks_t *feature_callback;
        system_bluetooth_adapterStateCallbackData *data;
        callback_info_t *callback_info;

        feature_callback = (feature_bluetooth_bluetooth_callbacks_t *)bt_list_node(node);
        if (!feature_callback) {
            return;
        }

        FEATURE_LOG_INFO("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->on_adapter_state_changed_cb_id);
        if (!FeatureCheckCallbackId(feature_callback->feature_ins, feature_callback->on_adapter_state_changed_cb_id)) {
            return;
        }

        if (state != BT_ADAPTER_STATE_ON && state != BT_ADAPTER_STATE_OFF) {
            return;
        }

        data = system_bluetoothMallocadapterStateCallbackData();
        if (!data) {
            continue;
        }

        data->available = state == BT_ADAPTER_STATE_ON;
        data->discovering = bt_adapter_is_discovering(bt_ins);

        callback_info = (callback_info_t *)calloc(1, sizeof(callback_info_t));
        if (!callback_info) {
            return;
        }

        callback_info->callback_id = ON_ADAPTER_STATE_CHANGE;
        callback_info->feature_callback_id = feature_callback->on_adapter_state_changed_cb_id;
        callback_info->feature = feature_callback->feature_ins;
        callback_info->data = data;
        FeaturePost(feature_callback->feature_ins, feature_bluetooth_deal_callback, callback_info);
        node = bt_list_next(callbacks, node);
    }
}

static void on_discovery_state_changed_cb(void *cookie, bt_discovery_state_t state)
{
    bt_instance_t *bt_ins = (bt_instance_t *)cookie;
    feature_bluetooth_features_callbacks_t *features_callbacks;
    bt_list_t *callbacks;
    bt_list_node_t *node;

    FEATURE_LOG_INFO("discovery state change callback, state: %d", state);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    callbacks = features_callbacks->feature_bluetooth_callbacks.callbacks;
    if (!callbacks) {
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        return;
    }

    while (node) {
        feature_bluetooth_bluetooth_callbacks_t *feature_callback;
        callback_info_t *callback_info;
        system_bluetooth_adapterStateCallbackData *data;

        feature_callback = (feature_bluetooth_bluetooth_callbacks_t *)bt_list_node(node);
        if (!feature_callback) {
            return;
        }

        FEATURE_LOG_INFO("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->on_adapter_state_changed_cb_id);
        if (!FeatureCheckCallbackId(feature_callback->feature_ins, feature_callback->on_adapter_state_changed_cb_id)) {
            return;
        }

        data = system_bluetoothMallocadapterStateCallbackData();
        if (!data) {
            continue;
        }

        data->available = bt_adapter_get_state(bt_ins) == BT_ADAPTER_STATE_ON;
        data->discovering = state == BT_DISCOVERY_STATE_STARTED;

        callback_info = (callback_info_t *)calloc(1, sizeof(callback_info_t));
        if (!callback_info) {
            return;
        }

        callback_info->callback_id = ON_ADAPTER_STATE_CHANGE;
        callback_info->feature_callback_id = feature_callback->on_adapter_state_changed_cb_id;
        callback_info->feature = feature_callback->feature_ins;
        callback_info->data = data;
        FeaturePost(feature_callback->feature_ins, feature_bluetooth_deal_callback, callback_info);
        node = bt_list_next(callbacks, node);
    }
}

static void on_discovery_result_cb(void *cookie, bt_discovery_result_t *result)
{
    bt_instance_t *bt_ins = (bt_instance_t *)cookie;
    feature_bluetooth_features_callbacks_t *features_callbacks;
    bt_list_t *callbacks;
    bt_list_node_t *node;

    FEATURE_LOG_INFO("discovery result callback");
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    callbacks = features_callbacks->feature_bluetooth_bt_callbacks.callbacks;
    if (!callbacks) {
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        return;
    }

    while (node) {
        feature_bluetooth_bluetooth_bt_callbacks_t *feature_callback;
        system_bluetooth_bt_DiscoveryResultCallbackData *data;
        callback_info_t *callback_info;
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

        feature_callback = (feature_bluetooth_bluetooth_bt_callbacks_t *)bt_list_node(node);
        if (!feature_callback) {
            return;
        }

        FEATURE_LOG_INFO("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->on_discovery_result_cb_id);
        if (!FeatureCheckCallbackId(feature_callback->feature_ins, feature_callback->on_discovery_result_cb_id)) {
            return;
        }

        data = system_bluetooth_btMallocDiscoveryResultCallbackData();
        if (!data) {
            continue;
        }

        data->name = StringToFtString(result->name);
        bt_addr_ba2str(&result->addr, addr_str);
        data->deviceId = StringToFtString(addr_str);
        data->cod = result->cod;
        data->rssi = -result->rssi;

        callback_info = (callback_info_t *)calloc(1, sizeof(callback_info_t));
        if (!callback_info) {
            return;
        }

        callback_info->callback_id = ON_DISCOVERY_RESULT;
        callback_info->feature_callback_id = feature_callback->on_discovery_result_cb_id;
        callback_info->feature = feature_callback->feature_ins;
        callback_info->data = data;
        FeaturePost(feature_callback->feature_ins, feature_bluetooth_deal_callback, callback_info);
        node = bt_list_next(callbacks, node);
    }
}

static void on_pair_request_cb(void *cookie, bt_address_t *addr)
{
    bt_instance_t *bt_ins = (bt_instance_t *)cookie;
    bt_device_pair_request_reply(bt_ins, addr, true);
}

static void on_connect_request_cb(void *cookie, bt_address_t *addr)
{
    bt_instance_t *bt_ins = (bt_instance_t *)cookie;
    bt_device_connect_request_reply(bt_ins, addr, true);
}

static void on_bond_state_changed_cb(void *cookie, bt_address_t *addr, bt_transport_t transport, bond_state_t state, bool is_ctkd)
{
    bt_instance_t *bt_ins = (bt_instance_t *)cookie;
    feature_bluetooth_features_callbacks_t *features_callbacks;
    bt_list_t *callbacks;
    bt_list_node_t *node;

    FEATURE_LOG_INFO("bond state callback");
    if (transport != BT_TRANSPORT_BREDR) {
        return;
    }

    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    callbacks = features_callbacks->feature_bluetooth_bt_callbacks.callbacks;
    if (!callbacks) {
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        return;
    }

    while (node) {
        feature_bluetooth_bluetooth_bt_callbacks_t *feature_callback;
        system_bluetooth_bt_onBondStateChangeData *data;
        callback_info_t *callback_info;
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

        feature_callback = (feature_bluetooth_bluetooth_bt_callbacks_t *)bt_list_node(node);
        if (!feature_callback) {
            return;
        }

        FEATURE_LOG_INFO("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->on_bond_state_changed_cb_id);
        if (!FeatureCheckCallbackId(feature_callback->feature_ins, feature_callback->on_bond_state_changed_cb_id)) {
            return;
        }

        data = system_bluetooth_btMalloconBondStateChangeData();
        if (!data) {
            continue;
        }

        bt_addr_ba2str(addr, addr_str);
        data->deviceId = StringToFtString(addr_str);
        data->bondState = state;

        callback_info = (callback_info_t *)calloc(1, sizeof(callback_info_t));
        if (!callback_info) {
            return;
        }

        callback_info->callback_id = ON_BOND_STATE_CHANGE;
        callback_info->feature_callback_id = feature_callback->on_bond_state_changed_cb_id;
        callback_info->feature = feature_callback->feature_ins;
        callback_info->data = data;

        FeaturePost(feature_callback->feature_ins, feature_bluetooth_deal_callback, callback_info);
        node = bt_list_next(callbacks, node);
    }
}

const static adapter_callbacks_t g_adapter_cbs = {
    .on_adapter_state_changed = on_adapter_state_changed_cb,
    .on_discovery_state_changed = on_discovery_state_changed_cb,
    .on_discovery_result = on_discovery_result_cb,
    .on_pair_request = on_pair_request_cb,
    .on_connect_request = on_connect_request_cb,
    .on_bond_state_changed = on_bond_state_changed_cb,
};

static void a2dp_sink_connection_state_cb(void *cookie, bt_address_t *addr, profile_connection_state_t state)
{
    bt_instance_t *bt_ins = (bt_instance_t *)cookie;
    feature_bluetooth_features_callbacks_t *features_callbacks;
    bt_list_t *callbacks;
    bt_list_node_t *node;

    FEATURE_LOG_INFO("a2dp sink connection state cb");
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    callbacks = features_callbacks->feature_a2dp_sink_callbacks.callbacks;
    if (!callbacks) {
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        return;
    }

    while (node) {
        feature_bluetooth_a2dp_sink_callbacks_t *feature_callback;
        system_bluetooth_bt_a2dpsink_OnConnectStateChangeData *data;
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        callback_info_t *callback_info;

        feature_callback = (feature_bluetooth_a2dp_sink_callbacks_t *)bt_list_node(node);
        if (!feature_callback) {
            return;
        }

        FEATURE_LOG_INFO("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->a2dp_sink_connection_state_cb_id);
        if (!FeatureCheckCallbackId(feature_callback->feature_ins, feature_callback->a2dp_sink_connection_state_cb_id)) {
            return;
        }

        bt_addr_ba2str(addr, addr_str);
        data = system_bluetooth_bt_a2dpsinkMallocOnConnectStateChangeData();
        if (!data) {
            continue;
        }

        data->deviceId = StringToFtString(addr_str);
        data->connectState = state;

        callback_info = (callback_info_t *)calloc(1, sizeof(callback_info_t));
        if (!callback_info) {
            return;
        }

        callback_info->callback_id = A2DPSINK_ON_CONNECT_STATE_CHANGE;
        callback_info->feature_callback_id = feature_callback->a2dp_sink_connection_state_cb_id;
        callback_info->feature = feature_callback->feature_ins;
        callback_info->data = data;
        FeaturePost(feature_callback->feature_ins, feature_bluetooth_deal_callback, callback_info);
        node = bt_list_next(callbacks, node);
    }
}

static const a2dp_sink_callbacks_t a2dp_sink_cbs = {
    sizeof(a2dp_sink_cbs),
    a2dp_sink_connection_state_cb,
};

void feature_bluetooth_add_feature_callback(FeatureInstanceHandle handle, feature_bluetooth_type_t feature_type)
{
    bt_instance_t *bt_ins;
    feature_bluetooth_features_callbacks_t *features_callbacks;

    bt_ins = feature_bluetooth_get_bt_ins(handle);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    switch (feature_type) {
    case BLUETOOTH_FEATURE:
        add_feature_callback(features_callbacks->feature_bluetooth_callbacks, feature_bluetooth_bluetooth_callbacks_t, handle);
        break;
    case BLUETOOTH_BT_FEATURE:
        add_feature_callback(features_callbacks->feature_bluetooth_bt_callbacks, feature_bluetooth_bluetooth_bt_callbacks_t, handle);
        break;
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    case A2DPSINK_FEATURE:
        add_feature_callback(features_callbacks->feature_a2dp_sink_callbacks, feature_bluetooth_a2dp_sink_callbacks_t, handle);
        break;
#endif
    default:
        break;
    }
}

void feature_bluetooth_free_feature_callback(FeatureInstanceHandle handle, feature_bluetooth_type_t feature_type)
{
    bt_instance_t *bt_ins;
    feature_bluetooth_features_callbacks_t *features_callbacks;

    bt_ins = feature_bluetooth_get_bt_ins(handle);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    switch (feature_type) {
    case BLUETOOTH_FEATURE:
        free_feature_callback(&features_callbacks->feature_bluetooth_callbacks, handle, get_callback_bluetooth);
        break;
    case BLUETOOTH_BT_FEATURE:
        free_feature_callback(&features_callbacks->feature_bluetooth_bt_callbacks, handle, get_callback_bluetooth_bt);
        break;
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    case A2DPSINK_FEATURE:
        free_feature_callback(&features_callbacks->feature_a2dp_sink_callbacks, handle, get_callback_a2dp_sink);
        break;
#endif
    default:
        break;
    }
}

void feature_bluetooth_set_feature_callback(FeatureInstanceHandle handle, FtCallbackId callback_id, feature_bluetooth_callback_t callback_type)
{
    bt_instance_t *bt_ins;
    feature_bluetooth_features_callbacks_t *features_callbacks;

    bt_ins = feature_bluetooth_get_bt_ins(handle);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    switch (callback_type) {
    case ON_ADAPTER_STATE_CHANGE:
        set_feature_callback(features_callbacks->feature_bluetooth_callbacks, feature_bluetooth_bluetooth_callbacks_t, get_callback_bluetooth, handle, callback_id, on_adapter_state_changed_cb_id);
        break;
    case ON_DISCOVERY_RESULT:
        set_feature_callback(features_callbacks->feature_bluetooth_bt_callbacks, feature_bluetooth_bluetooth_bt_callbacks_t, get_callback_bluetooth_bt, handle, callback_id, on_discovery_result_cb_id);
        break;
    case ON_BOND_STATE_CHANGE:
        set_feature_callback(features_callbacks->feature_bluetooth_bt_callbacks, feature_bluetooth_bluetooth_bt_callbacks_t, get_callback_bluetooth_bt, handle, callback_id, on_bond_state_changed_cb_id);
        break;
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    case A2DPSINK_ON_CONNECT_STATE_CHANGE:
        set_feature_callback(features_callbacks->feature_a2dp_sink_callbacks, feature_bluetooth_a2dp_sink_callbacks_t, get_callback_a2dp_sink, handle, callback_id, a2dp_sink_connection_state_cb_id);
        break;
#endif
    default:
        break;
    }
}

FtCallbackId feature_bluetooth_get_feature_callback(FeatureInstanceHandle handle, feature_bluetooth_callback_t callback_type)
{
    bt_instance_t *bt_ins;
    feature_bluetooth_features_callbacks_t *features_callbacks;
    FtCallbackId callback_id = -1;

    bt_ins = feature_bluetooth_get_bt_ins(handle);
    if (!bt_ins) {
        return callback_id;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)bt_ins->context;
    if (!features_callbacks) {
        return callback_id;
    }

    switch (callback_type) {
    case ON_ADAPTER_STATE_CHANGE:
        set_feature_callback(features_callbacks->feature_bluetooth_callbacks, feature_bluetooth_bluetooth_callbacks_t, get_callback_bluetooth, handle, callback_id, on_adapter_state_changed_cb_id);
        break;
    case ON_DISCOVERY_RESULT:
        set_feature_callback(features_callbacks->feature_bluetooth_bt_callbacks, feature_bluetooth_bluetooth_bt_callbacks_t, get_callback_bluetooth_bt, handle, callback_id, on_discovery_result_cb_id);
        break;
    case ON_BOND_STATE_CHANGE:
        set_feature_callback(features_callbacks->feature_bluetooth_bt_callbacks, feature_bluetooth_bluetooth_bt_callbacks_t, get_callback_bluetooth_bt, handle, callback_id, on_bond_state_changed_cb_id);
        break;
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    case A2DPSINK_ON_CONNECT_STATE_CHANGE:
        get_feature_callback(features_callbacks->feature_a2dp_sink_callbacks, feature_bluetooth_a2dp_sink_callbacks_t, get_callback_a2dp_sink, handle, callback_id, a2dp_sink_connection_state_cb_id);
        break;
#endif
    default:
        break;
    }

    return callback_id;
}

void feature_bluetooth_callback_init(bt_instance_t *bt_ins)
{
    feature_bluetooth_features_callbacks_t *features_callbacks;

    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_callbacks_t *)malloc(sizeof(feature_bluetooth_features_callbacks_t));
    if (!features_callbacks) {
        return;
    }

    features_callbacks->feature_bluetooth_callbacks.callbacks = bt_list_new(free);
    uv_mutex_init(&features_callbacks->feature_bluetooth_callbacks.mutex);

    features_callbacks->feature_bluetooth_bt_callbacks.callbacks = bt_list_new(free);
    uv_mutex_init(&features_callbacks->feature_bluetooth_bt_callbacks.mutex);

    bt_ins->adapter_cookie = bt_adapter_register_callback(bt_ins, &g_adapter_cbs);

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    features_callbacks->feature_a2dp_sink_callbacks.callbacks = bt_list_new(free);
    uv_mutex_init(&features_callbacks->feature_a2dp_sink_callbacks.mutex);
    bt_ins->a2dp_sink_cookie = bt_a2dp_sink_register_callbacks(bt_ins, &a2dp_sink_cbs);
#endif

    bt_ins->context = features_callbacks;
}

void feature_bluetooth_callback_uninit(bt_instance_t *bt_ins)
{
    feature_bluetooth_features_callbacks_t *features_callbacks;

    if (!bt_ins) {
        return;
    }

    features_callbacks = bt_ins->context;
    bt_ins->context = NULL;
    if (!features_callbacks) {
        return;
    }

    bt_list_free(features_callbacks->feature_bluetooth_callbacks.callbacks);
    uv_mutex_destroy(&features_callbacks->feature_bluetooth_callbacks.mutex);

    bt_list_free(features_callbacks->feature_bluetooth_bt_callbacks.callbacks);
    uv_mutex_destroy(&features_callbacks->feature_bluetooth_bt_callbacks.mutex);

    bt_adapter_unregister_callback(bt_ins, bt_ins->adapter_cookie);

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_list_free(features_callbacks->feature_a2dp_sink_callbacks.callbacks);
    uv_mutex_destroy(&features_callbacks->feature_a2dp_sink_callbacks.mutex);
    bt_a2dp_sink_unregister_callbacks(bt_ins, bt_ins->a2dp_sink_cookie);
#endif

    free(features_callbacks);
}