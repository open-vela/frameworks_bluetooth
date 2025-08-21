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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt_addr.h"
#include "bt_gatt_defs.h"
#include "bt_gatt_feature.h"
#include "bt_gattc.h"
#include "bt_list.h"
#include "bt_status.h"
#include "bt_uuid.h"

#if defined(BT_FEATURE_LOG_ON)
#define BT_FEATURE_LOG(fmt, ...) syslog(LOG_INFO, "[feature] " fmt "\n", ##__VA_ARGS__)
#else
#define BT_FEATURE_LOG(fmt, ...) \
    do {                         \
    } while (0)
#endif

#define BT_GATTC_FEATURE_DISCOVERY_DONE 0U /* Special marker: indicates all services discovered */

#define BT_GATTC_FEATURE_INVOKE_CB(client, cb_field, ...) \
    do {                                                  \
        if ((client) && (client)->callbacks.cb_field)     \
            (client)->callbacks.cb_field(__VA_ARGS__);    \
    } while (0)

typedef struct gatt_client gatt_client_t;

typedef struct {
    gatt_client_t* client;
    bool service_range_ends;
    bool services_discover_ends;
} get_attr_user_data_t;

struct gatt_client {
    gattc_handle_t conn;
    bt_address_t addr;
    bt_gattc_feature_callbacks_t callbacks;
    bool connected;
    bool services_discovering;

    struct {
        bt_gattc_feature_create_client_cb_t create_client_cb;
        void* create_userdata;
    } create_client_ctx;

    struct {
        bt_gattc_feature_delete_client_cb_t delete_client_cb;
        void* delete_userdata;
    } delete_client_ctx;

    bt_instance_t* ins;
    bt_list_t* services_db;
    bt_list_t* temp_attrs;
};

static bt_list_t* g_gatt_client_list = NULL;

static void gatt_feature_free_service(void* data)
{
    size_t i;
    gatt_descriptor_t* descriptor_array;
    gatt_service_t* service;

    if (!data)
        return;

    service = (gatt_service_t*)data;

    if (service->characteristics) {
        for (i = 0; i < service->characteristic_count; ++i) {
            descriptor_array = service->characteristics[i].descriptors;

            if (descriptor_array) {
                free(descriptor_array);
                service->characteristics[i].descriptors = NULL;
                service->characteristics[i].descriptor_count = 0;
            }
        }

        free(service->characteristics);
        service->characteristics = NULL;
        service->characteristic_count = 0;
    }

    if (service->included_services) {
        free(service->included_services);
        service->included_services = NULL;
        service->included_service_count = 0;
    }

    free(service);
}

bool discovery_database_create(gatt_client_t* client)
{
    if (!client)
        return false;

    if (!client->temp_attrs) {
        client->temp_attrs = bt_list_new(free);
        if (!client->temp_attrs) {
            return false;
        }
    }
    if (!client->services_db) {
        client->services_db = bt_list_new(gatt_feature_free_service);
        if (!client->services_db) {
            bt_list_free(client->temp_attrs);
            return false;
        }
    }
    return true;
}

void discovery_database_clear(gatt_client_t* client)
{
    if (!client)
        return;

    if (client->services_db)
        bt_list_clear(client->services_db);
    if (client->temp_attrs)
        bt_list_clear(client->temp_attrs);
}

void discovery_database_destroy(gatt_client_t* client)
{
    if (!client)
        return;

    if (client->services_db) {
        bt_list_free(client->services_db);
        client->services_db = NULL;
    }
    if (client->temp_attrs) {
        bt_list_free(client->temp_attrs);
        client->temp_attrs = NULL;
    }
}

static bool match_client_by_addr(void* element, void* context)
{
    bt_address_t* target_addr;
    gatt_client_t* client;

    target_addr = (bt_address_t*)context;
    client = (gatt_client_t*)element;

    return memcmp(&client->addr, target_addr, sizeof(bt_address_t)) == 0;
}

static gatt_client_t* find_client_by_addr(bt_address_t* addr)
{
    if (!g_gatt_client_list || !addr)
        return NULL;

    return (gatt_client_t*)bt_list_find(g_gatt_client_list, match_client_by_addr, (void*)addr);
}

static gatt_client_t* find_client_by_conn(gattc_handle_t conn)
{
    bt_list_node_t* node;
    gatt_client_t* client;

    if (!g_gatt_client_list)
        return NULL;

    for (node = bt_list_head(g_gatt_client_list); node;
         node = bt_list_next(g_gatt_client_list, node)) {

        client = (gatt_client_t*)bt_list_node(node);

        if (client && client->conn == conn)
            return client;
    }

    return NULL;
}

static void free_client_instance(void* data)
{
    gatt_client_t* client;

    if (!data)
        return;

    client = (gatt_client_t*)data;

    discovery_database_destroy(client);

    free(client);
}

static gatt_service_t* find_service_by_uuid(gatt_client_t* client, const bt_uuid_t* service_uuid)
{
    bt_list_node_t* node;
    gatt_service_t* service;

    if (!client || !client->services_db || !service_uuid)
        return NULL;

    for (node = bt_list_head(client->services_db); node;
         node = bt_list_next(client->services_db, node)) {

        service = (gatt_service_t*)bt_list_node(node);

        if (service && bt_uuid_compare(&service->uuid, service_uuid) == 0)
            return service;
    }

    return NULL;
}

static gatt_characteristic_t* find_char_by_uuid(const gatt_service_t* service, const bt_uuid_t* char_uuid)
{
    size_t i;
    gatt_characteristic_t* characteristic;

    if (!service || !service->characteristics || !char_uuid)
        return NULL;

    for (i = 0; i < service->characteristic_count; ++i) {
        characteristic = &service->characteristics[i];

        if (bt_uuid_compare(&characteristic->uuid, char_uuid) == 0)
            return characteristic;
    }

    return NULL;
}

static gatt_descriptor_t* find_desc_by_uuid(const gatt_characteristic_t* characteristic, const bt_uuid_t* desc_uuid)
{
    size_t i;
    gatt_descriptor_t* descriptor;

    if (!characteristic || !desc_uuid || !characteristic->descriptors)
        return NULL;

    for (i = 0; i < characteristic->descriptor_count; ++i) {
        descriptor = &characteristic->descriptors[i];

        if (bt_uuid_compare(&descriptor->uuid, desc_uuid) == 0)
            return descriptor;
    }

    return NULL;
}

static gatt_characteristic_t* find_char_by_value_handle(gatt_client_t* client, uint16_t value_handle)
{
    bt_list_node_t* node;
    gatt_service_t* service;
    size_t i;
    gatt_characteristic_t* characteristic;

    if (!client || !client->services_db)
        return NULL;

    for (node = bt_list_head(client->services_db); node; node = bt_list_next(client->services_db, node)) {
        service = (gatt_service_t*)bt_list_node(node);

        if (!service || !service->characteristics)
            continue;

        for (i = 0; i < service->characteristic_count; ++i) {
            characteristic = &service->characteristics[i];

            if (characteristic->value_handle == value_handle)
                return characteristic;
        }
    }

    return NULL;
}

static gatt_descriptor_t* find_desc_by_attr_handle(gatt_client_t* client, uint16_t attr_handle)
{
    bt_list_node_t* node;
    gatt_service_t* service;
    size_t i, j;
    gatt_characteristic_t* characteristic;
    gatt_descriptor_t* descriptor;

    if (!client || !client->services_db)
        return NULL;

    for (node = bt_list_head(client->services_db); node; node = bt_list_next(client->services_db, node)) {
        service = (gatt_service_t*)bt_list_node(node);

        if (!service || !service->characteristics)
            continue;

        for (i = 0; i < service->characteristic_count; ++i) {
            characteristic = &service->characteristics[i];

            for (j = 0; j < characteristic->descriptor_count; ++j) {

                descriptor = &characteristic->descriptors[j];

                if (descriptor->attr_handle == attr_handle)
                    return descriptor;
            }
        }
    }

    return NULL;
}

static gatt_service_t* build_service_from_attr_list(bt_list_t* attr_list)
{
    gatt_service_t* service;
    gatt_characteristic_t* current_char;
    bt_list_node_t* node;
    gatt_attr_desc_t* attr;

    gatt_characteristic_t* new_chars;
    gatt_descriptor_t* new_descs;
    gatt_descriptor_t* desc;
    gatt_include_service_t* new_includes;
    gatt_include_service_t* inc;

    if (!attr_list)
        return NULL;

    service = (gatt_service_t*)zalloc(sizeof(gatt_service_t));

    if (!service)
        return NULL;

    current_char = NULL;

    for (node = bt_list_head(attr_list); node; node = bt_list_next(attr_list, node)) {
        attr = (gatt_attr_desc_t*)bt_list_node(node);

        if (!attr)
            continue;

        switch (attr->type) {
        case GATT_PRIMARY_SERVICE:
        case GATT_SECONDARY_SERVICE:
            service->uuid = attr->uuid;
            service->attr_handle = attr->handle;
            service->is_primary = (attr->type == GATT_PRIMARY_SERVICE);
            break;

        case GATT_CHARACTERISTIC:
            service->characteristic_count++;
            new_chars = (gatt_characteristic_t*)realloc(service->characteristics,
                service->characteristic_count * sizeof(gatt_characteristic_t));

            if (!new_chars)
                goto error;

            service->characteristics = new_chars;
            current_char = &service->characteristics[service->characteristic_count - 1];
            memset(current_char, 0, sizeof(gatt_characteristic_t));
            current_char->service_uuid = service->uuid;
            current_char->uuid = attr->uuid;
            current_char->properties = attr->properties;
            current_char->value_handle = attr->handle;
            break;

        case GATT_DESCRIPTOR:
            if (current_char) {
                current_char->descriptor_count++;
                new_descs = (gatt_descriptor_t*)realloc(current_char->descriptors,
                    current_char->descriptor_count * sizeof(gatt_descriptor_t));

                if (!new_descs)
                    goto error;

                current_char->descriptors = new_descs;
                desc = &current_char->descriptors[current_char->descriptor_count - 1];
                memset(desc, 0, sizeof(gatt_descriptor_t));
                desc->service_uuid = service->uuid;
                desc->characteristic_uuid = current_char->uuid;
                desc->uuid = attr->uuid;
                desc->attr_handle = attr->handle;
            }
            break;

        case GATT_INCLUDED_SERVICE:
            service->included_service_count++;
            new_includes = (gatt_include_service_t*)realloc(service->included_services,
                service->included_service_count * sizeof(gatt_include_service_t));

            if (!new_includes)
                goto error;

            service->included_services = new_includes;
            inc = &service->included_services[service->included_service_count - 1];
            memset(inc, 0, sizeof(gatt_include_service_t));
            inc->attr_handle = attr->handle;
            inc->start_handle = 0;
            inc->end_handle = 0;
            inc->included_service_uuid = service->uuid;
            break;

        default:
            break;
        }
    }

    return service;

error:
    gatt_feature_free_service(service);
    return NULL;
}

static void feature_on_connected(void* conn_handle, bt_address_t* addr)
{
    gatt_client_t* client;

    client = find_client_by_conn((gattc_handle_t)conn_handle);

    if (!client)
        return;

    client->connected = true;
    memcpy(&client->addr, addr, sizeof(bt_address_t));

    BT_FEATURE_LOG("connected: conn=%p", conn_handle);

    BT_GATTC_FEATURE_INVOKE_CB(client, on_connected, client->ins, BT_STATUS_SUCCESS, client->conn);
}

static void feature_on_disconnected(void* conn_handle, bt_address_t* addr)
{
    gatt_client_t* client;

    (void)addr;

    client = find_client_by_conn((gattc_handle_t)conn_handle);

    if (!client)
        return;

    client->connected = false;
    BT_FEATURE_LOG("disconnected: conn=%p", conn_handle);

    client->services_discovering = false;
    discovery_database_clear(client);

    BT_GATTC_FEATURE_INVOKE_CB(client, on_disconnected, client->ins, BT_STATUS_SUCCESS,
        client->conn);
}

static void feature_get_attribute_cb(bt_instance_t* ins, bt_status_t status,
    gatt_attr_desc_t* attr_desc, void* userdata)
{
    get_attr_user_data_t* user_data;
    gatt_client_t* client;
    gatt_attr_desc_t* attr_node;
    gatt_service_t* service;

    (void)ins;

    user_data = (get_attr_user_data_t*)userdata;

    if (!user_data) {
        BT_FEATURE_LOG("no user data");
        return;
    }

    client = user_data->client;

    if (!client) {
        BT_FEATURE_LOG("no client");
        free(user_data);
        return;
    }

    if (!client->services_discovering) {
        BT_FEATURE_LOG("not in discovering state");
        free(user_data);
        return;
    }

    if (user_data->services_discover_ends) {
        BT_FEATURE_LOG("last service");

        free(user_data);

        client->services_discovering = false;
        BT_GATTC_FEATURE_INVOKE_CB(client, on_discovered,
            client->ins, GATT_STATUS_SUCCESS,
            client->conn, NULL);

        return;
    }

    if (status != BT_STATUS_SUCCESS || !attr_desc) {
        BT_FEATURE_LOG("status=%d", status);
        free(user_data);
        return;
    }

    attr_node = (gatt_attr_desc_t*)malloc(sizeof(gatt_attr_desc_t));
    memcpy(attr_node, attr_desc, sizeof(gatt_attr_desc_t));

    bt_list_add_tail(client->temp_attrs, attr_node);

    if (user_data->service_range_ends) {
        service = build_service_from_attr_list(client->temp_attrs);

        bt_list_clear(client->temp_attrs);

        if (service) {
            bt_list_add_tail(client->services_db, service);
        }

        BT_GATTC_FEATURE_INVOKE_CB(client, on_discovered, client->ins, BT_STATUS_SUCCESS,
            client->conn, service);
    }

    free(user_data);
}

static void feature_on_discovered(void* conn_handle, gatt_status_t status,
    bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle)
{
    gatt_client_t* client;
    uint16_t handle;
    get_attr_user_data_t* cb_data;
    bt_status_t ret;

    client = find_client_by_conn((gattc_handle_t)conn_handle);
    if (!client)
        return;

    if (status != GATT_STATUS_SUCCESS) {
        client->services_discovering = false;
        BT_GATTC_FEATURE_INVOKE_CB(client, on_discovered, client->ins, status,
            client->conn, NULL);
        return;
    }

    if (!client->services_discovering) {
        BT_FEATURE_LOG("not in discovering state");
        return;
    }

    if (!uuid || !uuid->type) {
        BT_FEATURE_LOG("get_service done");

        cb_data = (get_attr_user_data_t*)zalloc(sizeof(*cb_data));
        if (cb_data) {
            cb_data->client = client;
            cb_data->services_discover_ends = true;

            ret = bt_gattc_get_attribute_by_handle_async(client->conn, BT_GATTC_FEATURE_DISCOVERY_DONE,
                feature_get_attribute_cb, cb_data);

            if (ret == BT_STATUS_SUCCESS) {
                return;
            }

            free(cb_data);
        } else {
            BT_FEATURE_LOG("malloc fail");
        }
        client->services_discovering = false;
        BT_GATTC_FEATURE_INVOKE_CB(client, on_discovered,
            client->ins, GATT_STATUS_FAILURE,
            client->conn, NULL);

        return;
    }

    for (handle = start_handle; handle <= end_handle; ++handle) {
        cb_data = (get_attr_user_data_t*)zalloc(sizeof(get_attr_user_data_t));

        if (!cb_data) {
            BT_FEATURE_LOG("malloc fail");
            continue;
        }

        cb_data->client = client;
        cb_data->service_range_ends = (handle == end_handle) ? true : false;

        ret = bt_gattc_get_attribute_by_handle_async(client->conn, handle, feature_get_attribute_cb, cb_data);

        if (ret != BT_STATUS_SUCCESS) {
            free(cb_data);
        }
    }
}

static void feature_on_read(void* conn_handle, gatt_status_t status,
    uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gatt_client_t* client;
    gatt_descriptor_t* descriptor;
    gatt_characteristic_t* characteristic;

    client = find_client_by_conn((gattc_handle_t)conn_handle);

    if (!client)
        return;

    characteristic = find_char_by_value_handle(client, attr_handle);

    if (characteristic) {
        characteristic->value = value;
        characteristic->value_len = (size_t)length;

        BT_GATTC_FEATURE_INVOKE_CB(client, on_read_char,
            client->ins, status, client->conn, characteristic);

        return;
    }

    descriptor = find_desc_by_attr_handle(client, attr_handle);

    if (descriptor) {
        descriptor->value = value;
        descriptor->value_len = (size_t)length;

        BT_GATTC_FEATURE_INVOKE_CB(client, on_read_desc,
            client->ins, status, client->conn, descriptor);

        return;
    }

    BT_FEATURE_LOG("%s: unknown attr handle 0x%04x", __func__, attr_handle);
}

static void feature_on_written(void* conn_handle, gatt_status_t status, uint16_t attr_handle)
{
    gatt_client_t* client = find_client_by_conn((gattc_handle_t)conn_handle);

    if (!client)
        return;

    gatt_characteristic_t* characteristic = find_char_by_value_handle(client, attr_handle);

    if (characteristic) {
        BT_GATTC_FEATURE_INVOKE_CB(client, on_write_char,
            client->ins, status, client->conn);
        return;
    }

    gatt_descriptor_t* descriptor = find_desc_by_attr_handle(client, attr_handle);

    if (descriptor) {
        BT_GATTC_FEATURE_INVOKE_CB(client, on_write_desc,
            client->ins, status, client->conn);
        return;
    }

    BT_FEATURE_LOG("%s: unknown attr handle 0x%04x", __func__, attr_handle);
}

static void feature_on_subscribed(void* conn_handle, gatt_status_t status,
    uint16_t attr_handle, bool enable)
{
    gatt_client_t* client = find_client_by_conn((gattc_handle_t)conn_handle);

    if (!client)
        return;

    BT_GATTC_FEATURE_INVOKE_CB(client, on_subscribed, client->ins, status, client->conn,
        enable);
}

static void feature_on_notified(void* conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gatt_client_t* client = find_client_by_conn((gattc_handle_t)conn_handle);
    if (!client)
        return;

    gatt_characteristic_t* characteristic = find_char_by_value_handle(client, attr_handle);

    if (!characteristic)
        return;

    characteristic->value = value;
    characteristic->value_len = (size_t)length;

    BT_GATTC_FEATURE_INVOKE_CB(client, on_notified,
        client->ins, client->conn, characteristic);
}

static void feature_on_mtu_updated(void* conn_handle, gatt_status_t status, uint32_t mtu)
{
    gatt_client_t* client = find_client_by_conn((gattc_handle_t)conn_handle);
    if (!client)
        return;

    BT_GATTC_FEATURE_INVOKE_CB(client, on_mtu_updated,
        client->conn, status, mtu);
}

static gattc_callbacks_t s_feature_gattc_cbs = {
    sizeof(s_feature_gattc_cbs),
    feature_on_connected,
    feature_on_disconnected,
    feature_on_discovered,
    feature_on_read,
    feature_on_written,
    feature_on_subscribed,
    feature_on_notified,
    feature_on_mtu_updated,
    NULL,
    NULL,
    NULL,
    NULL,
};

static void create_client_cb(bt_instance_t* ins, bt_status_t status, gattc_handle_t* phandle,
    void* userdata)
{
    gatt_client_t* client;
    bt_gattc_feature_create_client_cb_t user_cb;
    void* user_ud;
    gattc_handle_t conn_handle;

    client = (gatt_client_t*)userdata;

    if (!client)
        return;

    user_cb = client->create_client_ctx.create_client_cb;
    user_ud = client->create_client_ctx.create_userdata;
    conn_handle = client->conn;

    if (phandle && client->conn != *phandle) {
        assert(0);
    }

    if (status != BT_STATUS_SUCCESS && g_gatt_client_list) {
        bt_list_remove(g_gatt_client_list, client);
        conn_handle = NULL;
    } else {
        client->connected = true;
    }

    if (user_cb) {
        user_cb(ins, status, conn_handle, user_ud);
    }
}

bt_status_t bt_gattc_feature_create_client_async(bt_instance_t* ins, bt_address_t* addr,
    bt_gattc_feature_create_client_cb_t cb, bt_gattc_feature_callbacks_t* callbacks,
    void* userdata)
{
    bt_status_t status;

    if (!ins || !addr || !cb || !callbacks || callbacks->size > sizeof(bt_gattc_feature_callbacks_t)) {
        return BT_STATUS_PARM_INVALID;
    }

    if (!g_gatt_client_list) {
        g_gatt_client_list = bt_list_new(free_client_instance);
        if (!g_gatt_client_list)
            return BT_STATUS_NOMEM;
    }

    if (bt_list_length(g_gatt_client_list) >= CONFIG_BLUETOOTH_GATTC_MAX_CONNECTIONS)
        return BT_STATUS_NOMEM;

    gatt_client_t* client = (gatt_client_t*)zalloc(sizeof(gatt_client_t));
    if (!client) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    memcpy(&client->addr, addr, sizeof(bt_address_t));
    client->ins = ins;

    memcpy(&client->callbacks, callbacks, callbacks->size);

    client->create_client_ctx.create_client_cb = cb;
    client->create_client_ctx.create_userdata = userdata;

    if (!discovery_database_create(client)) {
        free(client);
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    bt_list_add_tail(g_gatt_client_list, client);

    status = bt_gattc_create_connect_async(
        ins, &client->conn, &s_feature_gattc_cbs, create_client_cb, client);

    if (status != BT_STATUS_SUCCESS) {
        bt_list_remove(g_gatt_client_list, client);
        goto fail;
    }

    return BT_STATUS_SUCCESS;

fail:
    if (!bt_list_length(g_gatt_client_list)) {
        bt_list_free(g_gatt_client_list);
        g_gatt_client_list = NULL;
    }

    return status;
}

static void delete_client_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    gatt_client_t* client;
    bt_gattc_feature_delete_client_cb_t user_cb;
    void* user_ud;
    gattc_handle_t conn_handle;

    client = (gatt_client_t*)userdata;

    if (!client)
        return;

    user_cb = client->delete_client_ctx.delete_client_cb;
    user_ud = client->delete_client_ctx.delete_userdata;
    conn_handle = client->conn;

    if (status == BT_STATUS_SUCCESS && g_gatt_client_list) {
        bt_list_remove(g_gatt_client_list, client);

        if (!bt_list_length(g_gatt_client_list)) {
            bt_list_free(g_gatt_client_list);
            g_gatt_client_list = NULL;
        }
    }

    if (user_cb)
        user_cb(ins, status, conn_handle, user_ud);
}

bt_status_t bt_gattc_feature_delete_client_async(bt_instance_t* ins, bt_address_t* addr,
    bt_gattc_feature_delete_client_cb_t cb, void* userdata)
{
    gatt_client_t* client;

    if (!ins || !addr || !cb)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_addr(addr);
    if (!client)
        return BT_STATUS_PARM_INVALID;

    client->delete_client_ctx.delete_client_cb = cb;
    client->delete_client_ctx.delete_userdata = userdata;

    return bt_gattc_delete_connect_async(client->conn, delete_client_cb, client);
}

bt_status_t bt_gattc_feature_connect_async(gattc_handle_t conn_handle, bt_address_t* addr, ble_addr_type_t addr_type,
    bt_status_cb_t cb, void* userdata)
{
    gatt_client_t* client;

    if (!conn_handle || !addr)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_conn(conn_handle);
    if (!client)
        return BT_STATUS_NOT_READY;

    return bt_gattc_connect_async(conn_handle, addr, addr_type, cb, userdata);
}

bt_status_t bt_gattc_feature_disconnect_async(gattc_handle_t conn_handle, bt_status_cb_t cb, void* userdata)
{
    gatt_client_t* client;

    if (!conn_handle)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_conn(conn_handle);
    if (!client)
        return BT_STATUS_NOT_READY;

    return bt_gattc_disconnect_async(conn_handle, cb, userdata);
}

bt_status_t bt_gattc_feature_get_service_async(gattc_handle_t conn_handle, bt_status_cb_t cb, void* userdata)
{
    gatt_client_t* client;
    bt_status_t status;

    if (!conn_handle || !cb)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_conn(conn_handle);
    if (!client)
        return BT_STATUS_NOT_READY;

    if (client->services_discovering) {
        return BT_STATUS_BUSY;
    }

    discovery_database_clear(client);

    status = bt_gattc_discover_service_async(client->conn, /*filter_uuid*/ NULL, cb, userdata);

    if (status == BT_STATUS_SUCCESS) {
        client->services_discovering = true;
        return BT_STATUS_SUCCESS;
    }

    return status;
}

bt_status_t bt_gattc_feature_read_characteristic_value_async(gattc_handle_t conn_handle,
    const bt_uuid_t* service_uuid, const bt_uuid_t* characteristic_uuid,
    bt_status_cb_t cb, void* userdata)
{
    gatt_client_t* client;
    gatt_service_t* service;
    gatt_characteristic_t* characteristic;

    if (!conn_handle || !service_uuid || !characteristic_uuid || !cb)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_conn(conn_handle);
    if (!client)
        return BT_STATUS_NOT_READY;

    service = find_service_by_uuid(client, service_uuid);
    if (!service)
        return BT_STATUS_FAIL;

    characteristic = find_char_by_uuid(service, characteristic_uuid);
    if (!characteristic)
        return BT_STATUS_FAIL;

    return bt_gattc_read_async(conn_handle, characteristic->value_handle, cb, userdata);
}

bt_status_t bt_gattc_feature_read_descriptor_value_async(gattc_handle_t conn_handle,
    const bt_uuid_t* service_uuid, const bt_uuid_t* characteristic_uuid,
    const bt_uuid_t* descriptor_uuid,
    bt_status_cb_t cb, void* userdata)
{
    gatt_client_t* client;
    gatt_service_t* service;
    gatt_characteristic_t* characteristic;
    gatt_descriptor_t* descriptor;

    if (!conn_handle || !service_uuid || !characteristic_uuid || !descriptor_uuid || !cb)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_conn(conn_handle);
    if (!client)
        return BT_STATUS_NOT_READY;

    service = find_service_by_uuid(client, service_uuid);
    if (!service)
        return BT_STATUS_FAIL;

    characteristic = find_char_by_uuid(service, characteristic_uuid);
    if (!characteristic)
        return BT_STATUS_FAIL;

    descriptor = find_desc_by_uuid(characteristic, descriptor_uuid);
    if (!descriptor)
        return BT_STATUS_FAIL;

    return bt_gattc_read_async(conn_handle, descriptor->attr_handle, cb, userdata);
}

bt_status_t bt_gattc_feature_write_characteristic_value_async(gattc_handle_t conn_handle,
    const gatt_characteristic_t* characteristic, bt_status_cb_t cb, void* userdata)
{
    gatt_client_t* client;
    gatt_characteristic_t* db_characteristic;
    gatt_service_t* service;

    if (!conn_handle || !characteristic || !cb)
        return BT_STATUS_PARM_INVALID;

    if (!characteristic->value)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_conn(conn_handle);
    if (!client)
        return BT_STATUS_NOT_READY;

    service = find_service_by_uuid(client, &characteristic->service_uuid);
    if (!service)
        return BT_STATUS_FAIL;

    /* Because the application does not maintain attribute handle */
    db_characteristic = find_char_by_uuid(service, &characteristic->uuid);
    if (!db_characteristic)
        return BT_STATUS_FAIL;

    return bt_gattc_write_async(conn_handle, db_characteristic->value_handle,
        characteristic->value, (uint16_t)characteristic->value_len,
        cb, userdata);
}

bt_status_t bt_gattc_feature_write_descriptor_value_async(gattc_handle_t conn_handle,
    const gatt_descriptor_t* descriptor, bt_status_cb_t cb, void* userdata)
{
    gatt_client_t* client;
    gatt_service_t* service;
    gatt_characteristic_t* characteristic;
    gatt_descriptor_t* db_descriptor;

    if (!conn_handle || !descriptor || !cb)
        return BT_STATUS_PARM_INVALID;

    if (!descriptor->value)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_conn(conn_handle);
    if (!client)
        return BT_STATUS_NOT_READY;

    service = find_service_by_uuid(client, &descriptor->service_uuid);
    if (!service)
        return BT_STATUS_FAIL;

    characteristic = find_char_by_uuid(service, &descriptor->characteristic_uuid);
    if (!characteristic)
        return BT_STATUS_FAIL;

    db_descriptor = find_desc_by_uuid(characteristic, &descriptor->uuid);
    if (!db_descriptor)
        return BT_STATUS_FAIL;

    return bt_gattc_write_async(conn_handle, db_descriptor->attr_handle,
        descriptor->value, (uint16_t)descriptor->value_len,
        cb, userdata);
}

bt_status_t bt_gattc_feature_exchange_mtu_async(gattc_handle_t conn_handle, uint32_t mtu,
    bt_status_cb_t cb, void* userdata)
{
    return bt_gattc_exchange_mtu_async(conn_handle, mtu, cb, userdata);
}

bt_status_t bt_gattc_feature_set_notify_characteristic_changed_async(gattc_handle_t conn_handle,
    const gatt_characteristic_t* characteristic, bool enable,
    bt_status_cb_t cb, void* userdata)
{
    gatt_client_t* client;
    uint16_t ccc_value;
    gatt_service_t* service;
    gatt_characteristic_t* db_characteristic;

    if (!conn_handle || !characteristic || !cb)
        return BT_STATUS_PARM_INVALID;

    client = find_client_by_conn(conn_handle);
    if (!client)
        return BT_STATUS_NOT_READY;

    service = find_service_by_uuid(client, &characteristic->service_uuid);
    if (!service)
        return BT_STATUS_FAIL;

    db_characteristic = find_char_by_uuid(service, &characteristic->uuid);
    if (!db_characteristic)
        return BT_STATUS_FAIL;

    if (!enable) {
        return bt_gattc_unsubscribe_async(conn_handle, db_characteristic->value_handle,
            cb, userdata);
    }

    if (db_characteristic->properties & GATT_PROP_NOTIFY) {
        ccc_value = 0x0001; // Notify is preferred
    } else if (db_characteristic->properties & GATT_PROP_INDICATE) {
        ccc_value = 0x0002; // Indicate
    } else {
        return BT_STATUS_PARM_INVALID;
    }

    return bt_gattc_subscribe_async(conn_handle, db_characteristic->value_handle,
        ccc_value, cb, userdata);
}
