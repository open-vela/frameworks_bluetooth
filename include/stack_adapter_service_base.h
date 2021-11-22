/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_service_base.h
Abstract:
    Bluetooth Stack service definitions.
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef __SERVICE_STACK_ADAPTER_BASE_H__
#define __SERVICE_STACK_ADAPTER_BASE_H__

#define UUID_SIZE 16
#define BD_ADDR_SIZE 6
#define BD_NAME_MAX_SIZE 32
#define BT_COMMON_KEY_SIZE 16

typedef enum {
    SERVICE_BT_STATUS_SUCCESS,
    SERVICE_BT_STATUS_FAIL,
    SERVICE_BT_STATUS_NOT_READY,
    SERVICE_BT_STATUS_NOMEM,
    SERVICE_BT_STATUS_BUSY,
    SERVICE_BT_STATUS_DONE,
    SERVICE_BT_STATUS_UNSUPPORTED,
    SERVICE_BT_STATUS_PARM_INVALID,
    SERVICE_BT_STATUS_UNHANDLED,
    SERVICE_BT_STATUS_AUTH_FAILURE,     /* remote accepts AUTH request, but AUTH failure */
    SERVICE_BT_STATUS_RMT_DEV_DOWN,     /* remote device not in BT range */
    SERVICE_BT_STATUS_AUTH_REJECTED,    /* remote rejects AUTH request */
    SERVICE_BT_STATUS_RMT_DEV_TERMINATE /* remote disconnect the link actively */
} SERVICE_BT_STATUS;

typedef uint8_t BT_UUID_T[UUID_SIZE];

typedef uint8_t BD_ADDR[BD_ADDR_SIZE];

typedef void *BT_TIMER_OBJ;

typedef uint8_t BT_COMMON_KEY[BT_COMMON_KEY_SIZE];

/* * Bluetooth ACL connection state */
typedef enum {
    SERVICE_BT_ACL_STATE_CONNECTED,
    SERVICE_BT_ACL_STATE_CONNECTING,
    SERVICE_BT_ACL_STATE_CONNECT_REQUEST, /* receive incoming connect request */
    SERVICE_BT_ACL_STATE_DISCONNECTED,
    SERVICE_BT_ACL_STATE_LE_CONNECTED,
    SERVICE_BT_ACL_STATE_LE_CONNECTING,
    SERVICE_BT_ACL_STATE_LE_DISCONNECTED
} SERVICE_BT_ACL_STATE;

/* * Bluetooth Bond state */
typedef enum {
    SERVICE_BT_BOND_STATE_NONE,
    SERVICE_BT_BOND_STATE_BONDING,
    SERVICE_BT_BOND_STATE_BONDED,
    SERVICE_BT_BOND_STATE_SDP_DONE,
    SERVICE_BT_BOND_STATE_BLE_NONE,
    SERVICE_BT_BOND_STATE_BLE_BONDING,
    SERVICE_BT_BOND_STATE_BLE_BONDED
} SERVICE_BT_BOND_STATE;

/* * Bluetooth link mode */
typedef enum { SERVICE_BT_MODE_ACTIVE, SERVICE_BT_MODE_SNIFF } SERVICE_BT_LINK_MODE;

/* * Profile Connection State */
typedef enum {
    SERVICE_PROFILE_DISCONNECTED,
    SERVICE_PROFILE_CONNECTING,
    SERVICE_PROFILE_CONNECTED,
    SERVICE_PROFILE_DISCONNECTING
} SERVICE_PROFILE_CONNECTION_STATE;

/* AVDTP Codec Type */
typedef enum {
    SERVICE_AVDTP_CODEC_TYPE_SBC,
    SERVICE_AVDTP_CODEC_TYPE_MPEG1_2_AUDIO,
    SERVICE_AVDTP_CODEC_TYPE_MPEG2_4_AAC,
    SERVICE_AVDTP_CODEC_TYPE_ATRAC,
    SERVICE_AVDTP_CODEC_TYPE_OPUS,
    SERVICE_AVDTP_CODEC_TYPE_H263,
    SERVICE_AVDTP_CODEC_TYPE_MPEG4_VSP,
    SERVICE_AVDTP_CODEC_TYPE_H263_PROF3,
    SERVICE_AVDTP_CODEC_TYPE_H263_PROF8,
    SERVICE_AVDTP_CODEC_TYPE_LHDC,
    SERVICE_AVDTP_CODEC_TYPE_NON_A2DP
} SERVICE_AVDTP_CODEC_TYPE;

/* PCM channel mode */
typedef enum { SERVICE_CHANNEL_MONO, SERVICE_CHANNEL_STEREO } SERVICE_CHANNEL_MODE;

/* PCM encoding format */
typedef enum { SERVICE_BIT_WIDTH_8, SERVICE_BIT_WIDTH_16 } SERVICE_BIT_WIDTH;

/* Stream config */
typedef struct {
    uint32_t sample_rate;
    uint8_t codec;
    uint8_t channel;
    uint8_t bit_width;
    uint8_t reserved;
} SERVICE_A2DP_STREAM_CONFIG_S;

/* * SCO Connection State */
typedef enum { SERVICE_HFP_SCO_CONNECTED, SERVICE_HFP_SCO_DISCONNECTED, SERVICE_HFP_SCO_UNKNOWN } SERVICE_HFP_SCO_STATE;

/* * Codec Type */
typedef enum {
    SERVICE_HFP_CODEC_UNKONWN, /* init state */
    SERVICE_HFP_CODEC_MSBC,
    SERVICE_HFP_CODEC_CVSD
} SERVICE_HFP_CODEC_TYPE;

/* hfp config */
typedef struct {
    uint32_t sample_rate;
    uint8_t codec;
    uint8_t bit_width;
    uint8_t reserved1;
    uint8_t reserved2;
} SERVICE_HFP_CONFIG_S;

/* SPP Connection Request Type */
typedef enum { SERVICE_SPP_CONNECTED_REQUEST, SERVICE_SPP_DISCONNECTED_REQUEST } SERVICE_SPP_CONNECTION_REQUEST;
// ************ux and stack common end************

#endif
