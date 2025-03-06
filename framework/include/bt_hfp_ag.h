/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef __BT_HFP_AG_H__
#define __BT_HFP_AG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_hfp.h"
#include <stddef.h>

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

#define HFP_PHONE_NUMBER_MAX 80

/**
 * @cond
 */

/**
 * @brief HFP AG call state
 *
 */
typedef enum {
    HFP_AG_CALL_STATE_ACTIVE,
    HFP_AG_CALL_STATE_HELD,
    HFP_AG_CALL_STATE_DIALING,
    HFP_AG_CALL_STATE_ALERTING,
    HFP_AG_CALL_STATE_INCOMING,
    HFP_AG_CALL_STATE_WAITING,
    HFP_AG_CALL_STATE_IDLE,
    HFP_AG_CALL_STATE_DISCONNECTED
} hfp_ag_call_state_t;

/**
 * @endcond
 */

/**
 * @brief Callback for HFP AG connection state changed.
 *
 * HFP connection states include DISCONNECTED, CONNECTING, CONNECTED, and
 * DISCONNECTING. During HFP AG initialization, callback functions will be
 * registered. This callback is triggered when the state of HFP AG connection
 * changed.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param state - HFP profile connection state.
 *
 * **Example:**
 * @code
 void hfp_ag_connection_state_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
 {
    printf("hfp_ag_connection_state_cb, state: %d\n", state);
 }
 * @endcode
 */
typedef void (*hfp_ag_connection_state_callback)(void* cookie, bt_address_t* addr, profile_connection_state_t state);

/**
 * @brief Callback for HFP AG audio state changed.
 *
 * The audio data transmission in HFP requires the use of a specific transmission
 * link, which is the audio connection. HFP audio connection states include
 * DISCONNECTED, CONNECTING, CONNECTED, and DISCONNECTING. During HFP AG
 * initialization, callback functions will be registered. This callback is
 * triggered when the audio connection state changed.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param state - HFP audio connection state.
 *
 * **Example:**
 * @code
void hfp_ag_audio_state_cb(void* cookie, bt_address_t* addr, hfp_audio_state_t state)
{
    printf("hfp_ag_audio_state_cb, state: %d\n", state);
}
 * @endcode
 */
typedef void (*hfp_ag_audio_state_callback)(void* cookie, bt_address_t* addr, hfp_audio_state_t state);

/**
 * @brief Callback for HFP AG VR state changed.
 *
 * HFP voice recognition activation states includes STOPPED and STARTED. During
 * HFP AG initialization, callback functions will be registered. This callback
 * is triggered when the VR state changed. When the application receives a
 * notification through this callback function, it needs to initiate an audio
 * connection if the connection is not established.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param started - Started or stopped states of voice recognition, true is started, false is stopped.
 *
 * **Example:**
 * @code
void hfp_ag_vr_cmd_cb(void* cookie, bt_address_t* addr, bool started)
{
    printf("hfp_ag_vr_cmd_cb, is start: %d\n", started);
    if(!bt_hfp_ag_is_audio_connected(ins, addr)) {
        bt_hfp_ag_connect_audio(ins, addr);
    }
}
 * @endcode
 */
typedef void (*hfp_ag_vr_cmd_callback)(void* cookie, bt_address_t* addr, bool started);

/**
 * @brief Callback for HF battery level updated.
 *
 * This callback is used to notify the application of the HF's battery level.
 * During HFP AG initialization, callback functions will be registered. This
 * callback will be triggered when AG receives an update of the HF battery
 * level information.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param value - HF battery level，range 0-100.
 *
 * **Example:**
 * @code
void hfp_ag_battery_update_cb(void* cookie, bt_address_t* addr, uint8_t value)
{
    printf("hfp_ag_battery_update_cb, battery level: %d\n", value);
}
 * @endcode
 */
typedef void (*hfp_ag_battery_update_callback)(void* cookie, bt_address_t* addr, uint8_t value);

/**
 * @brief Callback for HFP AG received volume control.
 *
 * This callback is used to notify the application of volume-related information,
 * including speaker gain or microphone gain. During HFP AG initialization,
 * callback functions will be registered. This callback will be triggered when
 * AG receives a volume control information.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param type - The type of volume, HFP_VOLUME_TYPE_SPK represents speaker gain,
 *               HFP_VOLUME_TYPE_MIC represents microphone gain.
 * @param volume - The gain level, range 0-15.
 *
 * **Example:**
 * @code
void hfp_ag_volume_control_cb(void* cookie, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    printf("hfp_ag_volume_control_cb, type: %d, volume: %d\n", type, volume);
}
 * @endcode
 */
typedef void (*hfp_ag_volume_control_callback)(void* cookie, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);

/**
 * @brief Callback for HFP AG received answer call.
 *
 * This callback function informs the application that a standard call answer
 * command has been received. During HFP AG initialization, callback functions
 * will be registered. This callback will be triggered when AG receives an
 * answer call.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 *
 * **Example:**
 * @code
void hfp_ag_answer_call_cb(void* cookie, bt_address_t* addr)
{
    printf("hfp_ag_answer_call_cb\n");
}
 * @endcode
 */
typedef void (*hfp_ag_answer_call_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Callback for HFP AG received reject call.
 *
 * The AT+CHUP command may turn into a reject_call_callback or a hangup_call_callback,
 * depending on whether the call is active. This callback is used to notify the
 * application of the reject call. During HFP AG initialization, callback functions
 * will be registered. This callback will be triggered when AG receives a standard
 * hang-up command while a call is in the setup process. The application shall
 * terminate calls that are still in the setup phase, i.e., calls that have not yet
 * been activated.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 *
 * **Example:**
 * @code
void hfp_ag_reject_call_cb(void* cookie, bt_address_t* addr)
{
    printf("hfp_ag_reject_call_cb\n");
}
 * @endcode
 */
typedef void (*hfp_ag_reject_call_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Callback for HFP AG received hangup call.
 *
 * The AT+CHUP command may turn into a reject_call_callback or a hangup_call_callback,
 * depending on whether the call is active. This callback is used to notify the
 * application of the hangup call. During HFP AG initialization, callback functions
 * will be registered. This callback will be triggered when AG receives a hangup call.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 *
 * **Example:**
 * @code
void hfp_ag_hangup_call_cb(void* cookie, bt_address_t* addr)
{
    printf("hfp_ag_hangup_call_cb\n");
}
 * @endcode
 */
typedef void (*hfp_ag_hangup_call_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Callback for HFP AG received dial.
 *
 * This callback is used to notify the application that a standard AT command
 * has been received for placing a call to a specific phone number. During HFP
 * AG initialization, callback functions will be registered. This callback will
 * be triggered when AG receives dial information.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param number - The dialed number, if number is NULL, redial.
 *
 * **Example:**
 * @code
void hfp_ag_dial_call_cb(void* cookie, bt_address_t* addr, const char* number)
{
    if(number)
        printf("hfp_ag_dial_call_cb, number: %s\n", number);
    else
        printf("hfp_ag_dial_call_cb, redial\n");
}
 * @endcode
 */
typedef void (*hfp_ag_dial_call_callback)(void* cookie, bt_address_t* addr, const char* number);

/**
 * @brief Callback for HFP AG received AT command.
 *
 * This callback is used to notify the application of the AT command. During
 * HFP AG initialization, callback functions will be registered. This callback
 * will be triggered when AG receives an AT command that not recognized
 * (or not handled) by Bluetooth service.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param at_command - AT command.
 *
 * **Example:**
 * @code
void hfp_ag_at_cmd_received_cb(void* cookie, bt_address_t* addr, const char* at_command)
{
    printf("hfp_ag_at_cmd_received_cb, at_command: %s\n", at_command);
}
 * @endcode
 */
typedef void (*hfp_ag_at_cmd_received_callback)(void* cookie, bt_address_t* addr, const char* at_command);

/**
 * @brief Callback for HFP AG received vendor specific AT command.
 *
 * This callback is used to notify the application of the vendor specific AT command. During
 * HFP AG initialization, callback functions will be registered. This callback will be triggered
 * when AG receives a vendor specific AT command.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param command - The prefix of the AT command.
 * @param company_id - Bluetooth company ID.
 * @param value - AT command value.
 *
 * **Example:**
 * @code
 void hfp_ag_vend_spec_at_cmd_received_cb(void* cookie, bt_address_t* addr, const char* command, uint16_t company_id, const char* value)
 {
        printf("hfp_ag_vend_spec_at_cmd_received_cb, command: %s, company_id: %d, value: %s\n", command, company_id, value);
 }
 * @endcode
 */
typedef void (*hfp_ag_vend_spec_at_cmd_received_callback)(void* cookie, bt_address_t* addr, const char* command, uint16_t company_id, const char* value);

/**
 * @brief HFP CLCC command received callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 *
 * **Example:**
 * @code
 void hfp_ag_clcc_cmd_received_callback(void* cookie, bt_address_t* addr)
 {
        printf("hfp_ag_clcc_cmd_received_callback\n");
 }
 * @endcode
 */
typedef void (*hfp_ag_clcc_cmd_received_callback)(void* cookie, bt_address_t* addr);

/**
 * @cond
 */

/**
 * @brief HFP AG callback structure
 *
 */
typedef struct
{
    size_t size;
    hfp_ag_connection_state_callback connection_state_cb;
    hfp_ag_audio_state_callback audio_state_cb;
    hfp_ag_vr_cmd_callback vr_cmd_cb;
    hfp_ag_battery_update_callback hf_battery_update_cb;
    hfp_ag_volume_control_callback volume_control_cb;
    hfp_ag_answer_call_callback answer_call_cb;
    hfp_ag_reject_call_callback reject_call_cb;
    hfp_ag_hangup_call_callback hangup_call_cb;
    hfp_ag_dial_call_callback dial_call_cb;
    hfp_ag_at_cmd_received_callback at_cmd_cb;
    hfp_ag_vend_spec_at_cmd_received_callback vender_specific_at_cmd_cb;
    hfp_ag_clcc_cmd_received_callback clcc_cmd_cb;
} hfp_ag_callbacks_t;

/**
 * @endcond
 */

/**
 * @brief Register HFP AG callback functions.
 *
 * This function is used to register callback functions in an application
 * to the HFP AG service. The types of callback functions are included in
 * the hfp_ag_callbacks_t structure.
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - HFP AG callback functions.
 * @return void* - Callback cookie, if the callback is registered successfuly.
 *                 NULL if the callback is already registered or registration fails.
 *
 * **Example:**
 * @code
void* ag_callbacks;
const static hfp_ag_callbacks_t hfp_ag_cbs = {
    sizeof(hfp_ag_cbs),
    ag_connection_state_cb,
    ag_audio_state_cb,
    ag_vr_cmd_cb,
    ag_battery_update_cb,
    ag_volume_control_cb,
    ag_answer_call_cb,
    ag_reject_call_cb,
    ag_hangup_call_cb,
    ag_dial_call_cb,
    ag_at_cmd_cb,
    ag_vender_specific_at_cmd_cb,
};

void app_init_hfp_ag(bt_instance_t* ins)
{
    ag_callbacks = bt_hfp_ag_register_callbacks(ins, &hfp_ag_cbs);
    if(!ag_callbacks)
        printf("register callbacks failed\n");
    else
        printf("register callbacks success\n");
}
* @endcode
 */
void* BTSYMBOLS(bt_hfp_ag_register_callbacks)(bt_instance_t* ins, const hfp_ag_callbacks_t* callbacks);

/**
 * @brief Unregister HFP AG callback functions.
 *
 * This function is used to unregister callback functions with HFP AG service
 * in an application. The application shall unregister all callbacks once it
 * is no longer interested in them, such as when the application is quitting.
 *
 * @param ins - Bluetooth client instance.
 * @param cookie - Callback cookie.
 * @return true - Unregister successfully.
 * @return false - Callback cookie not found.
 *
 * **Example:**
 * @code
void app_deinit_hfp_ag(bt_instance_t* ins)
{
    if(ag_callbacks) {
        if(!bt_hfp_ag_unregister_callbacks(ins, ag_callbacks))
            printf("unregister callbacks failed\n");
    }
}
 * @endcode
 */
bool BTSYMBOLS(bt_hfp_ag_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check HFP AG is connected
 *
 * This function is used to check whether a service level connection is established
 * between AG and HF device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return true - HFP AG is connected.
 * @return false - HFP AG is not connected.
 *
 * **Example:**
 * @code
 void app_check_hfp_ag_connected(bt_instance_t* ins, bt_address_t* addr)
 {
    if(bt_hfp_ag_is_connected(ins, addr))
        printf("HFP AG is connected\n");
    else
        printf("HFP AG is not connected\n");
 }
 * @endcode
 */
bool BTSYMBOLS(bt_hfp_ag_is_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Check HFP AG audio connection is connected.
 *
 * This function is used to check if HFP audio connection has been established.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return true - AG audio is connected.
 * @return false - AG audio is not connected.
 *
 * **Example:**
 * @code
void app_check_hfp_ag_audio_connected(bt_instance_t* ins, bt_address_t* addr)
{
    if(bt_hfp_ag_is_audio_connected(ins, addr))
        printf("HFP AG audio is connected\n");
    else
        printf("HFP AG audio is not connected\n");
}
 * @endcode
 */
bool BTSYMBOLS(bt_hfp_ag_is_audio_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get HFP AG connection state.
 *
 * This function is used for the application to obtain the service level connection
 * state from HFP AG service. Connection states includes DISCONNECTED, CONNECTING,
 * CONNECTED, and DISCONNECTING.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return profile_connection_state_t - Connection state.
 *
 * **Example:**
 * @code
void app_get_hfp_ag_connection_state(bt_instance_t* ins, bt_address_t* addr)
{
    profile_connection_state_t state;
    state = bt_hfp_ag_get_connection_state(ins, addr);
    switch (state) {
    case PROFILE_STATE_DISCONNECTED: {
        printf("HFP AG is disconnected\n");
        return;
    }
    case PROFILE_STATE_CONNECTING: {
        printf("HFP AG is connecting\n");
        return;
    }
    case PROFILE_STATE_CONNECTED: {
        printf("HFP AG is connected\n");
        return;
    }
    case PROFILE_STATE_DISCONNECTING: {
        printf("HFP AG is disconnecting\n");
        return;
    }
    }
}
 * @endcode
 */
profile_connection_state_t BTSYMBOLS(bt_hfp_ag_get_connection_state)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish service level connection with peer HF device
 *
 * This function is used to initiate an HFP connection to a specified device
 * and establish the service level connection. Therefore, calling this function
 * successfully implies the execution of a set of AT commands and responses
 * specified by the profile, which is necessary to synchronize the state of the
 * HF with that of the AG.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_init_hfp_ag_connection(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_ag_connect(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("connect failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from HFP service level connection.
 *
 * This function is used to initiate disconnection of the service level connection
 * from the specified device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_quit_hfp_ag_connection(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_ag_disconnect(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("disconnect failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish audio connection with peer HF device.
 *
 * This function is used for applications on the AG side to initiate an audio
 * connection request to a specified HF.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - Bluetooth The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_init_hfp_ag_audio_connection(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_ag_connect_audio(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("connect audio failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_connect_audio)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect audio connection.
 *
 * This function is used for applications on the AG side to initiate an audio
 * disconnection request to a specified HF.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_quit_hfp_ag_audio_connection(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_ag_disconnect_audio(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("disconnect audio failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_disconnect_audio)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Start SCO using virtual voice call.
 *
 * There are three modes for SCO audio: telecom call, virtual call and voice
 * recognition. When one mode is active, other mode cannot be started. This
 * function is used for applications on the AG side to start SCO using virtual
 * voice call with specified HF.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_start_hfp_ag_virtual_call(bt_instance_t* ins, bt_address_t* addr);
{

    bt_status_t status;
    status = bt_hfp_ag_start_virtual_call(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("start virtual call failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_start_virtual_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Stop SCO using virtual voice call.
 *
 * This function is used for applications on the AG side to stop SCO using
 * virtual voice call with specified HF.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_stop_hfp_ag_virtual_call(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_ag_stop_virtual_call(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("stop virtual call failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_stop_virtual_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Start voice recognition
 *
 * This function is used for applications on the AG side to start voice recognition.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_start_hfp_ag_voice_recognition(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_ag_start_voice_recognition(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("start voice recognition failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_start_voice_recognition)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Stop voice recognition
 *
 * This function is used for applications on the AG side to stop voice recognition.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_stop_hfp_ag_voice_recognition(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_ag_stop_voice_recognition(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("stop voice recognition failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_stop_voice_recognition)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Send phone state change.
 *
 * This function is used to inform the HF device about phone state changes.
 * The phone state information includes the number of active calls and held
 * calls, along with details of the current call, such as the call state (e.g.
 * dialing, alerting, or active), call type (e.g. national or international),
 * phone number, and name of the caller if applicable.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param num_active - The number of active calls
 * @param num_held - The number of held calls
 * @param call_state - The state of call
 * @param type - The type of call
 * @param number - The call number
 * @param name - The call name
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_inform_specific_phone state(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    char number[HFP_MAX_NUMBER_LEN] = "123456789";
    char name[HFP_MAX_NAME_LEN] = "<NAME>";
    uint8_t num_active = 1;
    uint8_t num_held = 0;
    hfp_ag_call_state_t call_state = HFP_AG_CALL_STATE_ACTIVE;
    hfp_call_addrtype_t type = HFP_CALL_ADDRTYPE_INTERNATIONAL;

    status = bt_hfp_ag_phone_state_change(ins, addr, num_active, num_held,
        call_state, type, number, name);
    if (status != BT_STATUS_SUCCESS)
        printf("phone state change failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_phone_state_change)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t num_active, uint8_t num_held,
    hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
    const char* number, const char* name);

/**
 * @brief Notify device states
 *
 * This function is used to notify the device states to the specified HF device.
 * Device states information includes network service states, roaming states,
 * signal strength, and battery level.
 *
 * @param ins - The Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param network - The state of network service.
 * @param roam - The state of roaming.
 * @param signal - The signal strength, range 0-5.
 * @param battery - The battery level, range 0-5.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_notify_specific_device_status(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    hfp_network_state_t network = HFP_NETWORK_STATE_AVAILABLE;
    hfp_roaming_state_t roam = HFP_ROAM_STATE_NO_ROAMING;
    uint8_t signal = 5;
    uint8_t battery = 3;

    status = bt_hfp_ag_notify_device_status(ins, addr, network, roam, signal, battery);
    if (status != BT_STATUS_SUCCESS)
        printf("notify device status failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_notify_device_status)(bt_instance_t* ins, bt_address_t* addr,
    hfp_network_state_t network, hfp_roaming_state_t roam,
    uint8_t signal, uint8_t battery);

/**
 * @brief Send volume control to HF.
 *
 * Audio Volume Control enables applications to modify the speaker volume and
 * microphone gain of the HF from the AG. This function is used to send volume
 * control to the specified HF device. The address parameter is used to specify
 * the peer HF device. In addition to using this function, it is also necessary
 * to specify the type of volume control and the specific volume level. The values
 * of volume level are absolute values, and relate to a particular (implementation
 * dependent) volume level controlled by the HF.
 *
 * @param ins - The Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param type - The type of volume, HFP_VOLUME_TYPE_SPk represents speaker gain
 *               HFP_VOLUME_TYPE_MIC represents microphone gain.
 * @param volume - The gain level, range 0-15, 0 is the minimum and 15 is the maximum.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_send_specific_volume_control(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    hfp_volume_type_t type = HFP_VOLUME_TYPE_SPK;
    uint8_t volume = 10;

    status = bt_hfp_ag_volume_control(ins, addr, type, volume);
    if (status != BT_STATUS_SUCCESS)
        printf("volume control failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_volume_control)(bt_instance_t* ins, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);

/**
 * @brief Send an AT Command to HF device.
 *
 * This function is used to send specific AT commands to the specified HF device. The
 * address parameter is used to specify the peer HF device.
 *
 * @note This function will be deprecated in the future. Please use
 *       bt_hfp_ag_send_vendor_specific_at_command() instead.
 *
 * @param ins - The Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param at_command - The AT command to be send.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_send_specific_at_command(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    char at_command[] = "+CNUM: ,"5551212",129,,4";

    status = bt_hfp_ag_send_at_command(ins, addr, at_command);
    if (status != BT_STATUS_SUCCESS)
        printf("send AT command failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_send_at_command)(bt_instance_t* ins, bt_address_t* addr, const char* at_command);

/**
 * @brief Send a vendor specific AT Command
 *
 * This function is used to send specific vendor AT commands to the specified HF device. The
 * address parameter is used to specify the peer HF device.
 *
 * @param ins - The Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param command - The prefix of the AT command to be send.
 * @param value - The value of the AT command to be send.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_send_specific_at_command(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    char at_command[] = "+XIAOMI";
    char value[] = "123456";

    status = bt_hfp_ag_send_at_command(ins, addr, at_command, value);
    if (status != BT_STATUS_SUCCESS)
        printf("send AT command failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_send_vendor_specific_at_command)(bt_instance_t* ins, bt_address_t* addr, const char* command, const char* value);

/**
 * @brief Send CLCC Response
 *
 * This function shall be invoked for each call when there are multiple calls. When all calls are
 * transmitted, the application shall invoke bt_hfp_ag_clcc_response(ins, addr, 0, 0, 0, 0, NULL)
 * to finalize a query procedure.
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @param index - index of the call.
 * @param dir - direction of the call.
 * @param state - state of the call.
 * @param mode - mode of the call.
 * @param mpty - whether the call is multi party.
 * @param type - type of the call.
 * @param number - phone number of the call.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int bt_hfp_ag_send_clcc_response(bt_instance_t* ins, bt_address_t* addr, uint32_t index,
    hfp_call_direction_t dir, hfp_ag_call_state_t state, hfp_call_mode_t mode,
    hfp_call_mpty_type_t mpty, hfp_call_addrtype_t type, const char* number)
{
    bt_status_t status;
    uint32_t index = 1;
    hfp_call_direction_t dir = HFP_CALL_DIRECTION_INCOMING;
    hfp_ag_call_state_t state = HFP_AG_CALL_STATE_INCOMING;
    hfp_call_mode_t mode = HFP_CALL_MODE_VOICE;
    hfp_call_mpty_type_t mpty = HFP_CALL_MPTY_TYPE_SINGLE;
    hfp_call_addrtype_t type = HFP_CALL_ADDRTYPE_NATIONAL;
    char number[] = "12345678900";
    status = bt_hfp_ag_send_clcc_response(ins, addr, index, dir, state, mode, mpty, type, number);
    if (status != BT_STATUS_SUCCESS)
        printf("send clcc response failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_send_clcc_response)(bt_instance_t* ins, bt_address_t* addr,
    uint32_t index, hfp_call_direction_t dir, hfp_ag_call_state_t state, hfp_call_mode_t mode,
    hfp_call_mpty_type_t mpty, hfp_call_addrtype_t type, const char* number);
#ifdef __cplusplus
}
#endif

#endif /* __BT_HFP_AG_H__ */
