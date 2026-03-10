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

#ifndef __SAL_PA_SYNC_INTERFACE_H__
#define __SAL_PA_SYNC_INTERFACE_H__

#include "bluetooth.h"

#ifndef SAL_BIT
#define SAL_BIT(x) (1UL << (x))
#endif

/** Bit 0 in Options:
 *  - 0: Use the Advertising_SID, Advertiser_Address_Type, and Advertiser_Address parameters to
 *       determine which advertiser to listen to
 *  - 1: Use the Periodic Advertiser List to determine which advertiser to listen to
 *       See @ref bt_sal_pa_list_add */
#define BT_SAL_PA_SYNC_OPTION_USE_LIST SAL_BIT(0)

/** Bit 1 in Options:
 *  - 0: Reporting initially enabled
 *  - 1: Reporting initially disabled */
#define BT_SAL_PA_SYNC_OPTION_REPORTING_DISABLED SAL_BIT(1)

/** Bit 2 in Options:
 *  - 0: Duplicate filtering initially disabled
 *  - 1: Duplicate filtering initially enabled */
#define BT_SAL_PA_SYNC_OPTION_FILTER_ENABLED SAL_BIT(2)

/** Bit 0 in Sync_CTE_Type:
 *  - 1: Do not sync to packets with an AoA Constant Tone Extension */
#define BT_SAL_PA_SYNC_CTE_TYPE_NO_AOA SAL_BIT(0)

/** Bit 1 in Sync_CTE_Type:
 *  - 1: Do not sync to packets with an AoD Constant Tone Extension with 1 μs slots */
#define BT_SAL_PA_SYNC_CTE_TYPE_NO_AOD_1US SAL_BIT(1)

/** Bit 2 in Sync_CTE_Type:
 *  - 1: Do not sync to packets with an AoD Constant Tone Extension with 2 μs slots */
#define BT_SAL_PA_SYNC_CTE_TYPE_NO_AOD_2US SAL_BIT(2)

/** Bit 3 in Sync_CTE_Type: (RFU)
 *  - 1: Do not sync to packets with a type 3 Constant Tone Extension */
#define BT_SAL_PA_SYNC_CTE_TYPE_NO_AOD_TYPE3 SAL_BIT(3)

/** Bit 4 in Sync_CTE_Type:
 *  - 1: Do not sync to packets without a Constant Tone Extension */
#define BT_SAL_PA_SYNC_CTE_TYPE_CTE_ONLY SAL_BIT(4)

typedef struct bt_sal_pa_sync_param {
    /** Options, e.g., BT_SAL_PA_SYNC_OPTION_USE_LIST */
    uint8_t options;

    /** `Advertising_SID`, refers to the Advertising SID subfield in the `ADI` field used to
     *  identify the Periodic Advertising. Range from 0x00 to 0x0F. Only valid when
     *  `BT_SAL_PA_SYNC_OPTION_USE_LIST` is not set in `options`, otherwise ignored */
    uint8_t sid;

    /** `Advertiser_Address` and `Advertiser_Address_Type` of the remote device. Only valid when
     *  `BT_SAL_PA_SYNC_OPTION_USE_LIST` is not set in `options`, otherwise ignored */
    bt_le_address_t addr;

    /** The maximum number of periodic advertising events that can be skipped after a successful
     *  reception. Range from 0x0000 to 0x01F3 */
    uint16_t skip;

    /** Synchronization timeout for the periodic advertising train, measured in 10 ms units.
     *  Range from 0x000A to 0x4000 */
    uint16_t timeout;

    /** Sync_CTE_Type, e.g., BT_SAL_PA_SYNC_CTE_TYPE_NO_AOA */
    uint8_t cte;
} bt_sal_pa_sync_param_t;

/**
 * @brief LE Periodic Advertising Create Sync command.
 *
 * This command is used to synchronize with a single periodic advertising train from an advertiser
 * and begin receiving periodic advertising packets.
 *
 * @param[in] id The Controller ID.
 * @param[in] params The parameters used to synchronize the periodic advertising train.
 *
 * @note A @ref SYNC_ESTABLISHED event, associated with a `handle`, shall be generated when starting
 *       to receive periodic advertising packets.
 * @note A @ref SYNC_TERMINATED event shall be generated if the sync fails to establish.
 * @note A @ref SYNC_REPORT event may be generated when a periodic advertising packet is received.
 */
bt_status_t bt_sal_pa_create_sync(bt_controller_id_t id, const bt_sal_pa_sync_param_t* params);

/**
 * @brief LE Periodic Advertising Terminate Sync command.
 *
 * This command is used to stop reception of the periodic advertising train identified by the
 * `handle`.
 *
 * @param[in] id The Controller ID.
 * @param[in] sid The Advertising SID.
 * @param[in] addr Public device address, random device address, public identity address, or random
 *                 (static) identity address of the advertiser.
 *
 * @note A @ref SYNC_TERMINATED event shall be generated when this command is completed.
 */
bt_status_t bt_sal_pa_terminate_sync(bt_controller_id_t id, uint8_t sid, 
    const bt_le_address_t* addr);

/**
 * @brief LE Add Device To Periodic Advertiser List command.
 *
 * This command is used to add an entry, consisting of a single device address and SID, to the
 * Periodic Advertiser list stored in the Controller.
 *
 * @param[in] id The Controller ID.
 * @param[in] sid The Advertising SID.
 * @param[in] addr Public device address, random device address, public identity address, or random
 *                 (static) identity address of the advertiser.
 */
bt_status_t bt_sal_pa_list_add(bt_controller_id_t id, uint8_t sid, const bt_le_address_t* addr);

/**
 * @brief LE Remove Device From Periodic Advertiser List command.
 *
 * This command is used to remove one entry from the list of Periodic Advertisers stored in the
 * Controller.
 *
 * @param[in] id The Controller ID.
 * @param[in] sid The Advertising SID.
 * @param[in] addr Public device address, random device address, public identity address, or random
 *                 (static) identity address of the advertiser.
 */
bt_status_t bt_sal_pa_list_remove(bt_controller_id_t id, uint8_t sid, const bt_le_address_t* addr);

/**
 * @brief LE Clear Periodic Advertiser List command.
 *
 * This command is used to remove all entries from the list of periodic advertisers in the
 * Controller.
 *
 * @param[in] id The Controller ID.
 */
bt_status_t bt_sal_pa_list_clear(bt_controller_id_t id);

#endif /* __SAL_PA_SYNC_INTERFACE_H__ */