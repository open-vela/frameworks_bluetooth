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
#ifndef _BT_DFX_EVENT_H_
#define _BT_DFX_EVENT_H_

// vela bluetooth event id
#define BT_DFX_BASE_VELA_BLUETOOTH (923020000)

// event group
#define BT_DFXG_BR_GAP (0)
#define BT_DFXG_LE_GAP (500)
#define BT_DFXG_RFCOMM (1000)
#define BT_DFXG_L2CAP (1100)
#define BT_DFXG_GATT (1200)
#define BT_DFXG_A2DP (3000)
#define BT_DFXG_AVRCP (3200)
#define BT_DFXG_HFP (3400)
#define BT_DFXG_LE_AUDIO (4000)
#define BT_DFXG_HID (6000)
#define BT_DFXG_MESH (6100)
#define BT_DFXG_CHANNEL_SOUNDING (7000)
#define BT_DFXG_OTHERS (9000)

// event subcode
// group: BT_DFXG_BR_GAP
#define BT_DFXC_BR_GAP_INQUIRY (0)
#define BT_DFXC_BR_GAP_CONN (100)
#define BT_DFXC_BR_GAP_DISCONN (110)
#define BT_DFXC_BR_GAP_PAIR (200)

// group: BT_DFXG_LE_GAP
#define BT_DFXC_LE_GAP_SCAN (0)
#define BT_DFXC_LE_GAP_CONN (100)
#define BT_DFXC_LE_GAP_DISCONN (110)
#define BT_DFXC_LE_GAP_PAIR (200)

// group: BT_DFXG_A2DP
#define BT_DFXC_A2DP_CONN (0)
#define BT_DFXC_A2DP_OFFLOAD (100)

// group: BT_DFXG_HFP
#define BT_DFXC_HFP_CONN (0)
#define BT_DFXC_HFP_OFFLOAD (100)

// group: BT_DFXG_OTHERS
#define BT_DFXC_OPEN (0)

#define BT_DFX_BUILD_CODE(group, subcode) ((BT_DFX_BASE_VELA_BLUETOOTH) + (group) + (subcode))

#endif /* _BT_DFX_EVENT_H_ */