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
#ifndef _BT_CONTROLLER_VENDOR_H__
#define _BT_CONTROLLER_VENDOR_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bt_vendor_common.h"
#include "lea_audio_common.h"

/****************************************************************************
 * Public Fucntion
 ****************************************************************************/

bool a2dp_offload_start_builder(a2dp_offload_config_t* config,
    uint8_t* offload, size_t* size);

bool a2dp_offload_stop_builder(a2dp_offload_config_t* config,
    uint8_t* offload, size_t* size);

bool hfp_offload_start_builder(hfp_offload_config_t* config,
    uint8_t* offload, size_t* size);

bool hfp_offload_stop_builder(hfp_offload_config_t* config,
    uint8_t* offload, size_t* size);

bool lea_offload_start_builder(lea_offload_config_t* config,
    uint8_t* offload, size_t* size);

bool lea_offload_stop_builder(lea_offload_config_t* config,
    uint8_t* offload, size_t* size);

bool acl_bandwidth_config_builder(acl_bandwitdh_config_t* config,
    uint8_t* cmd, size_t* size);

bool acl_bandwidth_deconfig_builder(acl_bandwitdh_config_t* config,
    uint8_t* cmd, size_t* size);

bool le_dlf_enable_builder(le_dlf_config_t* config,
    uint8_t* data, size_t* size);

bool le_dlf_disable_builder(le_dlf_config_t* config,
    uint8_t* data, size_t* size);

#endif /* _BT_CONTROLLER_VENDOR_H__ */
