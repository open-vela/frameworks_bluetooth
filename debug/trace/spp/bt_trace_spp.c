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

/**
 * SPP Trace Specification — probe implementations for HCI / RFCOMM / SPP.
 *
 * Pure System Trace backend: all events are written via bt_trace_write_var()
 * which calls sched_note_event().  No IRQ FIFO, no merged records.
 * noteram driver handles timestamps and supports IRQ-context writes.
 *
 * event_id encoding (16-bit):
 *   bit [15]    : direction   0 = TX,  1 = RX
 *   bit [14..8] : layer       0 = HCI, 2 = RFCOMM, 4 = SPP
 *   bit [7..0]  : sequence    per-layer event index
 *
 * event_id table:
 *   0x0000  HCI_STACK_TX_ACL   — stack enqueues ACL to H4
 *   0x0003  HCI_H4_TX_DONE     — H4 write() returns (data handed to UART)
 *   0x0004  HCI_NOCP           — Number Of Completed Packets (TX flow-control)
 *   0x8000  HCI_IRQ_RX_ACL     — IRQ receives ACL (uart_bth4 driver)
 *   0x8001  HCI_H4_RX_ACL      — H4 receives ACL (task context)
 *   0x8002  HCI_STACK_RX_ACL   — stack receives ACL packet
 *   0x8003  HCI_STACK_RX_EVT   — stack receives HCI Event (NOCP)
 *   0x8005  HCI_IRQ_RX_EVT     — IRQ receives HCI Event (uart_bth4 driver)
 *   0x8006  HCI_H4_RX_EVT      — H4 receives HCI Event (task context)
 *
 * All payloads are standard 9-byte format (no merged 13-byte records).
 */

#include "bt_trace_spp.h"
#include "bt_event_trace.h"

#include "utils/log.h"
#include <string.h>

/* H4 packet-type indicator is always 1 byte */
#define H4_HEADER_SIZE 1

/* Early-exit macro: check event filter before building payload */
#define TRACE_FILTER_OR_RETURN(event_id) \
    do {                                 \
        if (!spp_filter_event(event_id)) \
            return;                      \
    } while (0)

/*------------------------------------------------------------------------
 * event_id definitions
 *------------------------------------------------------------------------*/

#define _LAYER_HCI 0U
#define _LAYER_RFCOMM 2U
#define _LAYER_SPP 4U

#define _EVT(dir, layer, seq) \
    (((uint16_t)(dir) << 15) | ((uint16_t)(layer) << 8) | (uint16_t)(seq))

/* HCI TX — stack layer */
#define BT_EVT_HCI_STACK_TX_ACL _EVT(0, _LAYER_HCI, 0) /* 0x0000 */
#define BT_EVT_HCI_STACK_TX_CMD _EVT(0, _LAYER_HCI, 1) /* 0x0001 */
#define BT_EVT_HCI_STACK_TX_SCO _EVT(0, _LAYER_HCI, 2) /* 0x0002 */

/* HCI TX — H4 transport layer */
#define BT_EVT_HCI_H4_TX_DONE _EVT(0, _LAYER_HCI, 3) /* 0x0003 */

/* HCI TX — parsed events */
#define BT_EVT_HCI_NOCP _EVT(0, _LAYER_HCI, 4) /* 0x0004 */

/* HCI RX — IRQ layer (uart_bth4 driver) */
#define BT_EVT_HCI_IRQ_RX_ACL _EVT(1, _LAYER_HCI, 0) /* 0x8000 */

/* HCI RX — H4 layer */
#define BT_EVT_HCI_H4_RX_ACL _EVT(1, _LAYER_HCI, 1) /* 0x8001 */

/* HCI RX — stack layer */
#define BT_EVT_HCI_STACK_RX_ACL _EVT(1, _LAYER_HCI, 2) /* 0x8002 */
#define BT_EVT_HCI_STACK_RX_EVT _EVT(1, _LAYER_HCI, 3) /* 0x8003 */
#define BT_EVT_HCI_STACK_RX_SCO _EVT(1, _LAYER_HCI, 4) /* 0x8004 */

/* HCI RX — IRQ EVT */
#define BT_EVT_HCI_IRQ_RX_EVT _EVT(1, _LAYER_HCI, 5) /* 0x8005 */

/* HCI RX — H4 EVT */
#define BT_EVT_HCI_H4_RX_EVT _EVT(1, _LAYER_HCI, 6) /* 0x8006 */

/* RFCOMM TX / RX */
#define BT_EVT_RFCOMM_TX _EVT(0, _LAYER_RFCOMM, 0) /* 0x0200 */
#define BT_EVT_RFCOMM_RX _EVT(1, _LAYER_RFCOMM, 0) /* 0x8200 */

/* SPP TX */
#define BT_EVT_SPP_TX_START _EVT(0, _LAYER_SPP, 0) /* 0x0400 */
#define BT_EVT_SPP_TX_SEND _EVT(0, _LAYER_SPP, 1) /* 0x0401 */
#define BT_EVT_SPP_TX_DONE _EVT(0, _LAYER_SPP, 2) /* 0x0402 */

/* SPP RX */
#define BT_EVT_SPP_RX_START _EVT(1, _LAYER_SPP, 0) /* 0x8400 */
#define BT_EVT_SPP_RX_DONE _EVT(1, _LAYER_SPP, 1) /* 0x8401 */

/*------------------------------------------------------------------------
 * Filter state
 *------------------------------------------------------------------------*/

#define PORT_FILTER_EN 0x8000U

typedef struct {
    uint8_t dir_mask;
    uint8_t layer_mask;
    uint8_t dlci;
    uint16_t port;
    uint16_t acl_handle;
} spp_trace_filter_t;

static spp_trace_filter_t g_filter = {
    .dir_mask = BT_TRACE_DIR_ALL,
    .layer_mask = BT_TRACE_LAYER_ALL,
    .dlci = 0,
    .port = 0,
    .acl_handle = 0,
};

static inline uint8_t layer_idx_to_bit(uint8_t layer_idx)
{
    return 1U << layer_idx;
}

static inline bool spp_filter_event(uint16_t event_id)
{
    uint8_t dir_bit = (event_id & 0x8000U) ? BT_TRACE_DIR_RX : BT_TRACE_DIR_TX;
    if (!(g_filter.dir_mask & dir_bit))
        return false;

    uint8_t layer_idx = (event_id >> 8) & 0x7FU;
    if (!(g_filter.layer_mask & layer_idx_to_bit(layer_idx)))
        return false;

    return true;
}

static inline bool spp_filter_hci_conn(uint8_t pkt_type,
    const uint8_t* hdr, uint16_t hdr_len)
{
    if (g_filter.acl_handle == 0)
        return true;

    if (pkt_type == 0x02) {
        if (hdr_len < 4)
            return true;
        uint16_t handle = ((uint16_t)hdr[1] << 8 | hdr[0]) & 0x0FFFU;
        if (handle != g_filter.acl_handle)
            return false;
        return true;
    }

    if (pkt_type == 0x04) {
        if (hdr_len >= 5 && hdr[0] == 0x13) {
            uint16_t handle = ((uint16_t)hdr[4] << 8 | hdr[3]) & 0x0FFFU;
            if (handle != g_filter.acl_handle)
                return false;
        }
        return true;
    }

    return true;
}

/*------------------------------------------------------------------------
 * Filter management API
 *------------------------------------------------------------------------*/

void bt_trace_spp_set_direction(uint8_t dir_mask)
{
    g_filter.dir_mask = dir_mask & BT_TRACE_DIR_ALL;
}

void bt_trace_spp_set_layer(uint8_t layer_mask)
{
    g_filter.layer_mask = layer_mask;
}

void bt_trace_spp_set_conn_port(uint16_t port)
{
    g_filter.port = PORT_FILTER_EN | (port & 0x7FFFU);
}

void bt_trace_spp_set_dlci(uint8_t dlci)
{
    g_filter.dlci = dlci;
}

void bt_trace_spp_set_acl_handle(uint16_t acl_handle)
{
    g_filter.acl_handle = acl_handle;
}

void bt_trace_spp_filter_reset(void)
{
    g_filter.dir_mask = BT_TRACE_DIR_ALL;
    g_filter.layer_mask = BT_TRACE_LAYER_ALL;
    g_filter.dlci = 0;
    g_filter.port = 0;
    g_filter.acl_handle = 0;
}

/*------------------------------------------------------------------------
 * HCI probe implementations
 *------------------------------------------------------------------------*/

/*
 * Write HCI EVT packet record.  Standard 9-byte payload.
 * Payload: [0-1] pkt_len(2) + [2-8] header(7)
 */
static inline void hci_write_evt_record(uint16_t event_id,
    const uint8_t* hdr, uint16_t hdr_len,
    uint16_t total_len)
{
    uint8_t payload[9];
    uint8_t copy;

    payload[0] = (uint8_t)(total_len & 0xFF);
    payload[1] = (uint8_t)(total_len >> 8);

    copy = (hdr_len > 7) ? 7 : (uint8_t)hdr_len;
    if (copy > 0)
        memcpy(&payload[2], hdr, copy);
    if (copy < 7)
        memset(&payload[2 + copy], 0, 7 - copy);

    bt_trace_write_var(event_id, payload, sizeof(payload));
}

/*
 * Write HCI ACL packet record with structured RFCOMM fields.
 * Standard 9-byte payload.
 *
 * Payload: [0-1] pkt_len(2) + [2-3] acl_handle(2)
 *          + [4] dlci(1) + [5-6] info_len(2) + [7] credits(1)
 *          + [8] acl_quota(1)
 *
 * Returns true if a record was written, false if skipped.
 */
static inline bool hci_write_acl_record(uint16_t event_id,
    const uint8_t* hdr, uint16_t hdr_len,
    uint16_t total_len, uint8_t acl_quota)
{
    uint8_t payload[9];
    uint16_t acl_handle;
    uint8_t dlci, control, len_bytes;
    uint16_t info_len = 0;
    uint8_t credits = 0;

    if (hdr_len < 2)
        return false;
    if (((hdr[1] >> 4) & 0x03) == 0x01) {
        return false;
    }
    if (hdr_len < 11) {
        return false;
    }

    uint16_t cid = (uint16_t)hdr[7] << 8 | hdr[6];
    if (cid < 0x0040) {
        return false;
    }

    dlci = hdr[8] >> 2;
    if (dlci == 0) {
        return false;
    }
    if (g_filter.dlci && dlci != g_filter.dlci) {
        return false;
    }

    acl_handle = ((uint16_t)hdr[1] << 8 | hdr[0]) & 0x0FFFU;
    control = hdr[9];

    if ((control & 0xEF) != 0xEF) {
        return false;
    }

    if (hdr[10] & 0x01) {
        len_bytes = 1;
        info_len = hdr[10] >> 1;
    } else {
        len_bytes = 2;
        if (hdr_len < 12) {
            return false;
        }
        info_len = (hdr[10] >> 1) | ((uint16_t)hdr[11] << 7);
    }

    if (control & 0x10) {
        uint8_t credit_off = 10 + len_bytes;
        if (credit_off < hdr_len)
            credits = hdr[credit_off];
        else {
            return false;
        }
    }

    payload[0] = (uint8_t)(total_len & 0xFF);
    payload[1] = (uint8_t)(total_len >> 8);
    payload[2] = (uint8_t)(acl_handle & 0xFF);
    payload[3] = (uint8_t)(acl_handle >> 8);
    payload[4] = dlci;
    payload[5] = (uint8_t)(info_len & 0xFF);
    payload[6] = (uint8_t)(info_len >> 8);
    payload[7] = credits;
    payload[8] = acl_quota;

    bt_trace_write_var(event_id, payload, sizeof(payload));
    return true;
}

/*
 * IRQ probe: directly write to systrace via bt_trace_write_var().
 * noteram uses spinlock and supports IRQ context — no FIFO needed.
 * Writes standard 9-byte ACL or EVT records.
 */
void bt_probe_hci_h4_rx_irq(uint8_t pkt_type, const uint8_t* buf, uint16_t len)
{
    TRACE_FILTER_OR_RETURN(BT_EVT_HCI_IRQ_RX_ACL);

    if (!buf || len < 1)
        return;

    if (pkt_type == 0x02) {
        if (!spp_filter_hci_conn(0x02, buf, len))
            return;
        hci_write_acl_record(BT_EVT_HCI_IRQ_RX_ACL, buf, len, len + 1, 0);
    } else if (pkt_type == 0x04) {
        if (len < 1 || buf[0] != 0x13)
            return;
        if (!spp_filter_hci_conn(0x04, buf, len))
            return;
        hci_write_evt_record(BT_EVT_HCI_IRQ_RX_EVT, buf, len, len + 1);
    }
}

/*
 * H4 task-context probe: standard 9-byte records.
 * No FIFO pop, no merged records.
 */
void bt_probe_hci_h4_rx(const uint8_t* buf, uint16_t len)
{
    uint8_t pkt_type;

    TRACE_FILTER_OR_RETURN(BT_EVT_HCI_H4_RX_ACL);

    if (!buf || len < 1)
        return;

    pkt_type = buf[0];

    if (pkt_type == 0x02) {
        if (!spp_filter_hci_conn(0x02, buf + 1, len - 1))
            return;
        hci_write_acl_record(BT_EVT_HCI_H4_RX_ACL,
            buf + 1, len - 1, len, 0);
    } else if (pkt_type == 0x04) {
        if (len < 2 || buf[1] != 0x13)
            return;
        if (!spp_filter_hci_conn(0x04, buf + 1, len - 1))
            return;
        hci_write_evt_record(BT_EVT_HCI_H4_RX_EVT,
            buf + 1, len - 1, len);
    }
}

void bt_probe_hci_h4_tx_done(const uint8_t* buf, uint16_t len)
{
    TRACE_FILTER_OR_RETURN(BT_EVT_HCI_H4_TX_DONE);

    if (!buf || len < 1)
        return;
    if (buf[0] != 0x02)
        return;
    if (!spp_filter_hci_conn(0x02, buf + 1, len - 1))
        return;

    hci_write_acl_record(BT_EVT_HCI_H4_TX_DONE,
        buf + 1, len - 1, len, 0);
}

/* --- Stack layer probes --- */

void bt_probe_hci_stack_tx_acl(const uint8_t* buf, uint16_t len, uint8_t acl_quota)
{
    TRACE_FILTER_OR_RETURN(BT_EVT_HCI_STACK_TX_ACL);

    if (!spp_filter_hci_conn(0x02, buf, len))
        return;

    hci_write_acl_record(BT_EVT_HCI_STACK_TX_ACL,
        buf, len, len, acl_quota);
}

void bt_probe_hci_stack_rx_acl(const uint8_t* buf, uint16_t len)
{
    TRACE_FILTER_OR_RETURN(BT_EVT_HCI_STACK_RX_ACL);

    if (!spp_filter_hci_conn(0x02, buf, len))
        return;

    hci_write_acl_record(BT_EVT_HCI_STACK_RX_ACL,
        buf, len, len, 0);
}

void bt_probe_hci_stack_rx_evt(const uint8_t* buf, uint16_t len)
{
    /* Only record NOCP events (evt_code = 0x13) */
    if (!buf || len < 1 || buf[0] != 0x13)
        return;

    TRACE_FILTER_OR_RETURN(BT_EVT_HCI_STACK_RX_EVT);

    if (!spp_filter_hci_conn(0x04, buf, len))
        return;

    hci_write_evt_record(BT_EVT_HCI_STACK_RX_EVT,
        buf, len, len);
}

/* Not needed for SPP trace — empty implementations */
void bt_probe_hci_stack_tx_cmd(const uint8_t* buf, uint16_t len)
{
    (void)buf;
    (void)len;
}

void bt_probe_hci_stack_tx_sco(const uint8_t* buf, uint16_t len)
{
    (void)buf;
    (void)len;
}

void bt_probe_hci_stack_rx_sco(const uint8_t* buf, uint16_t len)
{
    (void)buf;
    (void)len;
}

/* --- Parsed NOCP --- */

void bt_probe_hci_nocp(uint16_t handle, uint16_t num_completed,
    uint16_t acl_quota)
{
    uint8_t payload[6];

    TRACE_FILTER_OR_RETURN(BT_EVT_HCI_NOCP);

    if (g_filter.acl_handle && handle != g_filter.acl_handle)
        return;

    payload[0] = (uint8_t)(handle & 0xFF);
    payload[1] = (uint8_t)(handle >> 8);
    payload[2] = (uint8_t)(num_completed & 0xFF);
    payload[3] = (uint8_t)(num_completed >> 8);
    payload[4] = (uint8_t)(acl_quota & 0xFF);
    payload[5] = (uint8_t)(acl_quota >> 8);

    bt_trace_write_var(BT_EVT_HCI_NOCP, payload, sizeof(payload));
}

/*------------------------------------------------------------------------
 * RFCOMM probe implementations
 *------------------------------------------------------------------------*/

void bt_probe_rfcomm_tx(const uint8_t* buf, uint16_t len,
    uint8_t peer_credits)
{
    uint8_t payload[5];
    uint8_t dlci;
    uint16_t data_len;

    TRACE_FILTER_OR_RETURN(BT_EVT_RFCOMM_TX);

    if (!buf || len < 3)
        return;

    /* Parse RFCOMM frame header from raw buffer */
    dlci = buf[0] >> 2;

    if (g_filter.dlci && dlci != g_filter.dlci)
        return;

    /* Parse length field: check EA bit (bit 0) */
    uint8_t len_bytes;
    uint8_t local_credits_given = 0;

    if (buf[2] & 0x01) {
        len_bytes = 1;
        data_len = buf[2] >> 1;
    } else {
        if (len < 4)
            return;
        len_bytes = 2;
        data_len = (buf[2] >> 1) | ((uint16_t)buf[3] << 7);
    }

    /* Extract piggybacked credits if P/F bit is set (control & 0x10) */
    if (buf[1] & 0x10) {
        uint8_t credit_off = 2 + len_bytes;
        if (credit_off < len)
            local_credits_given = buf[credit_off];
    }

    payload[0] = dlci;
    payload[1] = (uint8_t)(data_len & 0xFF);
    payload[2] = (uint8_t)(data_len >> 8);
    payload[3] = peer_credits;
    payload[4] = local_credits_given;

    bt_trace_write_var(BT_EVT_RFCOMM_TX, payload, sizeof(payload));
}

void bt_probe_rfcomm_rx(uint8_t dlci, uint16_t len,
    uint8_t local_credits, uint8_t peer_credits_given)
{
    uint8_t payload[5];

    TRACE_FILTER_OR_RETURN(BT_EVT_RFCOMM_RX);

    if (g_filter.dlci && dlci != g_filter.dlci)
        return;

    payload[0] = dlci;
    payload[1] = (uint8_t)(len & 0xFF);
    payload[2] = (uint8_t)(len >> 8);
    payload[3] = local_credits;
    payload[4] = peer_credits_given;

    bt_trace_write_var(BT_EVT_RFCOMM_RX, payload, sizeof(payload));
}

/*------------------------------------------------------------------------
 * SPP probe implementations
 *------------------------------------------------------------------------*/

void bt_probe_spp_tx_start(uint16_t port, uint16_t len)
{
    uint8_t payload[4];

    TRACE_FILTER_OR_RETURN(BT_EVT_SPP_TX_START);

    if ((g_filter.port & PORT_FILTER_EN) && port != (g_filter.port & 0x7FFFU))
        return;

    payload[0] = (uint8_t)(port & 0xFF);
    payload[1] = (uint8_t)(port >> 8);
    payload[2] = (uint8_t)(len & 0xFF);
    payload[3] = (uint8_t)(len >> 8);

    bt_trace_write_var(BT_EVT_SPP_TX_START, payload, sizeof(payload));
}

void bt_probe_spp_tx_send(uint16_t port, uint16_t len, uint8_t quota)
{
    uint8_t payload[5];

    TRACE_FILTER_OR_RETURN(BT_EVT_SPP_TX_SEND);

    if ((g_filter.port & PORT_FILTER_EN) && port != (g_filter.port & 0x7FFFU))
        return;

    payload[0] = (uint8_t)(port & 0xFF);
    payload[1] = (uint8_t)(port >> 8);
    payload[2] = (uint8_t)(len & 0xFF);
    payload[3] = (uint8_t)(len >> 8);
    payload[4] = quota;

    bt_trace_write_var(BT_EVT_SPP_TX_SEND, payload, sizeof(payload));
}

void bt_probe_spp_tx_done(uint16_t port, uint8_t quota)
{
    uint8_t payload[3];

    TRACE_FILTER_OR_RETURN(BT_EVT_SPP_TX_DONE);

    if ((g_filter.port & PORT_FILTER_EN) && port != (g_filter.port & 0x7FFFU))
        return;

    payload[0] = (uint8_t)(port & 0xFF);
    payload[1] = (uint8_t)(port >> 8);
    payload[2] = quota;

    bt_trace_write_var(BT_EVT_SPP_TX_DONE, payload, sizeof(payload));
}

void bt_probe_spp_rx_start(uint16_t port, uint16_t len)
{
    uint8_t payload[4];

    TRACE_FILTER_OR_RETURN(BT_EVT_SPP_RX_START);

    if ((g_filter.port & PORT_FILTER_EN) && port != (g_filter.port & 0x7FFFU))
        return;

    payload[0] = (uint8_t)(port & 0xFF);
    payload[1] = (uint8_t)(port >> 8);
    payload[2] = (uint8_t)(len & 0xFF);
    payload[3] = (uint8_t)(len >> 8);

    bt_trace_write_var(BT_EVT_SPP_RX_START, payload, sizeof(payload));
}

void bt_probe_spp_rx_done(uint16_t port)
{
    uint8_t payload[2];

    TRACE_FILTER_OR_RETURN(BT_EVT_SPP_RX_DONE);

    if ((g_filter.port & PORT_FILTER_EN) && port != (g_filter.port & 0x7FFFU))
        return;

    payload[0] = (uint8_t)(port & 0xFF);
    payload[1] = (uint8_t)(port >> 8);

    bt_trace_write_var(BT_EVT_SPP_RX_DONE, payload, sizeof(payload));
}
