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
#ifndef __BT_TRACE_SPP_H__
#define __BT_TRACE_SPP_H__

#include "probe/bt_probe_hci.h"
#include "probe/bt_probe_rfcomm.h"
#include "probe/bt_probe_spp.h"

/*------------------------------------------------------------------------
 * event_id encoding (16-bit):
 *
 *   bit [15]    : direction   0 = TX,  1 = RX
 *   bit [14..8] : layer       0 = HCI, 2 = RFCOMM, 4 = SPP, ...
 *   bit [7..0]  : sequence    per-layer event index
 *
 * Filter helpers:
 *   direction  = event_id >> 15
 *   layer      = (event_id >> 8) & 0x7F
 *------------------------------------------------------------------------*/

/* Direction bits */
#define BT_TRACE_DIR_TX         0x01U
#define BT_TRACE_DIR_RX         0x02U
#define BT_TRACE_DIR_ALL        (BT_TRACE_DIR_TX | BT_TRACE_DIR_RX)

/* Layer bits (for layer_mask) */
#define BT_TRACE_LAYER_HCI      (1U << 0)
#define BT_TRACE_LAYER_RFCOMM   (1U << 2)
#define BT_TRACE_LAYER_SDP      (1U << 3)
#define BT_TRACE_LAYER_SPP      (1U << 4)
#define BT_TRACE_LAYER_ALL      0xFFU

#ifdef CONFIG_BT_TRACE_SPP

/**
 * bt_trace_spp_set_direction - Set direction filter.
 * @dir_mask: BT_TRACE_DIR_TX, BT_TRACE_DIR_RX, or BT_TRACE_DIR_ALL.
 */
void bt_trace_spp_set_direction(uint8_t dir_mask);

/**
 * bt_trace_spp_set_layer - Set layer filter.
 * @layer_mask: OR of BT_TRACE_LAYER_xxx bits.
 */
void bt_trace_spp_set_layer(uint8_t layer_mask);

/**
 * bt_trace_spp_set_conn_port - Set SPP port filter (SPP layer only).
 * @port: SPP service port (conn_id) as reported in spp_connection_state_callback.
 *        Since port=0 is a valid value, the enable flag is encoded in bit[15]
 *        of the internal filter field. Use bt_trace_spp_filter_reset() to disable.
 *        For HCI/RFCOMM layer filtering use bt_trace_spp_set_dlci() instead.
 */
void bt_trace_spp_set_conn_port(uint16_t port);

/**
 * bt_trace_spp_set_dlci - Set RFCOMM DLCI filter (HCI and RFCOMM layers).
 * @dlci: Exact RFCOMM DLCI value to match (0 = don't filter).
 *        DLCI = (scn << 1) | direction_bit, valid range 2-61.
 *        Does not affect the conn_port filter.
 */
void bt_trace_spp_set_dlci(uint8_t dlci);

/**
 * bt_trace_spp_set_acl_handle - Set HCI ACL handle filter (HCI layer only).
 * @acl_handle: HCI ACL handle (0 = don't filter).
 */
void bt_trace_spp_set_acl_handle(uint16_t acl_handle);

/**
 * bt_trace_spp_filter_reset - Reset all filters to default (record everything).
 */
void bt_trace_spp_filter_reset(void);

#else

#define bt_trace_spp_set_direction(m)
#define bt_trace_spp_set_layer(m)
#define bt_trace_spp_set_conn_port(p)
#define bt_trace_spp_set_dlci(d)
#define bt_trace_spp_set_acl_handle(h)
#define bt_trace_spp_filter_reset()

#endif /* CONFIG_BT_TRACE_SPP */
#endif /* __BT_TRACE_SPP_H__ */
