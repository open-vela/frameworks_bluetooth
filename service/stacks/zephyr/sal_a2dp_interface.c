/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#define LOG_TAG "sal_a2dp"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "a2dp_device.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "sal_a2dp_sink_interface.h"
#include "sal_a2dp_source_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "utils/log.h"

#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/sdp.h>

#ifdef CONFIG_BLUETOOTH_A2DP
#include "a2dp_codec.h"

#define A2DP_PEER_ENDPOINT_MAX 10

#define BT_A2DP_MEDIA_CONNECTED(state) (state | 0xF0)
#define BT_A2DP_SIGNALING_CONNECTED(state) (state | 0x0F)
#define BT_A2DP_MEDIA_DISCONNECTED(state) (state & 0x0F)
#define BT_A2DP_SIGNALING_DISCONNECTED(state) (state & 0xF0)

#define BT_A2DP_FIND_MEDIA_CONNECTION(state) (state & 0xF0)
#define BT_A2DP_FIND_SIGNALING_CONNECTION(state) (state & 0x0F)

typedef enum {
    A2DP_INT = 0,
    A2DP_ACP = 1,
} a2dp_int_acp_t;

struct zblue_a2dp_info_t {
    struct bt_a2dp* a2dp;
    struct bt_conn* conn;
    struct bt_a2dp_stream stream;
    bt_address_t bd_addr;
    uint8_t peer_endpoint_count;
    struct bt_a2dp_codec_ie peer_sbc_capabilities[A2DP_PEER_ENDPOINT_MAX];
    struct bt_a2dp_ep found_peer_endpoint[A2DP_PEER_ENDPOINT_MAX];
    a2dp_int_acp_t int_acp;
    uint8_t role;
    bool is_cleanup; // cleanup flag，if true, free bt_a2dp_conn

    /*
     * The upper 8 bits represent the status of the media channel,
     * and the lower 8 bits represent the status of the signaling channel.
     */
    uint8_t state;
    bool disconnect; // disconnect flag, Avoid repeatedly disconnecting A2DP during cleanup.
    uint8_t codec_type; // The codec type to be set during reconfiguration.
};

static bt_list_t* bt_a2dp_conn = NULL;

NET_BUF_POOL_DEFINE(bt_a2dp_tx_pool, CONFIG_BT_MAX_CONN,
    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
/* codec information elements for the endpoint */
static struct bt_a2dp_codec_ie sbc_src_ie = {
    .len = 4, /* BT_A2DP_SBC_IE_LENGTH */
    .codec_ie = {
        0x2B, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
        0xFF, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
        0x02, /* min bitpool */
        0x35, /* max bitpool */
    },
};

static struct bt_a2dp_ep a2dp_sbc_src_endpoint_local = {
    .codec_type = 0x00, /* BT_A2DP_SBC */
    .codec_cap = (struct bt_a2dp_codec_ie*)&sbc_src_ie,
    .sep = {
        .sep_info = {
            .media_type = 0x00, /* BT_AVDTP_AUDIO */
            .tsep = 0, /* BT_AVDTP_SOURCE */
        },
    },
    .stream = NULL,
};

static struct bt_a2dp_codec_ie src_sbc_ie_default[] = {
    {
        .len = 4,
        .codec_ie = {
            0x28, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x22, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x21, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
};

static struct bt_a2dp_codec_cfg src_sbc_cfg_preferred[] = {
    {
        .codec_config = &src_sbc_ie_default[0],
    },
    {
        .codec_config = &src_sbc_ie_default[1],
    },
    {
        .codec_config = &src_sbc_ie_default[2],
    },
};
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
/* codec information elements for the endpoint */
static struct bt_a2dp_codec_ie sbc_snk_ie = {
    .len = 4, /* BT_A2DP_SBC_IE_LENGTH */
    .codec_ie = {
        0x3F, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
        0xFF, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
        0x02, /* min bitpool */
        0x35, /* max bitpool */
    },
};

static struct bt_a2dp_ep a2dp_sbc_snk_endpoint_local = {
    .codec_type = 0x00, /* BT_A2DP_SBC */
    .codec_cap = (struct bt_a2dp_codec_ie*)&sbc_snk_ie,
    .sep = {
        .sep_info = {
            .media_type = 0x00, /* BT_AVDTP_AUDIO */
            .tsep = 1, /* BT_AVDTP_SINK */
        },
    },
    .stream = NULL,
};

static struct bt_a2dp_codec_ie snk_sbc_ie_default[] = {
    {
        .len = 4,
        .codec_ie = {
            0x28, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x24, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x22, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x21, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x18, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x14, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x12, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
    {
        .len = 4,
        .codec_ie = {
            0x11, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
            0x15, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
            0x02, /* min bitpool */
            0x35, /* max bitpool */
        },
    },
};

static struct bt_a2dp_codec_cfg snk_sbc_cfg_preferred[] = {
    {
        .codec_config = &snk_sbc_ie_default[0],
    },
    {
        .codec_config = &snk_sbc_ie_default[1],
    },
    {
        .codec_config = &snk_sbc_ie_default[2],
    },
    {
        .codec_config = &snk_sbc_ie_default[3],
    },
    {
        .codec_config = &snk_sbc_ie_default[4],
    },
    {
        .codec_config = &snk_sbc_ie_default[5],
    },
    {
        .codec_config = &snk_sbc_ie_default[6],
    },
    {
        .codec_config = &snk_sbc_ie_default[7],
    }
};
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
static struct bt_a2dp_codec_ie aac_src_ie = {
    .len = 6, /* BT_A2DP_MPEG_2_4_IE_LENGTH */
    .codec_ie = {
        0x80, /* MPEG2 AAC LC | MPEG4 AAC LC | MPEG AAC LTP | MPEG4 AAC Scalable | MPEG4 HE-AAC | MPEG4 HE-AACv2 | MPEG4 HE-AAC-ELDv2 */
        0x01, /* 8000 | 11025 | 12000 | 16000 | 22050 | 24000 | 32000 | 44100 */
        0x0C, /* 48000 | 64000 | 88200 | 96000 | Channels 1 | Channels 2 | Channels 5.1 | Channels 7.1 */
#ifdef A2DP_AAC_MAX_BIT_RATE
        0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
        ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
        (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
        0xFF, /* VBR | bit rate[22:16] */
        0xFF, /* bit rate[15:8] */
        0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
    },
};

static struct bt_a2dp_ep a2dp_aac_src_endpoint_local = {
    .codec_type = 0x02, /* BT_A2DP_SBC */
    .codec_cap = (struct bt_a2dp_codec_ie*)&aac_src_ie,
    .sep = {
        .sep_info = {
            .media_type = 0x00, /* BT_AVDTP_AUDIO */
            .tsep = 0, /* BT_AVDTP_SOURCE */
        },
    },
    .stream = NULL,
};

static struct bt_a2dp_codec_ie src_aac_ie_default[] = {
    {
        .len = 6,
        .codec_ie = {
            0x80, 0x01, 0x08,
#ifdef A2DP_AAC_MAX_BIT_RATE
            0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
            ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
            (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
            0xFF, /* VBR | bit rate[22:16] */
            0xFF, /* bit rate[15:8] */
            0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
        },
    },
    {
        .len = 6,
        .codec_ie = {
            0x80, 0x01, 0x04,
#ifdef A2DP_AAC_MAX_BIT_RATE
            0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
            ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
            (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
            0xFF, /* VBR | bit rate[22:16] */
            0xFF, /* bit rate[15:8] */
            0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
        },
    },
};

static struct bt_a2dp_codec_cfg src_aac_cfg_preferred[] = {
    {
        .codec_config = &src_aac_ie_default[0],
    },
    {
        .codec_config = &src_aac_ie_default[1],
    },
};
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
static struct bt_a2dp_codec_ie aac_snk_ie = {
    .len = 6, /* BT_A2DP_MPEG_2_4_IE_LENGTH */
    .codec_ie = {
        0x80, /* MPEG2 AAC LC | MPEG4 AAC LC | MPEG AAC LTP | MPEG4 AAC Scalable | MPEG4 HE-AAC | MPEG4 HE-AACv2 | MPEG4 HE-AAC-ELDv2 */
        0x01, /* 8000 | 11025 | 12000 | 16000 | 22050 | 24000 | 32000 | 44100 */
        0x8C, /* 48000 | 64000 | 88200 | 96000 | Channels 1 | Channels 2 | Channels 5.1 | Channels 7.1 */
#ifdef A2DP_AAC_MAX_BIT_RATE
        0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
        ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
        (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
        0xFF, /* VBR | bit rate[22:16] */
        0xFF, /* bit rate[15:8] */
        0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
    },
};

static struct bt_a2dp_ep a2dp_aac_snk_endpoint_local = {
    .codec_type = 0x02, /* BT_A2DP_SBC */
    .codec_cap = (struct bt_a2dp_codec_ie*)&aac_snk_ie,
    .sep = {
        .sep_info = {
            .media_type = 0x00, /* BT_AVDTP_AUDIO */
            .tsep = 1, /* BT_AVDTP_SINK */
        },
    },
    .stream = NULL,
};

static struct bt_a2dp_codec_ie snk_aac_ie_default[] = {
    {
        .len = 6,
        .codec_ie = {
            0x80, 0x01, 0x08,
#ifdef A2DP_AAC_MAX_BIT_RATE
            0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
            ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
            (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
            0xFF, /* VBR | bit rate[22:16] */
            0xFF, /* bit rate[15:8] */
            0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
        },
    },
    {
        .len = 6,
        .codec_ie = {
            0x80, 0x01, 0x04,
#ifdef A2DP_AAC_MAX_BIT_RATE
            0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
            ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
            (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
            0xFF, /* VBR | bit rate[22:16] */
            0xFF, /* bit rate[15:8] */
            0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
        },
    },
    {
        .len = 6,
        .codec_ie = {
            0x80, 0x00, 0x18,
#ifdef A2DP_AAC_MAX_BIT_RATE
            0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
            ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
            (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
            0xFF, /* VBR | bit rate[22:16] */
            0xFF, /* bit rate[15:8] */
            0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
        },
    },
    {
        .len = 6,
        .codec_ie = {
            0x80, 0x00, 0x14,
#ifdef A2DP_AAC_MAX_BIT_RATE
            0x80 | ((A2DP_AAC_MAX_BIT_RATE >> 16) & 0x7F), /* VBR | bit rate[22:16] */
            ((A2DP_AAC_MAX_BIT_RATE >> 8) & 0xFF), /* bit rate[15:8] */
            (A2DP_AAC_MAX_BIT_RATE & 0xFF), /* bit rate[7:0]*/
#else
            0xFF, /* VBR | bit rate[22:16] */
            0xFF, /* bit rate[15:8] */
            0xFF, /* bit rate[7:0]*/
#endif /* A2DP_AAC_MAX_BIT_RATE */
        },
    },
};

static struct bt_a2dp_codec_cfg snk_aac_cfg_preferred[] = {
    {
        .codec_config = &snk_aac_ie_default[0],
    },
    {
        .codec_config = &snk_aac_ie_default[1],
    },
    {
        .codec_config = &snk_aac_ie_default[2],
    },
    {
        .codec_config = &snk_aac_ie_default[3],
    }
};
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
#endif /* CONFIG_BLUETOOTH_A2DP_AAC_CODEC */

#define BT_SDP_RECORD(_attrs)               \
    {                                       \
        .attrs = _attrs,                    \
        .attr_count = ARRAY_SIZE((_attrs)), \
    }

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
static struct bt_sdp_attribute a2dp_source_attrs[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                BT_SDP_ARRAY_16(BT_SDP_AUDIO_SOURCE_SVCLASS) }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) }, ) },
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(0x0100U) }, ) }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(0x0103U) }, ) }, )),
    BT_SDP_SERVICE_NAME("A2DPSink"),
    BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_source_rec = BT_SDP_RECORD(a2dp_source_attrs);
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
static struct bt_sdp_attribute a2dp_sink_attrs[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), /* 35 03 */
        BT_SDP_DATA_ELEM_LIST(
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
                BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS) /* 11 0B */
            }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16), /* 35 10 */
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
                BT_SDP_DATA_ELEM_LIST(
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) /* 01 00 */
                    },
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
                        BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
                    }, ) },
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
                BT_SDP_DATA_ELEM_LIST(
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
                        BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
                    },
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
                        BT_SDP_ARRAY_16(0x0100U) /* AVDTP version: 01 00 */
                    }, ) }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), /* 35 08 */
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
                BT_SDP_DATA_ELEM_LIST(
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
                        BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) /* 11 0d */
                    },
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
                        BT_SDP_ARRAY_16(0x0103U) /* 01 03 */
                    }, ) }, )),
    BT_SDP_SERVICE_NAME("A2DPSink"),
    BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */

static void a2dp_info_destory(void* data)
{
    free(data);
}

static bool bt_a2dp_info_find_a2dp(void* data, void* context)
{
    struct zblue_a2dp_info_t* a2dp_info = (struct zblue_a2dp_info_t*)data;
    if (!a2dp_info)
        return false;

    return a2dp_info->a2dp == context;
}

static bool bt_a2dp_info_find_addr(void* data, void* context)
{
    struct zblue_a2dp_info_t* a2dp_info = (struct zblue_a2dp_info_t*)data;
    if (!a2dp_info || !context)
        return false;

    return memcmp(&a2dp_info->bd_addr, context, sizeof(bt_address_t)) == 0;
}

static bool bt_a2dp_info_find_conn(void* data, void* context)
{
    struct zblue_a2dp_info_t* a2dp_info = (struct zblue_a2dp_info_t*)data;
    if (!a2dp_info)
        return false;

    return a2dp_info->conn == context;
}

bt_status_t bt_sal_a2dp_get_role(struct bt_conn* conn, uint8_t* a2dp_role)
{
    if (!bt_a2dp_conn)
        return BT_STATUS_PARM_INVALID;

    struct zblue_a2dp_info_t* a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_conn, conn);

    if (!a2dp_info)
        return BT_STATUS_PARM_INVALID;

    if (a2dp_info->role == SEP_INVALID)
        return BT_STATUS_PARM_INVALID;

    *a2dp_role = a2dp_info->role;

    return BT_STATUS_SUCCESS;
}

static a2dp_codec_index_t zephyr_codec_2_sal_codec(uint8_t codec)
{
    switch (codec) {
    case BT_A2DP_SBC:
        return BTS_A2DP_TYPE_SBC;
    case BT_A2DP_MPEG1:
        return BTS_A2DP_TYPE_MPEG1_2_AUDIO;
    case BT_A2DP_MPEG2:
        return BTS_A2DP_TYPE_MPEG2_4_AAC;
    case BT_A2DP_VENDOR:
        return BTS_A2DP_TYPE_NON_A2DP;
    default:
        BT_LOGW("%s, invalid codec: 0x%x", __func__, codec);
        return BTS_A2DP_TYPE_SBC;
    }
}

static a2dp_codec_channel_mode_t zephyr_sbc_channel_mode_2_sal_channel_mode(
    struct bt_a2dp_codec_sbc_params* sbc_codec)
{
    if (sbc_codec->config[0] & (A2DP_SBC_CH_MODE_JOINT | A2DP_SBC_CH_MODE_STREO | A2DP_SBC_CH_MODE_DUAL)) {
        return BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
    } else if (sbc_codec->config[0] & A2DP_SBC_CH_MODE_MONO) {
        return BTS_A2DP_CODEC_CHANNEL_MODE_MONO;
    } else {
        BT_LOGW("%s, invalid channel mode", __func__);
        return BTS_A2DP_CODEC_CHANNEL_MODE_STEREO;
    }
}

static bt_status_t check_local_remote_codec_sbc(uint8_t* local_ie, uint8_t* remote_ie, uint8_t* prefered_ie)
{
    uint8_t bit_map = 0;
    uint8_t bit_pool_min = 0;
    uint8_t bit_pool_max = 0;

    struct bt_a2dp_codec_sbc_params* local_sbc_codec = (struct bt_a2dp_codec_sbc_params*)local_ie;
    struct bt_a2dp_codec_sbc_params* remote_sbc_codec = (struct bt_a2dp_codec_sbc_params*)remote_ie;
    struct bt_a2dp_codec_sbc_params* prefered_sbc_codec = (struct bt_a2dp_codec_sbc_params*)prefered_ie;

    bit_map = (BT_A2DP_SBC_SAMP_FREQ(local_sbc_codec) & BT_A2DP_SBC_SAMP_FREQ(remote_sbc_codec) & BT_A2DP_SBC_SAMP_FREQ(prefered_sbc_codec));
    if (!bit_map)
        return BT_STATUS_FAIL;

    bit_map = (BT_A2DP_SBC_CHAN_MODE(local_sbc_codec) & BT_A2DP_SBC_CHAN_MODE(remote_sbc_codec) & BT_A2DP_SBC_CHAN_MODE(prefered_sbc_codec));
    if (!bit_map)
        return BT_STATUS_FAIL;

    bit_map = (BT_A2DP_SBC_BLK_LEN(local_sbc_codec) & BT_A2DP_SBC_BLK_LEN(remote_sbc_codec) & BT_A2DP_SBC_BLK_LEN(prefered_sbc_codec));
    if (!bit_map)
        return BT_STATUS_FAIL;

    bit_map = (BT_A2DP_SBC_SUB_BAND(local_sbc_codec) & BT_A2DP_SBC_SUB_BAND(remote_sbc_codec) & BT_A2DP_SBC_SUB_BAND(prefered_sbc_codec));
    if (!bit_map)
        return BT_STATUS_FAIL;

    bit_map = (BT_A2DP_SBC_ALLOC_MTHD(local_sbc_codec) & BT_A2DP_SBC_ALLOC_MTHD(remote_sbc_codec) & BT_A2DP_SBC_ALLOC_MTHD(prefered_sbc_codec));
    if (!bit_map)
        return BT_STATUS_FAIL;

    bit_pool_min = MAX(local_ie[2], MAX(remote_ie[2], prefered_ie[2]));
    bit_pool_max = MIN(local_ie[3], MIN(remote_ie[3], prefered_ie[3]));

    if (bit_pool_min > bit_pool_max)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

static bt_status_t check_local_remote_codec_aac(uint8_t* local_ie, uint8_t* remote_ie, uint8_t* prefered_ie)
{
    return BT_STATUS_FAIL;
}

static int find_remote_codec(struct bt_a2dp_ep* local_ep, struct zblue_a2dp_info_t* a2dp_info, struct bt_a2dp_codec_cfg* config)
{
    int index = 0;
    bt_status_t flag;

    for (; index < a2dp_info->peer_endpoint_count; index++) {
        if (local_ep->codec_type != a2dp_info->found_peer_endpoint[index].codec_type)
            continue;

        if (local_ep->sep.sep_info.inuse != 0)
            continue;

        if (a2dp_info->found_peer_endpoint[index].sep.sep_info.inuse != 0)
            continue;

        if (local_ep->sep.sep_info.media_type != a2dp_info->found_peer_endpoint[index].sep.sep_info.media_type)
            continue;

        if (local_ep->sep.sep_info.tsep == a2dp_info->found_peer_endpoint[index].sep.sep_info.tsep)
            continue;

        if (local_ep->codec_cap->len != a2dp_info->found_peer_endpoint[index].codec_cap->len)
            continue;

        if (local_ep->codec_type == BT_A2DP_SBC) {
            flag = check_local_remote_codec_sbc(local_ep->codec_cap->codec_ie,
                a2dp_info->found_peer_endpoint[index].codec_cap->codec_ie, config->codec_config->codec_ie);
            if (flag == BT_STATUS_SUCCESS)
                return index;
        } else if (local_ep->codec_type == BT_A2DP_MPEG2) {
            flag = check_local_remote_codec_aac(local_ep->codec_cap->codec_ie,
                a2dp_info->found_peer_endpoint[index].codec_cap->codec_ie, config->codec_config->codec_ie);
            if (flag == BT_STATUS_SUCCESS)
                return index;
        }
    }
    index = -1;
    return index;
}

static void zblue_on_stream_configured(struct bt_a2dp_stream* stream)
{
    a2dp_event_t* event;
    a2dp_codec_config_t codec_config; /* framework codec */
    struct bt_a2dp_codec_ie* codec_cfg; /* zblue codec */
    struct zblue_a2dp_info_t* a2dp_info;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, stream->a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp info not found", __func__);
        return;
    }

    a2dp_info->role = stream->local_ep->sep.sep_info.tsep;
    codec_config.codec_type = zephyr_codec_2_sal_codec(stream->local_ep->codec_type);
    codec_cfg = &stream->codec_config;

    switch (stream->local_ep->codec_type) {
    case BT_A2DP_SBC:
        codec_config.sample_rate = bt_a2dp_sbc_get_sampling_frequency(
            (struct bt_a2dp_codec_sbc_params*)&codec_cfg->codec_ie[0]);
        codec_config.bits_per_sample = BTS_A2DP_CODEC_BITS_PER_SAMPLE_16;
        codec_config.channel_mode = zephyr_sbc_channel_mode_2_sal_channel_mode(
            (struct bt_a2dp_codec_sbc_params*)&codec_cfg->codec_ie[0]);
        codec_config.packet_size = 1024;
        memcpy(codec_config.specific_info, codec_cfg->codec_ie, sizeof(codec_cfg->codec_ie));
        break;
    case BT_A2DP_MPEG2:
        break;
    default:
        BT_LOGE("%s, codec not supported: 0x%x", __func__, stream->local_ep->codec_type);
        return;
    }

    if ((a2dp_info->role == SEP_SRC) || (a2dp_info->role == SEP_SNK && a2dp_info->int_acp == A2DP_INT))
        SAL_CHECK_RET(bt_a2dp_stream_establish(stream), 0);

    event = a2dp_event_new(CODEC_CONFIG_EVT, &a2dp_info->bd_addr);
    event->event_data.data = malloc(sizeof(codec_config));
    memcpy(event->event_data.data, &codec_config, sizeof(codec_config));

    if (a2dp_info->role == SEP_SRC) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(event);
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* SEP_SNK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(event);
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

static void zblue_on_stream_established(struct bt_a2dp_stream* stream)
{
    struct zblue_a2dp_info_t* a2dp_info;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, stream->a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp info not found", __func__);
        return;
    }

    a2dp_info->state = BT_A2DP_MEDIA_CONNECTED(a2dp_info->state);

    if (a2dp_info->role == SEP_SRC) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(a2dp_event_new(CONNECTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* SEP_SNK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(CONNECTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

static void zblue_on_stream_released(struct bt_a2dp_stream* stream)
{
    struct zblue_a2dp_info_t* a2dp_info;
    BT_LOGI("%s, stream released", __func__);

    if (bt_a2dp_conn == NULL) {
        BT_LOGE("%s, bt_a2dp_conn is null", __func__);
        return;
    }

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, stream->a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp info not found", __func__);
        return;
    }

    a2dp_info->state = BT_A2DP_MEDIA_DISCONNECTED(a2dp_info->state);

    if (a2dp_info->role == SEP_SRC) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_CLOSED_EVT, &a2dp_info->bd_addr));
        bt_sal_a2dp_source_event_callback(a2dp_event_new(DISCONNECTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* SEP_SNK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_CLOSED_EVT, &a2dp_info->bd_addr));
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(DISCONNECTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
    if (a2dp_info->disconnect == true && BT_A2DP_SIGNALING_CONNECTED(a2dp_info->state)) {
        bt_a2dp_disconnect(a2dp_info->a2dp);
    }
}

static void zblue_on_stream_started(struct bt_a2dp_stream* stream)
{
    struct zblue_a2dp_info_t* a2dp_info;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, stream->a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp info not found", __func__);
        return;
    }

    if (a2dp_info->role == SEP_SRC) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        /* TODO: check if a2dp stream should be accepted */
        bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_STARTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* SEP_SNK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_STARTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

static void zblue_on_stream_suspended(struct bt_a2dp_stream* stream)
{
    struct zblue_a2dp_info_t* a2dp_info;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, stream->a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp info not found", __func__);
        return;
    }

    if (a2dp_info->role == SEP_SRC) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_SUSPENDED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* SEP_SNK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_SUSPENDED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
static void zblue_on_stream_recv(struct bt_a2dp_stream* stream,
    struct net_buf* buf, uint16_t seq_num, uint32_t ts)
{
    a2dp_event_t* event;
    a2dp_sink_packet_t* packet;
    uint8_t num_of_frames;
    uint16_t seq;
    uint32_t timestamp;
    struct zblue_a2dp_info_t* a2dp_info;

    if (buf == NULL || buf->data == NULL)
        return;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, stream->a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp info not found", __func__);
        return;
    }

    if (!buf->len) {
        BT_LOGE("%s, invalid length: %d", __func__, buf->len);
        return;
    }

    seq = seq_num;
    timestamp = ts;
    packet = a2dp_sink_new_packet(timestamp, seq, buf->data, buf->len);

    if (packet == NULL) {
        BT_LOGE("%s, packet malloc failed", __func__);
        return;
    }
    event = a2dp_event_new(DATA_IND_EVT, &a2dp_info->bd_addr);
    event->event_data.packet = packet;
    bt_sal_a2dp_sink_event_callback(event);
}
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */

static struct bt_a2dp_stream_ops stream_ops = {
    .configured = zblue_on_stream_configured,
    .established = zblue_on_stream_established,
    .released = zblue_on_stream_released,
    .started = zblue_on_stream_started,
    .suspended = zblue_on_stream_suspended,
    .aborted = NULL,
#if defined(CONFIG_BLUETOOTH_A2DP_SINK)
    .recv = zblue_on_stream_recv,
#endif
#if defined(CONFIG_BLUETOOTH_A2DP_SOURCE)
    .sent = NULL,
#endif
};

static uint8_t bt_a2dp_discover_endpoint_cb(struct bt_a2dp* a2dp,
    struct bt_a2dp_ep_info* info, struct bt_a2dp_ep** ep)
{

    int remote_index = 0;
    uint8_t local_index = 0;
    struct zblue_a2dp_info_t* a2dp_info;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, a2dp);
    if (!a2dp_info) {
        BT_LOGW("a2dp not found");
        return BT_A2DP_DISCOVER_EP_STOP;
    }

    if (info) {
        a2dp_info->found_peer_endpoint[a2dp_info->peer_endpoint_count].codec_cap = &a2dp_info->peer_sbc_capabilities[a2dp_info->peer_endpoint_count];
        if (ep != NULL)
            *ep = &a2dp_info->found_peer_endpoint[a2dp_info->peer_endpoint_count++];
        else
            return BT_A2DP_DISCOVER_EP_CONTINUE;

        if (a2dp_info->peer_endpoint_count >= A2DP_PEER_ENDPOINT_MAX)
            return BT_A2DP_DISCOVER_EP_STOP;

        return BT_A2DP_DISCOVER_EP_CONTINUE;
    }

    bt_a2dp_stream_cb_register(&a2dp_info->stream, &stream_ops);

    if (a2dp_info->role == SEP_SRC) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        local_index = 0;
        while (local_index < ARRAY_SIZE(src_sbc_cfg_preferred)) {
            remote_index = find_remote_codec(&a2dp_sbc_src_endpoint_local, a2dp_info,
                &src_sbc_cfg_preferred[local_index]);
            if (remote_index < 0) {
                local_index++;
                continue;
            }
            memcpy(&a2dp_info->stream.codec_config, src_sbc_cfg_preferred[local_index].codec_config, sizeof(struct bt_a2dp_codec_ie));

            bt_a2dp_stream_config(a2dp, &a2dp_info->stream,
                &a2dp_sbc_src_endpoint_local, &a2dp_info->found_peer_endpoint[remote_index],
                &src_sbc_cfg_preferred[local_index]);
            return BT_A2DP_DISCOVER_EP_STOP;
        }

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
        local_index = 0;
        while (local_index < ARRAY_SIZE(src_aac_cfg_preferred)) {
            remote_index = find_remote_codec(&a2dp_aac_src_endpoint_local, a2dp_info,
                &src_aac_cfg_preferred[local_index]);
            if (remote_index < 0) {
                local_index++;
                continue;
            }
            memcpy(&a2dp_info->stream.codec_config, src_aac_cfg_preferred[local_index].codec_config, sizeof(struct bt_a2dp_codec_ie));

            bt_a2dp_stream_config(a2dp, &a2dp_info->stream,
                &a2dp_aac_src_endpoint_local, &a2dp_info->found_peer_endpoint[remote_index],
                &src_aac_cfg_preferred[local_index]);
            return BT_A2DP_DISCOVER_EP_STOP;
        }
#endif
#endif
    } else if (a2dp_info->role == SEP_SNK && a2dp_info->int_acp == A2DP_INT) {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        local_index = 0;
        while (local_index < ARRAY_SIZE(snk_sbc_cfg_preferred)) {
            remote_index = find_remote_codec(&a2dp_sbc_snk_endpoint_local, a2dp_info,
                &snk_sbc_cfg_preferred[local_index]);
            if (remote_index < 0) {
                local_index++;
                continue;
            }
            memcpy(&a2dp_info->stream.codec_config, snk_sbc_cfg_preferred[local_index].codec_config, sizeof(struct bt_a2dp_codec_ie));

            bt_a2dp_stream_config(a2dp, &a2dp_info->stream,
                &a2dp_sbc_snk_endpoint_local, &a2dp_info->found_peer_endpoint[remote_index],
                &snk_sbc_cfg_preferred[local_index]);
            return BT_A2DP_DISCOVER_EP_STOP;
        }

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
        local_index = 0;
        while (local_index < ARRAY_SIZE(snk_aac_cfg_preferred)) {
            remote_index = find_remote_codec(&a2dp_aac_snk_endpoint_local, a2dp_info,
                &snk_aac_cfg_preferred[local_index]);
            if (remote_index < 0) {
                local_index++;
                continue;
            }
            memcpy(&a2dp_info->stream.codec_config, snk_aac_cfg_preferred[local_index].codec_config, sizeof(struct bt_a2dp_codec_ie));

            bt_a2dp_stream_config(a2dp, &a2dp_info->stream,
                &a2dp_aac_snk_endpoint_local, &a2dp_info->found_peer_endpoint[remote_index],
                &snk_aac_cfg_preferred[local_index]);
            return BT_A2DP_DISCOVER_EP_STOP;
        }
#endif
#endif
    }

    return BT_A2DP_DISCOVER_EP_STOP;
}

// TODO: Check if there is another implementation that is more appropriate.
static struct bt_avdtp_sep_info peer_seps[10];
struct bt_a2dp_discover_param bt_discover_param = {
    .cb = bt_a2dp_discover_endpoint_cb,
    .seps_info = &peer_seps[0], /* it saves endpoint info internally. */
    .sep_count = A2DP_PEER_ENDPOINT_MAX,
};

static void zblue_on_connected(struct bt_a2dp* a2dp, int err)
{
    struct zblue_a2dp_info_t* a2dp_info;
    struct bt_conn* conn;
    BT_LOGI("%s", __func__);

    a2dp_info = bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, a2dp);

    if (a2dp_info) {
        BT_LOGW("a2dp_info already exists");
        a2dp_info->state = BT_A2DP_SIGNALING_CONNECTED(a2dp_info->state);
        if (a2dp_info->int_acp == A2DP_INT) {
            bt_a2dp_discover(a2dp, &bt_discover_param);
            return;
        }
    }

    a2dp_info = (struct zblue_a2dp_info_t*)malloc(sizeof(struct zblue_a2dp_info_t));
    if (!a2dp_info) {
        BT_LOGW("malloc fail");
        return;
    }

    conn = bt_a2dp_get_conn(a2dp);
    if (conn == NULL) {
        BT_LOGE("conn is null");
        free(a2dp_info);
        return;
    }

    bt_conn_unref(conn);

    if (bt_sal_get_remote_address(conn, &a2dp_info->bd_addr) != BT_STATUS_SUCCESS) {
        free(a2dp_info);
        return;
    }

    a2dp_info->a2dp = a2dp;
    a2dp_info->conn = conn;
    memset(&a2dp_info->stream, 0, sizeof(a2dp_info->stream));
    a2dp_info->peer_endpoint_count = 0;
    a2dp_info->int_acp = A2DP_ACP;
    a2dp_info->role = SEP_INVALID;
    a2dp_info->is_cleanup = false;
    a2dp_info->state = BT_A2DP_SIGNALING_CONNECTED(a2dp_info->state);
    a2dp_info->disconnect = false;

    bt_list_add_tail(bt_a2dp_conn, a2dp_info);
}

static void bt_list_remove_a2dp_info(struct zblue_a2dp_info_t* a2dp_info)
{
    if (bt_a2dp_conn == NULL) {
        BT_LOGE("%s, bt_a2dp_conn is null", __func__);
        return;
    }
    if (a2dp_info == NULL) {
        BT_LOGE("%s, a2dp_info is null", __func__);
        return;
    }
    if (a2dp_info->state != 0)
        return;

    if (a2dp_info->is_cleanup && (bt_list_length(bt_a2dp_conn) == 1)) {
        BT_LOGI("cleanup done, free bt_a2dp_conn");
        bt_list_free(bt_a2dp_conn);
        bt_a2dp_conn = NULL;
        return;
    }

    BT_LOGI("a2dp disconnected, remove a2dp_info");
    bt_list_remove(bt_a2dp_conn, a2dp_info);
}

static void zblue_on_disconnected(struct bt_a2dp* a2dp)
{
    struct zblue_a2dp_info_t* a2dp_info;
    BT_LOGI("%s", __func__);

    if (bt_a2dp_conn == NULL) {
        BT_LOGE("%s, bt_a2dp_conn is null", __func__);
        return;
    }

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, a2dp);
    if (!a2dp_info) {
        BT_LOGW("a2dp_info not found");
        return;
    }
    a2dp_info->state = BT_A2DP_SIGNALING_DISCONNECTED(a2dp_info->state);

    if (a2dp_info->role == SEP_SRC) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(a2dp_event_new(STREAM_CLOSED_EVT, &a2dp_info->bd_addr));
        bt_sal_a2dp_source_event_callback(a2dp_event_new(DISCONNECTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* SEP_SNK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(STREAM_CLOSED_EVT, &a2dp_info->bd_addr));
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(DISCONNECTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }

    bt_list_remove_a2dp_info(a2dp_info);
}

static uint8_t bt_avdtp_codec_sanity_check(uint8_t local, uint8_t config, uint8_t offset, uint8_t len, uint8_t err)
{
    uint8_t codec;

    codec = (config >> offset) & ((1 << len) - 1); /* The collection of codec parameters to be checked */
    if (!codec)
        return err;

    if ((codec - 1) & codec) { /* The codec parameter to be checked has only one option available. */
        BT_LOGE("%s, Configuration has multiple options.", __func__);
        return err;
    }

    if (((config & local) >> offset) & ((1 << len) - 1)) {
        err = BT_AVDTP_SUCCESS;
    } else {
        /* Not_Supported error */
        err++;
    }

    return err;
}

static void bt_avdtp_codec_check_sbc(struct bt_a2dp_codec_ie* local, struct bt_a2dp_codec_ie* codec_cfg, uint8_t* rsp_err_code)
{
    if (local->len != codec_cfg->len) {
        *rsp_err_code = BT_AVDTP_BAD_LENGTH;
        return;
    }

    /* Sampling Frequency */
    *rsp_err_code = bt_avdtp_codec_sanity_check(local->codec_ie[0], codec_cfg->codec_ie[0], 4, 4, BT_A2DP_INVALID_SAMPLING_FREQUENCY);
    if (*rsp_err_code != BT_AVDTP_SUCCESS)
        return;

    /* Channel Mode */
    *rsp_err_code = bt_avdtp_codec_sanity_check(local->codec_ie[0], codec_cfg->codec_ie[0], 0, 4, BT_A2DP_INVALID_CHANNEL_MODE);
    if (*rsp_err_code != BT_AVDTP_SUCCESS)
        return;

    /* Block Length */
    *rsp_err_code = bt_avdtp_codec_sanity_check(local->codec_ie[1], codec_cfg->codec_ie[1], 4, 4, BT_A2DP_INVALID_BLOCK_LENGTH);
    if (*rsp_err_code != BT_AVDTP_SUCCESS)
        return;

    /* Subbands */
    *rsp_err_code = bt_avdtp_codec_sanity_check(local->codec_ie[1], codec_cfg->codec_ie[1], 2, 2, BT_A2DP_INVALID_SUBBANDS);
    if (*rsp_err_code != BT_AVDTP_SUCCESS)
        return;

    /* Allocation Method */
    *rsp_err_code = bt_avdtp_codec_sanity_check(local->codec_ie[1], codec_cfg->codec_ie[1], 0, 2, BT_A2DP_INVALID_ALLOCATION_METHOD);
    if (*rsp_err_code != BT_AVDTP_SUCCESS)
        return;

    /* Bitpool */
    if (codec_cfg->codec_ie[2] > codec_cfg->codec_ie[3]) {
        *rsp_err_code = BT_A2DP_INVALID_MINIMUM_BITPOOL_VALUE;
        return;
    }
    if ((codec_cfg->codec_ie[2] < 2) || (codec_cfg->codec_ie[2] > 250)) {
        *rsp_err_code = BT_A2DP_INVALID_MINIMUM_BITPOOL_VALUE;
        return;
    }
    if ((codec_cfg->codec_ie[3] < 2) || (codec_cfg->codec_ie[3] > 250)) {
        *rsp_err_code = BT_A2DP_INVALID_MAXIMUM_BITPOOL_VALUE;
        return;
    }

    if ((codec_cfg->codec_ie[2] < local->codec_ie[2]) || (codec_cfg->codec_ie[2] > local->codec_ie[3])) {
        *rsp_err_code = BT_A2DP_NOT_SUPPORTED_MINIMUM_BITPOOL_VALUE;
        return;
    }
    if (codec_cfg->codec_ie[3] > local->codec_ie[3]) {
        *rsp_err_code = BT_A2DP_NOT_SUPPORTED_MAXIMUM_BITPOOL_VALUE;
        return;
    }
}

static void bt_avdtp_set_config_check(struct bt_a2dp_ep* ep, struct bt_a2dp_codec_cfg* codec_cfg, uint8_t* rsp_err_code)
{
    switch (ep->codec_type) {
    case BT_A2DP_SBC:
        bt_avdtp_codec_check_sbc(ep->codec_cap, codec_cfg->codec_config, rsp_err_code);
        break;
    case BT_A2DP_MPEG1:
    case BT_A2DP_MPEG2:
    case BT_A2DP_ATRAC:
    case BT_A2DP_VENDOR:
    default:
        *rsp_err_code = BT_A2DP_NOT_SUPPORTED_CODEC_TYPE;
        break;
    }
}

static int zblue_on_config_req(struct bt_a2dp* a2dp, struct bt_a2dp_ep* ep,
    struct bt_a2dp_codec_cfg* codec_cfg, struct bt_a2dp_stream** stream,
    uint8_t* rsp_err_code)
{
    struct zblue_a2dp_info_t* a2dp_info;

    *rsp_err_code = BT_AVDTP_SUCCESS;

    if (ep == NULL || codec_cfg == NULL) {
        *rsp_err_code = BT_AVDTP_BAD_STATE;
        return -1;
    }

    if (ep->sep.sep_info.inuse) {
        BT_LOGE("%s, local SEP has already been used.", __func__);
        *rsp_err_code = BT_AVDTP_SEP_IN_USE;
        return -1;
    }

    bt_avdtp_set_config_check(ep, codec_cfg, rsp_err_code);

    if (*rsp_err_code != BT_AVDTP_SUCCESS) {
        BT_LOGE("%s, config fail: 0x%02x", __func__, *rsp_err_code);
        return -1;
    }

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp_info not found", __func__);
        *rsp_err_code = BT_AVDTP_BAD_STATE;
        return -1;
    }

    *stream = &a2dp_info->stream; /* The a2dp_stream saved in SAL is assigned a value in zblue. */
    bt_a2dp_stream_cb_register(&a2dp_info->stream, &stream_ops);
    *rsp_err_code = BT_AVDTP_SUCCESS;
    return 0;
}

static int zblue_on_reconfig_req(struct bt_a2dp_stream* stream, struct bt_a2dp_codec_cfg* codec_cfg,
    uint8_t* rsp_err_code)
{
    *rsp_err_code = BT_AVDTP_SUCCESS;
    return 0;
}

static void zblue_on_config_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code)
{
    if (rsp_err_code != 0)
        BT_LOGE("%s, config fail: %d", __func__, rsp_err_code);
}

static int zblue_on_establish_req(struct bt_a2dp_stream* stream, uint8_t* rsp_err_code)
{
    *rsp_err_code = BT_AVDTP_SUCCESS;
    return 0;
}

static void zblue_on_establish_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code)
{
    if (rsp_err_code == 0)
        return;

    struct zblue_a2dp_info_t* a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, stream->a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp_info not found", __func__);
        return;
    }

    if (a2dp_info->role == SEP_SRC) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        bt_sal_a2dp_source_event_callback(a2dp_event_new(DISCONNECTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
    } else { /* SEP_SNK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bt_sal_a2dp_sink_event_callback(a2dp_event_new(DISCONNECTED_EVT, &a2dp_info->bd_addr));
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
    }
}

static int zblue_on_release_req(struct bt_a2dp_stream* stream, uint8_t* rsp_err_code)
{
    *rsp_err_code = BT_AVDTP_SUCCESS;
    return 0;
}

static void zblue_on_release_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code)
{
    if (rsp_err_code == 0)
        return;

    BT_LOGE("%s, close fail: %d", __func__, rsp_err_code);
    struct zblue_a2dp_info_t* a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_a2dp, stream->a2dp);
    if (!a2dp_info) {
        BT_LOGE("%s, a2dp_info not found", __func__);
        return;
    }

    a2dp_info->state = BT_A2DP_MEDIA_DISCONNECTED(a2dp_info->state);
    if (a2dp_info->disconnect == true && BT_A2DP_SIGNALING_CONNECTED(a2dp_info->state)) {
        BT_LOGI("Failed to disconnect the media channel, disconnect the signaling channel");
        bt_a2dp_disconnect(a2dp_info->a2dp);
    }
}

static int zblue_on_start_req(struct bt_a2dp_stream* stream, uint8_t* rsp_err_code)
{
    *rsp_err_code = BT_AVDTP_SUCCESS;
    return 0;
}

static void zblue_on_start_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code)
{
    if (rsp_err_code != 0)
        BT_LOGE("%s, start fail: %d", __func__, rsp_err_code);
}

static int zblue_on_suspend_req(struct bt_a2dp_stream* stream, uint8_t* rsp_err_code)
{
    *rsp_err_code = BT_AVDTP_SUCCESS;
    return 0;
}
static void zblue_on_suspend_rsp(struct bt_a2dp_stream* stream, uint8_t rsp_err_code)
{
    if (rsp_err_code != 0)
        BT_LOGE("%s, suspend fail: %d", __func__, rsp_err_code);
}

static struct bt_a2dp_cb a2dp_cbks = {
    .connected = zblue_on_connected,
    .disconnected = zblue_on_disconnected,
    .config_req = zblue_on_config_req,
    .reconfig_req = zblue_on_reconfig_req,
    .config_rsp = zblue_on_config_rsp,
    .establish_req = zblue_on_establish_req,
    .establish_rsp = zblue_on_establish_rsp,
    .release_req = zblue_on_release_req,
    .release_rsp = zblue_on_release_rsp,
    .start_req = zblue_on_start_req,
    .start_rsp = zblue_on_start_rsp,
    .suspend_req = zblue_on_suspend_req,
    .suspend_rsp = zblue_on_suspend_rsp,
    .abort_req = NULL,
    .abort_rsp = NULL,
};

bt_status_t bt_sal_a2dp_source_init(uint8_t max_connections)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    uint8_t media_type = BT_AVDTP_AUDIO;
    uint8_t role = 0; /* BT_AVDTP_SOURCE */
    int ret;

    if (!bt_a2dp_conn)
        bt_a2dp_conn = bt_list_new(a2dp_info_destory);

    if (bt_list_length(bt_a2dp_conn)) {
        bt_list_clear(bt_a2dp_conn);
    }

    bt_sdp_register_service(&a2dp_source_rec);

    /* Mandatory support for SBC */
    ret = bt_a2dp_register_ep(&a2dp_sbc_src_endpoint_local, media_type, role);
    if (ret != 0 && ret != -EALREADY) {
        BT_LOGE("%s, register SEP failed:%d", __func__, ret);
        return BT_STATUS_FAIL;
    }

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    /* Optional support for AAC */
    ret = bt_a2dp_register_ep(&a2dp_aac_src_endpoint_local, media_type, role);
    if (ret != 0 && ret != -EALREADY) {
        BT_LOGE("%s, register SEP failed:%d", __func__, ret);
        return BT_STATUS_FAIL;
    }
#endif /* CONFIG_BLUETOOTH_A2DP_AAC_CODEC */

    SAL_CHECK_RET(bt_a2dp_register_cb(&a2dp_cbks), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_init(uint8_t max_connections)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    uint8_t media_type = BT_AVDTP_AUDIO;
    uint8_t role = 1; /* BT_AVDTP_SINK */
    int ret;

    if (!bt_a2dp_conn) {
        bt_a2dp_conn = bt_list_new(a2dp_info_destory);
    }

    if (bt_list_length(bt_a2dp_conn)) {
        bt_list_clear(bt_a2dp_conn);
    }

    bt_sdp_register_service(&a2dp_sink_rec);
    /* Mandatory support for SBC */
    ret = bt_a2dp_register_ep(&a2dp_sbc_snk_endpoint_local, media_type, role);
    if (ret != 0 && ret != -EALREADY) {
        BT_LOGE("%s, register SEP failed:%d", __func__, ret);
        return BT_STATUS_FAIL;
    }

#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    /* Optional support for AAC */
    ret = bt_a2dp_register_ep(&a2dp_aac_snk_endpoint_local, media_type, role);
    if (ret != 0 && ret != -EALREADY) {
        BT_LOGE("%s, register SEP failed:%d", __func__, ret);
        return BT_STATUS_FAIL;
    }
#endif /* CONFIG_BLUETOOTH_A2DP_AAC_CODEC */

    SAL_CHECK_RET(bt_a2dp_register_cb(&a2dp_cbks), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

bt_status_t bt_sal_a2dp_source_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    struct zblue_a2dp_info_t* a2dp_info;
    struct bt_a2dp* a2dp;

    if (!conn) {
        BT_LOGE("%s, acl not connected", __func__);
        return BT_STATUS_FAIL;
    }

    a2dp_info = bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_conn, conn);
    if (a2dp_info) {
        BT_LOGW("%s, A2DP connect already exists", __func__);
        goto error;
    }

    a2dp = bt_a2dp_connect(conn);
    if (!a2dp) {
        BT_LOGE("%s, A2DP connect failed", __func__);
        goto error;
    }

    a2dp_info = (struct zblue_a2dp_info_t*)malloc(sizeof(struct zblue_a2dp_info_t));
    if (!a2dp_info) {
        BT_LOGE("%s, malloc failed", __func__);
        goto error;
    }

    memcpy(&a2dp_info->bd_addr, addr, sizeof(bt_address_t));
    a2dp_info->a2dp = a2dp;
    a2dp_info->conn = conn;
    memset(&a2dp_info->stream, 0, sizeof(a2dp_info->stream));
    a2dp_info->peer_endpoint_count = 0;
    a2dp_info->int_acp = A2DP_INT;
    a2dp_info->role = SEP_SRC;
    a2dp_info->is_cleanup = false;
    a2dp_info->state = 0;
    a2dp_info->disconnect = false;

    bt_list_add_tail(bt_a2dp_conn, a2dp_info);
    bt_conn_unref(conn);
    return BT_STATUS_SUCCESS;

error:
    bt_conn_unref(conn);
    return BT_STATUS_FAIL;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    struct zblue_a2dp_info_t* a2dp_info;
    struct bt_a2dp* a2dp;

    if (!conn) {
        BT_LOGE("%s, acl not connected", __func__);
        return BT_STATUS_FAIL;
    }

    a2dp_info = bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_conn, conn);
    if (a2dp_info) {
        BT_LOGW("%s, A2DP connect already exists", __func__);
        goto error;
    }

    a2dp = bt_a2dp_connect(conn);
    if (!a2dp) {
        BT_LOGE("%s, A2DP connect failed", __func__);
        goto error;
    }

    a2dp_info = (struct zblue_a2dp_info_t*)malloc(sizeof(struct zblue_a2dp_info_t));
    if (!a2dp_info) {
        BT_LOGE("%s, malloc failed", __func__);
        goto error;
    }

    memcpy(&a2dp_info->bd_addr, addr, sizeof(bt_address_t));
    a2dp_info->a2dp = a2dp;
    a2dp_info->conn = conn;
    memset(&a2dp_info->stream, 0, sizeof(a2dp_info->stream));
    a2dp_info->peer_endpoint_count = 0;
    a2dp_info->int_acp = A2DP_INT;
    a2dp_info->role = SEP_SNK;
    a2dp_info->is_cleanup = false;
    a2dp_info->state = 0;
    a2dp_info->disconnect = false;

    bt_list_add_tail(bt_a2dp_conn, a2dp_info);
    bt_conn_unref(conn);
    return BT_STATUS_SUCCESS;

error:
    bt_conn_unref(conn);
    return BT_STATUS_FAIL;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

static bt_status_t bt_sal_a2dp_disconnect(struct zblue_a2dp_info_t* a2dp_info)
{
    int res_media = 0;
    int res_signaling = 0;

    if (!a2dp_info)
        return BT_STATUS_SUCCESS;

    if (a2dp_info->disconnect) {
        BT_LOGW("%s, disconnecting", __func__);
        return BT_STATUS_SUCCESS;
    }

    a2dp_info->disconnect = true;
    if (BT_A2DP_FIND_MEDIA_CONNECTION(a2dp_info->state)) {
        BT_LOGW("%s, media connection exists, disconnect", __func__);
        return bt_a2dp_stream_release(&a2dp_info->stream);
    } else if (BT_A2DP_FIND_SIGNALING_CONNECTION(a2dp_info->state)) {
        BT_LOGW("%s, signaling connection exists, disconnect", __func__);
        return bt_a2dp_disconnect(a2dp_info->a2dp);
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_a2dp_source_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct zblue_a2dp_info_t* a2dp_info;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_addr, addr);
    if (!a2dp_info) {
        BT_LOGW("%s, a2dp_info is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    return bt_sal_a2dp_disconnect(a2dp_info);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    struct zblue_a2dp_info_t* a2dp_info;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_addr, addr);
    if (!a2dp_info) {
        BT_LOGW("%s, a2dp_info is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    return bt_sal_a2dp_disconnect(a2dp_info);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

bt_status_t bt_sal_a2dp_source_start_stream(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct zblue_a2dp_info_t* a2dp_info;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_addr, addr);
    if (!a2dp_info) {
        BT_LOGW("%s, a2dp_info is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(bt_a2dp_stream_start(&a2dp_info->stream), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_source_suspend_stream(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct zblue_a2dp_info_t* a2dp_info;
    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_addr, addr);
    if (!a2dp_info) {
        BT_LOGW("%s, a2dp_info is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(bt_a2dp_stream_suspend(&a2dp_info->stream), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_source_send_data(bt_controller_id_t id, bt_address_t* remote_addr,
    uint8_t* buf, uint16_t nbytes, uint8_t nb_frames, uint64_t timestamp, uint32_t seq)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    struct net_buf* media_packet_buf;
    struct zblue_a2dp_info_t* a2dp_info;
    int ret;

    if (!buf)
        return BT_STATUS_PARM_INVALID;

    a2dp_info = (struct zblue_a2dp_info_t*)bt_list_find(bt_a2dp_conn, bt_a2dp_info_find_addr, remote_addr);
    if (!a2dp_info) {
        BT_LOGW("%s, a2dp_info is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    media_packet_buf = net_buf_alloc(&bt_a2dp_tx_pool, K_FOREVER);

    // Reserve space for the A2DP header
    net_buf_reserve(media_packet_buf, BT_A2DP_STREAM_BUF_RESERVE);

    // buf = Media Packet Header(AVDTP_RTP_HEADER_LEN) + Media Payload
    // nbytes = Media Payload Length
    net_buf_add_mem(media_packet_buf, &buf[AVDTP_RTP_HEADER_LEN], nbytes);

    ret = bt_a2dp_stream_send(&a2dp_info->stream, media_packet_buf, seq, timestamp);
    if (ret < 0)
        goto error;

    return BT_STATUS_SUCCESS;

error:
    BT_LOGE("%s, bt_a2dp_stream_send failed, ret: %d", __func__, ret);
    net_buf_unref(media_packet_buf);
    return BT_STATUS_FAIL;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

bt_status_t bt_sal_a2dp_sink_start_stream(bt_controller_id_t id, bt_address_t* addr)
{
    /* Note: this interface is used to accept an AVDTP Start Request */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

void bt_sal_a2dp_source_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    bt_list_t* list = bt_a2dp_conn;
    bt_list_node_t* node;
    uint8_t media_type = BT_AVDTP_AUDIO;
    uint8_t role = 0; /* BT_AVDTP_SOURCE */

    if (!list)
        return;

    bt_sdp_unregister_service(&a2dp_source_rec);
    bt_a2dp_unregister_ep(&a2dp_sbc_src_endpoint_local);

    if (bt_list_length(bt_a2dp_conn) == 0) {
        bt_list_free(bt_a2dp_conn);
        bt_a2dp_conn = NULL;
        return;
    }

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        struct zblue_a2dp_info_t* a2dp_info = bt_list_node(node);

        a2dp_info->is_cleanup = true;
        bt_sal_a2dp_disconnect(a2dp_info);
    }

    return;
#else
    return;
#endif /* CONFIG_BLUETOOTH_A2DP_SOURCE */
}

void bt_sal_a2dp_sink_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_list_t* list = bt_a2dp_conn;
    bt_list_node_t* node;
    uint8_t media_type = BT_AVDTP_AUDIO;
    uint8_t role = 1; /* BT_AVDTP_SINK */

    if (!list)
        return;

    bt_sdp_unregister_service(&a2dp_sink_rec);
    bt_a2dp_unregister_ep(&a2dp_sbc_snk_endpoint_local);

    if (bt_list_length(bt_a2dp_conn) == 0) {
        bt_list_free(bt_a2dp_conn);
        bt_a2dp_conn = NULL;
        return;
    }

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        struct zblue_a2dp_info_t* a2dp_info = bt_list_node(node);

        a2dp_info->is_cleanup = true;
        bt_sal_a2dp_disconnect(a2dp_info);
    }

    return;
#else
    return;
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
}

#endif /* CONFIG_BLUETOOTH_A2DP */