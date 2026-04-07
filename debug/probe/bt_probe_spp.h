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
#ifndef __BT_PROBE_SPP_H__
#define __BT_PROBE_SPP_H__

#include <stdint.h>

#ifdef CONFIG_BT_PROBE_SPP

/*------------------------------------------------------------------------
 * SPP service layer probes — called from spp_service.c
 *
 * All probes use pre-decoded metadata (no raw buffer parsing).
 * @port: SPP connection identifier (device->conn_id), as reported in
 *        spp_connection_state_callback.  Note: port=0 is a valid value.
 *------------------------------------------------------------------------*/

/* TX: called when app data is read from the pipe, before any stack write.
 * This is an instant marker only — the actual send begins at tx_send.
 * Multiple tx_start events may fire before the first tx_done arrives
 * (async: pipe reads are faster than stack completions).
 * @port: SPP conn_id.
 * @len:  total bytes read from app pipe (may be split into multiple
 *        bt_sal_spp_write() calls if larger than RFCOMM MTU).
 */
void bt_probe_spp_tx_start(uint16_t port, uint16_t len);

/* TX: called after each bt_sal_spp_write() succeeds (one RFCOMM fragment).
 * This marks the real send point — use as slice begin in visualization.
 * Paired with tx_done (slice end) for per-fragment send latency.
 * @port:  SPP conn_id.
 * @len:   fragment size sent to stack in this write call.
 * @quota: remaining RFCOMM TX credits BEFORE this send decrements one.
 *         quota=1 means this is the last credit — next send will block.
 */
void bt_probe_spp_tx_send(uint16_t port, uint16_t len, uint8_t quota);

/* TX: called on outgoing-complete callback (stack confirms one fragment sent).
 * This marks the send completion — use as slice end in visualization.
 * The interval tx_send→tx_done includes RFCOMM→HCI→H4→NOCP round-trip.
 * @port:  SPP conn_id.
 * @quota: remaining RFCOMM TX credits AFTER this completion restores one.
 */
void bt_probe_spp_tx_done(uint16_t port, uint8_t quota);

/* RX: called when data arrives from stack (spp_on_incoming_data_received),
 *     before writing to the app pipe.
 * @port: SPP conn_id.
 * @len:  bytes received from RFCOMM in this callback.
 */
void bt_probe_spp_rx_start(uint16_t port, uint16_t len);

/* RX: called when euv_pipe_write completes (data delivered to app pipe).
 * @port: SPP conn_id.
 * Note: no length parameter — the length was already recorded by rx_start.
 */
void bt_probe_spp_rx_done(uint16_t port);
#else
#define bt_probe_spp_tx_start(port, len)
#define bt_probe_spp_tx_send(port, len, quota)
#define bt_probe_spp_tx_done(port, quota)
#define bt_probe_spp_rx_start(port, len)
#define bt_probe_spp_rx_done(port)
#endif /* CONFIG_BT_PROBE_SPP */

#endif /* __BT_PROBE_SPP_H__ */
