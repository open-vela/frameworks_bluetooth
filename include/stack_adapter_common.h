/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_types.h
Abstract:
    Bluetooth Stack types
Author:
    Y.D.X
---------------------------------------------------------------------------*/

/*******************************************************************************
 * COMMON
 ******************************************************************************/
#ifndef __STACK_ADAPTER_COMMON_H__
#define __STACK_ADAPTER_COMMON_H__

#include "stack_adapter_platform_dep.h"
#include "stack_adapter_service_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * HFP
 ******************************************************************************/

/* * HFP HF supported features - bit mask */
#define BT_HFP_BRSF_HF_NREC 0x00000001                 /* * 0, EC and/or NR function */
#define BT_HFP_BRSF_HF_3WAYCALL 0x00000002             /* * 1, Call waiting and 3-way calling */
#define BT_HFP_BRSF_HF_CLIP 0x00000004                 /* * 2, CLI presentation capability */
#define BT_HFP_BRSF_HF_BVRA 0x00000008                 /* * 3, Voice recognition activation */
#define BT_HFP_BRSF_HF_RMTVOLCTRL 0x00000010           /* * 4, Remote volume control */
#define BT_HFP_BRSF_HF_ENHANCED_CALLSTATUS 0x00000020  /* * 5, Enhanced call status */
#define BT_HFP_BRSF_HF_ENHANCED_CALLCONTROL 0x00000040 /* * 6, Enhanced call control */
#define BT_HFP_BRSF_HF_CODEC_NEGOTIATION 0x00000080    /* * 7, Codec negotiation */
#define BT_HFP_BRSF_HF_HFINDICATORS 0x00000100         /* * 8, HF Indicators */
#define BT_HFP_BRSF_HF_ESCO_S4T2_SETTING 0x00000200    /* * 9, eSCO S4 (and T2) settings supported */

/* * Call setup state */
typedef enum {
    HFP_CALL_SETUP_STATE_NONE,
    HFP_CALL_SETUP_STATE_IN,
    HFP_CALL_SETUP_STATE_OUT,
    HFP_CALL_SETUP_STATE_ALERT
} SERVICE_HFP_CALL_SETUP_STATE;

/* * Call active state */
typedef enum { HFP_CALL_ACTIVE_STATE_NONE, HFP_CALL_ACTIVE_STATE_ACTIVE } SERVICE_HFP_CALL_ACTIVE_STATE;

/* * Call held state */
typedef enum { HFP_CALL_HELD_STATE_NONE, HFP_CALL_HELD_STATE_HELD } SERVICE_HFP_CALL_HELD_STATE;

typedef struct {
    char *at_string;
    uint16_t at_length;
} SERVICE_HFP_AT_CMD_S;

typedef enum { VOLUME_MIC, VOLUME_SPEAKER } SERVICE_HFP_VOLUME_TYPE;

/* * Possible values of n value of AT+CHLD=<n> command. */
typedef enum {
    HFP_CALL_CONTROL_CHLD_0,
    HFP_CALL_CONTROL_CHLD_1,
    HFP_CALL_CONTROL_CHLD_2,
    HFP_CALL_CONTROL_CHLD_3,
    HFP_CALL_CONTROL_CHLD_4
} SERVICE_HFP_CALL_CONTROL_CODE;

/* * Current call - dir. */
typedef enum { HFP_CURRENT_CALL_DIR_OUTGOING, HFP_CURRENT_CALL_DIR_INCOMING } SERVICE_HFP_CURRENT_CALL_DIR;

/* * Current call - status. */
typedef enum {
    HFP_CURRENT_CALL_STATUS_ACTIVE,
    HFP_CURRENT_CALL_STATUS_HELD,
    HFP_CURRENT_CALL_STATUS_DIALING,
    HFP_CURRENT_CALL_STATUS_ALERTING,
    HFP_CURRENT_CALL_STATUS_INCOMING,
    HFP_CURRENT_CALL_STATUS_WAITING,
    HFP_CURRENT_CALL_STATUS_BTRH_HOLD
} SERVICE_HFP_CURRENT_CALL_STATUS;

/* * Current call - status. */
typedef enum { HFP_CURRENT_CALL_MODE_VOICE, HFP_CURRENT_CALL_MODE_DATA, HFP_CURRENT_CALL_MODE_FAX } SERVICE_HFP_CURRENT_CALL_MODE;

/* * Current call - mpty. */
typedef enum { HFP_CURRENT_CALL_MPTY_NO, HFP_CURRENT_CALL_MPTY_YES } SERVICE_HFP_CURRENT_CALL_MPTY;

/*******************************************************************************
 * A2DP
 ******************************************************************************/

/* * A2DP stream state */
typedef enum {
    A2DP_STREAM_UNKNOWN, /* no a2dp */
    A2DP_STREAM_IDLE,
    A2DP_STREAM_CLOSED,
    A2DP_STREAM_OPENED,
    A2DP_STREAM_SUSPENDED,
    A2DP_STREAM_STREAMING
} SERVICE_A2DP_STREAM_STATE;

/* A2DP stream reqest type */
typedef enum {
    A2DP_STREAM_REQUEST_OPEN,
    A2DP_STREAM_REQUEST_CLOSE,
    A2DP_STREAM_REQUEST_START,
    A2DP_STREAM_REQUEST_SUSPEND
} SERVICE_A2DP_STREAM_REQUEST;

typedef struct {
    uint8_t version;
    uint8_t padding;
    uint8_t marker;
    uint8_t payloadType;
    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint32_t ssrc;
    uint8_t csrcCount;
    uint32_t csrcList[15];
} SERVICE_A2DP_HEADER_S;

typedef struct {
    SERVICE_A2DP_HEADER_S header;
    uint16_t data_length;
    uint8_t data[0];
} SERVICE_A2DP_PACKET_S;

/* * A2DP sink audio data */
typedef struct {
    uint8_t *p_buffer;  // in out
    uint16_t length;    // in
} SERVICE_A2DP_SINK_DATA_S;

/* * A2DP sink audio data */
typedef SERVICE_A2DP_PACKET_S SERVICE_A2DP_SOURCE_PACKET_S;

typedef struct {
    uint16_t data_length;
    uint8_t data[0];
} SERVICE_A2DP_SOURCE_PACKAGE_S;

/*******************************************************************************
 * AVRCP
 ******************************************************************************/

/* * Stack Avrcp Panel Operation */
typedef enum {
    AVRCP_OPERATION_SELECT,
    AVRCP_OPERATION_UP,
    AVRCP_OPERATION_DOWN,
    AVRCP_OPERATION_LEFT,
    AVRCP_OPERATION_RIGHT,
    AVRCP_OPERATION_RIGHT_UP,
    AVRCP_OPERATION_RIGHT_DOWN,
    AVRCP_OPERATION_LEFT_UP,
    AVRCP_OPERATION_LEFT_DOWN,
    AVRCP_OPERATION_ROOT_MENU,
    AVRCP_OPERATION_SETUP_MENU,
    AVRCP_OPERATION_CONTENTS_MENU,
    AVRCP_OPERATION_FAVORITE_MENU,
    AVRCP_OPERATION_EXIT,
    AVRCP_OPERATION_0,
    AVRCP_OPERATION_1,
    AVRCP_OPERATION_2,
    AVRCP_OPERATION_3,
    AVRCP_OPERATION_4,
    AVRCP_OPERATION_5,
    AVRCP_OPERATION_6,
    AVRCP_OPERATION_7,
    AVRCP_OPERATION_8,
    AVRCP_OPERATION_9,
    AVRCP_OPERATION_DOT,
    AVRCP_OPERATION_ENTER,
    AVRCP_OPERATION_CLEAR,
    AVRCP_OPERATION_CHANNEL_UP,
    AVRCP_OPERATION_CHANNEL_DOWN,
    AVRCP_OPERATION_PREVIOUS_CHANNEL,
    AVRCP_OPERATION_SOUND_SELECT,
    AVRCP_OPERATION_INPUT_SELECT,
    AVRCP_OPERATION_DISPLAY_INFO,
    AVRCP_OPERATION_HELP,
    AVRCP_OPERATION_PAGE_UP,
    AVRCP_OPERATION_PAGE_DOWN,
    AVRCP_OPERATION_POWER,
    AVRCP_OPERATION_VOLUME_UP,
    AVRCP_OPERATION_VOLUME_DOWN,
    AVRCP_OPERATION_MUTE,
    AVRCP_OPERATION_PLAY,
    AVRCP_OPERATION_STOP,
    AVRCP_OPERATION_PAUSE,
    AVRCP_OPERATION_RECORD,
    AVRCP_OPERATION_REWIND,
    AVRCP_OPERATION_FAST_FORWARD,
    AVRCP_OPERATION_EJECT,
    AVRCP_OPERATION_FORWARD,
    AVRCP_OPERATION_BACKWARD,
    AVRCP_OPERATION_ANGLE,
    AVRCP_OPERATION_SUBPICTURE,
    AVRCP_OPERATION_F1,
    AVRCP_OPERATION_F2,
    AVRCP_OPERATION_F3,
    AVRCP_OPERATION_F4,
    AVRCP_OPERATION_F5,
    AVRCP_OPERATION_VENDOR_UNIQUE,
    AVRCP_OPERATION_NEXT_GROUP,
    AVRCP_OPERATION_PREV_GROUP,
    AVRCP_OPERATION_RESERVED
} SERVICE_AVRCP_PANEL_OPERATION;

/* * Stack Avrcp Panel State */
typedef enum { AVRCP_PANEL_PRESS, AVRCP_PANEL_RELEASE, AVRCP_PANEL_HOLD } SERVICE_AVRCP_PANEL_STATE;

/* * Stack Avrcp Response Type */
typedef enum {
    SERVICE_AVRCP_RESPONSE_NOT_IMPLEMENTED,
    SERVICE_AVRCP_RESPONSE_ACCEPTED,
    SERVICE_AVRCP_RESPONSE_REJECTED,
    SERVICE_AVRCP_RESPONSE_IN_TRANSITION,
    SERVICE_AVRCP_RESPONSE_IMPLEMENTED_STABLE,
    SERVICE_AVRCP_RESPONSE_CHANGED,
    SERVICE_AVRCP_RESPONSE_INTERIM,
    SERVICE_AVRCP_RESPONSE_BROWSING,
    SERVICE_AVRCP_RESPONSE_SKIPPED,
    SERVICE_AVRCP_RESPONSE_TIMEOUT
} SERVICE_AVRCP_RESPONSE;

/* * Stack Avrcp Result Status */
typedef enum {
    SERVICE_AVRCP_STATUS_INVALID_COMMAND,
    SERVICE_AVRCP_STATUS_INVALID_PARAMETER,
    SERVICE_AVRCP_STATUS_PARAMETER_CONTENT_ERROR,
    SERVICE_AVRCP_STATUS_INTERNAL_ERROR,
    SERVICE_AVRCP_STATUS_COMPLETED_SUCCESSFULLY,
    SERVICE_AVRCP_STATUS_UID_CHANGED,
    SERVICE_AVRCP_STATUS_RESERVED_6,
    SERVICE_AVRCP_STATUS_INVALID_DIRECTION,
    SERVICE_AVRCP_STATUS_NOT_DIRECTORY,
    SERVICE_AVRCP_STATUS_NOT_EXIST,
    SERVICE_AVRCP_STATUS_INVALID_SCOPE,
    SERVICE_AVRCP_STATUS_RANGE_OUT_OF_BOUNDS,
    SERVICE_AVRCP_STATUS_ITEM_NOT_PLAYABLE,
    SERVICE_AVRCP_STATUS_MEDIA_IN_USE,
    SERVICE_AVRCP_STATUS_NOW_PLAYING_LIST_FULL,
    SERVICE_AVRCP_STATUS_SEARCH_NOT_SUPPORTED,
    SERVICE_AVRCP_STATUS_SEARCH_IN_PROGRESS,
    SERVICE_AVRCP_STATUS_INVALID_PLAYER_ID,
    SERVICE_AVRCP_STATUS_FOLDER_NOT_BROWSABLE,
    SERVICE_AVRCP_STATUS_PLAYER_NOT_ADDRESSED,
    SERVICE_AVRCP_STATUS_NO_VALID_SEARCH_RESULTS,
    SERVICE_AVRCP_STATUS_NO_AVAILABLE_PLAYERS,
    SERVICE_AVRCP_STATUS_ADDRESSED_PLAYER_CHANGED
} SERVICE_AVRCP_STATUS;

/* * Stack Avrcp notification events */
typedef enum {
    AVRCP_NOTIFICATION_MEDIA_STATUS_CHANGED,
    AVRCP_NOTIFICATION_TRACK_CHANGED,
    AVRCP_NOTIFICATION_TRACK_END,
    AVRCP_NOTIFICATION_TRACK_START,
    AVRCP_NOTIFICATION_PLAY_POS_CHANGED,
    AVRCP_NOTIFICATION_BATTERY_STATUS_CHANGED,
    AVRCP_NOTIFICATION_SYSTEM_STATUS_CHANGED,
    AVRCP_NOTIFICATION_APP_SETTING_CHANGED,
    AVRCP_NOTIFICATION_NOW_PLAYING_CONTENT_CHANGED,
    AVRCP_NOTIFICATION_AVAILABLE_PLAYERS_CHANGED,
    AVRCP_NOTIFICATION_ADDRESSED_PLAYER_CHANGED,
    AVRCP_NOTIFICATION_UIDS_CHANGED,
    AVRCP_NOTIFICATION_VOLUME_CHANGED,
    AVRCP_NOTIFICATION_FLAG_INTERIM
} SERVICE_AVRCP_NOTIFICATION_EVENT;

typedef enum {
    MEDIA_STOPPED,
    MEDIA_PLAYING,
    MEDIA_PAUSED,
    MEDIA_FWD_SEEK,
    MEDIA_REV_SEEK,
    MEDIA_ERROR
} SERVICE_AVRCP_MEDIA_STATUS;

typedef enum {
    ATTR_TITLE,
    ATTR_ARTIST_NAME,
    ATTR_ALBUM_NAME,
    ATTR_MEDIA_NUMBER,
    ATTR_MEDIA_TOTAL_NUMBER,
    ATTR_GENRE,
    ATTR_PLAYING_TIME_MS
} SERVICE_AVRCP_MEDIA_ATTR_TYPE;

/* * Stack Avrcp Capability ID */
typedef enum {
    SERVICE_AVRCP_CAPABILITY_ID_COMPANY_ID = 2,
    SERVICE_AVRCP_CAPABILITY_ID_EVENTS_SUPPORTED
} SERVICE_AVRCP_CAPABILITY_ID;

typedef union {
    SERVICE_AVRCP_MEDIA_STATUS media_status; /* AVRCP_NOTIFICATION_MEDIA_STATUS_CHANGED */
    uint32_t position;                       /* AVRCP_NOTIFICATION_PLAY_POS_CHANGED */
    uint8_t volume;                          /* AVRCP_NOTIFICATION_VOLUME_CHANGED */
} SERVICE_AVRCP_NOTIFICATION_VALUE_U;

/*******************************************************************************
 * SPP
 ******************************************************************************/

/* * SPP TWS Data Prefix */
extern const uint8_t SERVICE_SPP_TWS_DATA_PREFIX[];

/*
 * SPP PORT: 10bits(id) + 5bits((server channel) + 1bit(1-server,0-client)
 */
typedef uint16_t SERVICE_SPP_PORT;

typedef uint8_t *SERVICE_SPP_BUFFER;

/*******************************************************************************
 * GATT
 ******************************************************************************/

/* Attribute permissions */
#define GATT_PERMISSION_READABLE 0x01
#define GATT_PERMISSION_WRITABLE 0x02
#define GATT_PERMISSION_ENCRYPTIION_REQUIRED 0x04
#define GATT_PERMISSION_AUTHENTICATION_REQUIRED 0x08
#define GATT_PERMISSION_MITM_REQUIRED 0x10

/* Characteristic Properties */
#define GATT_PROPERTY_BROADCAST 0x01
#define GATT_PROPERTY_READ 0x02
#define GATT_PROPERTY_WRITE_NO_RESPONSE 0x04
#define GATT_PROPERTY_WRITE 0x08
#define GATT_PROPERTY_NOTIFY 0x10
#define GATT_PROPERTY_INDICATE 0x20
#define GATT_PROPERTY_SIGNED_WRITE 0x40
#define GATT_PROPERTY_EXTENDED_PROPS 0x80

typedef enum {
    GATT_SUCCESS,
    GATT_FAILURE,
    GATT_REQUEST_NOT_SUPPORTED,
    GATT_INSUFFICIENT_AUTHENTICATION,
    GATT_INSUFFICIENT_ENCRYPTION,
    GATT_READ_NOT_PERMITTED,
    GATT_WRITE_NOT_PERMITTED,
    GATT_INVALID_ATTRIBUTE_LENGTH
} SERVICE_GATT_STATUS;

typedef enum {
    PRIMARY_SERVICE,
    SECONDARY_SERVICE,
    INCLUDED_SERVICE,
    CHARACTERISTIC,
    DESCRIPTOR
} SERVICE_GATT_ELEMENT_TYPE;

typedef struct {
    uint32_t id; /* For the server application, this shall be assigned and managed by the application to identify each element uniquely.
                  The id of an INCLUDED_SERVICE shall be the same as that of the PRIMARY_SERVICE or SECONDARY_SERVICE being included. */
    /* For the client application, this is the attribute handle returned from service discovery procedure. */
    BT_UUID_T uuid;
    SERVICE_GATT_ELEMENT_TYPE type;
    uint32_t properties; /* bit masks, characteristic properties - for characteristic type only */
    uint32_t permissions; /* bit masks, attribute permissions - for all types */
} SERVICE_GATT_ELEMENT_S;

typedef struct {
    uint32_t request_id;
    SERVICE_GATT_STATUS status;
    uint16_t length; /* value length */
    uint8_t value[0];
} SERVICE_GATT_RESPONSE_S;

/*******************************************************************************
 * HFP AG
 ******************************************************************************/

/* * HFP HF supported features - bit mask */
#define BT_HFP_BRSF_AG_3WAYCALL 0x00000001             /* * 0, Three-way calling */
#define BT_HFP_BRSF_AG_NREC 0x00000002                 /* * 1, EC and/or NR function */
#define BT_HFP_BRSF_AG_BVRA 0x00000004                 /* * 2, Voice recognition function */
#define BT_HFP_BRSF_AG_INBANDRING 0x00000008           /* * 3, In-band ring tone capability */
#define BT_HFP_BRSF_AG_BINP 0x00000010                 /* * 4, Attach a number to a voice tag */
#define BT_HFP_BRSF_AG_REJECT_CALL 0x00000020          /* * 5, Ability to reject a call */
#define BT_HFP_BRSF_AG_ENHANCED_CALLSTATUS 0x00000040  /* * 6, Enhanced call status */
#define BT_HFP_BRSF_AG_ENHANCED_CALLCONTROL 0x00000080 /* * 7, Enhanced call control */
#define BT_HFP_BRSF_AG_EXTENDED_ERRORRESULT 0x00000100 /* * 8, Extended Error Result Codes */
#define BT_HFP_BRSF_AG_CODEC_NEGOTIATION 0x00000200    /* * 9, Codec negotiation */
#define BT_HFP_BRSF_AG_HFINDICATORS 0x00000400         /* * 10, HF Indicators */
#define BT_HFP_BRSF_AG_eSCO_S4T2_SETTING 0x00000800    /* * 11, eSCO S4 (and T2) settings supported */

typedef enum {
    AG_CALL_STATE_ACTIVE,
    AG_CALL_STATE_HELD,
    AG_CALL_STATE_DIALING,
    AG_CALL_STATE_ALERTING,
    AG_CALL_STATE_INCOMING,
    AG_CALL_STATE_WAITING,
    AG_CALL_STATE_IDLE,
    AG_CALL_STATE_DISCONNECTED
} SERVICE_HFP_AG_CALL_STATE;

typedef struct {
    uint8_t type;
    uint16_t number_length;
    char number[0];
} SERVICE_HFP_AG_PHONE_NUMBER_S;

typedef struct {
    uint8_t service;
    uint8_t signal;
    uint8_t roam;
    uint8_t battery;
} SERVICE_HFP_AG_DEVICE_STATUS_S;

typedef struct {
    SERVICE_HFP_AG_DEVICE_STATUS_S device_status;
    uint8_t call;
    SERVICE_HFP_CALL_SETUP_STATE call_setup;
    uint8_t call_held;
} SERVICE_HFP_AG_CIND_RESPONSE_S;

typedef struct {
    uint32_t index;
    uint8_t dir;
    SERVICE_HFP_AG_CALL_STATE status;
    uint8_t mode;
    uint8_t mpty;
    SERVICE_HFP_AG_PHONE_NUMBER_S number;
} SERVICE_HFP_AG_CLCC_RESPONSE_S;

typedef struct {
    char *at_string;
    uint16_t at_length;
} SERVICE_HFP_AG_AT_CMD_S;

/*******************************************************************************
 * HID
 ******************************************************************************/

/* * HID Status */
typedef enum {
    BTHID_OK = 0,
    BTHID_HANDSHAKE_HID_NOT_READY,
    BTHID_HANDSHAKE_INVALID_REPORT_ID,
    BTHID_HANDSHAKE_TRANS_NOT_SPT,
    BTHID_HANDSHAKE_INVALID_PARAM,
    BTHID_HANDSHAKE_UNSPECIFIED_ERROR,
    BTHID_UNSPECIFIED_ERROR,
    BTHID_ERROR_SDP,
    BTHID_ERROR_SET_PROTOCOL,
    BTHID_ERROR_DATABASE_FULL,
    BTHID_ERROR_DEVICE_TYPE_UNSUPPORTED,
    BTHID_ERROR_NO_RESOURCES,
    BTHID_ERROR_AUTHENTICATION_FAILED,
    BTHID_ERROR_OPERATION_NOT_ALLOWED
} SERVICE_HID_STATUS;

/* * Protocol modes */
typedef enum {
    BTHID_REPORT_MODE = 0x00,
    BTHID_BOOT_MODE = 0x01,
    BTHID_UNSUPPORTED_MODE = 0xff
} SERVICE_HID_PROTOCOL_MODE;

/* * Report types */
typedef enum {
    BTHID_INPUT_REPORT = 1,
    BTHID_OUTPUT_REPORT,
    BTHID_FEATURE_REPORT,

    /* Following are used for reports received only */
    BTHID_BOOT_KB_REPORT = 0x80,
    BTHID_BOOT_MOUSE_REPORT = 0x81
} SERVICE_HID_REPORT_TYPE;

/* * HID Message Type */
typedef enum {
    BTHID_MSG_CONTROL = 1,
    BTHID_MSG_GET_REPORT = 4,
    BTHID_MSG_SET_REPORT,
    BTHID_MSG_GET_PROTOCOL,
    BTHID_MSG_SET_PROTOCOL,
    BTHID_MSG_GET_IDLE,
    BTHID_MSG_SET_IDLE,

    BTHID_MSG_CONTROL_SUSPEND = 23,
    BTHID_MSG_CONTROL_EXIT_SUSPEND,
} SERVICE_HID_MESSAGE_TYPE;

/* * HID supported features - bit mask */
#define BTHID_ATTR_MASK_VIRTUAL_CABLE           0x0001
#define BTHID_ATTR_MASK_RECONNECT_INITIATE      0x0002
#define BTHID_ATTR_MASK_BOOT_DEVICE             0x0004
#define BTHID_ATTR_MASK_BATTERY_POWER           0x0010
#define BTHID_ATTR_MASK_REMOTE_WAKE             0x0020
#define BTHID_ATTR_MASK_SUPERVISION_TIMEOUT     0x0080
#define BTHID_ATTR_MASK_NORMALLY_CONNECTABLE    0x0100
#define BTHID_ATTR_MASK_SSR_MAX_LATENCY         0x0200
#define BTHID_ATTR_MASK_SSR_MIN_TIMEOUT         0x0400
#define BTHID_ATTR_MASK_BREDR                   0x8000

typedef struct {
    uint32_t attr_mask;/* BTHID_ATTR_MASK_VIRTUAL_CABLE etc. */
    uint8_t sub_class;
    uint8_t country_code;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t version;
    uint16_t supervision_timeout;
    uint16_t ssr_max_latency;
    uint16_t ssr_min_timeout;
    uint16_t dsc_list_length;/* Length of desc_list */
    uint8_t *dsc_list;/* List of descriptors. Each descriptor is constructed as: Type(1 Byte), Length(2 Bytes, Little Endian), Values(Length Bytes) */
} SERVICE_HID_INFO_S;

/*******************************************************************************
 * GAP
 ******************************************************************************/
/* * Bluetooth device of class. Example: (BT_COD_SERVICE_AUDIO | BT_COD_AV_LOUD_SPEAKER)
 */
/* Major Service Classe bit mask */
#define BT_COD_SERVICE_BITS(c) (c & 0xFFE000)      /* The major service classes field */
#define BT_COD_DEVICE_MAJOR_BITS(c) (c & 0x001F00) /* The major device classes field */
#define BT_COD_DEVICE_CLASS_BITS(c) (c & 0x001FFC) /* The device classes field, including major and minor */

#define BT_COD_SERVICE_LDM 0x002000         /* Limited Discoverable Mode */
#define BT_COD_SERVICE_POSITION 0x010000    /* Positioning (Location identification) */
#define BT_COD_SERVICE_NETWORK 0x020000     /* Networking (LAN, Ad hoc, ...) */
#define BT_COD_SERVICE_RENDERING 0x040000   /* Rending (Printing, Speaker, ...) */
#define BT_COD_SERVICE_CAPTURING 0x080000   /* Capturing (Scanner, Microphone, ...) */
#define BT_COD_SERVICE_OBJECT 0x100000      /* Object Transfer (v-Inbox, v-Folder, ...) */
#define BT_COD_SERVICE_AUDIO 0x200000       /* Audio (Speaker, Microphone, Headset service, ...) */
#define BT_COD_SERVICE_TELEPHONY 0x400000   /* Telephony (Cordless telephony, Modem, Headset service, ...) */
#define BT_COD_SERVICE_INFORMATION 0x800000 /* Information (WEB-server, WAP-server, ...) */

/* * Major Device Classes bit mask */
#define BT_COD_DEVICE_MISCELLANEOUS 0x000000 /* Major Device Class - Miscellaneous */
#define BT_COD_DEVICE_COMPUTER 0x000100 /* Major Device Class - Computer (desktop, notebook, PDA, organizers, ...) */
#define BT_COD_DEVICE_PHONE 0x000200    /* Major Device Class - Phone (cellular, cordless, payphone, modem, ...) */
#define BT_COD_DEVICE_LAP 0x000300      /* Major Device Class - LAN/Network Access Point */
#define BT_COD_DEVICE_AV \
    0x000400 /* Major Device Class - Audio/Video (headset, speaker, stereo, video display, vcr...) */
#define BT_COD_DEVICE_PERIPHERAL 0x000500 /* Major Device Class - Peripheral (mouse, joystick, keyboards, ...) */
#define BT_COD_DEVICE_IMAGING 0x000600    /* Major Device Class - Imaging (printing, scanner, camera, display, ...) */
#define BT_COD_DEVICE_WEARABLE 0x000700   /* Major Device Class - Wearable */
#define BT_COD_DEVICE_TOY 0x000800        /* Major Device Class - Toy */
#define BT_COD_DEVICE_HEALTH 0x000900     /* Major Device Class - Health */
#define BT_COD_DEVICE_UNCLASSIFIED                                                     \
    0x001F00 /* Major Device Class - Uncategorized, specific device code not specified \
              */

/* * Minor Device Class - Computer major class */
#define BT_COD_COMPUTER_UNCLASSIFIED (BT_COD_DEVICE_COMPUTER | 0x000000)
#define BT_COD_COMPUTER_DESKTOP (BT_COD_DEVICE_COMPUTER | 0x000004)
#define BT_COD_COMPUTER_SERVER (BT_COD_DEVICE_COMPUTER | 0x000008)
#define BT_COD_COMPUTER_LAPTOP (BT_COD_DEVICE_COMPUTER | 0x00000C)
#define BT_COD_COMPUTER_HANDHELD (BT_COD_DEVICE_COMPUTER | 0x000010)
#define BT_COD_COMPUTER_PALMSIZED (BT_COD_DEVICE_COMPUTER | 0x000014)
#define BT_COD_COMPUTER_WEARABLE (BT_COD_DEVICE_COMPUTER | 0x000018)

/* * Minor Device Class - Phone major class */
#define BT_COD_PHONE_UNCLASSIFIED (BT_COD_DEVICE_PHONE | 0x000000)
#define BT_COD_PHONE_CELLULAR (BT_COD_DEVICE_PHONE | 0x000004)
#define BT_COD_PHONE_CORDLESS (BT_COD_DEVICE_PHONE | 0x000008)
#define BT_COD_PHONE_SMARTPHONE (BT_COD_DEVICE_PHONE | 0x00000C)
#define BT_COD_PHONE_WIREDMODEM (BT_COD_DEVICE_PHONE | 0x000010)
#define BT_COD_PHONE_COMMONISDNACCESS (BT_COD_DEVICE_PHONE | 0x000014)
#define BT_COD_PHONE_SIMCARDREADER (BT_COD_DEVICE_PHONE | 0x000018)

/* * Minor Device Class - LAN/Network access point major class */
#define BT_COD_LAP_FULLY_AVAILABLE (BT_COD_DEVICE_LAP | 0x000000)
#define BT_COD_LAP_17_UTILIZED (BT_COD_DEVICE_LAP | 0x000020)
#define BT_COD_LAP_33_UTILIZED (BT_COD_DEVICE_LAP | 0x000040)
#define BT_COD_LAP_50_UTILIZED (BT_COD_DEVICE_LAP | 0x000060)
#define BT_COD_LAP_67_UTILIZED (BT_COD_DEVICE_LAP | 0x000080)
#define BT_COD_LAP_83_UTILIZED (BT_COD_DEVICE_LAP | 0x0000A0)
#define BT_COD_LAP_99_UTILIZED (BT_COD_DEVICE_LAP | 0x0000C0)
#define BT_COD_LAP_UNAVAILABLE (BT_COD_DEVICE_LAP | 0x0000E0)

/* * Minor Device Class - Audio/Video major class */
#define BT_COD_AV_UNCLASSIFIED (BT_COD_DEVICE_AV | 0x000000)
#define BT_COD_AV_HEADSET (BT_COD_DEVICE_AV | 0x000004)
#define BT_COD_AV_HANDSFREE (BT_COD_DEVICE_AV | 0x000008)
#define BT_COD_AV_HEADANDHAND (BT_COD_DEVICE_AV | 0x00000C)
#define BT_COD_AV_MICROPHONE (BT_COD_DEVICE_AV | 0x000010)
#define BT_COD_AV_LOUD_SPEAKER (BT_COD_DEVICE_AV | 0x000014)
#define BT_COD_AV_HEADPHONES (BT_COD_DEVICE_AV | 0x000018)
#define BT_COD_AV_PORTABLE_AUDIO (BT_COD_DEVICE_AV | 0x00001C)
#define BT_COD_AV_CAR_AUDIO (BT_COD_DEVICE_AV | 0x000020)
#define BT_COD_AV_SETTOPBOX (BT_COD_DEVICE_AV | 0x000024)
#define BT_COD_AV_HIFI_AUDIO (BT_COD_DEVICE_AV | 0x000028)
#define BT_COD_AV_VCR (BT_COD_DEVICE_AV | 0x00002C)
#define BT_COD_AV_VIDEO_CAMERA (BT_COD_DEVICE_AV | 0x000030)
#define BT_COD_AV_CAMCORDER (BT_COD_DEVICE_AV | 0x000034)
#define BT_COD_AV_VIDEO_MONITOR (BT_COD_DEVICE_AV | 0x000038)
#define BT_COD_AV_DISPLAY_AND_SPEAKER (BT_COD_DEVICE_AV | 0x00003C)
#define BT_COD_AV_VIDEO_CONFERENCING (BT_COD_DEVICE_AV | 0x000040)
#define BT_COD_AV_GAME_OR_TOY (BT_COD_DEVICE_AV | 0x000048)

/* * Minor Device Class - Peripheral major class */
#define BT_COD_PERIPHERAL_UNCLASSIFIED (BT_COD_DEVICE_PERIPHERAL | 0x000000)
#define BT_COD_PERIPHERAL_JOYSTICK (BT_COD_DEVICE_PERIPHERAL | 0x000004)
#define BT_COD_PERIPHERAL_GAMEPAD (BT_COD_DEVICE_PERIPHERAL | 0x000008)
#define BT_COD_PERIPHERAL_REMCONTROL (BT_COD_DEVICE_PERIPHERAL | 0x00000C)
#define BT_COD_PERIPHERAL_SENSE (BT_COD_DEVICE_PERIPHERAL | 0x000010)
#define BT_COD_PERIPHERAL_TABLET (BT_COD_DEVICE_PERIPHERAL | 0x000014)
#define BT_COD_PERIPHERAL_SIMCARDREADER (BT_COD_DEVICE_PERIPHERAL | 0x000018)
#define BT_COD_PERIPHERAL_KEYBOARD (BT_COD_DEVICE_PERIPHERAL | 0x000040)
#define BT_COD_PERIPHERAL_POINT (BT_COD_DEVICE_PERIPHERAL | 0x000080)
#define BT_COD_PERIPHERAL_KEYORPOINT (BT_COD_DEVICE_PERIPHERAL | 0x0000C0)

/* * Minor Device Class - Imaging major class */
#define BT_COD_IMAGING_DISPLAY (BT_COD_DEVICE_IMAGING | 0x000010)
#define BT_COD_IMAGING_CAMERA (BT_COD_DEVICE_IMAGING | 0x000020)
#define BT_COD_IMAGING_SCANNER (BT_COD_DEVICE_IMAGING | 0x000040)
#define BT_COD_IMAGING_PRINTER (BT_COD_DEVICE_IMAGING | 0x000080)

/* * Minor Device Class - Wearable major class */
#define BT_COD_WERABLE_WATCH (BT_COD_DEVICE_WEARABLE | 0x000004)
#define BT_COD_WERABLE_PAGER (BT_COD_DEVICE_WEARABLE | 0x000008)
#define BT_COD_WERABLE_JACKET (BT_COD_DEVICE_WEARABLE | 0x00000C)
#define BT_COD_WERABLE_HELMET (BT_COD_DEVICE_WEARABLE | 0x000010)
#define BT_COD_WERABLE_GLASSES (BT_COD_DEVICE_WEARABLE | 0x000014)

/* * Minor Device Class - Toy major class */
#define BT_COD_TOY_ROBOT (BT_COD_DEVICE_TOY | 0x000004)
#define BT_COD_TOY_VEHICLE (BT_COD_DEVICE_TOY | 0x000008)
#define BT_COD_TOY_DOLL (BT_COD_DEVICE_TOY | 0x00000C)
#define BT_COD_TOY_CONROLLER (BT_COD_DEVICE_TOY | 0x000010)
#define BT_COD_TOY_GAME (BT_COD_DEVICE_TOY | 0x000014)

/* * Minor Device Class - Health major class */
#define BT_COD_HEALTH_BLOOD_PRESURE (BT_COD_DEVICE_HEALTH | 0x000004)
#define BT_COD_HEALTH_THERMOMETER (BT_COD_DEVICE_HEALTH | 0x000008)
#define BT_COD_HEALTH_WEIGHING_SCALE (BT_COD_DEVICE_HEALTH | 0x00000C)
#define BT_COD_HEALTH_GLUCOSE_METER (BT_COD_DEVICE_HEALTH | 0x000010)
#define BT_COD_HEALTH_PULSE_OXIMETER (BT_COD_DEVICE_HEALTH | 0x000014)
#define BT_COD_HEALTH_RATE_MONITOR (BT_COD_DEVICE_HEALTH | 0x000018)
#define BT_COD_HEALTH_DATA_DISPLAY (BT_COD_DEVICE_HEALTH | 0x00001C)

// todo:clarify detals of REASON CODE from stack - If possible, suggest using the standard HCI error code.
typedef uint32_t SERVICE_ACL_DISCONNECTED_REASON;

/* * Local IO capability, shall be the same value defined in HCI Specification. */
typedef enum {
    SERVICE_BT_IO_CAPABILITY_DISPLAYONLY,
    SERVICE_BT_IO_CAPABILITY_DISPLAYYESNO,
    SERVICE_BT_IO_CAPABILITY_KEYBOARDONLY,
    SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT,
    SERVICE_BT_IO_CAPABILITY_KEYBOARDDISPLAY
} SERVICE_BT_IO_CAPABILITY;

/* * GAP SSP Bonding Type */
typedef enum {
    GAP_SPP_TYPE_PASSKEY_CONFIRMATION,
    GAP_SPP_TYPE_PASSKEY_ENTRY,
    GAP_SPP_TYPE_CONSENT,
    GAP_SPP_TYPE_PASSKEY_NOTIFICATION
} GAP_SERVICE_SPP_TYPE;

/* * Bluetooth Device Type */
typedef enum { BT_DEVICE_DEVTYPE_BREDR, BT_DEVICE_DEVTYPE_BLE, BT_DEVICE_DEVTYPE_DUAL } SERVICE_BT_DEVICE_TYPE;

/* * Bluetooth Scan Mode */
typedef enum { SCAN_MODE_NONE, SCAN_MODE_CONNECTABLE, SCAN_MODE_CONNECTABLE_DISCOVERABLE } SERVICE_BT_SCAN_MODE;

/* * Bluetooth stack state */
typedef enum { BT_STATE_OFF, BT_STATE_ON } SERVICE_BT_STACK_STATE;

/* * Bluetooth discovery state */
typedef enum { BT_DISCOVERY_STOPPED, BT_DISCOVERY_STARTED } SERVICE_BT_DISCOVERY_STATE;

/* * BLE address type */
typedef enum {
    BLE_ADDR_PUBLIC,
    BLE_ADDR_RANDOM,
    BLE_ADDR_PUBLIC_ID,
    BLE_ADDR_RANDOM_ID,
    BLE_ADDR_ANONYMOUS
} SERVICE_BLE_ADDR_TYPE;

/* * BLE PHY type */
typedef enum { BLE_1M_PHY, BLE_2M_PHY, BLE_CODED_PHY } SERVICE_BLE_PHY_TYPE;

/* * BLE advertising filter type */
typedef enum {
    BLE_ADV_FILTER_WHITE_LIST_FOR_NONE,/* Scan and Connection requests from ANY devices */
    BLE_ADV_FILTER_WHITE_LIST_FOR_SCAN,/* Connection requests from ANY devices; Scan requests from devices in the White List */
    BLE_ADV_FILTER_WHITE_LIST_ROR_CONNECTION,/* Scan request form ANY devices; Connection requests from devices in the White List */
    BLE_ADV_FILTER_WHITE_LIST_FOR_ALL /* Scan and Connection reqeusts from devices in the White List */
} SERVICE_BLE_ADVERTISING_FILTER_POLICY;

/* * BLE connection filter type */
typedef enum { BLE_CONNECT_FILTER_ADDR, BLE_CONNECT_FILTER_WHITE_LIST } SERVICE_BLE_CONNECT_FILTER_POLICY;

/* * BLE event type */
typedef enum {
    BLE_ADV_IND,
    BLE_ADV_DIRECT_IND,
    BLE_ADV_SCAN_IND,
    BLE_ADV_NONCONN_IND,
    BLE_SCAN_RSP
} SERVICE_BLE_EVENT_TYPE;

/* * BLE ADV channel map */
typedef enum {
    ADV_CHANNEL_DEFAULT,
    ADV_CHANNEL_37_ONLY,
    ADV_CHANNEL_38_ONLY,
    ADV_CHANNEL_39_ONLY
} SERVICE_BLE_ADV_CHANNEL;

/* * Bluetooth link role */
typedef enum { BT_ROLE_MASTER, BT_ROLE_SLAVE, BT_ROLE_UNKNOWN } SERVICE_BT_LINK_ROLE;

/* * Bluetooth link policy */
typedef enum {
    BT_LINK_POLICY_DISABLE_ALL,
    BT_LINK_POLICY_ENABLE_ROLE_SWITCH,
    BT_LINK_POLICY_ENABLE_SNIFF,
    BT_LINK_POLICY_ENABLE_ROLE_SWITCH_AND_SNIFF
} SERVICE_BT_LINK_POLICY;

/* * Bluetooth test mode */
typedef enum { BT_TESTMODE_DUT, BT_TESTMODE_BLE } SERVICE_BT_TEST_MODE;

/* * Bluetooth Link Key Type */
typedef enum {
    BT_KEY_TYPE_COMBINATION_KEY,
    BT_KEY_TYPE_LOCAL_UNIT_KEY,
    BT_KEY_TYPE_REMOTE_UNIT_KEY,
    BT_KEY_TYPE_DEBUG_COMBINATION_KEY,
    BT_KEY_TYPE_UNAUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192,
    BT_KEY_TYPE_AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192,
    BT_KEY_TYPE_CHANGED_COMBINATION_KEY,
    BT_KEY_TYPE_UNAUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256,
    BT_KEY_TYPE_AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256
} SERVICE_LINK_KEY_TYPE;

/* * HCI Event Filter - Filter Type */
typedef enum {
    BT_EVENT_FILTER_CLEAR_ALL,
    BT_EVENT_FILTER_INQUIRY_RESULT,
    BT_EVENT_FILTER_CONNECTION_SETUP
} SERVICE_BT_EVENT_FILTER_TYPE;

/* * HCI Event Filter - Filter Condition Type */
typedef enum {
    BT_EVENT_FILTER_CONDITION_ALL_DEVICES,
    BT_EVENT_FILTER_CONDITION_DEVICE_CLASS,
    BT_EVENT_FILTER_CONDITION_BDADDR
} SERVICE_BT_EVENT_FILTER_CONDITION_TYPE;

/* * HCI Event Filter - Auto Accept Flag */
typedef enum {
    BT_CONNECTION_AUTO_ACCEPT_OFF = 1u,
    BT_CONNECTION_AUTO_ACCEPT_ON_WITHOUT_ROLE_SWITCH,
    BT_CONNECTION_AUTO_ACCEPT_ON_WITH_ROLE_SWITCH
} SERVICE_BT_AUTO_ACCEPT_FLAG;

/* * HCI Set Event Filter Command param */
typedef struct {
    SERVICE_BT_EVENT_FILTER_TYPE filter_type;
    SERVICE_BT_EVENT_FILTER_CONDITION_TYPE condition_type; /* * Shall be set when filter_type is not
                                                              BT_EVENT_FILTER_CLEAR_ALL, otherwise it is ignored. */
    uint32_t condition_device_class; /* * Shall be set when condition_type is BT_EVENT_FILTER_CONDITION_DEVICE_CLASS,
                                        otherwise it is ignored. */
    BD_ADDR condition_bdaddr; /* * Shall be set when condition_type is BT_EVENT_FILTER_CONDITION_BDADDR, otherwise it is
                                 ignored. */
    SERVICE_BT_AUTO_ACCEPT_FLAG auto_accept_flag; /* * Shall be set when filter_type is
                                                     BT_EVENT_FILTER_CONNECTION_SETUP, otherwise it is ignored. */
} SERVICE_BT_EVENT_FILTER_S;

typedef struct {
    int8_t avrcp_vol;
    int8_t hfp_vol;
} SERVICE_BT_DEVICE_VOLUME_S;

/* * BT sniff param */
typedef struct {
    uint16_t sniff_interval;
    uint16_t sniff_attempt;
    uint16_t sniff_timeout;
} SERVICE_BT_SNIFF_PARAM_S;

typedef struct {
    bool hfp_supported;
    bool hsp_supported;
    bool a2dp_supported;
    SERVICE_AVDTP_CODEC_TYPE a2dp_codectype;
} SERVICE_BT_DEVICE_PROFILE_S;

/* * Remote device Info */
typedef struct {
    BD_ADDR bd_addr;
    SERVICE_BLE_ADDR_TYPE addr_type;
    // char* bt_name;
    BT_UUID_T uuids[10];
    uint8_t link_key[16];
    SERVICE_LINK_KEY_TYPE link_key_type;
    SERVICE_BT_DEVICE_VOLUME_S device_volume;
    SERVICE_BT_DEVICE_PROFILE_S device_profile;
    uint32_t cod;
    SERVICE_BT_DEVICE_TYPE device_type;
    int8_t rssi;
} SERVICE_REMOTE_DEVICE_S;

/* * Remote BR service Info */
typedef struct {
    BT_UUID_T uuid;
    uint16_t profile_version;       /* Profile version of this service. 0x100 by default. */
    uint8_t server_channel;         /* Server channel if this service is RFCOMM based. */
    uint32_t features;              /* Supportedfeatures attribute value of this service. It is profile dependent. */
} SERVICE_BR_SERVICE_S;

/* * BLE security Info */
#define SERVICE_SMP_KEYS_MAX_SIZE   80
typedef struct {
    uint8_t smp_keys[SERVICE_SMP_KEYS_MAX_SIZE];
} SERVICE_BLE_KEYS_S;

/* * BLE ADV Parameters */
typedef struct {
    SERVICE_BLE_EVENT_TYPE adv_type;
    BD_ADDR peer_addr;/* For BLE_ADV_DIRECT_IND only */
    SERVICE_BLE_ADDR_TYPE peer_addr_type;/* For BLE_ADV_DIRECT_IND only */
    uint32_t interval;
    int8_t tx_power; /* *Range:-20~10 */
    SERVICE_BLE_ADV_CHANNEL channel_map;
    SERVICE_BLE_ADVERTISING_FILTER_POLICY filter_policy;
} SERVICE_BLE_ADV_PARAMS_S;

/* * pin request data */
typedef struct {
    BD_ADDR remote_addr;
    uint32_t cod;
    bool min_16_digit;
    char bt_name[BD_NAME_MAX_SIZE];
} SERVICE_PIN_REQUEST_DATA_S;

/* * ssp request data */
typedef struct {
    BD_ADDR remote_addr;
    uint32_t cod;
    GAP_SERVICE_SPP_TYPE ssp_type;
    uint32_t pass_key;
    char bt_name[BD_NAME_MAX_SIZE];
} SERVICE_SSP_REQUEST_DATA_S;

/* * BLE scan result data */
typedef struct {
    BD_ADDR remote_addr;
    SERVICE_BT_DEVICE_TYPE device_type;
    int8_t rssi;
    SERVICE_BLE_ADDR_TYPE addr_type;
    SERVICE_BLE_EVENT_TYPE evt_type;
    uint8_t length;
    char adv_data[1];
} SERVICE_SCAN_RESULT_DATA_S;

// BLE adv parameter
typedef struct {
    uint8_t adv_id;                   // advertising id specified by upper layer
    SERVICE_BLE_ADV_PARAMS_S params;  // advertising parameters
    uint16_t adv_length;
    char *adv_data;  // advertising data
    uint16_t scan_rsp_length;
    char *scan_rsp_data;  // scan response data
    int duration;         // duration for sending BLE ADV
} SERVICE_SCAN_ADV_PARAMS_S;

// BLE scan parameter
typedef struct {
    int scan_interval;
    int scan_window;
    SERVICE_BLE_PHY_TYPE scan_phy;
} SERVICE_SCAN_PARAMS_S;

// BLE scan filter
typedef struct {
    BD_ADDR bd_addr;           // remote device addr
    uint8_t length;            // length of the adv_data_mask
    uint8_t adv_data_mask[1];  // only reported to service layer if adv_data contains adv_data_mask
} SERVICE_BLE_SCAN_FILTER_S;

// BLE connect parameter
typedef struct {
    SERVICE_BLE_CONNECT_FILTER_POLICY filter_policy;
    BD_ADDR peer_addr;/* For BLE_CONNECT_FILTER_ADDR only */
    SERVICE_BLE_ADDR_TYPE
    peer_addr_type;/* For BLE_CONNECT_FILTER_ADDR only. Set to BLE_ADDR_ANONYMOUS if unknown. */
    bool use_default_params;/* If TRUE, the following parameters are ignored. */
    SERVICE_BLE_PHY_TYPE init_phy;
    uint16_t scan_interval;
    uint16_t scan_window;
    uint16_t connection_interval_min;
    uint16_t connection_interval_max;
    uint16_t connection_latency;
    uint16_t supervision_timeout;
    uint16_t min_ce_length;
    uint16_t max_ce_length;
} SERVICE_LE_CONNECT_PARAMS_S;

// ssp reply data
typedef struct {
    BD_ADDR remote_addr;        // Remote BT address
    bool accept;                // Accept pairing request
    GAP_SERVICE_SPP_TYPE type;  // type of the SSP reply
    uint32_t passkey;           // passkey value if type is GAP_SPP_TYPE_PASSKEY_ENTRY
} SERVICE_SSP_REPLY_DATA_S;

// details of acl state changed
typedef struct {
    BD_ADDR remote_addr;       // Remote BT address
    SERVICE_BLE_ADDR_TYPE addr_type;
    SERVICE_BT_STATUS status;  //
    SERVICE_BT_ACL_STATE state;
    SERVICE_ACL_DISCONNECTED_REASON reasonCode;
} SERVICE_ACL_STATE_PARAM_S;

typedef struct {
    SERVICE_BT_SCAN_MODE scan_mode;
} SERVICE_BT_SCAN_MODE_PARAM_S;

typedef enum { BLE_ADV_STATE_STARTED, BLE_ADV_STATE_STOPPED } SERVICE_BLE_ADV_STATE;

// details of ble adv state changed
typedef struct {
    uint8_t adv_id;
    SERVICE_BLE_ADV_STATE state;
} SERVICE_BLE_ADV_STATE_PARAM_S;

// Type of the event created by the ctroller when a command is completed
typedef enum {
    SERVICE_HCI_COMMAND_COMPLETED_BY_NONE, /* None of the following complete event is created for this command */

    SERVICE_HCI_COMMAND_COMPLETED_BY_COMMAND_COMPLETE_EVENT, /* HCI Command Complete event completes this command */
    SERVICE_HCI_COMMAND_COMPLETED_BY_VENDOR_SPECIFIC_EVENT, /* A HCI Vendor Specific event completes this command */

    SERVICE_HCI_COMMAND_COMPLETED_EVENT_TYPE_END /* End of definition, new event type shall be added before it */
} SERVICE_HCI_COMMAND_COMPLETE_EVENT_TYPE;

// details of HCI event from controller
typedef struct {
    uint8_t evt_code;  // HCI event code
    uint8_t length;    // length of the params
    char params[0];    // parameters
} SERVICE_BT_HCI_EVENT_S;

#ifdef __cplusplus
}
#endif
#endif  // #ifndef __STACK_ADAPTER_COMMON_H__
