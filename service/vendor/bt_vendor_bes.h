/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef _BT_CONTROLLER_VENDOR_BES_H__
#define _BT_CONTROLLER_VENDOR_BES_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>

#include "bt_utils.h"
#include "bt_vendor.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef CONFIG_VSC_MAX_LEN
#define CONFIG_VSC_MAX_LEN 64

/****************************************************************************
 * Private Types
 ****************************************************************************/

static inline bool bes_bandwidth_config_builder(acl_bandwitdh_config_t* config,
    uint8_t* cmd, size_t* size, bool enable)
{
    uint8_t* param = cmd;

    UINT8_TO_STREAM(param, 0x3F); /* OGF */
    UINT16_TO_STREAM(param, 0x00D7); /* OCF */

    UINT8_TO_STREAM(param, 0x04); /* Len */
    UINT8_TO_STREAM(param, 0x02); /* Sub-opcode */
    if (enable) {
        UINT8_TO_STREAM(param, 0x01); /* Start */
    } else {
        UINT8_TO_STREAM(param, 0x00); /* Stop */
    }
    UINT16_TO_STREAM(param, config->acl_hdl); /* Connection Handle */

    *size = param - cmd;
    return true;
}

static inline bool bes_dlf_enable_command_builder(le_dlf_config_t* config, uint8_t* data, size_t* size)
{
    uint8_t* param = data;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x00d7); // fill ocf

    // vendor specified fields
    UINT8_TO_STREAM(param, 0x05); // data length
    UINT8_TO_STREAM(param, 0x01); // subcode
    UINT8_TO_STREAM(param, (uint8_t)config->connection_handle);
    UINT8_TO_STREAM(param, 0x00);
    UINT16_TO_STREAM(param, config->dlf_timeout);

    *size = param - data;

    return true;
}

static inline bool bes_dlf_disable_command_builder(le_dlf_config_t* config, uint8_t* data, size_t* size)
{
    uint8_t* param = data;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x00d7); // fill ocf

    // vendor specified fields
    UINT8_TO_STREAM(param, 0x05); // data length
    UINT8_TO_STREAM(param, 0x01); // subcode
    UINT8_TO_STREAM(param, (uint8_t)config->connection_handle);
    UINT8_TO_STREAM(param, 0x01);
    UINT16_TO_STREAM(param, 0x0000);

    *size = param - data;

    return true;
}

#endif /* _BT_CONTROLLER_VENDOR_BES_H__ */
