/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#define LOG_TAG "teleif"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(CONFIG_LIB_DBUS_RPMSG_SERVER_CPUNAME) || defined(CONFIG_OFONO)

#include <dbus/dbus.h>

#include "bluetooth.h"
#include "bt_list.h"
#include "gdbus.h"
#include "telephony_interface.h"
#include "utils/log.h"
#define OFONO_SERVICE "org.ofono"
#define OFONO_MANAGER_PATH "/"
#define OFONO_MANAGER_INTERFACE OFONO_SERVICE ".Manager"
#define OFONO_MODEM_INTERFACE OFONO_SERVICE ".Modem"
#define OFONO_VOICECALL_MANAGER_INTERFACE OFONO_SERVICE ".VoiceCallManager"
#define OFONO_VOICECALL_INTERFACE OFONO_SERVICE ".VoiceCall"
#define OFONO_NETWORK_REGISTRATION_INTERFACE OFONO_SERVICE ".NetworkRegistration"
#define OFONO_NETWORK_OPERATOR_INTERFACE OFONO_SERVICE ".NetworkOperator"
#define OFONO_CALL_BARRING_INTERFACE OFONO_SERVICE ".CallBarring"
#define OFONO_CALL_FORWARDING_INTERFACE OFONO_SERVICE ".CallForwarding"
#define OFONO_CALL_SETTINGS_INTERFACE OFONO_SERVICE ".CallSettings"
#define OFONO_MESSAGE_MANAGER_INTERFACE OFONO_SERVICE ".MessageManager"

typedef struct tele_client_ {
    DBusConnection* dbus_sys;
    DBusConnection* dbus_session;
    GDBusClient* dbus_client;
    bt_list_t* modems;
    tele_callbacks_t* cbs;
    bool is_ready;
} tele_client_t;

typedef struct tele_modem_ {
    tele_client_t* client;
    GDBusProxy* proxy;
    GDBusProxy* network_operator;
    GDBusProxy* network_registration;
    GDBusProxy* voicecall_managers;
    bt_list_t* voicecalls;
    bool online;
} tele_modem_t;

typedef bool (*property_parser_func_t)(void* user_data, char* key,
    DBusMessageIter* val, uint8_t flag);

static bool tele_support_interface(const char* interface)
{
    if ((strcmp(interface, OFONO_VOICECALL_INTERFACE) == 0)
        || (strcmp(interface, OFONO_VOICECALL_MANAGER_INTERFACE) == 0)
        || (strcmp(interface, OFONO_NETWORK_REGISTRATION_INTERFACE) == 0)
        || (strcmp(interface, OFONO_NETWORK_OPERATOR_INTERFACE) == 0)
        || (strcmp(interface, OFONO_MODEM_INTERFACE) == 0)) {
        return true;
    }

    return false;
}

static gboolean proxy_filter(GDBusClient* client, const char* path,
    const char* interface)
{
    /* only support interface isn't filter out and will create proxy */
    return tele_support_interface(interface) ? FALSE : TRUE;
}

static gboolean object_filter(GDBusProxy* proxy)
{
    const char* interface = g_dbus_proxy_get_interface(proxy);
    if (interface == NULL)
        return TRUE;

    /* only follow interface will get interface's properties.
     * if support interface no need get prop, modify here.
     */
    if (tele_support_interface(interface))
        return FALSE;

    BT_LOGE("not get proper for unsupport interface:%s", interface);
    return TRUE;
}

static bool property_parser(DBusMessageIter* iter, property_parser_func_t func,
    void* user_data, uint8_t flag)
{
    DBusMessageIter value;
    char* key;

    /* get call property key from iter */
    if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_STRING) {
        BT_LOGE("%s, error key is not string type", __func__);
        return false;
    }

    dbus_message_iter_get_basic(iter, &key);

    /* get call property value iter */
    dbus_message_iter_next(iter);
    if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_VARIANT) {
        BT_LOGE("%s, error value is not variant type", __func__);
        return false;
    }
    dbus_message_iter_recurse(iter, &value);

    /* get property value from valueiter by user */
    if (func)
        return func(user_data, key, &value, flag);

    return false;
}

static bool properties_parser(DBusMessageIter* iter,
    property_parser_func_t func, void* user_data,
    uint8_t flag)
{
    while (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry;

        /* get key entry from message iter */
        dbus_message_iter_recurse(iter, &entry);

        /* parser property key-value */
        if (!property_parser(&entry, func, user_data, flag))
            return false;

        /* get next key entry */
        dbus_message_iter_next(iter);
    }

    return true;
}

static int disconnect_reason_to_value(const char* reason)
{
    if (!strcmp(reason, "local"))
        return CALL_DISCONNECT_REASON_LOCAL_HANGUP;
    else if (!strcmp(reason, "remote"))
        return CALL_DISCONNECT_REASON_REMOTE_HANGUP;

    return CALL_DISCONNECT_REASON_UNKNOWN;
}

static uint8_t call_string_to_state(char* call_state_str)
{
    uint8_t state;
    if (!strcmp(call_state_str, "active"))
        state = CALL_STATUS_ACTIVE;
    else if (!strcmp(call_state_str, "held"))
        state = CALL_STATUS_HELD;
    else if (!strcmp(call_state_str, "dialing"))
        state = CALL_STATUS_DIALING;
    else if (!strcmp(call_state_str, "alerting"))
        state = CALL_STATUS_ALERTING;
    else if (!strcmp(call_state_str, "incoming"))
        state = CALL_STATUS_INCOMING;
    else if (!strcmp(call_state_str, "waiting"))
        state = CALL_STATUS_WAITING;
    else
        state = CALL_STATUS_DISCONNECTED;

    return state;
}

static int registration_status_to_value(const char* str)
{
    if (!strcmp(str, "unregistered"))
        return NETWORK_REG_STATUS_NOT_REGISTERED;
    else if (!strcmp(str, "registered"))
        return NETWORK_REG_STATUS_REGISTERED;
    else if (!strcmp(str, "searching"))
        return NETWORK_REG_STATUS_SEARCHING;
    else if (!strcmp(str, "denied"))
        return NETWORK_REG_STATUS_DENIED;
    else if (!strcmp(str, "unknown"))
        return NETWORK_REG_STATUS_UNKNOWN;
    else if (!strcmp(str, "roaming"))
        return NETWORK_REG_STATUS_ROAMING;
    else if (!strcmp(str, "registered"))
        return NETWORK_REG_STATUS_REGISTERED;

    return NETWORK_REG_STATUS_UNKNOWN;
}

static int operator_status_to_value(const char* str)
{
    if (!strcmp(str, "available"))
        return OPERATOR_STATUS_AVAILABLE;
    else if (!strcmp(str, "current"))
        return OPERATOR_STATUS_CURRENT;
    else if (!strcmp(str, "forbidden"))
        return OPERATOR_STATUS_FORBIDDEN;

    return OPERATOR_STATUS_UNKNOWN;
}

static tele_call_t* tele_voicecall_new(tele_modem_t* modem, GDBusProxy* proxy)
{
    tele_call_t* call = malloc(sizeof(tele_call_t));

    memset(call, 0, sizeof(tele_call_t));
    call->client = modem->client;
    call->proxy = proxy;

    return call;
}

static void tele_voicecall_delete(tele_call_t* call)
{
    free(call);
}

static tele_call_t* voicecall_proxy_added(tele_modem_t* modem, GDBusProxy* proxy)
{
    tele_call_t* call = tele_voicecall_new(modem, proxy);
    if (call)
        bt_list_add_tail(modem->voicecalls, call);

    return call;
}

static void voicecall_proxy_remove(tele_modem_t* modem, GDBusProxy* proxy)
{
    tele_call_t* call;
    bt_list_node_t* node;
    bt_list_t* list = modem->voicecalls;

    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        if (call->proxy == proxy) {
            bt_list_remove_node(list, node);
            tele_voicecall_delete(call);
            break;
        }
    }
}

static int tele_call_get_call_info(tele_client_t* tele, tele_call_t* call)
{
    if (!call)
        return TELE_INV_PARAM;

    DBusMessageIter iter;
    GDBusProxy* proxy = call->proxy;
    void* p_basic = NULL;
    dbus_bool_t ret;

    if (g_dbus_proxy_get_property(proxy, "Multiparty", &iter)) {
        dbus_message_iter_get_basic(&iter, &ret);
        call->is_multiparty = ret;
    }
    if (g_dbus_proxy_get_property(proxy, "RemoteMultiparty", &iter)) {
        dbus_message_iter_get_basic(&iter, &ret);
        call->is_remote_multiparty = ret;
    }
    if (g_dbus_proxy_get_property(proxy, "State", &iter)) {
        dbus_message_iter_get_basic(&iter, &p_basic);
        call->call_state = call_string_to_state((char*)p_basic);
    }
    if (g_dbus_proxy_get_property(proxy, "StartTime", &iter)) {
        dbus_message_iter_get_basic(&iter, &p_basic);
        snprintf(call->start_time, 128, "%s", (char*)p_basic);
    }
    if (g_dbus_proxy_get_property(proxy, "LineIdentification", &iter)) {
        dbus_message_iter_get_basic(&iter, &p_basic);
        snprintf(call->line_identification, TELE_MAX_PHONE_NUMBER_LENGTH, "%s",
            (char*)p_basic);
    }
    if (g_dbus_proxy_get_property(proxy, "IncomingLine", &iter)) {
        dbus_message_iter_get_basic(&iter, &p_basic);
        snprintf(call->incoming_line, TELE_MAX_PHONE_NUMBER_LENGTH, "%s",
            (char*)p_basic);
    }
    if (g_dbus_proxy_get_property(proxy, "Name", &iter)) {
        dbus_message_iter_get_basic(&iter, &p_basic);
        snprintf(call->name, TELE_MAX_CALLER_NAME_LENGTH, "%s", (char*)p_basic);
    }
    if (g_dbus_proxy_get_property(proxy, "RemoteHeld", &iter)) {
        dbus_message_iter_get_basic(&iter, &ret);
        call->is_remote_held = ret;
    }
    if (g_dbus_proxy_get_property(proxy, "Emergency", &iter)) {
        dbus_message_iter_get_basic(&iter, &ret);
        call->is_emergency = ret;
    }

    return TELE_SUCCESS;
}

/*
 * modem
 */

static tele_modem_t* tele_modem_new(tele_client_t* tele, GDBusProxy* proxy)
{
    tele_modem_t* modem = malloc(sizeof(tele_modem_t));
    if (!modem)
        return NULL;

    memset(modem, 0, sizeof(tele_modem_t));
    modem->client = tele;
    modem->proxy = proxy;
    modem->online = false;
    modem->voicecalls = bt_list_new(NULL);

    return modem;
}

static void tele_modem_delete(tele_modem_t* modem)
{
    bt_list_free(modem->voicecalls);
    free(modem);
}

static tele_modem_t* modem_proxy_added(tele_client_t* tele, GDBusProxy* proxy)
{
    tele_modem_t* modem = tele_modem_new(tele, proxy);
    if (!modem)
        return NULL;

    bt_list_add_tail(tele->modems, modem);

    return modem;
}

static void modem_proxy_remove(tele_client_t* tele, GDBusProxy* proxy)
{
    tele_modem_t* modem;
    bt_list_node_t* node;
    bt_list_t* list = tele->modems;

    for (node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        modem = bt_list_node(node);
        if (modem->proxy == proxy) {
            bt_list_remove_node(list, node);
            tele_modem_delete(modem);
            break;
        }
    }
}

static tele_modem_t* modem_find_by_path(tele_client_t* tele, const char* path)
{
    tele_modem_t* modem;
    bt_list_node_t* node;
    bt_list_t* modems = tele->modems;

    for (node = bt_list_head(modems); node != NULL;
         node = bt_list_next(modems, node)) {
        modem = bt_list_node(node);
        const char* modem_path = g_dbus_proxy_get_path(modem->proxy);
        if (strncmp(modem_path, path, strlen(modem_path)) == 0)
            return modem;
    }

    return NULL;
}

static void ofono_connect_handler(DBusConnection* connection, void* user_data)
{
    BT_LOGD("org.ofono appeared");
}

static void ofono_disconnect_handler(DBusConnection* connection,
    void* user_data)
{
    tele_client_t* tele = user_data;
    BT_LOGD("org.ofono disappeared");

    tele->is_ready = false;

    if (tele->cbs && tele->is_ready)
        tele->cbs->connection_state_cb(tele, false);
}

static tele_call_t* find_voicecall(tele_modem_t* modem, void* proxy)
{
    tele_call_t* call;
    bt_list_node_t* node;
    bt_list_t* list = modem->voicecalls;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        call = bt_list_node(node);
        if (call->proxy == proxy)
            return call;
    }

    return NULL;
}

static bool voicecall_property_parser(void* user_data, char* key,
    DBusMessageIter* val, uint8_t flag)
{
    void* p_basic;
    tele_call_t* call = user_data;

    /* get call property value from valueiter */
    dbus_message_iter_get_basic(val, &p_basic);
    if (!strcmp(key, "Multiparty"))
        call->is_multiparty = PTR2INT(uint64_t) p_basic;
    else if (!strcmp(key, "RemoteMultiparty"))
        call->is_remote_multiparty = PTR2INT(uint64_t) p_basic;
    else if (!strcmp(key, "State")) {
        call->call_state = call_string_to_state((char*)p_basic);
    } else if (!strcmp(key, "StartTime"))
        snprintf(call->start_time, 128, "%s", (char*)p_basic);
    else if (!strcmp(key, "LineIdentification"))
        snprintf(call->line_identification, TELE_MAX_PHONE_NUMBER_LENGTH, "%s",
            (char*)p_basic);
    else if (!strcmp(key, "IncomingLine"))
        snprintf(call->incoming_line, TELE_MAX_PHONE_NUMBER_LENGTH, "%s",
            (char*)p_basic);
    else if (!strcmp(key, "Name"))
        snprintf(call->name, TELE_MAX_CALLER_NAME_LENGTH, "%s", (char*)p_basic);
    else if (!strcmp(key, "RemoteHeld"))
        call->is_remote_held = PTR2INT(uint64_t) p_basic;
    else if (!strcmp(key, "Emergency"))
        call->is_emergency = PTR2INT(uint64_t) p_basic;
    else {
        BT_LOGE("%s, unknown property key:%s", __func__, key);
        return false;
    }

    return true;
}

static void voicecall_manager_signal_process(tele_client_t* tele,
    DBusMessage* message,
    const char* interface,
    const char* signal)
{
    DBusMessageIter iter;
    const char* path;
    tele_call_t* call;

    if (!dbus_message_iter_init(message, &iter)) {
        BT_LOGE("%s, message has no arguments", __func__);
        return;
    }

    /* get call patch */
    dbus_message_iter_get_basic(&iter, &path);

    /* get call proxy by path */
    GDBusProxy* proxy = g_dbus_proxy_new(tele->dbus_client, path, OFONO_VOICECALL_INTERFACE);
    if (proxy == NULL) {
        BT_LOGE("%s, %s-%s proxy not found", __func__, path,
            OFONO_VOICECALL_INTERFACE);
        return;
    }

    tele_modem_t* modem = modem_find_by_path(tele, path);
    if (!modem) {
        BT_LOGE("%s, failed to find modem, path:%s", __func__, path);
        return;
    }

    call = find_voicecall(modem, proxy);

    if (!strcmp(signal, "CallAdded")) {
        DBusMessageIter props;
        if (!call)
            call = voicecall_proxy_added(modem, proxy);

        dbus_message_iter_next(&iter);
        dbus_message_iter_recurse(&iter, &props);
        properties_parser(&props, voicecall_property_parser, call, 0);
        /* notify user call added */
        if (tele->cbs && tele->cbs->call_added_cb)
            tele->cbs->call_added_cb(tele, call);
    } else if (!strcmp(signal, "CallRemoved")) {
        /* notify user call removed */
        if (tele->cbs && tele->cbs->call_removed_cb)
            tele->cbs->call_removed_cb(tele, call);
    }
}

static void voicecall_signal_process(tele_client_t* tele,
    DBusMessage* message,
    const char* interface,
    const char* signal)
{
    DBusMessageIter iter;
    void* basic;
    const char* path = dbus_message_get_path(message);
    GDBusProxy* proxy = g_dbus_proxy_new(tele->dbus_client, path, OFONO_VOICECALL_INTERFACE);
    if (!dbus_message_iter_init(message, &iter)) {
        BT_LOGE("%s, message has no arguments", __func__);
        return;
    }

    dbus_message_iter_get_basic(&iter, &basic);
    if (!strcmp(signal, "DisconnectReason")) {
        int reason = disconnect_reason_to_value((const char*)basic);
        tele_modem_t* modem = modem_find_by_path(tele, path);
        if (!modem) {
            BT_LOGE("%s, failed to find modem, path:%s", __func__, path);
            return;
        }

        tele_call_t* call = find_voicecall(modem, proxy);
        if (!call) {
            BT_LOGE("%s, failed to find call", __func__);
            return;
        }

        tele_call_callbacks_t* cbs = call->call_cbs;
        if (cbs)
            cbs->call_disconnect_reason_cb(tele, call, reason);
    }
}

static void ofono_interface_signal_callback(DBusConnection* connection,
    DBusMessage* message,
    void* user_data)
{
    tele_client_t* tele = (tele_client_t*)user_data;
    const char* interface = dbus_message_get_interface(message);
    const char* signal = dbus_message_get_member(message);

    assert(tele->dbus_sys == connection);

    if (!strcmp(interface, OFONO_VOICECALL_MANAGER_INTERFACE) && (!strcmp(signal, "CallAdded") || !strcmp(signal, "CallRemoved")))
        voicecall_manager_signal_process(tele, message, interface, signal);
    else if (!strcmp(interface, OFONO_VOICECALL_INTERFACE))
        voicecall_signal_process(tele, message, interface, signal);
}

static void modem_based_proxy_added(tele_client_t* tele, GDBusProxy* proxy)
{
    tele_modem_t* modem;
    const char* path;
    const char* interface;

    path = g_dbus_proxy_get_path(proxy);
    interface = g_dbus_proxy_get_interface(proxy);
    modem = modem_find_by_path(tele, path);
    if (!modem)
        return;

    if (!strcmp(interface, OFONO_VOICECALL_MANAGER_INTERFACE))
        modem->voicecall_managers = proxy;
    else if (!strcmp(interface, OFONO_VOICECALL_INTERFACE)) {
        tele_call_t* call = voicecall_proxy_added(modem, proxy);
        tele_call_get_call_info(tele, call);
/* notify user call added */
#if 0
        if (tele->cbs && tele->cbs->call_added_cb)
            tele->cbs->call_added_cb(tele, call);
#endif
    } else if (!strcmp(interface, OFONO_NETWORK_REGISTRATION_INTERFACE))
        modem->network_registration = proxy;
    else if (!strcmp(interface, OFONO_NETWORK_OPERATOR_INTERFACE))
        modem->network_operator = proxy;
}

static void modem_based_proxy_removed(tele_client_t* tele, GDBusProxy* proxy)
{
    tele_modem_t* modem;
    const char* path;
    const char* interface;

    path = g_dbus_proxy_get_path(proxy);
    interface = g_dbus_proxy_get_interface(proxy);
    modem = modem_find_by_path(tele, path);
    if (!modem)
        return;

    if (!strcmp(interface, OFONO_VOICECALL_MANAGER_INTERFACE))
        modem->voicecall_managers = NULL;
    else if (!strcmp(interface, OFONO_VOICECALL_INTERFACE)) {
        voicecall_proxy_remove(modem, proxy);
#if 0
        tele_call_t *call = find_voicecall(modem, proxy);
        if (tele->cbs && tele->cbs->call_removed_cb)
            tele->cbs->call_removed_cb(tele, call);
        bt_list_remove(modem->voicecalls, call);
        tele_voicecall_delete(call);
#endif
    } else if (!strcmp(interface, OFONO_NETWORK_REGISTRATION_INTERFACE))
        modem->network_registration = NULL;
    else if (!strcmp(interface, OFONO_NETWORK_OPERATOR_INTERFACE))
        modem->network_operator = NULL;
}

static tele_modem_t* get_modem(tele_client_t* tele, int slot)
{
    bt_list_t* modems = tele->modems;
    bt_list_node_t* node;
    int index = 0;

    if (slot > bt_list_length(modems))
        return NULL;

    for (node = bt_list_head(modems); node != NULL; node = bt_list_next(modems, node)) {
        if (index == slot)
            return (tele_modem_t*)bt_list_node(node);

        index++;
    }

    return NULL;
}

static GDBusProxy* get_voice_callmanager(tele_client_t* tele, int slot)
{
    tele_modem_t* modem = get_modem(tele, slot);

    return modem ? modem->voicecall_managers : NULL;
}

static GDBusProxy* get_network_operator(tele_client_t* tele, int slot)
{
    tele_modem_t* modem = get_modem(tele, slot);

    return modem ? modem->network_operator : NULL;
}

static GDBusProxy* get_network_registration(tele_client_t* tele, int slot)
{
    tele_modem_t* modem = get_modem(tele, slot);

    return modem ? modem->network_registration : NULL;
}

static void ofono_interface_proxy_added(GDBusProxy* proxy, void* user_data)
{
    tele_client_t* tele = (tele_client_t*)user_data;
    const char* interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, OFONO_MODEM_INTERFACE))
        modem_proxy_added(tele, proxy);
    else
        modem_based_proxy_added(tele, proxy);
}

static void ofono_interface_proxy_removed(GDBusProxy* proxy, void* user_data)
{
    tele_client_t* tele = (tele_client_t*)user_data;
    const char* interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, OFONO_MODEM_INTERFACE))
        modem_proxy_remove(tele, proxy);
    else
        modem_based_proxy_removed(tele, proxy);
}

static void ofono_client_ready_cb(GDBusClient* client, void* user_data)
{
    tele_client_t* tele = user_data;

    if (tele->is_ready)
        return;

    tele->is_ready = true;

    if (tele->cbs)
        tele->cbs->connection_state_cb(tele, true);
}

void ofono_property_changed(GDBusProxy* proxy, const char* name,
    DBusMessageIter* iter, void* user_data)
{
    tele_client_t* tele = (tele_client_t*)user_data;
    const char* interface = g_dbus_proxy_get_interface(proxy);
    const char* path = g_dbus_proxy_get_path(proxy);

    if (!strcmp(interface, OFONO_MODEM_INTERFACE)) {
        if (!strcmp(name, "RadioState")) {
            int state;

            dbus_message_iter_get_basic(iter, &state);
            if (tele->cbs && tele->cbs->radio_state_change_cb)
                tele->cbs->radio_state_change_cb(tele, state);
        }
    } else if (!strcmp(interface, OFONO_VOICECALL_MANAGER_INTERFACE)) {
        /* todo: */
    } else if (!strcmp(interface, OFONO_VOICECALL_INTERFACE)) {
        tele_modem_t* modem = modem_find_by_path(tele, path);
        if (!modem) {
            BT_LOGE("%s, failed to find modem, path:%s", __func__, path);
            return;
        }

        tele_call_t* call = find_voicecall(modem, proxy);
        if (!call) {
            BT_LOGE("%s, failed to find call", __func__);
            return;
        }

        tele_call_get_call_info(tele, call);
        tele_call_callbacks_t* cbs = call->call_cbs;
        if (cbs && cbs->call_state_changed_cb)
            cbs->call_state_changed_cb(tele, call, call->call_state);
    } else if (!strcmp(interface, OFONO_NETWORK_REGISTRATION_INTERFACE)) {
        if (!strcmp(name, "Status")) {
            char* str = NULL;
            dbus_message_iter_get_basic(iter, &str);
            int status = registration_status_to_value(str);
            if (tele->cbs && tele->cbs->network_reg_state_changed_cb)
                tele->cbs->network_reg_state_changed_cb(tele, status);
        } else if (!strcmp(name, "Strength")) {
            int strength = -1;
            dbus_message_iter_get_basic(iter, &strength);
            if (tele->cbs && tele->cbs->signal_strength_changed_cb)
                tele->cbs->signal_strength_changed_cb(tele, strength);
        }
    } else if (!strcmp(interface, OFONO_NETWORK_OPERATOR_INTERFACE)) {
        void* basic;

        if (!strcmp(name, "Name")) {
            dbus_message_iter_get_basic(iter, &basic);
            if (tele->cbs && tele->cbs->operator_name_changed_cb)
                tele->cbs->operator_name_changed_cb(tele, name);
        } else if (!strcmp(name, "Status")) {
            dbus_message_iter_get_basic(iter, &basic);
            int status = operator_status_to_value((const char*)basic);
            if (tele->cbs && tele->cbs->operator_status_changed_cb)
                tele->cbs->operator_status_changed_cb(tele, status);
        }
    }
}

/*
 * Disconnect handler for private dbus connections.
 * This is necessary when calling dbus_connection_close(), otherwise
 * the corresponding thread will be removed.
 */
static void system_bus_disconnected(DBusConnection* conn, void* user_data)
{
    BT_LOGD("System bus has disconnected");
}

tele_client_t* teleif_client_connect(const char* name)
{
    GDBusClient* dbus_client;

    tele_client_t* tele = malloc(sizeof(tele_client_t));
    if (!tele)
        return NULL;

    tele->is_ready = false;
    tele->modems = bt_list_new(NULL);
    tele->dbus_sys = g_dbus_setup_private(DBUS_BUS_SYSTEM, NULL, NULL);
    if (!tele->dbus_sys) {
        BT_LOGE("Can't get on system bus");
        bt_list_free(tele->modems);
        free(tele);
        return NULL;
    }

    /* Set disconnect handler to avoid the thread being killed after dbus_connection_close() */
    g_dbus_set_disconnect_function(tele->dbus_sys, system_bus_disconnected, NULL, NULL);

    dbus_client = g_dbus_client_new(tele->dbus_sys, OFONO_SERVICE, OFONO_MANAGER_PATH);
    tele->dbus_client = dbus_client;
    g_dbus_client_set_proxy_filter(dbus_client, proxy_filter, tele);
    g_dbus_client_set_connect_watch(dbus_client, ofono_connect_handler, tele);
    g_dbus_client_set_disconnect_watch(dbus_client, ofono_disconnect_handler, tele);
    g_dbus_client_set_proxy_handlers(dbus_client, ofono_interface_proxy_added,
        ofono_interface_proxy_removed,
        object_filter,
        ofono_property_changed, tele);
    g_dbus_client_set_signal_watch(dbus_client, ofono_interface_signal_callback, tele);
    g_dbus_client_set_ready_watch(dbus_client, ofono_client_ready_cb, tele);

    return tele;
}

void teleif_client_disconnect(tele_client_t* tele)
{
    tele->is_ready = false;
    g_dbus_client_unref(tele->dbus_client);
    dbus_connection_close(tele->dbus_sys);
    dbus_connection_unref(tele->dbus_sys);
    bt_list_free(tele->modems);
    free(tele);
}

void teleif_register_callbacks(tele_client_t* tele, int slot, tele_callbacks_t* cbs)
{
    tele->cbs = cbs;
}

void teleif_unregister_callbacks(tele_client_t* tele, int slot, tele_callbacks_t* cbs)
{
    tele->cbs = NULL;
}

void teleif_call_register_callbacks(tele_client_t* tele, tele_call_t* call,
    tele_call_callbacks_t* cbs)
{
    call->call_cbs = cbs;
}

void teleif_call_unregister_callbacks(tele_client_t* tele, tele_call_t* call,
    tele_call_callbacks_t* cbs)
{
    call->call_cbs = NULL;
}

static void property_set_result(const DBusError* error, void* user_data)
{
}

static void property_set_destory(void* user_data)
{
}

int teleif_modem_set_radio_power(tele_client_t* tele, int slot, bool poweron)
{
    tele_modem_t* modem = get_modem(tele, slot);
    if (!modem)
        return TELE_ERR_PROXY;

    if (!g_dbus_proxy_set_property_basic(modem->proxy, "Online",
            DBUS_TYPE_BOOLEAN,
            &poweron, property_set_result,
            NULL, property_set_destory)) {
        return TELE_FAIL;
    }

    return TELE_SUCCESS;
}

bool teleif_modem_is_radio_on(tele_client_t* tele, int slot)
{
    DBusMessageIter iter;
    int state;

    tele_modem_t* modem = get_modem(tele, slot);
    if (!modem)
        return RADIO_STATUS_UNAVAILABLE;

    if (!g_dbus_proxy_get_property(modem->proxy, "RadioState", &iter))
        return RADIO_STATUS_UNAVAILABLE;

    dbus_message_iter_get_basic(&iter, &state);

    return state == RADIO_STATUS_ON;
}

bool teleif_modem_get_radio_power(tele_client_t* tele, int slot)
{
    DBusMessageIter iter;
    int power;

    tele_modem_t* modem = get_modem(tele, slot);
    if (!modem)
        return false;

    if (!g_dbus_proxy_get_property(modem->proxy, "Online", &iter))
        return false;

    dbus_message_iter_get_basic(&iter, &power);

    return power;
}

int teleif_get_all_calls(tele_client_t* tele, int slot, get_calls_callback_t cbs)
{
    tele_modem_t* modem = get_modem(tele, slot);
    bt_list_node_t* node;
    bt_list_t* list;
    int call_nums;
    tele_call_t** calls;
    int ind;

    if (!modem) {
        return TELE_FAIL;
    }

    list = modem->voicecalls;
    call_nums = bt_list_length(modem->voicecalls);
    if (!call_nums) {
        cbs(tele, NULL, 0);
        return TELE_SUCCESS;
    }

    calls = malloc(sizeof(tele_call_t*) * call_nums);
    if (!calls)
        return TELE_ERR_NOMEM;

    for (ind = 0, node = bt_list_head(list); node != NULL;
         node = bt_list_next(list, node)) {
        tele_call_t* call = bt_list_node(node);
        calls[ind] = call;
        ind++;
    }

    cbs(tele, calls, call_nums);

    return TELE_SUCCESS;
}

typedef struct dial_param {
    tele_client_t* cli;
    char* number;
    dial_callback_t cb;
} dial_param_t;

static void dial_setup(DBusMessageIter* iter, void* user_data)
{
    dial_param_t* param = user_data;
    char* hide_callerid = "default";

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param->number);
    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &hide_callerid);
}

static void dial_reply(DBusMessage* message, void* user_data)
{
    DBusError error;
    dial_param_t* param = user_data;
    bool result = true;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGE("%s, err:%s", __func__, error.name);
        dbus_error_free(&error);
        result = false;
    }

    if (param->cb)
        param->cb(param->cli, result);
}

static void dial_destory(void* user_data)
{
    free(user_data);
}

int teleif_call_dial_number(tele_client_t* tele, int slot, char* number,
    dial_callback_t cb)
{
    GDBusProxy* proxy = get_voice_callmanager(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find voicecall manager proxy", __func__);
        return TELE_ERR_PROXY;
    }

    dial_param_t* param = malloc(sizeof(dial_param_t));
    if (!param)
        return TELE_ERR_NOMEM;

    param->cli = tele;
    param->number = strdup(number);
    param->cb = cb;

    if (!g_dbus_proxy_method_call(proxy, "Dial", dial_setup,
            dial_reply, param, dial_destory)) {
        free(param);
        return TELE_FAIL;
    }

    return TELE_SUCCESS;
}

int teleif_call_answer_call(tele_client_t* tele, tele_call_t* call)
{
    if (!g_dbus_proxy_method_call(call->proxy, "Answer", NULL, NULL, NULL, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

int teleif_call_reject_call(tele_client_t* tele, tele_call_t* call)
{
    if (!g_dbus_proxy_method_call(call->proxy, "Hangup", NULL, NULL, NULL, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

int teleif_call_hangup_call(tele_client_t* tele, tele_call_t* call)
{
    if (!g_dbus_proxy_method_call(call->proxy, "Hangup", NULL, NULL, NULL, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

int teleif_call_hangup_all_call(tele_client_t* tele, int slot)
{
    GDBusProxy* proxy = get_voice_callmanager(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find voicecall manager proxy", __func__);
        return TELE_ERR_PROXY;
    }

    if (!g_dbus_proxy_method_call(proxy, "HangupAll", NULL, NULL, NULL, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

int teleif_call_release_and_answer(tele_client_t* tele, int slot)
{
    GDBusProxy* proxy = get_voice_callmanager(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find voicecall manager proxy", __func__);
        return TELE_ERR_PROXY;
    }

    if (!g_dbus_proxy_method_call(proxy, "ReleaseAndAnswer", NULL, NULL, NULL, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

int teleif_call_hold_and_answer(tele_client_t* tele, int slot)
{
    GDBusProxy* proxy = get_voice_callmanager(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find voicecall manager proxy", __func__);
        return TELE_ERR_PROXY;
    }

    if (!g_dbus_proxy_method_call(proxy, "HoldAndAnswer", NULL, NULL, NULL, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

int teleif_call_hold_call(tele_client_t* tele, int slot)
{
    GDBusProxy* proxy = get_voice_callmanager(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find voicecall manager proxy", __func__);
        return TELE_ERR_PROXY;
    }

    if (!g_dbus_proxy_method_call(proxy, "SwapCalls", NULL, NULL, NULL, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

int teleif_call_merge_call(tele_client_t* tele, int slot)
{
    GDBusProxy* proxy;

    proxy = get_voice_callmanager(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find voicecall manager proxy", __func__);
        return TELE_ERR_PROXY;
    }

    if (!g_dbus_proxy_method_call(proxy, "CreateMultiparty", NULL, NULL, NULL, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

static void dtmf_setup(DBusMessageIter* iter, void* user_data)
{
    const char* tone = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &tone);
}

int teleif_call_send_dtmf(tele_client_t* tele, int slot, const char* tones)
{
    GDBusProxy* proxy;

    proxy = get_voice_callmanager(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find voicecall manager proxy", __func__);
        return TELE_ERR_PROXY;
    }

    if (!g_dbus_proxy_method_call(proxy, "SendTones", dtmf_setup, NULL, (void*)tones, NULL))
        return TELE_FAIL;

    return TELE_SUCCESS;
}

int teleif_network_get_signal_strength(tele_client_t* tele, int slot, int* strength)
{
    DBusMessageIter iter;
    GDBusProxy* proxy;

    proxy = get_network_registration(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find network_registration proxy", __func__);
        return TELE_ERR_PROXY;
    }

    if (!g_dbus_proxy_get_property(proxy, "Strength", &iter)) {
        *strength = -1;
        return TELE_FAIL;
    }

    dbus_message_iter_get_basic(&iter, strength);

    return TELE_SUCCESS;
}

int teleif_network_get_operator(tele_client_t* tele, int slot, char** operator_name, int* status)
{
    DBusMessageIter iter;
    GDBusProxy* proxy;
    void* basic;

    proxy = get_network_operator(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find network_operator proxy", __func__);
        return TELE_ERR_PROXY;
    }

    if (!g_dbus_proxy_get_property(proxy, "Status", &iter)) {
        *status = OPERATOR_STATUS_UNKNOWN;
        return TELE_FAIL;
    }
    dbus_message_iter_get_basic(&iter, &basic);
    *status = operator_status_to_value((const char*)basic);

    if (!g_dbus_proxy_get_property(proxy, "Name", &iter)) {
        *operator_name = NULL;
        return TELE_FAIL;
    }
    dbus_message_iter_get_basic(&iter, operator_name);

    return TELE_SUCCESS;
}

bool teleif_network_is_roaming(tele_client_t* tele, int slot)
{
    DBusMessageIter iter;
    GDBusProxy* proxy;
    char* status;

    proxy = get_network_registration(tele, slot);
    if (!proxy) {
        BT_LOGE("%s, can't find network_registration proxy", __func__);
        return false;
    }

    if (!g_dbus_proxy_get_property(proxy, "Status", &iter))
        return false;

    dbus_message_iter_get_basic(&iter, &status);

    return strcmp(status, "roaming") == 0;
}
#endif
