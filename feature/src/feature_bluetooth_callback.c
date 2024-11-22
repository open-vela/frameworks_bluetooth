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
#include "bt_a2dp_sink.h"
#include "bt_adapter.h"
#include "bt_avrcp_control.h"
#include "bt_list.h"
#include "feature_bluetooth.h"
#include "feature_exports.h"
#include "feature_log.h"
#include "system_bluetooth.h"
#include "system_bluetooth_bt.h"
#include "system_bluetooth_bt_a2dpsink.h"
#include "system_bluetooth_bt_avrcpcontrol.h"
#include "uv.h"

#define REMOVE_CALLBACK(feature_callback, callback_type)                                                       \
    do {                                                                                                       \
        if (feature_callback->callback_type != -1) {                                                           \
            feature_bluetooth_post_task(feature_callback->feature_ins, feature_callback->callback_type, NULL); \
        }                                                                                                      \
        feature_callback->callback_type = -1;                                                                  \
    } while (0);

#define add_feature_callback(feature_callbacks, new_callbacks_type, handle)                         \
    {                                                                                               \
        new_callbacks_type* new_callback = (new_callbacks_type*)malloc(sizeof(new_callbacks_type)); \
        /* The hexadecimal representation of -1 is 0xFF */                                          \
        memset(new_callback, -1, sizeof(new_callbacks_type));                                       \
        new_callback->feature_ins = handle;                                                         \
        bt_list_add_tail(feature_callbacks, new_callback);                                          \
    }

#define set_feature_callback(feature_callbacks, callbacks_type, find_func, handle, callback_id, callback_type) \
    {                                                                                                          \
        callbacks_type* callbacks = (callbacks_type*)bt_list_find(feature_callbacks, find_func, handle);       \
        REMOVE_CALLBACK(callbacks, callback_type)                                                              \
        callbacks->callback_type = callback_id;                                                                \
    };

#define get_feature_callback(feature_callbacks, callbacks_type, find_func, handle, callback_id, callback_type) \
    {                                                                                                          \
        callbacks_type* callbacks = (callbacks_type*)bt_list_find(feature_callbacks, find_func, handle);       \
        callback_id = callbacks->callback_type;                                                                \
    };

static bool get_callback_bluetooth(void* data, void* feature_ins)
{
    feature_bluetooth_bluetooth_callbacks_t* callbacks = (feature_bluetooth_bluetooth_callbacks_t*)data;
    if (!callbacks) {
        return false;
    }

    return callbacks->feature_ins == feature_ins;
}

static bool get_callback_bluetooth_bt(void* data, void* feature_ins)
{
    feature_bluetooth_bluetooth_bt_callbacks_t* callbacks = (feature_bluetooth_bluetooth_bt_callbacks_t*)data;
    if (!callbacks) {
        return false;
    }

    return callbacks->feature_ins == feature_ins;
}

static bool get_callback_a2dp_sink(void* data, void* feature_ins)
{
    feature_bluetooth_a2dp_sink_callbacks_t* callbacks = (feature_bluetooth_a2dp_sink_callbacks_t*)data;
    if (!callbacks) {
        return false;
    }

    return callbacks->feature_ins == feature_ins;
}

static bool get_callback_avrcp_control(void* data, void* feature_ins)
{
    feature_bluetooth_avrcp_control_callbacks_t* callbacks = (feature_bluetooth_avrcp_control_callbacks_t*)data;
    if (!callbacks) {
        return false;
    }

    return callbacks->feature_ins == feature_ins;
}

static void free_feature_callback(bt_list_t* callbacks, FeatureInstanceHandle handle, bt_list_find_cb find_func)
{
    void* data;

    if (!callbacks) {
        return;
    }

    data = bt_list_find(callbacks, find_func, handle);
    if (data) {
        bt_list_remove(callbacks, data);
    }
}

static void free_feature_bluetooth_node(void* node)
{
    feature_bluetooth_bluetooth_callbacks_t* feature_callback = (feature_bluetooth_bluetooth_callbacks_t*)node;

    if (!feature_callback) {
        return;
    }

    REMOVE_CALLBACK(feature_callback, on_adapter_state_changed_cb_id);
    free(feature_callback);
}

static void free_feature_bluetooth_bt_node(void* node)
{
    feature_bluetooth_bluetooth_bt_callbacks_t* feature_callback = (feature_bluetooth_bluetooth_bt_callbacks_t*)node;

    if (!feature_callback) {
        return;
    }

    REMOVE_CALLBACK(feature_callback, on_bond_state_changed_cb_id);
    REMOVE_CALLBACK(feature_callback, on_discovery_result_cb_id);
    free(feature_callback);
}

static void free_feature_bluetooth_a2dp_sink_node(void* node)
{
    feature_bluetooth_a2dp_sink_callbacks_t* feature_callback = (feature_bluetooth_a2dp_sink_callbacks_t*)node;

    if (!feature_callback) {
        return;
    }

    REMOVE_CALLBACK(feature_callback, a2dp_sink_connection_state_cb_id);
    free(feature_callback);
}

static void free_feature_bluetooth_avrcp_control_node(void* node)
{
    feature_bluetooth_avrcp_control_callbacks_t* feature_callback = (feature_bluetooth_avrcp_control_callbacks_t*)node;

    if (!feature_callback) {
        return;
    }

    REMOVE_CALLBACK(feature_callback, avrcp_control_element_attribute_cb_id);
    free(feature_callback);
}

static void on_adapter_state_changed_cb(void* cookie, bt_adapter_state_t state)
{
    bt_instance_t* bt_ins = (bt_instance_t*)cookie;
    feature_bluetooth_features_info_t* features_callbacks;
    bt_list_t* callbacks;
    bt_list_node_t* node;

    FEATURE_LOG_DEBUG("adapter state change callback, state: %d", state);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    uv_mutex_lock(&features_callbacks->mutex);
    callbacks = features_callbacks->feature_bluetooth_callbacks;
    if (!callbacks) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    while (node) {
        feature_bluetooth_bluetooth_callbacks_t* feature_callback;
        system_bluetooth_adapterStateCallbackData* data;

        feature_callback = (feature_bluetooth_bluetooth_callbacks_t*)bt_list_node(node);
        if (!feature_callback) {
            break;
        }

        FEATURE_LOG_DEBUG("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->on_adapter_state_changed_cb_id);
        if (state != BT_ADAPTER_STATE_ON && state != BT_ADAPTER_STATE_OFF) {
            break;
        }

        data = system_bluetoothMallocadapterStateCallbackData();
        if (!data) {
            continue;
        }

        data->available = state == BT_ADAPTER_STATE_ON;
        data->discovering = bt_adapter_is_discovering(bt_ins);

        feature_bluetooth_post_task(feature_callback->feature_ins, feature_callback->on_adapter_state_changed_cb_id, data);
        node = bt_list_next(callbacks, node);
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

static void on_discovery_state_changed_cb(void* cookie, bt_discovery_state_t state)
{
    bt_instance_t* bt_ins = (bt_instance_t*)cookie;
    feature_bluetooth_features_info_t* features_callbacks;
    bt_list_t* callbacks;
    bt_list_node_t* node;

    FEATURE_LOG_DEBUG("discovery state change callback, state: %d", state);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;

    if (!features_callbacks) {
        return;
    }

    uv_mutex_lock(&features_callbacks->mutex);
    callbacks = features_callbacks->feature_bluetooth_callbacks;
    if (!callbacks) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    while (node) {
        feature_bluetooth_bluetooth_callbacks_t* feature_callback;
        system_bluetooth_adapterStateCallbackData* data;

        feature_callback = (feature_bluetooth_bluetooth_callbacks_t*)bt_list_node(node);
        if (!feature_callback) {
            break;
        }

        FEATURE_LOG_DEBUG("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->on_adapter_state_changed_cb_id);
        data = system_bluetoothMallocadapterStateCallbackData();
        if (!data) {
            continue;
        }

        data->available = bt_adapter_get_state(bt_ins) == BT_ADAPTER_STATE_ON;
        data->discovering = state == BT_DISCOVERY_STATE_STARTED;

        feature_bluetooth_post_task(feature_callback->feature_ins, feature_callback->on_adapter_state_changed_cb_id, data);
        node = bt_list_next(callbacks, node);
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

static void on_discovery_result_cb(void* cookie, bt_discovery_result_t* result)
{
    bt_instance_t* bt_ins = (bt_instance_t*)cookie;
    feature_bluetooth_features_info_t* features_callbacks;
    bt_list_t* callbacks;
    bt_list_node_t* node;

    FEATURE_LOG_DEBUG("discovery result callback");
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;

    if (!features_callbacks) {
        return;
    }

    uv_mutex_lock(&features_callbacks->mutex);
    callbacks = features_callbacks->feature_bluetooth_bt_callbacks;
    if (!callbacks) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    while (node) {
        feature_bluetooth_bluetooth_bt_callbacks_t* feature_callback;
        system_bluetooth_bt_DiscoveryResultCallbackData* data;
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

        feature_callback = (feature_bluetooth_bluetooth_bt_callbacks_t*)bt_list_node(node);
        if (!feature_callback) {
            break;
        }

        FEATURE_LOG_DEBUG("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->on_discovery_result_cb_id);
        data = system_bluetooth_btMallocDiscoveryResultCallbackData();
        if (!data) {
            continue;
        }

        data->name = StringToFtString(result->name);
        bt_addr_ba2str(&result->addr, addr_str);
        data->deviceId = StringToFtString(addr_str);
        data->cod = result->cod;
        data->rssi = -result->rssi;

        feature_bluetooth_post_task(feature_callback->feature_ins, feature_callback->on_discovery_result_cb_id, data);
        node = bt_list_next(callbacks, node);
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

static void on_bond_state_changed_cb(void* cookie, bt_address_t* addr, bt_transport_t transport, bond_state_t state, bool is_ctkd)
{
    bt_instance_t* bt_ins = (bt_instance_t*)cookie;
    feature_bluetooth_features_info_t* features_callbacks;
    bt_list_t* callbacks;
    bt_list_node_t* node;

    FEATURE_LOG_DEBUG("bond state callback");
    if (transport != BT_TRANSPORT_BREDR) {
        return;
    }

    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;

    if (!features_callbacks) {
        return;
    }

    uv_mutex_lock(&features_callbacks->mutex);
    callbacks = features_callbacks->feature_bluetooth_bt_callbacks;
    if (!callbacks) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    while (node) {
        feature_bluetooth_bluetooth_bt_callbacks_t* feature_callback;
        system_bluetooth_bt_onBondStateChangeData* data;
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

        feature_callback = (feature_bluetooth_bluetooth_bt_callbacks_t*)bt_list_node(node);
        if (!feature_callback) {
            break;
        }

        FEATURE_LOG_DEBUG("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->on_bond_state_changed_cb_id);
        data = system_bluetooth_btMalloconBondStateChangeData();
        if (!data) {
            continue;
        }

        bt_addr_ba2str(addr, addr_str);
        data->deviceId = StringToFtString(addr_str);
        data->bondState = state;

        feature_bluetooth_post_task(feature_callback->feature_ins, feature_callback->on_bond_state_changed_cb_id, data);
        node = bt_list_next(callbacks, node);
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

const static adapter_callbacks_t g_adapter_cbs = {
    .on_adapter_state_changed = on_adapter_state_changed_cb,
    .on_discovery_state_changed = on_discovery_state_changed_cb,
    .on_discovery_result = on_discovery_result_cb,
    .on_bond_state_changed = on_bond_state_changed_cb,
};

static void a2dp_sink_connection_state_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    bt_instance_t* bt_ins = (bt_instance_t*)cookie;
    feature_bluetooth_features_info_t* features_callbacks;
    bt_list_t* callbacks;
    bt_list_node_t* node;

    FEATURE_LOG_DEBUG("a2dp sink connection state cb");
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    uv_mutex_lock(&features_callbacks->mutex);
    callbacks = features_callbacks->feature_a2dp_sink_callbacks;
    if (!callbacks) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    while (node) {
        feature_bluetooth_a2dp_sink_callbacks_t* feature_callback;
        system_bluetooth_bt_a2dpsink_OnConnectStateChangeData* data;
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

        feature_callback = (feature_bluetooth_a2dp_sink_callbacks_t*)bt_list_node(node);
        if (!feature_callback) {
            break;
        }

        FEATURE_LOG_DEBUG("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->a2dp_sink_connection_state_cb_id);
        bt_addr_ba2str(addr, addr_str);
        data = system_bluetooth_bt_a2dpsinkMallocOnConnectStateChangeData();
        if (!data) {
            continue;
        }

        data->deviceId = StringToFtString(addr_str);
        data->connectState = state;

        feature_bluetooth_post_task(feature_callback->feature_ins, feature_callback->a2dp_sink_connection_state_cb_id, data);
        node = bt_list_next(callbacks, node);
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

static const a2dp_sink_callbacks_t a2dp_sink_cbs = {
    sizeof(a2dp_sink_cbs),
    a2dp_sink_connection_state_cb,
};

system_bluetooth_bt_avrcpcontrol_attr_info_t* get_attr_info(avrcp_element_attr_val_t* attr)
{
    system_bluetooth_bt_avrcpcontrol_attr_info_t* attr_info = system_bluetooth_bt_avrcpcontrolMallocattr_info_t();
    attr_info->attrId = attr->attr_id;
    attr_info->chrSet = attr->chr_set;
    attr_info->text = StringToFtString((char*)attr->text);
    return attr_info;
}

static void avrcp_control_get_element_attribute_cb(void* cookie, bt_address_t* addr, uint8_t attrs_count, avrcp_element_attr_val_t* attrs)
{
    int attr_index;
    bt_instance_t* bt_ins = (bt_instance_t*)cookie;
    feature_bluetooth_features_info_t* features_callbacks;
    bt_list_t* callbacks;
    bt_list_node_t* node;

    FEATURE_LOG_DEBUG("avrcp control element attribute cb");
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;
    if (!features_callbacks) {
        return;
    }

    uv_mutex_lock(&features_callbacks->mutex);
    callbacks = features_callbacks->feature_avrcp_control_callbacks;
    if (!callbacks) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    node = bt_list_head(callbacks);
    if (!node) {
        uv_mutex_unlock(&features_callbacks->mutex);
        return;
    }

    while (node) {
        feature_bluetooth_avrcp_control_callbacks_t* feature_callback;
        system_bluetooth_bt_avrcpcontrol_OnElementAttributeData* data;
        FtArray* attributes;

        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

        feature_callback = (feature_bluetooth_avrcp_control_callbacks_t*)bt_list_node(node);
        if (!feature_callback) {
            break;
        }

        FEATURE_LOG_DEBUG("feature:%p, callbackId:%d", feature_callback->feature_ins, feature_callback->avrcp_control_element_attribute_cb_id);
        bt_addr_ba2str(addr, addr_str);
        data = system_bluetooth_bt_avrcpcontrolMallocOnElementAttributeData();
        attributes = system_bluetooth_bt_avrcpcontrol_malloc_attr_info_t_struct_type_array();

        if (!data || !attributes) {
            continue;
        }

        data->deviceId = StringToFtString(addr_str);
        data->attrsCount = attrs_count;
        attributes->_size = attrs_count;
        attributes->_element = malloc(attributes->_size * sizeof(struct Attribute*));
        if (!attributes->_element) {
            continue;
        }
        for (attr_index = 0; attr_index < attributes->_size; attr_index++) {
            system_bluetooth_bt_avrcpcontrol_attr_info_t* attr_info = get_attr_info(&attrs[attr_index]);
            ((system_bluetooth_bt_avrcpcontrol_attr_info_t**)attributes->_element)[attr_index] = attr_info;
        }

        data->attrs = attributes;

        feature_bluetooth_post_task(feature_callback->feature_ins, feature_callback->avrcp_control_element_attribute_cb_id, data);
        node = bt_list_next(callbacks, node);
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

static const avrcp_control_callbacks_t avrcp_control_cbs = {
    .size = sizeof(avrcp_control_cbs),
    .get_element_attribute_cb = avrcp_control_get_element_attribute_cb,
};

void feature_bluetooth_add_feature_callback(FeatureInstanceHandle handle, feature_bluetooth_feature_type_t feature_type)
{
    bt_instance_t* bt_ins;
    feature_bluetooth_features_info_t* features_callbacks;

    bt_ins = feature_bluetooth_get_bt_ins(handle);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;

    if (!features_callbacks) {
        return;
    }

    uv_mutex_lock(&features_callbacks->mutex);
    switch (feature_type) {
    case FEATURE_BLUETOOTH:
        add_feature_callback(features_callbacks->feature_bluetooth_callbacks, feature_bluetooth_bluetooth_callbacks_t, handle);
        break;
    case FEATURE_BLUETOOTH_BT:
        add_feature_callback(features_callbacks->feature_bluetooth_bt_callbacks, feature_bluetooth_bluetooth_bt_callbacks_t, handle);
        break;
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    case FEATURE_BLUETOOTH_A2DPSINK:
        add_feature_callback(features_callbacks->feature_a2dp_sink_callbacks, feature_bluetooth_a2dp_sink_callbacks_t, handle);
        break;
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    case FEATURE_BLUETOOTH_AVRCPCONTROL:
        add_feature_callback(features_callbacks->feature_avrcp_control_callbacks, feature_bluetooth_avrcp_control_callbacks_t, handle);
        break;
#endif
    default:
        break;
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

void feature_bluetooth_free_feature_callback(FeatureInstanceHandle handle, feature_bluetooth_feature_type_t feature_type)
{
    bt_instance_t* bt_ins;
    feature_bluetooth_features_info_t* features_callbacks;

    bt_ins = feature_bluetooth_get_bt_ins(handle);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;

    if (!features_callbacks) {
        return;
    }
    uv_mutex_lock(&features_callbacks->mutex);
    switch (feature_type) {
    case FEATURE_BLUETOOTH:
        free_feature_callback(features_callbacks->feature_bluetooth_callbacks, handle, get_callback_bluetooth);
        break;
    case FEATURE_BLUETOOTH_BT:
        free_feature_callback(features_callbacks->feature_bluetooth_bt_callbacks, handle, get_callback_bluetooth_bt);
        break;
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    case FEATURE_BLUETOOTH_A2DPSINK:
        free_feature_callback(features_callbacks->feature_a2dp_sink_callbacks, handle, get_callback_a2dp_sink);
        break;
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    case FEATURE_BLUETOOTH_AVRCPCONTROL:
        free_feature_callback(features_callbacks->feature_avrcp_control_callbacks, handle, get_callback_avrcp_control);
        break;
#endif
    default:
        break;
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

void feature_bluetooth_set_feature_callback(FeatureInstanceHandle handle, FtCallbackId callback_id, feature_bluetooth_callback_t callback_type)
{
    bt_instance_t* bt_ins;
    feature_bluetooth_features_info_t* features_callbacks;

    bt_ins = feature_bluetooth_get_bt_ins(handle);
    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;

    if (!features_callbacks) {
        return;
    }
    uv_mutex_lock(&features_callbacks->mutex);
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
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    case AVRCPCONTROL_ELEMENT_ATTRIBUTE_CALLBACK:
        set_feature_callback(features_callbacks->feature_avrcp_control_callbacks, feature_bluetooth_avrcp_control_callbacks_t, get_callback_avrcp_control, handle, callback_id, avrcp_control_element_attribute_cb_id);
        break;
#endif
    default:
        break;
    }
    uv_mutex_unlock(&features_callbacks->mutex);
}

FtCallbackId feature_bluetooth_get_feature_callback(FeatureInstanceHandle handle, feature_bluetooth_callback_t callback_type)
{
    bt_instance_t* bt_ins;
    feature_bluetooth_features_info_t* features_callbacks;
    FtCallbackId callback_id = -1;

    bt_ins = feature_bluetooth_get_bt_ins(handle);
    if (!bt_ins) {
        return callback_id;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)bt_ins->context;

    if (!features_callbacks) {
        return callback_id;
    }

    uv_mutex_lock(&features_callbacks->mutex);
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
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    case AVRCPCONTROL_ELEMENT_ATTRIBUTE_CALLBACK:
        get_feature_callback(features_callbacks->feature_avrcp_control_callbacks, feature_bluetooth_avrcp_control_callbacks_t, get_callback_avrcp_control, handle, callback_id, avrcp_control_element_attribute_cb_id);
        break;
#endif
    default:
        break;
    }
    uv_mutex_unlock(&features_callbacks->mutex);
    return callback_id;
}

void feature_bluetooth_callback_init(bt_instance_t* bt_ins)
{
    feature_bluetooth_features_info_t* features_callbacks;

    if (!bt_ins) {
        return;
    }

    features_callbacks = (feature_bluetooth_features_info_t*)calloc(1, sizeof(feature_bluetooth_features_info_t));

    assert(features_callbacks);

    uv_mutex_init(&features_callbacks->mutex);

    features_callbacks->feature_bluetooth_callbacks = bt_list_new(free_feature_bluetooth_node);
    features_callbacks->feature_bluetooth_bt_callbacks = bt_list_new(free_feature_bluetooth_bt_node);

    bt_ins->adapter_cookie = bt_adapter_register_callback(bt_ins, &g_adapter_cbs);

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    features_callbacks->feature_a2dp_sink_callbacks = bt_list_new(free_feature_bluetooth_a2dp_sink_node);
    bt_ins->a2dp_sink_cookie = bt_a2dp_sink_register_callbacks(bt_ins, &a2dp_sink_cbs);
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    features_callbacks->feature_avrcp_control_callbacks = bt_list_new(free_feature_bluetooth_avrcp_control_node);
    bt_ins->avrcp_control_cookie = bt_avrcp_control_register_callbacks(bt_ins, &avrcp_control_cbs);
#endif

    bt_ins->context = features_callbacks;
}

void feature_bluetooth_callback_uninit(bt_instance_t* bt_ins)
{
    feature_bluetooth_features_info_t* features_callbacks;

    if (!bt_ins) {
        return;
    }

    features_callbacks = bt_ins->context;
    bt_ins->context = NULL;
    if (!features_callbacks) {
        return;
    }

    bt_adapter_unregister_callback(bt_ins, bt_ins->adapter_cookie);
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_a2dp_sink_unregister_callbacks(bt_ins, bt_ins->a2dp_sink_cookie);
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    bt_avrcp_control_unregister_callbacks(bt_ins, bt_ins->avrcp_control_cookie);
#endif

    uv_mutex_lock(&features_callbacks->mutex);
    bt_list_free(features_callbacks->feature_bluetooth_callbacks);
    bt_list_free(features_callbacks->feature_bluetooth_bt_callbacks);
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_list_free(features_callbacks->feature_a2dp_sink_callbacks);
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    bt_list_free(features_callbacks->feature_avrcp_control_callbacks);
#endif
    uv_mutex_unlock(&features_callbacks->mutex);

    uv_mutex_destroy(&features_callbacks->mutex);
    free(features_callbacks);
}