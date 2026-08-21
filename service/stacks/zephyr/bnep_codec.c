#include "bnep_codec.h"

#include <string.h>

static inline void put_be16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v & 0xff);
}

static inline uint16_t get_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

int bnep_encode_setup_req(uint8_t *out, size_t cap,
                          uint16_t dst_uuid16, uint16_t src_uuid16)
{
    if (cap < 7) { return BNEP_ERR_NOSPACE; }

    out[0] = BNEP_CONTROL;
    out[1] = BNEP_CTRL_SETUP_CONN_REQ;
    out[2] = 2;                       /* UUID Size: ONE byte, not two */
    put_be16(&out[3], dst_uuid16);
    put_be16(&out[5], src_uuid16);
    return 7;
}

int bnep_encode_setup_rsp(uint8_t *out, size_t cap, uint16_t rsp_code)
{
    if (cap < 4) { return BNEP_ERR_NOSPACE; }

    out[0] = BNEP_CONTROL;
    out[1] = BNEP_CTRL_SETUP_CONN_RSP;
    put_be16(&out[2], rsp_code);      /* no length field exists here */
    return 4;
}

int bnep_encode_filter_rsp(uint8_t *out, size_t cap,
                           uint8_t rsp_msg_type, uint16_t rsp_code)
{
    if (rsp_msg_type != BNEP_CTRL_FILTER_NET_TYPE_RSP &&
        rsp_msg_type != BNEP_CTRL_FILTER_MULTI_ADDR_RSP) {
        return BNEP_ERR_BADTYPE;
    }
    if (cap < 4) { return BNEP_ERR_NOSPACE; }

    out[0] = BNEP_CONTROL;
    out[1] = rsp_msg_type;
    put_be16(&out[2], rsp_code);
    return 4;
}

int bnep_encode_cmd_not_understood(uint8_t *out, size_t cap,
                                   uint8_t unknown_type)
{
    if (cap < 3) { return BNEP_ERR_NOSPACE; }

    out[0] = BNEP_CONTROL;
    out[1] = BNEP_CTRL_CMD_NOT_UNDERSTOOD;
    out[2] = unknown_type;
    return 3;
}

int bnep_parse_control(const uint8_t *ctrl, size_t len,
                       struct bnep_control *info)
{
    if (!ctrl || !info || len < 1) { return BNEP_ERR_TRUNCATED; }

    memset(info, 0, sizeof(*info));
    info->msg_type = ctrl[0];

    switch (ctrl[0]) {
    case BNEP_CTRL_CMD_NOT_UNDERSTOOD:
        if (len < 2) { return BNEP_ERR_TRUNCATED; }
        info->unknown_type = ctrl[1];
        return BNEP_OK;

    case BNEP_CTRL_SETUP_CONN_REQ: {
        if (len < 2) { return BNEP_ERR_TRUNCATED; }
        uint8_t usz = ctrl[1];
        if (usz != 2 && usz != 4 && usz != 16) { return BNEP_ERR_BADUUID; }
        if (len < (size_t)(2 + 2 * usz)) { return BNEP_ERR_TRUNCATED; }
        info->uuid_size = usz;
        /* The 16-bit value lives in the last two bytes of a 2- or 4-byte
         * UUID, and at offset 2..3 of a 128-bit UUID (Bluetooth Base UUID
         * layout: 0000xxxx-0000-1000-8000-00805F9B34FB). */
        const uint8_t *d = &ctrl[2];
        const uint8_t *s = &ctrl[2 + usz];
        info->dst_uuid16 = (usz == 16) ? get_be16(&d[2]) : get_be16(&d[usz - 2]);
        info->src_uuid16 = (usz == 16) ? get_be16(&s[2]) : get_be16(&s[usz - 2]);
        return BNEP_OK;
    }

    case BNEP_CTRL_SETUP_CONN_RSP:
        if (len < 3) { return BNEP_ERR_TRUNCATED; }
        info->rsp_code = get_be16(&ctrl[1]);
        info->is_success = (info->rsp_code == BNEP_RSP_SUCCESS);
        return BNEP_OK;

    case BNEP_CTRL_FILTER_NET_TYPE_RSP:
    case BNEP_CTRL_FILTER_MULTI_ADDR_RSP:
        if (len < 3) { return BNEP_ERR_TRUNCATED; }
        info->rsp_code = get_be16(&ctrl[1]);
        return BNEP_OK;

    case BNEP_CTRL_FILTER_NET_TYPE_SET:
    case BNEP_CTRL_FILTER_MULTI_ADDR_SET:
        /* We never negotiate filters; the caller answers UNSUPPORTED. */
        return BNEP_OK;

    default:
        return BNEP_ERR_BADTYPE;
    }
}

int bnep_encode_eth(uint8_t *out, size_t cap,
                    const uint8_t *eth_frame, size_t eth_len,
                    const uint8_t local_mac[6], const uint8_t peer_mac[6],
                    bool compress)
{
    if (!out || !eth_frame || !local_mac || !peer_mac) {
        return BNEP_ERR_TRUNCATED;
    }
    if (eth_len < BNEP_ETH_HDR_LEN) { return BNEP_ERR_TRUNCATED; }

    const uint8_t *dst = &eth_frame[0];
    const uint8_t *src = &eth_frame[6];
    const uint8_t *proto = &eth_frame[12];
    const uint8_t *payload = &eth_frame[BNEP_ETH_HDR_LEN];
    size_t payload_len = eth_len - BNEP_ETH_HDR_LEN;

    /* Broadcast and multicast must stay General: a compressed frame makes
     * the receiver rebuild the destination as its own unicast MAC, which
     * silently eats DHCP DISCOVER and every ARP request.
     *
     * The guard has to sit on BOTH omittable flags, not just dst_omittable.
     * A broadcast frame we originate has src == local_mac, so with the guard
     * only on dst the ladder falls through to DEST_ONLY (0x04) and still
     * emits a compressed frame. Confirmed by running the two tests above
     * against a src_omittable that lacked the guard: type came out 0x04
     * with n=37 instead of 0x00 with n=43. */
    bool bcast_or_mcast = bnep_mac_is_broadcast(dst)
                          || bnep_mac_is_multicast(dst);
    bool dst_omittable = compress && !bcast_or_mcast
                         && memcmp(dst, peer_mac, 6) == 0;
    bool src_omittable = compress && !bcast_or_mcast
                         && memcmp(src, local_mac, 6) == 0;

    uint8_t type;
    if (dst_omittable && src_omittable) {
        type = BNEP_COMPRESSED_ETHERNET;
    } else if (dst_omittable) {
        type = BNEP_COMPRESSED_ETHERNET_SRC_ONLY;
    } else if (src_omittable) {
        type = BNEP_COMPRESSED_ETHERNET_DEST_ONLY;
    } else {
        type = BNEP_GENERAL_ETHERNET;
    }

    size_t need = 1;
    if (type == BNEP_GENERAL_ETHERNET) { need += 12; }
    else if (type == BNEP_COMPRESSED_ETHERNET_SRC_ONLY) { need += 6; }
    else if (type == BNEP_COMPRESSED_ETHERNET_DEST_ONLY) { need += 6; }
    need += 2 + payload_len;

    if (cap < need) { return BNEP_ERR_NOSPACE; }

    size_t o = 0;
    out[o++] = type;
    if (type == BNEP_GENERAL_ETHERNET) {
        memcpy(&out[o], dst, 6); o += 6;
        memcpy(&out[o], src, 6); o += 6;
    } else if (type == BNEP_COMPRESSED_ETHERNET_SRC_ONLY) {
        memcpy(&out[o], src, 6); o += 6;
    } else if (type == BNEP_COMPRESSED_ETHERNET_DEST_ONLY) {
        memcpy(&out[o], dst, 6); o += 6;
    }
    out[o++] = proto[0];
    out[o++] = proto[1];
    memcpy(&out[o], payload, payload_len);
    o += payload_len;

    return (int)o;
}

int bnep_decode_eth(uint8_t *out, size_t cap,
                    const uint8_t *bnep, size_t bnep_len,
                    const uint8_t local_mac[6], const uint8_t peer_mac[6],
                    const uint8_t **ctrl_out, size_t *ctrl_len)
{
    if (!out || !bnep || !local_mac || !peer_mac || !ctrl_out || !ctrl_len) {
        return BNEP_ERR_TRUNCATED;
    }
    if (bnep_len < 1) { return BNEP_ERR_TRUNCATED; }

    uint8_t type = bnep[0] & BNEP_TYPE_MASK;
    bool has_ext = (bnep[0] & BNEP_EXT_FLAG) != 0;
    size_t i = 1;

    const uint8_t *dst = NULL;
    const uint8_t *src = NULL;

    switch (type) {
    case BNEP_CONTROL:
        /* Control payload starts right after the header byte and runs to
         * the end of the frame; extension headers, if any, follow the
         * control message and are not our concern here. */
        *ctrl_out = &bnep[1];
        *ctrl_len = bnep_len - 1;
        return BNEP_DECODE_IS_CONTROL;

    case BNEP_GENERAL_ETHERNET:
        if (bnep_len < i + 12) { return BNEP_ERR_TRUNCATED; }
        dst = &bnep[i]; i += 6;
        src = &bnep[i]; i += 6;
        break;

    case BNEP_COMPRESSED_ETHERNET:
        dst = local_mac;
        src = peer_mac;
        break;

    case BNEP_COMPRESSED_ETHERNET_SRC_ONLY:
        if (bnep_len < i + 6) { return BNEP_ERR_TRUNCATED; }
        dst = local_mac;
        src = &bnep[i]; i += 6;
        break;

    case BNEP_COMPRESSED_ETHERNET_DEST_ONLY:
        if (bnep_len < i + 6) { return BNEP_ERR_TRUNCATED; }
        dst = &bnep[i]; i += 6;
        src = peer_mac;
        break;

    default:
        return BNEP_ERR_BADTYPE;
    }
    /* Protocol type. */
    if (bnep_len < i + 2) { return BNEP_ERR_TRUNCATED; }
    uint8_t proto_hi = bnep[i];
    uint8_t proto_lo = bnep[i + 1];
    i += 2;

    /* Skip every extension header. Each is [more|type][len][payload].
     * Forgetting this loop hands extension bytes to the IP stack as if
     * they were payload. */
    while (has_ext) {
        if (bnep_len < i + 2) { return BNEP_ERR_TRUNCATED; }
        bool more = (bnep[i] & BNEP_EXT_FLAG) != 0;
        uint8_t ext_len = bnep[i + 1];
        i += 2;
        if (bnep_len < i + ext_len) { return BNEP_ERR_TRUNCATED; }
        i += ext_len;
        has_ext = more;
    }

    size_t payload_len = bnep_len - i;
    if (cap < BNEP_ETH_HDR_LEN + payload_len) { return BNEP_ERR_NOSPACE; }

    memcpy(&out[0], dst, 6);
    memcpy(&out[6], src, 6);
    out[12] = proto_hi;
    out[13] = proto_lo;
    if (payload_len) { memcpy(&out[BNEP_ETH_HDR_LEN], &bnep[i], payload_len); }

    return (int)(BNEP_ETH_HDR_LEN + payload_len);
}
