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

#ifndef __BT_HFP_HF_H__
#define __BT_HFP_HF_H__

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

/**
 * @cond
 */

/**
 * @brief HFP HF call state
 */
typedef enum {
    HFP_HF_CALL_STATE_ACTIVE = 0,
    HFP_HF_CALL_STATE_HELD,
    HFP_HF_CALL_STATE_DIALING,
    HFP_HF_CALL_STATE_ALERTING,
    HFP_HF_CALL_STATE_INCOMING,
    HFP_HF_CALL_STATE_WAITING,
    HFP_HF_CALL_STATE_HELD_BY_RESP_HOLD,
    HFP_HF_CALL_STATE_DISCONNECTED
} hfp_hf_call_state_t;

/**
 * @brief HFP HF channel type
 */
typedef enum {
    HFP_HF_CHANNEL_TYP_PHONE = 0,
    HFP_HF_CHANNEL_TYP_WEBCHAT,
} hfp_hf_channel_type_t;

/**
 * @brief HFP call info structure
 */
typedef struct {
    uint32_t index;
    uint8_t dir; /* hfp_call_direction_t */
    uint8_t state; /* hfp_hf_call_state_t */
    uint8_t mpty; /* hfp_call_mpty_type_t */
    uint8_t pad[1];
    char number[HFP_PHONENUM_DIGITS_MAX];
    char name[HFP_NAME_DIGITS_MAX];
} hfp_current_call_t;

/**
 * @endcond
 */

/**
 * @brief Callback for HFP HF connection state changed.
 *
 * HFP connection states include DISCONNECTED, CONNECTING, CONNECTED, and
 * DISCONNECTING. During HFP HF initialization, callback functions will be
 * registered. This callback is triggered when the state of HFP HF connection
 * changed.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param state - HFP profile connection state.
 *
 * **Example:**
 * @code
void hfp_hf_connection_state_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    printf("hfp_hf_connection_state_cb, state: %d\n", state);
}
 * @endcode
 */
typedef void (*hfp_hf_connection_state_callback)(void* cookie, bt_address_t* addr, profile_connection_state_t state);

/**
 * @brief HFP HF audio connection state changed callback
 *
 * The audio data transmission in HFP requires the use of a specific transmission
 * link, which is the audio connection. HFP audio connection states include
 * DISCONNECTED, CONNECTING, CONNECTED, and DISCONNECTING. During HFP HF
 * initialization, callback functions will be registered. This callback is
 * triggered when the audio connection state changed.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param state - hfp audio state.
 *
 * **Example:**
 * @code
void hfp_hf_audio_state_cb(void* cookie, bt_address_t* addr, hfp_audio_state_t state)
{
    printf("hfp_hf_audio_state_cb, state: %d\n", state);
}
 * @endcode
 */
typedef void (*hfp_hf_audio_state_callback)(void* cookie, bt_address_t* addr, hfp_audio_state_t state);

/**
 * @brief Callback for HFP HF VR state changed.
 *
 * HFP voice recognition activation states includes STOPPED and STARTED. During
 * HFP HF initialization, callback functions will be registered. This callback
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
void hfp_hf_vr_cmd_cb(void* cookie, bt_address_t* addr, bool started)
{
    printf("hfp_hf_vr_cmd_cb, started: %d\n", started);
    if (!bt_hfp_hf_is_audio_connected(ins, addr))
        hfp_hf_audio_connect(ins, addr);
}
 * @endcode
 */
typedef void (*hfp_hf_vr_cmd_callback)(void* cookie, bt_address_t* addr, bool started);

/**
 * @brief Callback for HFP HF call state changed.
 *
 * This function is used to notify the application on the HF side of the current
 * call states and related information. Call states include ACTIVE, HELD, DIALING,
 * ALERTING, INCOMING, WAITING, HELD_BY_RESP_HOLD, and DISCONNECTED. Other information
 * such as phone number will also be notified to the application through this callback.
 * During HFP HF initialization, callback functions will be registered. This callback
 * will be triggered when HFP HF receives a notification of the current call state.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param call - Call infomation.
 *
 * **Example:**
 * @code
void hfp_hf_call_state_change_cb(void* cookie, bt_address_t* addr, hfp_current_call_t* call)
{
    printf("hfp_hf_call_state_change_cb, call state: %d\n", call->state);
}
 * @endcode
 */
typedef void (*hfp_hf_call_state_change_callback)(void* cookie, bt_address_t* addr, hfp_current_call_t* call);

/**
 * @brief Callback for AT command complete.
 *
 * This callback is used to notify the application of the AT command response. During
 * HFP HF initialization, callback functions will be registered. This callback will
 * be triggered when HF receives a notification of the AT command response.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param resp - The response string of AT command.
 *
 * **Example:**
 * @code
void hfp_hf_cmd_complete_cb(void* cookie, bt_address_t* addr, const char* resp)
{
    printf("hfp_hf_cmd_complete_cb, resp: %s\n", resp);
}
 * @endcode
 */
typedef void (*hfp_hf_cmd_complete_callback)(void* cookie, bt_address_t* addr, const char* resp);

/**
 * @brief Callback for HFP HF ring indication.
 *
 * This callback is used to notify the application of the ring indication. During
 * HFP HF initialization, callback functions will be registered. This callback will
 * be triggered when HF receives a notification of the ring indication.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param inband_ring_tone - True if the ring tone is inband, false if it is outband.
 *
 * **Example:**
 * @code
void hfp_hf_ring_indication_cb(void* cookie, bt_address_t* addr, bool inband_ring_tone)
{
    printf("hfp_hf_ring_indication_cb, inband_ring_tone: %d\n", inband_ring_tone);
}
 * @endcode
 */
typedef void (*hfp_hf_ring_indication_callback)(void* cookie, bt_address_t* addr, bool inband_ring_tone);

/**
 * @brief Callback for HFP HF network roaming state changed.
 *
 * This callback is used to notify the application of the network roaming state. During
 * HFP HF initialization, callback functions will be registered. This callback will
 * be triggered when HF receives a notification of the network roaming state.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param status - Roaming state, 0 represents roaming is not active, 1 represents roaming is active.
 *
 * **Example:**
 * @code
void hfp_hf_roaming_changed_cb(void* cookie, bt_address_t* addr, int status)
{
    printf("hfp_hf_roaming_changed_cb, status: %d\n", status);
}
 * @endcode
 */
typedef void (*hfp_hf_roaming_changed_callback)(void* cookie, bt_address_t* addr, int status);

/**
 * @brief Callback for HFP HF network state changed.
 *
 * This callback is used to notify the application of the network state. During
 * HFP HF initialization, callback functions will be registered. This callback will
 * be triggered when HF receives a notification of the latest network state.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param status - Network state, 0 means roaming is not active, 1 means a roaming is active.
 *
 * **Example:**
 * @code
void hfp_hf_network_state_changed_cb(void* cookie, bt_address_t* addr, int status)
{
    printf("hfp_hf_network_state_changed_cb, status: %d\n", status);
}
 * @endcode
 */
typedef void (*hfp_hf_network_state_changed_callback)(void* cookie, bt_address_t* addr, int status);

/**
 * @brief Callback for HFP HF signal strength changed.
 *
 * This callback is used to notify the application of the signal strength. During
 * HFP HF initialization, callback functions will be registered. This callback will
 * be triggered when HF receives a notification of the signal strength.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param signal - Signale strength, range 0-5.
 *
 * **Example:**
 * @code
void hfp_hf_signal_strength_changed_cb(void* cookie, bt_address_t* addr, int signal)
{
    printf("hfp_hf_signal_strength_changed_cb, signal: %d\n", signal);
}
 * @endcode
 */
typedef void (*hfp_hf_signal_strength_changed_callback)(void* cookie, bt_address_t* addr, int signal);

/**
 * @brief Callback for HFP HF network operator name changed.
 *
 * This callback is used to notify the application of the network operator name. During
 * HFP HF initialization, callback functions will be registered. This callback will
 * be triggered when HF receives a notification of the latest network operator name.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param name - Network operator name.
 *
 * **Example:**
 * @code
void hfp_hf_operator_changed_cb(void* cookie, bt_address_t* addr, char* name)
{
    printf("hfp_hf_operator_changed_cb, name: %s\n", name);
}
 * @endcode
 */
typedef void (*hfp_hf_operator_changed_callback)(void* cookie, bt_address_t* addr, char* name);

/**
 * @brief Callback for HFP HF volume changed.
 *
 * Audio Volume Control enables applications to modify the speaker volume and
 * microphone gain of the HF from the AG. This callback is used to notify the
 * application of the volume control information. During HFP HF initialization,
 * callback functions will be registered. This callback will be triggered when
 * HF receives a notification of the volume control information. The values
 * of volume level are absolute values, and relate to a particular (implementation
 * dependent) volume level
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param type - The type of volume, HFP_VOLUME_TYPE_SPk represents speaker gain
 *               HFP_VOLUME_TYPE_MIC represents microphone gain.
 * @param volume - The gain level, range 0-15, 0 is the minimum and 15 is the maximum.
 *
 * **Example:**
 * @code
void hfp_hf_volume_changed_cb(void* cookie, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    if (type == HFP_VOLUME_TYPE_SPK)
        printf("hfp_hf_volume_changed_cb, speaker volume: %d\n", volume);
    else
        printf("hfp_hf_volume_changed_cb, microphone volume: %d\n", volume);
}
 * @endcode
 */
typedef void (*hfp_hf_volume_changed_callback)(void* cookie, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);

/**
 * @brief Callback for HFP HF call indicator.
 *
 * This callback is used to notify the application of the call indicator. During
 * HFP HF initialization, callback functions will be registered. This callback will
 * be triggered when HF receives a notification of the call indicator.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param call - The call indicator.
 *
 * **Example:**
 * @code
void hfp_hf_call_cb(void* cookie, bt_address_t* addr, hfp_call_t call)
{
    printf("hfp_hf_call_cb, call: %d\n", call);
}
 * @endcode
 */
typedef void (*hfp_hf_call_callback)(void* cookie, bt_address_t* addr, hfp_call_t call);

/**
 * @brief Callback for HFP HF callsetup indicator.
 *
 * This callback is used to notify the application of the callsetup indicator.
 * The callsetup indicator includes NONE, INCOMING, OUTGOING and ALERTING. During
 * HFP HF initialization, callback functions will be registered. This callback will
 * be triggered when HF receives a notification of the callsetup indicator.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param callsetup - The callsetup indicator.
 *
 * **Example:**
 * @code
void hfp_hf_callsetup_cb(void* cookie, bt_address_t* addr, hfp_callsetup_t callsetup)
{
    printf("hfp_hf_callsetup_cb, callsetup: %d\n", callsetup);
}
 * @endcode
 */
typedef void (*hfp_hf_callsetup_callback)(void* cookie, bt_address_t* addr, hfp_callsetup_t callsetup);

/**
 * @brief Callback for HFP HF callheld indicator.
 *
 * This callback is used to notify the application of the callheld indicator.
 * The callheld indicator includes NONE and HELD. During HFP HF initialization,
 * callback functions will be registered. This callback will be triggered when HF
 * receives a notification of the callheld indicator.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param callheld - the callheld indicator.
 *
 * **Example:**
 * @code
void hfp_hf_callheld_cb(void* cookie, bt_address_t* addr, hfp_callheld_t callheld)
{
    printf("hfp_hf_callheld_cb, callheld: %d\n", callheld);
}
 * @endcode
 */
typedef void (*hfp_hf_callheld_callback)(void* cookie, bt_address_t* addr, hfp_callheld_t callheld);

/**
 * @cond
 */

/**
 * @brief HFP HF callback structure
 *
 */
typedef struct
{
    size_t size;
    hfp_hf_connection_state_callback connection_state_cb;
    hfp_hf_audio_state_callback audio_state_cb;
    hfp_hf_vr_cmd_callback vr_cmd_cb;
    hfp_hf_call_state_change_callback call_state_changed_cb;
    hfp_hf_cmd_complete_callback cmd_complete_cb;
    hfp_hf_ring_indication_callback ring_indication_cb;
    hfp_hf_volume_changed_callback volume_changed_cb;
    hfp_hf_call_callback call_cb;
    hfp_hf_callsetup_callback callsetup_cb;
    hfp_hf_callheld_callback callheld_cb;
} hfp_hf_callbacks_t;

/**
 * @endcond
 */

/**
 * @brief Register HFP HF callback functions.
 *
 * This function is used to register callback functions in an application
 * to the HFP HF service. The types of callback functions are included in
 * the hfp_hf_callbacks_t structure.
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - HFP HF callback functions.
 * @return void* - Callback cookie, if the callback is registered successfuly.
 *                 NULL if the callback is already registered or registration fails.
 *
 * **Example:**
 * @code
void* hf_callbacks;
const static hfp_hf_callbacks_t hfp_hf_cbs = {
    sizeof(hfp_hf_cbs),
    hf_connection_state_cb,
    hf_audio_state_cb,
    hf_vr_cmd_cb,
    hf_call_state_changed_cb,
    hf_cmd_complete_cb,
    hf_ring_indication_cb,
    hf_volume_changed_cb,
    hf_call_cb,
    hf_callsetup_cb,
    hf_callheld_cb,
};

void app_init_hfp_hf(bt_instance_t* ins)
{
    hf_callbacks = bt_hfp_hf_register_callbacks(ins, &hfp_hf_cbs);
    if(!hf_callbacks)
        printf("register callbacks failed\n");
    else
        printf("register callbacks success\n");
}
 * @endcode
 */
void* BTSYMBOLS(bt_hfp_hf_register_callbacks)(bt_instance_t* ins, const hfp_hf_callbacks_t* callbacks);

/**
 * @brief Unregister HFP HF callback functions.
 *
 * This function is used to unregister callback functions with HFP HF service
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
void app_deinit_hfp_hf(bt_instance_t* ins)
{
    if(hf_callbacks) {
        if(!bt_hfp_hf_unregister_callbacks(ins, hf_callbacks))
            printf("unregister callbacks failed\n");
    }
}
 * @endcode
 */
bool BTSYMBOLS(bt_hfp_hf_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check HFP HF is connected
 *
 * This function is used to check whether a service level connection is established
 * between AG and HF device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return true - HFP HF is connected.
 * @return false - HFP HF is not connected.
 *
 * **Example:**
 * @code
 void app_check_hfp_hf_connected(bt_instance_t* ins, bt_address_t* addr)
 {
    if(bt_hfp_hf_is_connected(ins, addr))
        printf("HFP HF is connected\n");
    else
        printf("HFP HF is not connected\n");
 }
 * @endcode
 */
bool BTSYMBOLS(bt_hfp_hf_is_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Check HFP HF audio connection is connected.
 *
 * This function is used to check if HFP audio connection has been established.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return true - HF audio is connected.
 * @return false - HF audio is not connected.
 *
 * **Example:**
 * @code
void app_check_hfp_hf_audio_connected(bt_instance_t* ins, bt_address_t* addr)
{
    if(bt_hfp_hf_is_audio_connected(ins, addr))
        printf("HFP HF audio is connected\n");
    else
        printf("HFP HF audio is not connected\n");
}
 * @endcode
 */
bool BTSYMBOLS(bt_hfp_hf_is_audio_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get HFP AG connection state.
 *
 * This function is used for the application to obtain the connection state from
 * HFP AG service. Connection states includes DISCONNECTED, CONNECTING, CONNECTED,
 * and DISCONNECTING.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return profile_connection_state_t - Connection state.
 *
 * **Example:**
 * @code
void app_get_hfp_hf_connection_state(bt_instance_t* ins, bt_address_t* addr)
{
    profile_connection_state_t state;
    state = bt_hfp_ag_get_connection_state(ins, addr);
    switch (state) {
    case PROFILE_STATE_DISCONNECTED: {
        printf("HFP HF is disconnected\n");
        return;
    }
    case PROFILE_STATE_CONNECTING: {
        printf("HFP HF is connecting\n");
        return;
    }
    case PROFILE_STATE_CONNECTED: {
        printf("HFP HF is connected\n");
        return;
    }
    case PROFILE_STATE_DISCONNECTING: {
        printf("HFP HF is disconnecting\n");
        return;
    }
    }
}
 * @endcode
 */
profile_connection_state_t BTSYMBOLS(bt_hfp_hf_get_connection_state)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish service level connection with peer AG device
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
int app_init_hfp_hf_connection(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_hf_connect(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("connect failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_connect)(bt_instance_t* ins, bt_address_t* addr);

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
int app_quit_hfp_hf_connection(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_hf_disconnect(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("disconnect failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set HF Connection policy.
 *
 * This function is used to set the connection policy for HFP. Applications on
 * the HF side can control the HFP connection process through this method. The
 * connection policy includes allowed and forbidden. When set to forbidden, the
 * HF service will refuse to initiate or accept HFP connection request.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param policy - Connection policy.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_set_hfp_hf_connection_specific_policy(bt_instance_t* ins, bt_address_t* addr)
{
    bt_status_t status;
    connection_policy_t policy = CONNECTION_POLICY_FORBIDDEN;

    status = bt_hfp_hf_set_connection_policy(ins, addr, policy);
    if (status != BT_STATUS_SUCCESS)
        printf("set connection policy failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_set_connection_policy)(bt_instance_t* ins, bt_address_t* addr, connection_policy_t policy);

/**
 * @brief Establish audio connection with peer AG device.
 *
 * This function is used for applications on the HF side to initiate an audio
 * connection request to a specified AG in order to establish an audio connection
 * with the AG.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_init_hfp_hf_audio_connection(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_hf_connect_audio(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("connect audio failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_connect_audio)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect audio connection.
 *
 * This function is used for applications on the HF side to initiate an audio
 * disconnection request to the AG.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_quit_hfp_hf_audio_connection(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_hf_disconnect_audio(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("disconnect audio failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_disconnect_audio)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Start voice recognition.
 *
 * Enable the voice recognition function in the AG. This function is used to send
 * a Bluetooth Voice Recognition Activation command used to indicate to the AG that
 * the HF is ready to render audio output.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_start_hfp_hf_voice_recognition(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_hf_start_voice_recognition(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("start voice recognition failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_start_voice_recognition)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Stop voice recognition.
 *
 * Disable the voice recognition function in the AG. This function is used to send
 * AT command used to indicate to the AG that the voice recognition function shall
 * be disabled.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_start_hfp_hf_voice_recognition(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    status = bt_hfp_hf_start_voice_recognition(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("start voice recognition failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_stop_voice_recognition)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Dial specified phone number.
 *
 * This function is used to initiate dialing on the HF side and notify the AG of the
 * dialing information. If the number provided by the application to the HF service
 * is empty, the HF service will use the last used number for dialing.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param number - Phone number.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_dial_specific_number(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    char number[] = "13800138000";

    status = bt_hfp_hf_dial(ins, addr, number);
    if (status != BT_STATUS_SUCCESS)
        printf("dial number failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_dial)(bt_instance_t* ins, bt_address_t* addr, const char* number);

/**
 * @brief Use memory dialing.
 *
 * The HF may initiate outgoing voice calls using the memory dialing feature of the AG.
 * The AG shall then start the call establishment procedure using the phone number stored
 * in the AG memory location given by HF. This function is used for the HF side application
 * to initiate a memory dialing to the AG. The application needs to provide the designated
 * memory location.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param memory - Memory location.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_dial_specific_memory_location(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    uint32_t memory = 1;

    status = bt_hfp_hf_dial_memory(ins, addr, memory);
    if (status != BT_STATUS_SUCCESS)
        printf("dial memory failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_dial_memory)(bt_instance_t* ins, bt_address_t* addr, uint32_t memory);

/**
 * @brief Dial last used number.
 *
 * The HF may initiate outgoing voice calls by recalling the last number dialed by the AG.
 * The AG shall then start the call establishment procedure using the last phone number
 * dialed by the AG. This function is used for the HF side application to initiate a redialing
 * to the AG.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_dial_last_number(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;

    status = bt_hfp_hf_redial(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("redial failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_redial)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Accept the incoming voice call.
 *
 * This function used to accept the incoming voice call by specified means. The acceptance method
 * is specified by the flag parameter. The HF shall then send the ATA command (see Section 5) to
 * the AG. The AG shall then begin the procedure for accepting the incoming call.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param flag - Accept method flag, including HFP_HF_CALL_ACCEPT_NONE, HFP_HF_CALL_ACCEPT_RELEASE, HFP_HF_CALL_ACCEPT_HOLD.
 *               HFP_HF_CALL_ACCEPT_NONE represents accepting an incoming call, invalid on no incoming call.
 *               HFP_HF_CALL_ACCEPT_RELEASE represents releasing all active calls (if any exist) and accepts the other (held or waiting) call.
 *               HFP_HF_CALL_ACCEPT_HOLD represents placing all active calls (if any exist) on hold and accepts the other (held or waiting) call.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_accept_call(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    hfp_call_accept_t flag = HFP_HF_CALL_ACCEPT_NONE;

    status = bt_hfp_hf_accept_call(ins, addr, flag);
    if (status != BT_STATUS_SUCCESS)
        printf("accept call failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_accept_call)(bt_instance_t* ins, bt_address_t* addr, hfp_call_accept_t flag);

/**
 * @brief Reject voice call.
 *
 * This funcation used to reject an incoming call if any exist, otherwise then releases
 * all held calls or a waiting call.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_reject_call(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;

    status = bt_hfp_hf_reject_call(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("reject call failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_reject_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Hold voice call
 *
 * This function is used to hold an active call in a three-way calling scenario.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_hold_call(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;

    status = bt_hfp_hf_hold_call(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("hold call failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_hold_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Terminate voice call
 *
 * This function is used to release all calls if any active, dialing or alerting
 * voice call exist, otherwise then releases all held calls. if don't want release
 * all calls, use bt_hfp_hf_control_call instead please.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_release_all_call(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;

    status = bt_hfp_hf_terminate_call(ins, addr);
    if (status != BT_STATUS_SUCCESS)
        printf("terminate call failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_terminate_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Enhanced call control
 *
 * The Enhanced Call Control mechanism used to release, hold calls or add the call to the
 * conversation. This function provides 5 call control methods, and the application can
 * specify one of them to control the call through this function. The call control methods
 * are specified by the chld parameter.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param chld - Call control methods.
 *               HFP_HF_CALL_CONTROL_CHLD_0 represents releasing all held calls or sets User Determined User Busy (UDUB) for a waiting call.
 *               HFP_HF_CALL_CONTROL_CHLD_1 represents releasing all active calls (if any exist) and accepts the other (held or waiting) call.
 *               HFP_HF_CALL_CONTROL_CHLD_2 represents placing all active calls (if any exist) on hold and accepts the other (held or waiting) call.
 *               HFP_HF_CALL_CONTROL_CHLD_3 represents adding a held call to the conversation.
 *               HFP_HF_CALL_CONTROL_CHLD_4 represents connecting the two calls and disconnects the subscriber from both calls (Explicit Call Transfer).
 *               Support for this value and its associated functionality is optional for the HF.
 * @param index - Call index, it does not work.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_enhanced_call_control(bt_instance_t* ins, bt_address_t* addr, hfp_call_control_t chld, uint8_t index);
{
    bt_status_t status;

    status = bt_hfp_hf_control_call(ins, addr, chld, index);
    if (status != BT_STATUS_SUCCESS)
        printf("control call failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_control_call)(bt_instance_t* ins, bt_address_t* addr, hfp_call_control_t chld, uint8_t index);

/**
 * @brief Query current calls
 *
 * This function is used to query the current call information for application. The
 * query result will be returned through the 'calls' parameter. The returned result
 * should be a two-dimensional array, and the relevant information entries in the
 * first dimension can be viewed in the definition of 'hfp_current_call_t'. Additionally,
 * the application using this function also needs to provide an allocator.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param[out] calls - Out calls infomation array.
 * @param[out] num - Out calls array size.
 * @param allocator - Array allocator.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_query_current_calls(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    hfp_current_call_t* calls;
    int num;

    status = bt_hfp_hf_query_current_calls(ins, addr, &calls, &num, NULL);
    if (status == BT_STATUS_SUCCESS) {
        printf("query current calls failed\n");
        return status;
    }

    if (num) {
        hfp_current_call_t* call = calls;
        for (int i = 0; i < num; i++) {
            printf("\tidx[%d], dir:%d, state:%d, number:%s, name:%s",
                (int)call->index, call->dir, call->state, call->number, call->name);
            call++;
        }
    }
    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_query_current_calls)(bt_instance_t* ins, bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator);

/**
 * @brief Send an AT Command to AG device.
 *
 * This function is used to send AT commands to the specified HF device. The
 * address parameter is used to specify the AG.
 *
 * @param ins - The Bluetooth client instance.
 * @param addr - The Bluetooth The Bluetooth address of the peer device.
 * @param at_command - The AT command to be send.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_send_specific_at_command(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    char at_command[] = "AT+CIND=?";

    status = bt_hfp_hf_send_at_command(ins, addr, at_command);
    if (status != BT_STATUS_SUCCESS)
        printf("send AT command failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_send_at_cmd)(bt_instance_t* ins, bt_address_t* addr, const char* cmd);

/**
 * @brief Update battery level to AG.
 *
 * This function is used for the application to notify the AG of the battery level
 * of the HF device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param level - the battery level, valid from 0 to 100.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_update_battery_level(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    uint8_t level = 70;

    status = bt_hfp_hf_update_battery_level(ins, addr, level);
    if (status != BT_STATUS_SUCCESS)
        printf("update battery level failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_update_battery_level)(bt_instance_t* ins, bt_address_t* addr, uint8_t level);

/**
 * @brief Send volume setting to AG.
 *
 * This function used for the application on the HF side to inform the AG of the
 * current gain settings corresponding to the HF’s speaker volume or microphone
 * gain. It is necessary to specify the type of volume control and the specific
 * volume level when using this function.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param type - the type of volume, 0:gain of speaker, 1:gain of microphone.
 * @param volume - the gain level, range 0-15.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_send_specific_volume_control(bt_instance_t* ins, bt_address_t* addr);
{
    bt_status_t status;
    hfp_volume_type_t type = HFP_VOLUME_TYPE_SPK;
    uint8_t volume = 10;

    status = bt_hfp_hf_volume_control(ins, addr, type, volume);
    if (status != BT_STATUS_SUCCESS)
        printf("volume control failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_volume_control)(bt_instance_t* ins, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);

/**
 * @brief Send Dual Tone Multi-Frequency (DTMF) code
 *
 * During an ongoing call, this function used to instruct the AG to transmit a specific
 * DTMF code to its network connection.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param dtmf - The DTMF code, one of ['0'-'9', 'A'-'D', '*', '#'].
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
int app_send_dtmf(bt_instance_t* ins, bt_address_t* addr, char dtmf);
{
    bt_status_t status;

    status = bt_hfp_hf_send_dtmf(ins, addr, dtmf);
    if (status != BT_STATUS_SUCCESS)
        printf("send dtmf failed\n");

    return status;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_send_dtmf)(bt_instance_t* ins, bt_address_t* addr, char dtmf);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HFP_HF_H__ */
