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
#ifndef __BT_PBAP_PCE_H__
#define __BT_PBAP_PCE_H__

#include <stddef.h>

#include "bt_addr.h"
#include "bt_device.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

#define BT_PBAP_PCE_PROPERTY_MAX_LEN 32
#define BT_PBAP_PCE_VCARD_HANDLE_MAX_LEN 64
#define BT_PBAP_PCE_NUMBER_MAX_COUNT 2

typedef enum {
    PCE_GET_CONTACT_BY_NAME,
    PCE_GET_CONTACT_BY_NUMBER
} bt_pce_get_contact_req_type_t;

typedef struct {
    char name[BT_PBAP_PCE_PROPERTY_MAX_LEN];
    char numbers[BT_PBAP_PCE_PROPERTY_MAX_LEN][BT_PBAP_PCE_NUMBER_MAX_COUNT];
} bt_pce_contact_t;

/**
 * @brief PCE connection state changed callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param addr - address of peer device.
 * @param state - pce connection state
 */
typedef void (*pce_connection_state_callback)(void* cookie, bt_address_t* addr, profile_connection_state_t state);

/**
 * @brief PCE vCard finished callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param property - used to get the properties of a contact.
 * @param status - property value for geting a contact.
 * @param contact - got the contact information.
 */
typedef void (*pce_contact_report_callback)(void* cookie, bt_status_t status, bt_pce_get_contact_req_type_t req_type, char* req_data, bt_pce_contact_t* contact);

/**
 * @brief PCE event callbacks structure
 */
typedef struct {
    size_t size;
    pce_connection_state_callback connection_state_cb;
    pce_contact_report_callback get_contact_end_cb;
} pbap_pce_callbacks_t;

/**
 * @brief Register callback functions for PCE service.
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - phone book client callback functions.
 * @return void* - callback cookie, NULL on failure.
 */
void* BTSYMBOLS(bt_pbap_pce_register_callbacks)(bt_instance_t* ins, const pbap_pce_callbacks_t* callbacks);

/**
 * @brief Unregister PCE callback function.
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callbacks cookie.
 * @return true - on callback unregister success.
 * @return false - on callback cookie not found.
 */
bool BTSYMBOLS(bt_pbap_pce_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Connect to the phone book server.
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from a phone book server.
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Retrive a specific contact by its name.
 *
 * @param[in] addr   address of peer device.
 * @param[in] number name of the contact.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_get_contact_by_name)(bt_instance_t* ins, bt_address_t* addr, char* name);

/**
 * @brief Retrive a specific contact by its phone number.
 *
 * @param[in] addr   address of peer device.
 * @param[in] number phone number of the contact.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_get_contact_by_number)(bt_instance_t* ins, bt_address_t* addr, char* number);

#endif /* __BT_PBAP_PCE_H__ */