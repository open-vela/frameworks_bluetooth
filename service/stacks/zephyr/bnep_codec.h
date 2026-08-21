/****************************************************************************
 * BNEP (Bluetooth Network Encapsulation Protocol) codec.
 *
 * Zero-dependency by design: no zblue, no NuttX, no libc beyond memcpy.
 * This is what makes it unit-testable on the host (docs_ble/tools/
 * bnep_codec_test). Keep it that way.
 ****************************************************************************/

#ifndef BNEP_CODEC_H_
#define BNEP_CODEC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Frame types: low 7 bits of the BNEP header byte. Bit 7 = extension flag. */
#define BNEP_TYPE_MASK                      0x7f
#define BNEP_EXT_FLAG                       0x80
#define BNEP_GENERAL_ETHERNET               0x00  /* + dst6 + src6 + proto2 */
#define BNEP_CONTROL                        0x01
#define BNEP_COMPRESSED_ETHERNET            0x02  /* + proto2              */
#define BNEP_COMPRESSED_ETHERNET_SRC_ONLY   0x03  /* + src6 + proto2       */
#define BNEP_COMPRESSED_ETHERNET_DEST_ONLY  0x04  /* + dst6 + proto2       */

/* Control message types (second byte of a BNEP_CONTROL frame). Note these
 * live in a DIFFERENT namespace than the frame types above -- 0x02 means
 * "Setup Connection Response" here and "Compressed Ethernet" there. Never
 * switch on both in one statement. */
#define BNEP_CTRL_CMD_NOT_UNDERSTOOD        0x00
#define BNEP_CTRL_SETUP_CONN_REQ            0x01
#define BNEP_CTRL_SETUP_CONN_RSP            0x02
#define BNEP_CTRL_FILTER_NET_TYPE_SET       0x03
#define BNEP_CTRL_FILTER_NET_TYPE_RSP       0x04
#define BNEP_CTRL_FILTER_MULTI_ADDR_SET     0x05
#define BNEP_CTRL_FILTER_MULTI_ADDR_RSP     0x06

/* Setup Connection Response codes (2 bytes, big endian). */
#define BNEP_RSP_SUCCESS                    0x0000
#define BNEP_RSP_INVALID_DST_UUID           0x0001
#define BNEP_RSP_INVALID_SRC_UUID           0x0002
#define BNEP_RSP_INVALID_UUID_SIZE          0x0003
#define BNEP_RSP_CONN_NOT_ALLOWED           0x0004

/* Filter response codes (2 bytes, big endian). */
#define BNEP_FILTER_RSP_ACCEPTED            0x0000
#define BNEP_FILTER_RSP_UNSUPPORTED         0x0001
#define BNEP_FILTER_RSP_INVALID_RANGE       0x0002
#define BNEP_FILTER_RSP_TOO_MANY            0x0003

/* PAN service UUIDs (16-bit). */
#define BNEP_UUID16_PANU                    0x1115
#define BNEP_UUID16_NAP                     0x1116
#define BNEP_UUID16_GN                      0x1117

/* Sizing. BNEP requires an L2CAP MTU of at least 1691. */
#define BNEP_MIN_L2CAP_MTU                  1691
#define BNEP_ETH_HDR_LEN                    14
#define BNEP_MAX_ETH_FRAME                  1514  /* 14 + 1500 */

/* Return codes. Non-negative return values are byte counts. */
enum {
    BNEP_OK             =  0,
    BNEP_ERR_TRUNCATED  = -1,
    BNEP_ERR_NOSPACE    = -2,
    BNEP_ERR_BADTYPE    = -3,
    BNEP_ERR_BADUUID    = -4,
};

/* Parsed BNEP control message. */
struct bnep_control {
    uint8_t  msg_type;    /* BNEP_CTRL_*                                   */
    uint8_t  uuid_size;   /* SETUP_CONN_REQ only: 2, 4 or 16               */
    uint16_t dst_uuid16;  /* SETUP_CONN_REQ only: low 16 bits of dst UUID   */
    uint16_t src_uuid16;  /* SETUP_CONN_REQ only: low 16 bits of src UUID   */
    uint16_t rsp_code;    /* SETUP_CONN_RSP / FILTER_*_RSP                  */
    bool     is_success;  /* SETUP_CONN_RSP: rsp_code == BNEP_RSP_SUCCESS   */
    uint8_t  unknown_type;/* CMD_NOT_UNDERSTOOD payload                     */
};

/* zblue's bt_addr_t.val is little-endian; Ethernet MACs are network order.
 * Getting this wrong lets the handshake and unicast work while ARP never
 * resolves, which reads like "the phone is not answering". */
static inline void bnep_mac_from_le48(uint8_t mac[6], const uint8_t le48[6])
{
    for (int i = 0; i < 6; i++) { mac[i] = le48[5 - i]; }
}

static inline bool bnep_mac_is_broadcast(const uint8_t mac[6])
{
    return (mac[0] & mac[1] & mac[2] & mac[3] & mac[4] & mac[5]) == 0xff;
}

static inline bool bnep_mac_is_multicast(const uint8_t mac[6])
{
    return (mac[0] & 0x01) != 0;
}

/* --- Control frames. Each writes a complete BNEP frame including the
 *     0x01 header byte, and returns the byte count. --- */
int bnep_encode_setup_req(uint8_t *out, size_t cap,
                          uint16_t dst_uuid16, uint16_t src_uuid16);
int bnep_encode_setup_rsp(uint8_t *out, size_t cap, uint16_t rsp_code);
int bnep_encode_filter_rsp(uint8_t *out, size_t cap,
                           uint8_t rsp_msg_type, uint16_t rsp_code);
int bnep_encode_cmd_not_understood(uint8_t *out, size_t cap,
                                   uint8_t unknown_type);

/* Parses a control payload -- ctrl points at the MESSAGE TYPE byte, i.e.
 * one byte past the 0x01 frame header. Returns BNEP_OK or an error. */
int bnep_parse_control(const uint8_t *ctrl, size_t len,
                       struct bnep_control *info);

/* --- Data frames (implemented in Task 2 / Task 3). --- */

/* Encodes a complete Ethernet frame (14-byte header included) as BNEP.
 * Broadcast and multicast destinations are always sent as General
 * Ethernet regardless of `compress`, because the receiver would otherwise
 * reconstruct the destination as its own unicast MAC. */
int bnep_encode_eth(uint8_t *out, size_t cap,
                    const uint8_t *eth_frame, size_t eth_len,
                    const uint8_t local_mac[6], const uint8_t peer_mac[6],
                    bool compress);

/* Decodes a BNEP frame into a complete Ethernet frame (14-byte header
 * reconstructed). Skips every extension header. For a control frame,
 * returns BNEP_DECODE_IS_CONTROL and sets *ctrl_out / *ctrl_len. */
#define BNEP_DECODE_IS_CONTROL (-100)
int bnep_decode_eth(uint8_t *out, size_t cap,
                    const uint8_t *bnep, size_t bnep_len,
                    const uint8_t local_mac[6], const uint8_t peer_mac[6],
                    const uint8_t **ctrl_out, size_t *ctrl_len);

#endif /* BNEP_CODEC_H_ */
