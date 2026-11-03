/* bnep_codec.c -- BNEP v1.0 framing, independent implementation. */
#include "bnep_codec.h"

#include <string.h>

int bnep_type_is_ethernet(uint8_t type)
{
    return type == BNEP_TYPE_GENERAL || type == BNEP_TYPE_COMPRESSED ||
           type == BNEP_TYPE_COMPRESSED_SRC_ONLY || type == BNEP_TYPE_COMPRESSED_DST_ONLY;
}

int bnep_decode(const uint8_t *p, size_t len, struct bnep_ctx *ctx, struct bnep_frame *out)
{
    uint8_t raw, type;
    size_t i;

    if (p == NULL || out == NULL) return BNEP_ERR_SHORT;
    memset(out, 0, sizeof(*out));
    if (len < 1) return BNEP_ERR_SHORT;

    raw = p[0];
    if (raw & BNEP_TYPE_EXTENSION_FLAG) return BNEP_ERR_BADTYPE;  /* ext unsupported */
    type = (uint8_t)(raw & 0x7f);
    out->type = type;

    /* ---- control ---- */


    if (!bnep_type_is_ethernet(type)) {
        /* must be control: BNEP control type field is 0x01 */
        struct bnep_ctrl *c = &out->u.ctrl;
        if (type != BNEP_TYPE_CONTROL) return BNEP_ERR_BADTYPE;
        if (len < 2) return BNEP_ERR_SHORT;
        c->type = p[1];

        switch (c->type) {
        case BNEP_CTRL_CMD_NOT_UNDERSTOOD:
            if (len < 3) return BNEP_ERR_SHORT;
            c->u.cmd_not_understood.offending_type = p[2];
            return BNEP_OK;

        case BNEP_CTRL_SETUP_CONN_REQ:
            if (len < 3) return BNEP_ERR_SHORT;
            c->u.setup_req.raw_size  = p[2];
            c->u.setup_req.uuid_size = p[2];
            if (p[2] != 2) return BNEP_ERR_BADUUID;      /* PAN Profile mandates 2 */
            if (len < 7) return BNEP_ERR_SHORT;
            c->u.setup_req.dst_uuid = (uint16_t)((p[3] << 8) | p[4]);
            c->u.setup_req.src_uuid = (uint16_t)((p[5] << 8) | p[6]);
            return BNEP_OK;

        case BNEP_CTRL_SETUP_CONN_RSP:
        case BNEP_CTRL_FILTER_NET_TYPE_RSP:
        case BNEP_CTRL_FILTER_MULTI_ADDR_RSP:
            if (len < 4) return BNEP_ERR_SHORT;
            c->u.rsp.response = (uint16_t)((p[2] << 8) | p[3]);
            return BNEP_OK;

        case BNEP_CTRL_FILTER_NET_TYPE_SET:
            if (len < 4) return BNEP_ERR_SHORT;
            c->u.filter_net.count = (uint16_t)((p[2] << 8) | p[3]);
            return BNEP_OK;

        case BNEP_CTRL_FILTER_MULTI_ADDR_SET:
            if (len < 4) return BNEP_ERR_SHORT;
            c->u.filter_multi.count = (uint16_t)((p[2] << 8) | p[3]);
            return BNEP_OK;

        default:
            return BNEP_ERR_BADTYPE;
        }
    }

    /* ---- ethernet ---- */
    i = 1;
    switch (type) {
    case BNEP_TYPE_GENERAL:
        if (len < 15) return BNEP_ERR_SHORT;
        out->u.eth.dst = p + 1;
        out->u.eth.src = p + 7;
        i = 13;
        break;
    case BNEP_TYPE_COMPRESSED_SRC_ONLY:
        if (len < 8) return BNEP_ERR_SHORT;
        out->u.eth.dst = (ctx && ctx->have_dst) ? ctx->dst : NULL;
        out->u.eth.src = p + 1;
        i = 7;
        break;
    case BNEP_TYPE_COMPRESSED_DST_ONLY:
        if (len < 8) return BNEP_ERR_SHORT;
        out->u.eth.dst = p + 1;
        out->u.eth.src = (ctx && ctx->have_src) ? ctx->src : NULL;
        i = 7;
        break;
    case BNEP_TYPE_COMPRESSED:
        out->u.eth.dst = (ctx && ctx->have_dst) ? ctx->dst : NULL;
        out->u.eth.src = (ctx && ctx->have_src) ? ctx->src : NULL;
        i = 1;
        break;
    default:
        return BNEP_ERR_BADTYPE;
    }

    if (len < i + 2) return BNEP_ERR_SHORT;
    out->u.eth.ethertype  = (uint16_t)((p[i] << 8) | p[i + 1]);
    out->u.eth.payload    = p + i + 2;
    out->u.eth.payload_len = len - (i + 2);

    /* update compression context only after a clean decode */
    if (ctx) {
        if (out->u.eth.dst) { memcpy(ctx->dst, out->u.eth.dst, 6); ctx->have_dst = 1; }
        if (out->u.eth.src) { memcpy(ctx->src, out->u.eth.src, 6); ctx->have_src = 1; }
    }
    return BNEP_OK;
}

int bnep_encode_ethernet(uint8_t *out, size_t cap, struct bnep_ctx *ctx, int force_general,
                         const uint8_t dst[6], const uint8_t src[6], uint16_t proto,
                         const uint8_t *payload, size_t len)
{
    size_t n = 0;
    int same_dst, same_src;

    if (out == NULL || dst == NULL || src == NULL || (len && payload == NULL))
        return BNEP_ERR_SHORT;
    if (13 + 2 + len > cap) return BNEP_ERR_NOSPACE;

    same_dst = (ctx && ctx->have_dst && memcmp(ctx->dst, dst, 6) == 0);
    same_src = (ctx && ctx->have_src && memcmp(ctx->src, src, 6) == 0);

    if (!force_general && same_dst && same_src) {
        out[n++] = BNEP_TYPE_COMPRESSED;
    } else if (!force_general && same_src && !same_dst) {
        out[n++] = BNEP_TYPE_COMPRESSED_DST_ONLY;
        memcpy(out + n, dst, 6); n += 6;
    } else if (!force_general && same_dst && !same_src) {
        out[n++] = BNEP_TYPE_COMPRESSED_SRC_ONLY;
        memcpy(out + n, src, 6); n += 6;
    } else {
        out[n++] = BNEP_TYPE_GENERAL;
        memcpy(out + n, dst, 6); n += 6;
        memcpy(out + n, src, 6); n += 6;
    }

    out[n++] = (uint8_t)(proto >> 8);
    out[n++] = (uint8_t)proto;
    if (len) { memcpy(out + n, payload, len); n += len; }

    if (ctx) {
        memcpy(ctx->dst, dst, 6); ctx->have_dst = 1;
        memcpy(ctx->src, src, 6); ctx->have_src = 1;
    }
    return (int)n;
}

int bnep_encode_setup_req(uint8_t *out, size_t cap,
                          const uint8_t dst_uuid[2], const uint8_t src_uuid[2],
                          uint8_t uuid_size)
{
    if (out == NULL || dst_uuid == NULL || src_uuid == NULL) return BNEP_ERR_SHORT;
    if (uuid_size != 2) return BNEP_ERR_BADUUID;
    if (cap < 7) return BNEP_ERR_NOSPACE;

    out[0] = 0x01;                    /* BNEP control */
    out[1] = BNEP_CTRL_SETUP_CONN_REQ;
    out[2] = uuid_size;               /* = 2 */
    out[3] = dst_uuid[0]; out[4] = dst_uuid[1];
    out[5] = src_uuid[0]; out[6] = src_uuid[1];
    return 7;
}

static int enc_rsp2(uint8_t *out, size_t cap, uint8_t ctrl_type, uint16_t code)
{
    if (out == NULL) return BNEP_ERR_SHORT;
    if (cap < 4) return BNEP_ERR_NOSPACE;
    out[0] = 0x01;
    out[1] = ctrl_type;
    out[2] = (uint8_t)(code >> 8);
    out[3] = (uint8_t)code;
    return 4;
}

int bnep_encode_setup_rsp(uint8_t *out, size_t cap, uint16_t code)
{
    return enc_rsp2(out, cap, BNEP_CTRL_SETUP_CONN_RSP, code);
}

int bnep_encode_filter_net_rsp(uint8_t *out, size_t cap, uint16_t code)
{
    return enc_rsp2(out, cap, BNEP_CTRL_FILTER_NET_TYPE_RSP, code);
}

int bnep_encode_filter_multi_rsp(uint8_t *out, size_t cap, uint16_t code)
{
    return enc_rsp2(out, cap, BNEP_CTRL_FILTER_MULTI_ADDR_RSP, code);
}

int bnep_encode_cmd_not_understood(uint8_t *out, size_t cap, uint8_t offending)
{
    if (out == NULL) return BNEP_ERR_SHORT;
    if (cap < 3) return BNEP_ERR_NOSPACE;
    out[0] = 0x01;
    out[1] = BNEP_CTRL_CMD_NOT_UNDERSTOOD;
    out[2] = offending;
    return 3;
}
