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

#include "bt_vendor.h"

#ifdef CONFIG_OBELISK_BT_VENDOR_ACTIONS
#include "bt_vendor_actions.h"
#endif

bool a2dp_offload_start_builder(a2dp_offload_config_t *config,
                                uint8_t *offload, size_t *size)
{
#ifdef CONFIG_OBELISK_BT_VENDOR_ACTIONS
    return actions_a2dp_offload_start_builder(config, offload, size);
#else
    return false;
#endif
}

bool a2dp_offload_stop_builder(a2dp_offload_config_t *config,
                               uint8_t *offload, size_t *size)
{
#ifdef CONFIG_OBELISK_BT_VENDOR_ACTIONS
    return actions_a2dp_offload_stop_builder(config, offload, size);
#else
    return false;
#endif
}

bool hfp_offload_start_builder(hfp_offload_config_t *config,
                               uint8_t *offload, size_t *size)
{
#ifdef CONFIG_OBELISK_BT_VENDOR_ACTIONS
    return actions_hfp_offload_start_builder(config, offload, size);
#else
    return false;
#endif
}

bool hfp_offload_stop_builder(hfp_offload_config_t *config,
                              uint8_t *offload, size_t *size)
{
#ifdef CONFIG_OBELISK_BT_VENDOR_ACTIONS
    return actions_hfp_offload_stop_builder(config, offload, size);
#else
    return false;
#endif
}
