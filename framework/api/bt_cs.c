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
#include <stdint.h>

#include "bt_cs.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "cs_service.h"
#include "service_manager.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LE_CS

static bt_cs_interface_t* get_profile_service(void)
{
    return (bt_cs_interface_t*)service_manager_get_profile(PROFILE_CS);
}
void* BTSYMBOLS(bt_cs_register_callbacks)(bt_instance_t* ins, const cs_callbacks_t* callbacks)
{
    bt_cs_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool BTSYMBOLS(bt_cs_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    bt_cs_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t BTSYMBOLS(bt_cs_start_distance_measurement)(bt_instance_t* ins, bt_distance_measurement_params_t* params)
{
    bt_cs_interface_t* profile = get_profile_service();

    return profile->start_distance_measurement(params);
}

bt_status_t BTSYMBOLS(bt_cs_stop_distance_measurement)(bt_instance_t* ins, bt_address_t* addr, uint8_t method, bool timeout)
{
    bt_cs_interface_t* profile = get_profile_service();

    return profile->stop_distance_measurement(addr, method, timeout);
}

bt_status_t BTSYMBOLS(bt_cs_set_config)(bt_instance_t* ins, bt_address_t* addr, const bt_cs_set_params_t* params)
{
    bt_cs_interface_t* profile = get_profile_service();

    return profile->set_config(addr, params);
}

#ifdef CONFIG_BT_CS_RAS_TEST
bt_status_t BTSYMBOLS(bt_cs_test)(bt_instance_t* ins, void* data, uint16_t len)
{
    bt_cs_interface_t* profile = get_profile_service();

    return profile->cs_test(data, len);
}
#endif /* CONFIG_BT_CS_RAS_TEST */

#endif /* CONFIG_BLUETOOTH_LE_CS */
