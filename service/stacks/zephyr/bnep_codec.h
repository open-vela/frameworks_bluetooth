/* bnep_codec.h -- BNEP v1.0 framing (SIG spec, independent implementation). */
#ifndef BNEP_CODEC_H
#define BNEP_CODEC_H

#include <stdint.h>
#include <stddef.h>

/* BNEP general header type field (bit7 = extension, not supported) */
#define BNEP_TYPE_GENERAL             0x00  /* dst+src MAC + ethertype + payload */
#define BNEP_TYPE_CONTROL             0x01  /* PAN-FIX: was called COMPRESSED! */
#define BNEP_TYPE_COMPRESSED          0x02  /* PAN-FIX: was 0x01! ethertype + payload, MACs from ctx */
#define BNEP_TYPE_COMPRESSED_SRC_ONLY 0x03  /* PAN-FIX: was 0x02! src MAC + ethertype + payload */
#define BNEP_TYPE_COMPRESSED_DST_ONLY 0x04  /* PAN-FIX: was 0x03! dst MAC + ethertype + payload */
#define BNEP_TYPE_EXTENSION_FLAG      0x80

/* BNEP control message types */
#define BNEP_CTRL_CMD_NOT_UNDERSTOOD     0x00
#define BNEP_CTRL_SETUP_CONN_REQ         0x01
#define BNEP_CTRL_SETUP_CONN_RSP         0x02
#define BNEP_CTRL_FILTER_NET_TYPE_SET    0x03
#define BNEP_CTRL_FILTER_NET_TYPE_RSP    0x04
#define BNEP_CTRL_FILTER_MULTI_ADDR_SET  0x05
#define BNEP_CTRL_FILTER_MULTI_ADDR_RSP  0x06

/* SETUP_CONN_RSP response codes */
#define BNEP_SETUP_OK                     0x0000
#define BNEP_SETUP_ERR_INVALID_DST        0x0001
#define BNEP_SETUP_ERR_INVALID_SRC        0x0002
#define BNEP_SETUP_ERR_INVALID_UUID_SIZE  0x0003
#define BNEP_SETUP_ERR_NOT_ALLOWED        0x0004

/* FILTER_*_RSP response codes */
#define BNEP_FILTER_OK               0x0000
#define BNEP_FILTER_ERR_UNSUPPORTED  0x0001
#define BNEP_FILTER_ERR_INVALID      0x0002
#define BNEP_FILTER_ERR_LIMIT        0x0003

/* PAN Profile minimum L2CAP MTU */
#define BNEP_MAX_FRAME 1691

enum {
    BNEP_OK            = 0,
    BNEP_ERR_SHORT     = -1,
    BNEP_ERR_BADTYPE   = -2,
    BNEP_ERR_BADUUID   = -3,
    BNEP_ERR_NOSPACE   = -4,
};

/* Compression context: remember the last Ethernet MAC pair on the session. */
struct bnep_ctx {
    uint8_t dst[6];
    uint8_t src[6];
    int     have_dst;
    int     have_src;
};

struct bnep_ctrl {
    uint8_t type;
    union {
        struct { uint8_t uuid_size; uint8_t raw_size; uint16_t dst_uuid, src_uuid; } setup_req;
        struct { uint16_t response; } rsp;
        struct { uint16_t count; } filter_net;
        struct { uint16_t count; } filter_multi;
        struct { uint8_t offending_type; } cmd_not_understood;
    } u;
};

struct bnep_frame {
    uint8_t type;
    union {
        struct bnep_ctrl ctrl;
        struct {
            const uint8_t *dst;        /* NULL if no compression context */
            const uint8_t *src;        /* NULL if no compression context */
            uint16_t       ethertype;
            const uint8_t *payload;
            size_t         payload_len;
        } eth;
    } u;
};

/* Decode one BNEP frame.  dst/src point into `p` or `ctx`; valid until next call. */
int bnep_decode(const uint8_t *p, size_t len, struct bnep_ctx *ctx,
                struct bnep_frame *out);

/* Encode one Ethernet BNEP frame.  Returns frame length, or negative BNEP_ERR_*. */
int bnep_encode_ethernet(uint8_t *out, size_t cap, struct bnep_ctx *ctx,
                         int force_general,
                         const uint8_t dst[6], const uint8_t src[6], uint16_t proto,
                         const uint8_t *payload, size_t len);

/* Control encoders.  uuid_size must be 2 (PAN Profile). */
int bnep_encode_setup_req(uint8_t *out, size_t cap,
                          const uint8_t dst_uuid[2], const uint8_t src_uuid[2],
                          uint8_t uuid_size);
int bnep_encode_setup_rsp(uint8_t *out, size_t cap, uint16_t code);
int bnep_encode_filter_net_rsp(uint8_t *out, size_t cap, uint16_t code);
int bnep_encode_filter_multi_rsp(uint8_t *out, size_t cap, uint16_t code);
int bnep_encode_cmd_not_understood(uint8_t *out, size_t cap, uint8_t offending);

int bnep_type_is_ethernet(uint8_t type);

#endif /* BNEP_CODEC_H */
