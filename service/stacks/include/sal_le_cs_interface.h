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

#ifndef __SAL_LE_CS_INTERFACE_H__
#define __SAL_LE_CS_INTERFACE_H__

#include "bluetooth.h"
#include <cs_service.h>
#include <stdint.h>
bt_status_t bt_sal_cs_read_remote_supported_capabilities(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_cs_set_default_settings(bt_controller_id_t id, bt_address_t* addr,
    bt_le_srv_cs_set_default_settings_param_t* params);
bt_status_t bt_sal_cs_read_remote_fae_table(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_cs_create_config(bt_controller_id_t id, bt_address_t* addr,
    bt_le_srv_cs_create_config_params_t* params,
    bt_le_srv_cs_create_config_context_t context);
bt_status_t bt_sal_cs_security_enable(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_cs_procedure_enable(bt_address_t* addr, const bt_le_srv_cs_procedure_enable_param_t* params);
bt_status_t bt_sal_cs_remove_config(bt_controller_id_t id, bt_address_t* addr, uint8_t config_id);
bt_status_t bt_sal_cs_set_procedure_parameters(bt_controller_id_t id, bt_address_t* addr,
    const bt_le_srv_cs_set_procedure_parameters_param_t* params);
bt_status_t bt_sal_cs_set_channel_classification(uint8_t channel_classification[10], bt_address_t* addr);
bt_status_t bt_sal_cs_read_local_supported_capabilities(bt_srv_conn_le_cs_capabilities_t* params, bt_address_t* addr);
bt_status_t bt_sal_cs_write_cached_remote_supported_capabilities(bt_srv_conn_le_cs_capabilities_t* params, bt_address_t* addr);

#endif //__SAL_LE_CS_INTERFACE_H__