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
#ifndef __BT_TETEPHONY_INTERFACE_H__
#define __BT_TETEPHONY_INTERFACE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TELE_MAX_PHONE_NUMBER_LENGTH 80
#define TELE_MAX_CALLER_NAME_LENGTH 80

enum {
    TELE_SUCCESS = 0,
    TELE_FAIL,
    TELE_ERR_PROXY,
    TELE_ERR_NOMEM,
    TELE_INV_PARAM,
};

enum radio_status {
    RADIO_STATUS_UNAVAILABLE = 0,
    RADIO_STATUS_ON = 1,
    RADIO_STATUS_OFF = 2,
    RADIO_STATUS_EMERGENCY_ONLY = 3,
};

enum network_registration_status {
    NETWORK_REG_STATUS_NOT_REGISTERED = 0,
    NETWORK_REG_STATUS_REGISTERED = 1,
    NETWORK_REG_STATUS_SEARCHING = 2,
    NETWORK_REG_STATUS_DENIED = 3,
    NETWORK_REG_STATUS_UNKNOWN = 4,
    NETWORK_REG_STATUS_ROAMING = 5,
    NETWORK_REG_STATUS_REGISTERED_SMS_EUTRAN = 6,
    NETWORK_REG_STATUS_ROAMING_SMS_EUTRAN = 7,
};

enum operator_status {
    OPERATOR_STATUS_UNKNOWN = 0,
    OPERATOR_STATUS_AVAILABLE = 1,
    OPERATOR_STATUS_CURRENT = 2,
    OPERATOR_STATUS_FORBIDDEN = 3,
};
typedef enum call_status {
    CALL_STATUS_ACTIVE = 0,
    CALL_STATUS_HELD,
    CALL_STATUS_DIALING,
    CALL_STATUS_ALERTING,
    CALL_STATUS_INCOMING,
    CALL_STATUS_WAITING,
    CALL_STATUS_DISCONNECTED,
} tele_call_status_t;

enum call_disconnect_reason {
    CALL_DISCONNECT_REASON_UNKNOWN = 0,
    CALL_DISCONNECT_REASON_LOCAL_HANGUP,
    CALL_DISCONNECT_REASON_REMOTE_HANGUP,
    CALL_DISCONNECT_REASON_ERROR,
};

typedef struct tele_client_ tele_client_t;

typedef struct tele_call_ {
    /* call context */
    tele_client_t* client;
    void* proxy;
    void* call_cbs;
    /* call info */
    uint8_t call_state;
    char line_identification[TELE_MAX_PHONE_NUMBER_LENGTH];
    char incoming_line[TELE_MAX_PHONE_NUMBER_LENGTH];
    char name[TELE_MAX_CALLER_NAME_LENGTH];
    char start_time[128];
    bool is_remote_held;
    bool is_emergency;
    bool is_multiparty;
    bool is_remote_multiparty;
    bool is_incoming;
} tele_call_t;

/* manager */
typedef void (*connection_state_callback_t)(tele_client_t* tele, bool connected);
typedef void (*radio_state_change_callback_t)(tele_client_t* tele, int radio_state);
typedef void (*call_added_callback_t)(tele_client_t* tele, tele_call_t* call);
typedef void (*call_removed_callback_t)(tele_client_t* tele, tele_call_t* call);
typedef void (*network_operator_status_changed_callback_t)(tele_client_t* tele, int status);
typedef void (*network_operator_name_changed_callback_t)(tele_client_t* tele, const char* name);
typedef void (*network_reg_state_changed_callback_t)(tele_client_t* tele, int status);
typedef void (*signal_strength_changed_callback_t)(tele_client_t* tele, int strength);
/* dial */
typedef void (*dial_callback_t)(tele_client_t* tele, bool succeeded);

/* current calls callback */
typedef void (*get_calls_callback_t)(tele_client_t* tele, tele_call_t** call, uint8_t nums);

/* call */
typedef void (*call_state_changed_callback_t)(tele_client_t* tele, tele_call_t* call, int state);
typedef void (*call_disconnect_reason_callback_t)(tele_client_t* tele, tele_call_t* call, int reason);

typedef struct {
    connection_state_callback_t connection_state_cb;
    radio_state_change_callback_t radio_state_change_cb;
    call_added_callback_t call_added_cb;
    call_removed_callback_t call_removed_cb;
    network_operator_status_changed_callback_t operator_status_changed_cb;
    network_operator_name_changed_callback_t operator_name_changed_cb;
    network_reg_state_changed_callback_t network_reg_state_changed_cb;
    signal_strength_changed_callback_t signal_strength_changed_cb;
} tele_callbacks_t;

typedef struct {
    call_state_changed_callback_t call_state_changed_cb;
    call_disconnect_reason_callback_t call_disconnect_reason_cb;
} tele_call_callbacks_t;

#if defined(CONFIG_LIB_DBUS_RPMSG_SERVER_CPUNAME) || defined(CONFIG_OFONO)
tele_client_t* teleif_client_connect(const char* name);
void teleif_client_disconnect(tele_client_t* tele);
void teleif_register_callbacks(tele_client_t* tele, int slot, tele_callbacks_t* cbs);
void teleif_unregister_callbacks(tele_client_t* tele, int slot, tele_callbacks_t* cbs);
void teleif_call_register_callbacks(tele_client_t* tele, tele_call_t* call,
    tele_call_callbacks_t* cbs);
void teleif_call_unregister_callbacks(tele_client_t* tele, tele_call_t* call,
    tele_call_callbacks_t* cbs);
int teleif_modem_set_radio_power(tele_client_t* tele, int slot, bool poweron);
bool teleif_modem_is_radio_on(tele_client_t* tele, int slot);
bool teleif_modem_get_radio_power(tele_client_t* tele, int slot);
int teleif_get_all_calls(tele_client_t* tele, int slot, get_calls_callback_t cbs);
int teleif_call_dial_number(tele_client_t* tele, int slot, char* number,
    dial_callback_t cb);
int teleif_call_answer_call(tele_client_t* tele, tele_call_t* call);
int teleif_call_reject_call(tele_client_t* tele, tele_call_t* call);
int teleif_call_hangup_call(tele_client_t* tele, tele_call_t* call);
int teleif_call_hangup_all_call(tele_client_t* tele, int slot);
int teleif_call_release_and_answer(tele_client_t* tele, int slot);
int teleif_call_hold_and_answer(tele_client_t* tele, int slot);
int teleif_call_hold_call(tele_client_t* tele, int slot);
int teleif_call_merge_call(tele_client_t* tele, int slot);
int teleif_call_send_dtmf(tele_client_t* tele, int slot, const char* tones);
int teleif_network_get_signal_strength(tele_client_t* tele, int slot, int* strength);
int teleif_network_get_operator(tele_client_t* tele, int slot, char** operator_name, int* status);
bool teleif_network_is_roaming(tele_client_t* tele, int slot);
#else
static inline tele_client_t* teleif_client_connect(const char* name)
{
    return NULL;
}
static inline void teleif_client_disconnect(tele_client_t* tele) { }
static inline void teleif_register_callbacks(tele_client_t* tele, int slot, tele_callbacks_t* cbs) { }
static inline void teleif_unregister_callbacks(tele_client_t* tele, int slot, tele_callbacks_t* cbs) { }
static inline void teleif_call_register_callbacks(tele_client_t* tele, tele_call_t* call,
    tele_call_callbacks_t* cbs) { }
static inline void teleif_call_unregister_callbacks(tele_client_t* tele, tele_call_t* call,
    tele_call_callbacks_t* cbs) { }
static inline int teleif_modem_set_radio_power(tele_client_t* tele, int slot, bool poweron) { return TELE_FAIL; }
static inline bool teleif_modem_is_radio_on(tele_client_t* tele, int slot) { return false; }
static inline bool teleif_modem_get_radio_power(tele_client_t* tele, int slot) { return false; }
static inline int teleif_get_all_calls(tele_client_t* tele, int slot, get_calls_callback_t cbs) { return TELE_FAIL; }
static inline int teleif_call_dial_number(tele_client_t* tele, int slot, char* number,
    dial_callback_t cb) { return TELE_FAIL; }
static inline int teleif_call_answer_call(tele_client_t* tele, tele_call_t* call) { return TELE_FAIL; }
static inline int teleif_call_reject_call(tele_client_t* tele, tele_call_t* call) { return TELE_FAIL; }
static inline int teleif_call_hangup_call(tele_client_t* tele, tele_call_t* call) { return TELE_FAIL; }
static inline int teleif_call_hangup_all_call(tele_client_t* tele, int slot) { return TELE_FAIL; }
static inline int teleif_call_release_and_answer(tele_client_t* tele, int slot) { return TELE_FAIL; }
static inline int teleif_call_hold_and_answer(tele_client_t* tele, int slot) { return TELE_FAIL; }
static inline int teleif_call_hold_call(tele_client_t* tele, int slot) { return TELE_FAIL; }
static inline int teleif_call_merge_call(tele_client_t* tele, int slot) { return TELE_FAIL; }
static inline int teleif_call_send_dtmf(tele_client_t* tele, int slot, const char* tones) { return TELE_FAIL; }
static inline int teleif_network_get_signal_strength(tele_client_t* tele, int slot, int* strength) { return TELE_FAIL; }
static inline int teleif_network_get_operator(tele_client_t* tele, int slot, char** operator_name, int* status) { return TELE_FAIL; }
static inline bool teleif_network_is_roaming(tele_client_t* tele, int slot) { return false; }
#endif

#endif /* __BT_TETEPHONY_INTERFACE_H__ */
