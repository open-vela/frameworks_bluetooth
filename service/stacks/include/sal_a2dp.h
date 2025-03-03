/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef __SAL_A2DP_EP_H__
#define __SAL_A2DP_EP_H__

#ifdef CONFIG_BLUETOOTH_A2DP

#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/sdp.h>

uint8_t bt_avrcp_get_a2dp_role(struct bt_conn* conn);

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
/* codec information elements for the endpoint */
static struct bt_a2dp_codec_ie sbc_src_ie = {
    .len = 4, /* BT_A2DP_SBC_IE_LENGTH */
    .codec_ie = {
        0x23, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
        0xFF, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
        0x02, /* min bitpool */
        0x35, /* max bitpool */
    },
};

static struct bt_a2dp_ep a2dp_sbc_src_endpoint = {
    .codec_type = 0x00,    /* BT_A2DP_SBC */
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
        .codec_ie = {0x21, 0x15, 0x02, 0x35,},
    },
    {
        .len = 4,
        .codec_ie = {0x22, 0x15, 0x02, 0x35,},
    },
};

static struct bt_a2dp_codec_cfg src_sbc_cfg_default[] = {
    {
        .codec_config = &src_sbc_ie_default[0],
    },
    {
        .codec_config = &src_sbc_ie_default[1],
    },
};
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
/* codec information elements for the endpoint */
static struct bt_a2dp_codec_ie sbc_snk_ie = {
    .len = 4, /* BT_A2DP_SBC_IE_LENGTH */
    .codec_ie = {
        0x33, /* 16000 | 32000 | 44100 | 48000 | mono | dual channel | stereo | join stereo */
        0xFF, /* Block length: 4/8/12/16, subbands:4/8, Allocation Method: SNR, Londness */
        0x02, /* min bitpool */
        0x35, /* max bitpool */
    },
};

static struct bt_a2dp_ep a2dp_sbc_snk_endpoint = {
    .codec_type = 0x00,    /* BT_A2DP_SBC */
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
        .codec_ie = {0x21, 0x15, 0x02, 0x35,},
    },
    {
        .len = 4,
        .codec_ie = {0x22, 0x15, 0x02, 0x35,},
    },
    {
        .len = 4,
        .codec_ie = {0x11, 0x15, 0x02, 0x35,},
    },
    {
        .len = 4,
        .codec_ie = {0x12, 0x15, 0x02, 0x35,},
    },
};

static struct bt_a2dp_codec_cfg snk_sbc_cfg_default[] = {
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

static struct bt_a2dp_ep a2dp_aac_src_endpoint = {
    .codec_type = 0x02,    /* BT_A2DP_SBC */
    .codec_cap = (struct bt_a2dp_codec_ie*)&aac_src_ie,
    .sep = {
        .sep_info = {
            .media_type = 0x00, /* BT_AVDTP_AUDIO */
            .tsep = 0, /* BT_AVDTP_SOURCE */
        },
    },
    .stream = NULL,
};

static struct bt_a2dp_codec_cfg src_aac_cfg_default[] = {
    {
        .codec_config = {
            .len = 6,
            .codec_ie = {0x80, 0x01, 0x08, 
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
        }
    },
    {
        .codec_config = {
            .len = 6,
            .codec_ie = {0x80, 0x01, 0x04, 
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
        }
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

static struct bt_a2dp_ep a2dp_aac_snk_endpoint = {
    .codec_type = 0x02,    /* BT_A2DP_SBC */
    .codec_cap = (struct bt_a2dp_codec_ie*)&aac_snk_ie,
    .sep = {
        .sep_info = {
            .media_type = 0x00, /* BT_AVDTP_AUDIO */
            .tsep = 1, /* BT_AVDTP_SINK */
        };
    },
    .stream = NULL,
};

static struct bt_a2dp_codec_cfg snk_aac_cfg_default[] = {
    {
        .codec_config = {
            .len = 6,
            .codec_ie = {0x80, 0x01, 0x08, 
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
        }
    },
    {
        .codec_config = {
            .len = 6,
            .codec_ie = {0x80, 0x01, 0x04, 
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
        }
    },
    {
        .codec_config = {
            .len = 6,
            .codec_ie = {0x80, 0x00, 0x18, 
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
        }
    },
    {
        .codec_config = {
            .len = 6,
            .codec_ie = {0x80, 0x00, 0x14, 
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
        }
    },
};
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */
#endif /* CONFIG_BLUETOOTH_A2DP_AAC_CODEC */

#define BT_SDP_RECORD(_attrs) \
{ \
	.attrs = _attrs, \
	.attr_count = ARRAY_SIZE((_attrs)), \
}

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
static struct bt_sdp_attribute a2dp_source_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AUDIO_SOURCE_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0100U)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0103U)
			},
			)
		},
		)
	),
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
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),/* 35 10 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),/* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) /* 01 00 */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),/* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(0x0100U) /* AVDTP version: 01 00 */
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), /* 35 08 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) /* 11 0d */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(0x0103U) /* 01 03 */
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};
static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);
#endif /* CONFIG_BLUETOOTH_A2DP_SINK */

#endif /* CONFIG_BLUETOOTH_A2DP */
#endif /* __SAL_A2DP_EP_H__ */