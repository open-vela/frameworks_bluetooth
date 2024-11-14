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
#ifndef __SPP_SERVICE_H__
#define __SPP_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_spp.h"

typedef struct spp_interface {
    size_t size;
    void* (*register_app)(void* remote, const char* name, const spp_callbacks_t* callbacks);
    bt_status_t (*unregister_app)(void** remote, void* handle);
    bt_status_t (*server_start)(void* handle, uint16_t scn, bt_uuid_t* uuid, uint8_t max_connection);
    bt_status_t (*server_stop)(void* handle, uint16_t scn);
    bt_status_t (*connect)(void* handle, bt_address_t* addr, int16_t scn, bt_uuid_t* uuid, uint16_t* port);
    bt_status_t (*disconnect)(void* handle, bt_address_t* addr, uint16_t port);
} spp_interface_t;

void spp_on_connection_state_changed(bt_address_t* addr, uint16_t conn_port,
    profile_connection_state_t state);
void spp_on_data_sent(uint16_t conn_port, uint8_t* buffer, uint16_t length,
    uint16_t sent_length);
void spp_on_data_received(bt_address_t* addr, uint16_t conn_port,
    uint8_t* buffer, uint16_t length);
void spp_on_server_recieve_connect_request(bt_address_t* addr, uint16_t scn);
void spp_on_connection_mfs_update(uint16_t conn_port, uint16_t mfs);

/*
 * register profile to service manager
 */
void register_spp_service(void);

#endif /* __SPP_SERVICE_H__ */