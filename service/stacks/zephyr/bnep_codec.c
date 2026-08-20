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
