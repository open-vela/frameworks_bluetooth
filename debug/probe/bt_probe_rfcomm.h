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
#ifndef __BT_PROBE_RFCOMM_H__
#define __BT_PROBE_RFCOMM_H__

#include <stdint.h>

#ifdef CONFIG_BT_PROBE_RFCOMM

/*------------------------------------------------------------------------
 * RFCOMM layer probes — called from the RFCOMM protocol implementation.
 *
 * All probes use pre-decoded metadata (no raw buffer parsing).
 * Only UIH data frames and credit-only frames are probed;
 * SABM/UA/DM/DISC and mux control (DLCI=0) are not recorded.
 *------------------------------------------------------------------------*/

/*
 * TX: called after building a UIH frame, before sending to L2CAP.
 *   @buf                — pointer to the complete RFCOMM frame:
 *                         [addr(1)][ctrl(1)][len(1-2)][credits?][payload][fcs]
 *                         trace internally parses dlci = buf[0] >> 2 and
 *                         payload_len from the length field.
 *   @len                — total RFCOMM frame length (bytes in buf).
 *   @peer_credits       — peer's remaining TX credits (our send quota) BEFORE
 *                         this send consumes one (from tx_credits semaphore).
 */
void bt_probe_rfcomm_tx(const uint8_t *buf, uint16_t len,
                        uint8_t peer_credits);

/*
 * RX: called after receiving and validating a UIH frame.
 *   @dlci               — RFCOMM DLCI (2-61).
 *   @len                — information payload length (0 for credit-only frames).
 *   @local_credits      — our remaining RX credits AFTER this receive.
 *   @peer_credits_given — credits piggybacked by peer in this UIH frame
 *                         (P/F=1 credit byte value), else 0.
 */
void bt_probe_rfcomm_rx(uint8_t dlci, uint16_t len,
                        uint8_t local_credits, uint8_t peer_credits_given);
#else
#define bt_probe_rfcomm_tx(buf, len, peer_credits)
#define bt_probe_rfcomm_rx(dlci, len, local_credits, peer_credits_given)
#endif /* CONFIG_BT_PROBE_RFCOMM */

#endif /* __BT_PROBE_RFCOMM_H__ */
