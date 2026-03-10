/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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

#ifndef __SAL_AURACAST_SINK_INTERFACE_H__
#define __SAL_AURACAST_SINK_INTERFACE_H__

#include "bluetooth.h"

#define BT_SAL_AURACAST_SINK_MSE_DONT_CARE 0x00

typedef struct bt_sal_auracast_sink_param {
    /** Maximum number of subevents that should be used to receive data payloads in each BIS event.
     *  Ranges from 0x01 to 0x1F. `BT_SAL_AURACAST_SINK_MSE_DONT_CARE` if not provided. */
    uint8_t mse;

    /** Synchronization timeout for the BIG, measured in 10 milliseconds. Ranges from 0x000A to
     *  0x4000. */
    uint16_t sync_timeout;

    /** Bitwise value of which BIS is to synchronize, e.g., BIS[x] is synchronized if bit x is set.
     *  The value of x ranges from 0x1 to 0x1F. Bit 0 is reserved. */
    uint32_t bis;

    /** 16-octet code used for deriving the session key for decrypting payloads of BISes in the BIG.
     *  NULL if not encrypted. */
    const uint8_t* broadcast_code;
} bt_sal_auracast_sink_param_t;

/** @brief Initialize the service adaptation layer for periodic advertising. */
bt_status_t bt_sal_auracast_sink_init(void);

/** @brief Clean up the service adaptation layer for periodic advertising. */
bt_status_t bt_sal_auracast_sink_cleanup(void);

/**
 * @brief LE BIG Create Sync command.
 *
 * This command is used to synchronize to a BIG described in the periodic advertising train.
 *
 * @param[in] id The Controller ID.
 * @param[in] sid The Advertising SID of the periodic advertising train.
 * @param[in] addr Public device address, random device address, public identity address, or random
 *                 (static) identity address of the advertiser.
 * @param[in] params The parameters used to synchronize the BIG.
 */
bt_status_t bt_sal_auracast_sink_create_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr, const bt_sal_auracast_sink_param_t* params);

/**
 * @brief LE BIG Terminate Sync command.
 *
 * This command is used to stop synchronizing or cancel the process of synchronizing to the BIG.
 *
 * @param[in] id The Controller ID.
 * @param[in] sid The Advertising SID of the periodic advertising train.
 * @param[in] addr Public device address, random device address, public identity address, or random
 *                 (static) identity address of the advertiser.
 */
bt_status_t bt_sal_auracast_sink_terminate_sync(bt_controller_id_t id, uint8_t sid,
    const bt_le_address_t* addr);

#endif /* __SAL_AURACAST_SINK_INTERFACE_H__ */