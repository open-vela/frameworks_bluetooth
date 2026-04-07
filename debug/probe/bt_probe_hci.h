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
#ifndef __BT_PROBE_HCI_H__
#define __BT_PROBE_HCI_H__

#include <stdint.h>

#ifdef CONFIG_BT_PROBE_HCI

/*------------------------------------------------------------------------
 * H4 / IRQ layer probes
 *
 * Buffer format: [pkt_type(1)] [hci_header...] [payload...]
 *   - pkt_type: 0x01=CMD, 0x02=ACL, 0x03=SCO, 0x04=EVT
 *   - buf[0] is always the H4 packet type indicator byte
 *   - len includes the pkt_type byte (total bytes in buf)
 *
 * Call site: H4 transport driver, before/after UART read/write.
 *------------------------------------------------------------------------*/

/* RX: called in IRQ context when a complete packet is received from UART.
 * @pkt_type: H4 packet type (0x02=ACL, 0x04=EVT, etc.).
 * @buf: [hci_header...] [payload...], NO pkt_type prefix.
 * @len: buffer length excluding pkt_type byte.
 */
void bt_probe_hci_h4_rx_irq(uint8_t pkt_type, const uint8_t *buf, uint16_t len);

/* RX: called in task context after H4 packet reassembly is complete.
 * @buf: [pkt_type(1)] [hci_header...] [payload...], pkt_type at buf[0].
 * @len: total buffer length including pkt_type byte.
 */
void bt_probe_hci_h4_rx(const uint8_t *buf, uint16_t len);

/* TX: called after H4 write() returns (data handed to UART).
 * @buf: [pkt_type(1)] [hci_header...] [payload...], pkt_type at buf[0].
 * @len: total buffer length including pkt_type byte.
 */
void bt_probe_hci_h4_tx_done(const uint8_t *buf, uint16_t len);

/*------------------------------------------------------------------------
 * Stack layer probes
 *
 * Buffer format: [hci_header...] [payload...]  — NO pkt_type prefix.
 *   - The packet type is implied by the API name (acl/cmd/sco/evt).
 *   - buf[0] is the first byte of the HCI-specific header.
 *
 * Call site: host stack, after header construction (TX) or before
 *            header stripping (RX).
 *------------------------------------------------------------------------*/

/* TX ACL: called after the 4-byte ACL header is pushed onto the buffer,
 *         just before handing the packet to the controller.
 * @buf: [ACL_handle+flags(2)] [ACL_data_len(2)] [L2CAP...] [RFCOMM...]
 *       buf MUST start with the 4-byte HCI ACL header.
 * @len: total buffer length (ACL header + L2CAP + payload).
 * @acl_quota: remaining controller ACL TX buffer slots (from stack credit
 *             tracking, e.g. semaphore count), BEFORE this packet consumes one.
 */
void bt_probe_hci_stack_tx_acl(const uint8_t *buf, uint16_t len, uint8_t acl_quota);

/* TX CMD: called after HCI command header is constructed.
 * @buf: [opcode(2)] [param_len(1)] [params...]
 * @len: total buffer length.
 */
void bt_probe_hci_stack_tx_cmd(const uint8_t *buf, uint16_t len);

/* TX SCO: called after SCO header is constructed.
 * @buf: [conn_handle(2)] [data_len(1)] [data...]
 * @len: total buffer length.
 */
void bt_probe_hci_stack_tx_sco(const uint8_t *buf, uint16_t len);

/* RX ACL: called BEFORE the ACL header is stripped from the buffer.
 * @buf: [ACL_handle+flags(2)] [ACL_data_len(2)] [L2CAP...] [RFCOMM...]
 *       buf MUST start with the 4-byte HCI ACL header.
 *       Do NOT call this after net_buf_pull_mem() strips the ACL header —
 *       the parser relies on the ACL header at buf[0..3] to extract
 *       handle, PB flags, L2CAP CID, and RFCOMM DLCI at fixed offsets.
 * @len: total buffer length (ACL header + L2CAP + payload).
 */
void bt_probe_hci_stack_rx_acl(const uint8_t *buf, uint16_t len);

/* RX EVT: called with the HCI event header still present in the buffer.
 * @buf: [evt_code(1)] [param_len(1)] [params...]
 *       buf[0] MUST be the HCI event code (e.g. 0x13 for NOCP).
 *       Do NOT call this after the event header has been stripped.
 * @len: total buffer length (event header + parameters).
 * Note: only NOCP (evt_code=0x13) events are recorded; others are ignored.
 */
void bt_probe_hci_stack_rx_evt(const uint8_t *buf, uint16_t len);

/* RX SCO: called with SCO header present.
 * @buf: [conn_handle(2)] [data_len(1)] [data...]
 * @len: total buffer length.
 */
void bt_probe_hci_stack_rx_sco(const uint8_t *buf, uint16_t len);

/*------------------------------------------------------------------------
 * Parsed event probes — pre-decoded fields, no raw buffer.
 *------------------------------------------------------------------------*/

/* Number Of Completed Packets (HCI Event 0x13), called from stack callback.
 * Called once per (handle, 1) pair inside the NOCP handler loop.
 * @handle:        ACL connection handle (host-endian, 12-bit).
 * @num_completed: number of completed packets for this handle (always 1
 *                 when called per-packet in a loop).
 * @acl_quota:     remaining controller ACL TX buffer slots AFTER releasing
 *                 this completed packet (i.e. after k_sem_give).
 */
void bt_probe_hci_nocp(uint16_t handle, uint16_t num_completed, uint16_t acl_quota);

#else

#define bt_probe_hci_h4_rx_irq(pkt_type, buf, len)
#define bt_probe_hci_h4_rx(buf, len)
#define bt_probe_hci_h4_tx_done(buf, len)
#define bt_probe_hci_stack_tx_acl(buf, len, acl_quota)
#define bt_probe_hci_stack_tx_cmd(buf, len)
#define bt_probe_hci_stack_tx_sco(buf, len)
#define bt_probe_hci_stack_rx_acl(buf, len)
#define bt_probe_hci_stack_rx_evt(buf, len)
#define bt_probe_hci_stack_rx_sco(buf, len)
#define bt_probe_hci_nocp(handle, num_completed, acl_quota)

#endif /* CONFIG_BT_PROBE_HCI */

#endif /* __BT_PROBE_HCI_H__ */
