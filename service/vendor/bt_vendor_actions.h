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
#ifndef _BT_CONTROLLER_VENDOR_ACTIONS_H__
#define _BT_CONTROLLER_VENDOR_ACTIONS_H__

#include "bt_utils.h"
#include "bt_vendor.h"

#include <stdbool.h>

static inline bool actions_a2dp_offload_start_builder(a2dp_offload_config_t *config,
                                                      uint8_t *offload, size_t *size)
{
    uint8_t *param = offload;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x0000); // fill ocf

    UINT8_TO_STREAM(param, 0x03); // cmd
    UINT8_TO_STREAM(param, 0x02); // offload  start
    UINT16_TO_STREAM(param, config->acl_hdl); // acl handle
    UINT8_TO_STREAM(param, 0x03); // codec type
    UINT16_TO_STREAM(param, config->l2c_rcid); // cid
    UINT16_TO_STREAM(param, config->frame_sample); // frame sample
    UINT16_TO_STREAM(param, 0x0000); // frame length, reserved
    UINT16_TO_STREAM(param, config->mtu); // MTU
    UINT8_TO_STREAM(param, 0x00); // Padding
    UINT8_TO_STREAM(param, 0x00); // Extension
    UINT8_TO_STREAM(param, 0x00); // Marker
    UINT8_TO_STREAM(param, 0x60); // Payload Type
    UINT8_TO_STREAM(param, 0x01); // SSRC

    *size = param - offload;
    return true;
}

static bool actions_a2dp_offload_stop_builder(a2dp_offload_config_t *config,
                                              uint8_t *offload, size_t *size)
{
    uint8_t *param = offload;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x0000); // fill ocf

    UINT8_TO_STREAM(param, 0x03);
    UINT8_TO_STREAM(param, 0x03); // offload  stop

    *size = param - offload;
    return true;
}

#endif /* _BT_CONTROLLER_VENDOR_H__ */
