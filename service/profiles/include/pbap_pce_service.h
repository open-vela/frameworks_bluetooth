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
#ifndef __PBAP_PCE_SERVICE_H__
#define __PBAP_PCE_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_pbap_pce.h"

typedef struct {
    bt_status_t status;
    bt_pce_get_contact_req_type_t type;
    void* req_data;
    bt_pce_contact_t* contact;
} pce_get_contact_end_evt_t;

typedef struct {
    size_t size;

    /**
     * @brief Register the phone book client event callbacks.
     *
     * @param[in] callbacks phone book client event callback function.
     */
    void* (*register_callbacks)(void* remote, const pbap_pce_callbacks_t* callbacks);

    /**
     * @brief Unregister the phone book client event callbacks.
     *
     * @param[in] cookie acquired when callback registered.
     */
    bool (*unregister_callbacks)(void** remote, void* cookie);

    /**
     * @brief Connect to the phone book server.
     *
     * @param[in] addr address of peer device.
     * @return BT_STATUS_SUCCESS on success; a negative errno value on failure.
     */
    bt_status_t (*connect)(bt_address_t* addr);

    /**
     * @brief Disconnect from a phone book server.
     *
     * @param[in] addr address of peer device.
     * @return BT_STATUS_SUCCESS on success; a negative errno value on failure.
     */
    bt_status_t (*disconnect)(bt_address_t* addr);

    /**
     * @brief Retrive a specific contact by its phone number.
     *
     * @param[in] addr   address of peer device.
     * @param[in] number phone number of the contact.
     * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
     */
    bt_status_t (*get_contact_by_number)(bt_address_t* addr, char* number);

    /**
     * @brief Retrive a specific contact by its name.
     *
     * @param[in] addr   address of peer device.
     * @param[in] name   contact name.
     * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
     */
    bt_status_t (*get_contact_by_name)(bt_address_t* addr, char* name);

    /**
     * @brief set an BT address to blacklist
     *
     * @param[in] addr  The BT address is added to the blacklist
     * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
     */
    bt_status_t (*add_to_blacklist)(bt_address_t* addr);

    /**
     * @brief set an BT address to blacklist.
     *
     * @param[in] addr  The BT address that is removed from the blacklist
     * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
     */
    bt_status_t (*remove_from_blacklist)(bt_address_t* addr);

    /**
     * @brief check if an BT address is in blacklist
     *
     * @param[in] addr  The BT address to check.
     * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
     */
    bool (*is_in_blacklist)(bt_address_t* addr);
} pbap_pce_interface_t;

/*
 * sal callback
 */
void pce_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state);
void pce_on_dir_changed(bt_address_t* addr, uint16_t status);
void pce_on_vcard_listing_data_received(bt_address_t* addr, char* obj, uint16_t len);
void pce_on_vcard_listing_end(bt_address_t* addr, uint16_t status);
void pce_on_vcard_data_received(bt_address_t* addr, char* obj, uint16_t len);
void pce_on_vcard_end(bt_address_t* addr, uint16_t status);
void pce_on_get_contact_end(bt_address_t* addr, pce_get_contact_end_evt_t* evt);

/*
 * register profile to service manager
 */
void register_pce_service(void);

/*
 *  statemachine callbacks
 */
void notify_pce_connection_state_changed(bt_address_t* addr, profile_connection_state_t state);
void notify_get_contact_end(bt_address_t* addr, pce_get_contact_end_evt_t* evt);

#endif /* __PBAP_PCE_SERVICE_H__ */