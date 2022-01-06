#ifndef __BTM_COMMON_DEFINE_H__
#define __BTM_COMMON_DEFINE_H__


#define BT_ADDR_LENGTH (6) /*!< define the address length*/
#define UUID_SIZE 16
#define MAX_UUID_NUM 10

/*!
* \def BT_DEV_NAME_MAX_SIZE
* Description
*/
#define BT_DEV_NAME_MAX_SIZE (63)

#define SMP_KEYS_MAX_SIZE   80
#define BT_COMMON_KEY_LENGTH 16
#define DEVICE_NAME_MAX_LEN  63
/* Attribute permissions */
#define GATT_ATT_PERMISSION_READABLE 0x01
#define GATT_ATT_PERMISSION_WRITABLE 0x02
#define GATT_ATT_PERMISSION_ENCRYPTIION_REQUIRED 0x04
#define GATT_ATT_PERMISSION_AUTHENTICATION_REQUIRED 0x08
#define GATT_ATT_PERMISSION_MITM_REQUIRED 0x10

/* Characteristic Properties */
#define GATT_ATT_PROPERTY_BROADCAST 0x01
#define GATT_ATT_PROPERTY_READ 0x02
#define GATT_ATT_PROPERTY_WRITE_NO_RESPONSE 0x04
#define GATT_ATT_PROPERTY_WRITE 0x08
#define GATT_ATT_PROPERTY_NOTIFY 0x10
#define GATT_ATT_PROPERTY_INDICATE 0x20
#define GATT_ATT_PROPERTY_SIGNED_WRITE 0x40
#define GATT_ATT_PROPERTY_EXTENDED_PROPS 0x80

/* * HID supported features - bit mask */
#define HID_ATTR_MASK_VIRTUAL_CABLE           0x0001
#define HID_ATTR_MASK_RECONNECT_INITIATE      0x0002
#define HID_ATTR_MASK_BOOT_DEVICE             0x0004
#define HID_ATTR_MASK_BATTERY_POWER           0x0010
#define HID_ATTR_MASK_REMOTE_WAKE             0x0020
#define HID_ATTR_MASK_SUPERVISION_TIMEOUT     0x0080
#define HID_ATTR_MASK_NORMALLY_CONNECTABLE    0x0100
#define HID_ATTR_MASK_SSR_MAX_LATENCY         0x0200
#define HID_ATTR_MASK_SSR_MIN_TIMEOUT         0x0400
#define HID_ATTR_MASK_BREDR                   0x8000


#define COD_SERVICE_BITS(c) (c & 0xFFE000)      /* The major service classes field */
#define COD_DEVICE_MAJOR_BITS(c) (c & 0x001F00) /* The major device classes field */
#define COD_DEVICE_CLASS_BITS(c) (c & 0x001FFC) /* The device classes field, including major and minor */

#define COD_SERVICE_LDM 0x002000         /* Limited Discoverable Mode */
#define COD_SERVICE_POSITION 0x010000    /* Positioning (Location identification) */
#define COD_SERVICE_NETWORK 0x020000     /* Networking (LAN, Ad hoc, ...) */
#define COD_SERVICE_RENDERING 0x040000   /* Rending (Printing, Speaker, ...) */
#define COD_SERVICE_CAPTURING 0x080000   /* Capturing (Scanner, Microphone, ...) */
#define COD_SERVICE_OBJECT 0x100000      /* Object Transfer (v-Inbox, v-Folder, ...) */
#define COD_SERVICE_AUDIO 0x200000       /* Audio (Speaker, Microphone, Headset service, ...) */
#define COD_SERVICE_TELEPHONY 0x400000   /* Telephony (Cordless telephony, Modem, Headset service, ...) */
#define COD_SERVICE_INFORMATION 0x800000 /* Information (WEB-server, WAP-server, ...) */

/* * Major Device Classes bit mask */
#define COD_DEVICE_MISCELLANEOUS 0x000000 /* Major Device Class - Miscellaneous */
#define COD_DEVICE_COMPUTER 0x000100 /* Major Device Class - Computer (desktop, notebook, PDA, organizers, ...) */
#define COD_DEVICE_PHONE 0x000200    /* Major Device Class - Phone (cellular, cordless, payphone, modem, ...) */
#define COD_DEVICE_LAP 0x000300      /* Major Device Class - LAN/Network Access Point */
#define COD_DEVICE_AV \
    0x000400 /* Major Device Class - Audio/Video (headset, speaker, stereo, video display, vcr...) */
#define COD_DEVICE_PERIPHERAL 0x000500 /* Major Device Class - Peripheral (mouse, joystick, keyboards, ...) */
#define COD_DEVICE_IMAGING 0x000600    /* Major Device Class - Imaging (printing, scanner, camera, display, ...) */
#define COD_DEVICE_WEARABLE 0x000700   /* Major Device Class - Wearable */
#define COD_DEVICE_TOY 0x000800        /* Major Device Class - Toy */
#define COD_DEVICE_HEALTH 0x000900     /* Major Device Class - Health */
#define COD_DEVICE_UNCLASSIFIED                                                     \
    0x001F00 /* Major Device Class - Uncategorized, specific device code not specified \
              */

/* * Minor Device Class - Computer major class */
#define COD_COMPUTER_UNCLASSIFIED (COD_DEVICE_COMPUTER | 0x000000)
#define COD_COMPUTER_DESKTOP (COD_DEVICE_COMPUTER | 0x000004)
#define COD_COMPUTER_SERVER (COD_DEVICE_COMPUTER | 0x000008)
#define COD_COMPUTER_LAPTOP (COD_DEVICE_COMPUTER | 0x00000C)
#define COD_COMPUTER_HANDHELD (COD_DEVICE_COMPUTER | 0x000010)
#define COD_COMPUTER_PALMSIZED (COD_DEVICE_COMPUTER | 0x000014)
#define COD_COMPUTER_WEARABLE (COD_DEVICE_COMPUTER | 0x000018)

/* * Minor Device Class - Phone major class */
#define COD_PHONE_UNCLASSIFIED (COD_DEVICE_PHONE | 0x000000)
#define COD_PHONE_CELLULAR (COD_DEVICE_PHONE | 0x000004)
#define COD_PHONE_CORDLESS (COD_DEVICE_PHONE | 0x000008)
#define COD_PHONE_SMARTPHONE (COD_DEVICE_PHONE | 0x00000C)
#define COD_PHONE_WIREDMODEM (COD_DEVICE_PHONE | 0x000010)
#define COD_PHONE_COMMONISDNACCESS (COD_DEVICE_PHONE | 0x000014)
#define COD_PHONE_SIMCARDREADER (COD_DEVICE_PHONE | 0x000018)

/* * Minor Device Class - LAN/Network access point major class */
#define COD_LAP_FULLY_AVAILABLE (COD_DEVICE_LAP | 0x000000)
#define COD_LAP_17_UTILIZED (COD_DEVICE_LAP | 0x000020)
#define COD_LAP_33_UTILIZED (COD_DEVICE_LAP | 0x000040)
#define COD_LAP_50_UTILIZED (COD_DEVICE_LAP | 0x000060)
#define COD_LAP_67_UTILIZED (COD_DEVICE_LAP | 0x000080)
#define COD_LAP_83_UTILIZED (COD_DEVICE_LAP | 0x0000A0)
#define COD_LAP_99_UTILIZED (COD_DEVICE_LAP | 0x0000C0)
#define COD_LAP_UNAVAILABLE (COD_DEVICE_LAP | 0x0000E0)

/* * Minor Device Class - Audio/Video major class */
#define COD_AV_UNCLASSIFIED (COD_DEVICE_AV | 0x000000)
#define COD_AV_HEADSET (COD_DEVICE_AV | 0x000004)
#define COD_AV_HANDSFREE (COD_DEVICE_AV | 0x000008)
#define COD_AV_HEADANDHAND (COD_DEVICE_AV | 0x00000C)
#define COD_AV_MICROPHONE (COD_DEVICE_AV | 0x000010)
#define COD_AV_LOUD_SPEAKER (COD_DEVICE_AV | 0x000014)
#define COD_AV_HEADPHONES (COD_DEVICE_AV | 0x000018)
#define COD_AV_PORTABLE_AUDIO (COD_DEVICE_AV | 0x00001C)
#define COD_AV_CAR_AUDIO (COD_DEVICE_AV | 0x000020)
#define COD_AV_SETTOPBOX (COD_DEVICE_AV | 0x000024)
#define COD_AV_HIFI_AUDIO (COD_DEVICE_AV | 0x000028)
#define COD_AV_VCR (COD_DEVICE_AV | 0x00002C)
#define COD_AV_VIDEO_CAMERA (COD_DEVICE_AV | 0x000030)
#define COD_AV_CAMCORDER (COD_DEVICE_AV | 0x000034)
#define COD_AV_VIDEO_MONITOR (COD_DEVICE_AV | 0x000038)
#define COD_AV_DISPLAY_AND_SPEAKER (COD_DEVICE_AV | 0x00003C)
#define COD_AV_VIDEO_CONFERENCING (COD_DEVICE_AV | 0x000040)
#define COD_AV_GAME_OR_TOY (COD_DEVICE_AV | 0x000048)

/* * Minor Device Class - Peripheral major class */
#define COD_PERIPHERAL_UNCLASSIFIED (COD_DEVICE_PERIPHERAL | 0x000000)
#define COD_PERIPHERAL_JOYSTICK (COD_DEVICE_PERIPHERAL | 0x000004)
#define COD_PERIPHERAL_GAMEPAD (COD_DEVICE_PERIPHERAL | 0x000008)
#define COD_PERIPHERAL_REMCONTROL (COD_DEVICE_PERIPHERAL | 0x00000C)
#define COD_PERIPHERAL_SENSE (COD_DEVICE_PERIPHERAL | 0x000010)
#define COD_PERIPHERAL_TABLET (COD_DEVICE_PERIPHERAL | 0x000014)
#define COD_PERIPHERAL_SIMCARDREADER (COD_DEVICE_PERIPHERAL | 0x000018)
#define COD_PERIPHERAL_KEYBOARD (COD_DEVICE_PERIPHERAL | 0x000040)
#define COD_PERIPHERAL_POINT (COD_DEVICE_PERIPHERAL | 0x000080)
#define COD_PERIPHERAL_KEYORPOINT (COD_DEVICE_PERIPHERAL | 0x0000C0)

/* * Minor Device Class - Imaging major class */
#define COD_IMAGING_DISPLAY (COD_DEVICE_IMAGING | 0x000010)
#define COD_IMAGING_CAMERA (COD_DEVICE_IMAGING | 0x000020)
#define COD_IMAGING_SCANNER (COD_DEVICE_IMAGING | 0x000040)
#define COD_IMAGING_PRINTER (COD_DEVICE_IMAGING | 0x000080)

/* * Minor Device Class - Wearable major class */
#define COD_WERABLE_WATCH (COD_DEVICE_WEARABLE | 0x000004)
#define COD_WERABLE_PAGER (COD_DEVICE_WEARABLE | 0x000008)
#define COD_WERABLE_JACKET (COD_DEVICE_WEARABLE | 0x00000C)
#define COD_WERABLE_HELMET (COD_DEVICE_WEARABLE | 0x000010)
#define COD_WERABLE_GLASSES (COD_DEVICE_WEARABLE | 0x000014)

/* * Minor Device Class - Toy major class */
#define COD_TOY_ROBOT (COD_DEVICE_TOY | 0x000004)
#define COD_TOY_VEHICLE (COD_DEVICE_TOY | 0x000008)
#define COD_TOY_DOLL (COD_DEVICE_TOY | 0x00000C)
#define COD_TOY_CONROLLER (COD_DEVICE_TOY | 0x000010)
#define COD_TOY_GAME (COD_DEVICE_TOY | 0x000014)

/* * Minor Device Class - Health major class */
#define COD_HEALTH_BLOOD_PRESURE (COD_DEVICE_HEALTH | 0x000004)
#define COD_HEALTH_THERMOMETER (COD_DEVICE_HEALTH | 0x000008)
#define COD_HEALTH_WEIGHING_SCALE (COD_DEVICE_HEALTH | 0x00000C)
#define COD_HEALTH_GLUCOSE_METER (COD_DEVICE_HEALTH | 0x000010)
#define COD_HEALTH_PULSE_OXIMETER (COD_DEVICE_HEALTH | 0x000014)
#define COD_HEALTH_RATE_MONITOR (COD_DEVICE_HEALTH | 0x000018)
#define COD_HEALTH_DATA_DISPLAY (COD_DEVICE_HEALTH | 0x00001C)


typedef uint8_t bt_address[BT_ADDR_LENGTH];
typedef uint8_t bt_uuid_t[UUID_SIZE];
typedef struct {
    uint8_t smp_keys[SMP_KEYS_MAX_SIZE];
} ble_keys_t;

typedef enum {
    BLE_CONNECT_FILTER_POLICY_ADDR,
    BLE_CONNECT_FILTER_POLICY_WHITE_LIST
} ble_connect_filter_policy;

typedef enum {
    BLE_ADDR_TYPE_PUBLIC,
    BLE_ADDR_TYPE_RANDOM,
    BLE_ADDR_TYPE_PUBLIC_ID,
    BLE_ADDR_TYPE_RANDOM_ID,
    BLE_ADDR_TYPE_ANONYMOUS,
    BLE_ADDR_TYPE_UNKNOWN = 0xFF
} ble_addr_type;

typedef enum {
    BLE_1M_PHY_TYPE,
    BLE_2M_PHY_TYPE,
    BLE_CODED_PHY_TYPE
} ble_phy_type;

typedef enum {
    SPP_TYPE_PASSKEY_CONFIRMATION,
    SPP_TYPE_PASSKEY_ENTRY,
    SPP_TYPE_CONSENT,
    SPP_TYPE_PASSKEY_NOTIFICATION
} gap_spp_type;

typedef enum {
    TESTMODE_DUT,
    TESTMODE_BLE
} test_mode;

typedef enum {
    DEVICE_DEVTYPE_BREDR,
    DEVICE_DEVTYPE_BLE,
    DEVICE_DEVTYPE_DUAL
} bt_device_type;

typedef enum {
    BLE_EVENT_ADV_IND,
    BLE_EVENT_ADV_DIRECT_IND,
    BLE_EVENT_ADV_SCAN_IND,
    BLE_EVENT_ADV_NONCONN_IND,
    BLE_EVENT_SCAN_RSP
} ble_event_type;

typedef enum {
    BT_DISCOVERY_STATE_STOPPED = 0,
    BT_DISCOVERY_STATE_STARTED
} bt_discovery_state;

/* * Bluetooth Bond state */
typedef enum {
    BT_BOND_STATE_NONE = 0,
    BT_BOND_STATE_BONDING,
    BT_BOND_STATE_BONDED,
    BT_BOND_STATE_SDP_DONE,
    BT_BOND_STATE_BLE_NONE,
    BT_BOND_STATE_BLE_BONDING,
    BT_BOND_STATE_BLE_BONDED
} bt_bond_state;

/* * Local IO capability, shall be the same value defined in HCI Specification. */
typedef enum {
    BT_IO_CAPABILITY_DISPLAYONLY = 0,
    BT_IO_CAPABILITY_DISPLAYYESNO,
    BT_IO_CAPABILITY_KEYBOARDONLY,
    BT_IO_CAPABILITY_NOINPUTNOOUTPUT,
    BT_IO_CAPABILITY_KEYBOARDDISPLAY
} bt_io_capability;

// Type of the event created by the ctroller when a command is completed
typedef enum {
    HCI_COMMAND_COMPLETED_BY_NONE = 0, ///< None of the following complete event is created for this command
    HCI_COMMAND_COMPLETED_BY_COMMAND_COMPLETE_EVENT, ///< HCI Command Complete event completes this command
    HCI_COMMAND_COMPLETED_BY_VENDOR_SPECIFIC_EVENT, ///< A HCI Vendor Specific event completes this command
    HCI_COMMAND_COMPLETED_EVENT_TYPE_END ///< End of definition, new event type shall be added before it
} hci_command_complete_event;

typedef enum {
    PROFILE_DISCONNECTED,
    PROFILE_CONNECTING,
    PROFILE_CONNECTED,
    PROFILE_DISCONNECTING
} profile_connection_state;

typedef enum {
    BLE_ADV_CHANNEL_DEFAULT,
    BLE_ADV_CHANNEL_37_ONLY,
    BLE_ADV_CHANNEL_38_ONLY,
    BLE_ADV_CHANNEL_39_ONLY
} ble_adv_channel;

typedef enum {
    ADV_FILTER_WHITE_LIST_FOR_NONE,/* Scan and Connection requests from ANY devices */
    ADV_FILTER_WHITE_LIST_FOR_SCAN,/* Connection requests from ANY devices; Scan requests from devices in the White List */
    ADV_FILTER_WHITE_LIST_ROR_CONNECTION,/* Scan request form ANY devices; Connection requests from devices in the White List */
    ADV_FILTER_WHITE_LIST_FOR_ALL /* Scan and Connection reqeusts from devices in the White List */
} ble_advertising_filter_policy;

typedef enum {
    GATT_PRIMARY_SERVICE,
    GATT_SECONDARY_SERVICE,
    GATT_INCLUDED_SERVICE,
    GATT_CHARACTERISTIC,
    GATT_DESCRIPTOR
} gatt_element_type;

typedef enum {
    GATT_STATUS_SUCCESS,
    GATT_STATUS_FAILURE,
    GATT_STATUS_REQUEST_NOT_SUPPORTED,
    GATT_STATUS_INSUFFICIENT_AUTHENTICATION,
    GATT_STATUS_INSUFFICIENT_ENCRYPTION,
    GATT_STATUS_READ_NOT_PERMITTED,
    GATT_STATUS_WRITE_NOT_PERMITTED,
    GATT_STATUS_INVALID_ATTRIBUTE_LENGTH
} gatt_status;

typedef enum {
    BT_STATUS_SUCCESS,
    BT_STATUS_FAIL,
    BT_STATUS_NOT_READY,
    BT_STATUS_NOMEM,
    BT_STATUS_BUSY,
    BT_STATUS_DONE,
    BT_STATUS_UNSUPPORTED,
    BT_STATUS_PARM_INVALID,
    BT_STATUS_UNHANDLED,
    BT_STATUS_AUTH_FAILURE,     /* remote accepts AUTH request, but AUTH failure */
    BT_STATUS_RMT_DEV_DOWN,     /* remote device not in BT range */
    BT_STATUS_AUTH_REJECTED,    /* remote rejects AUTH request */
    BT_STATUS_RMT_DEV_TERMINATE /* remote disconnect the link actively */
} bt_status;

typedef enum {
    BT_ACL_STATE_CONNECTED,
    BT_ACL_STATE_CONNECTING,
    BT_ACL_STATE_CONNECT_REQUEST, /* receive incoming connect request */
    BT_ACL_STATE_DISCONNECTED,
    BT_ACL_STATE_LE_CONNECTED,
    BT_ACL_STATE_LE_CONNECTING,
    BT_ACL_STATE_LE_DISCONNECTED
} bt_acl_state;

typedef enum {
    BT_SCAN_MODE_NONE,
    BT_SCAN_MODE_CONNECTABLE,
    BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE
} bt_scan_mode;

typedef enum {
    BT_LINK_ROLE_MASTER,
    BT_LINK_ROLE_SLAVE,
    BT_LINK_ROLE_UNKNOWN
} bt_link_role;

typedef enum {
    BT_MODE_ACTIVE,
    BT_MODE_SNIFF
} bt_link_mode;

typedef enum {
    LINK_POLICY_DISABLE_ALL,
    LINK_POLICY_ENABLE_ROLE_SWITCH,
    LINK_POLICY_ENABLE_SNIFF,
    LINK_POLICY_ENABLE_ROLE_SWITCH_AND_SNIFF
} bt_link_policy;

typedef enum {
    BTHD_STATE_NOT_REGISTERED,
    BTHD_STATE_REGISTERED
} hid_app_state;

/**@enum bt_result_code
* @brief Result code of bluetooth manager
*/
typedef enum {
    BT_RESULT_STATE_ALLREADY_ON = -7,
    BT_RESULT_STATE_ALLREADY_OFF = -6,
    BT_RESULT_STATE_NOT_ON = -5,
    BT_RESULT_ALLOC_BUFFER_FAILED = -4,
    BT_RESULT_CALLBACK_ALREADY_EXSIT = -3,
    BT_RESULT_PARAMETER_ERROR = -2,
    BT_RESULT_FAILED = -1, ///< genernal error code.
    BT_RESULT_SUCCESS = 0, ///< success code.
    BT_RESULT_WAITING_FOR_INIT_STATUS_CHANGED = 1,
    BT_RESULT_ENABLE_ALLREADY_ON_GOING = 2,
} bt_result_code;

/** State of bluetooth manager*/
typedef enum {
    BTM_STATE_OFF = 0,
    BTM_STATE_TURNING_OFF,
    BTM_STATE_TURNING_ON,
    BTM_STATE_ON
} btm_bt_state;

/** Bluetooth connection state*/
typedef enum {
    STATE_DISCONNECTED = 0,
    STATE_DISCONNECTING,
    STATE_CONNECTING,
    STATE_CONNECTED,
} bt_connection_state;

/** State of bluetooth manager*/
typedef enum {
    STATE_BLE_OFF = 0,
    STATE_BLE_TURNING_OFF,
    STATE_BLE_TURNING_ON,
    STATE_BLE_ON
} btm_ble_state;

/** Bluetooth profile interface IDs */
typedef enum {
    BT_PROFILE_COMMON_ID = 1,
    BT_PROFILE_GAP_ID,
    BT_PROFILE_HANDSFREE_AG_ID,
    BT_PROFILE_HANDSFREE_HF_ID,
    BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID,
    BT_PROFILE_ADVANCED_AUDIO_SINK_ID,
    BT_PROFILE_HIDHOST_ID,
    BT_PROFILE_HIDDEV_ID,
    BT_PROFILE_LESCAN_ID,
    BT_PROFILE_GATTC_ID,
    BT_PROFILE_LEADV_ID,
    BT_PROFILE_GATTS_ID,
    BT_PROFILE_AV_RC_TARGET_ID,
    BT_PROFILE_AV_RC_CTRL_ID,
    BT_PROFILE_SPP_ID,
    BT_PROFILE_LE_AUDIO_ID,
    BT_PROFILE_MAX_ID,
} bt_profile_id;

/* Possible HID Desriptor Type */
typedef enum {
    HID_DESC_TYPE_REPORT = 0x22,
    HID_DESC_TYPE_PHYSICAL = 0x23,
} hid_descriptor_type;

/* Possible HID Report ID for Boot mode */
typedef enum {
    HID_BOOT_KB_REPORT_ID = 0x01,
    HID_BOOT_MOUSE_REPORT_ID = 0x02,
} hid_boot_mode_report_id;


typedef uint8_t bt_common_key[BT_COMMON_KEY_LENGTH];

typedef struct
{
    bt_address addr;
    bt_device_type device_type;
    ble_addr_type addr_type;
    char name[DEVICE_NAME_MAX_LEN + 1];
    int rssi;
    uint32_t cod;
    bt_uuid_t uuids[MAX_UUID_NUM];
} bt_device_t;

typedef struct {
    bt_address remote_addr;        // Remote BT address
    bool accept;                // Accept pairing request
    gap_spp_type type;  // type of the SSP reply
    uint32_t passkey;           // passkey value if type is SPP_TYPE_PASSKEY_ENTRY
} spp_reply_data_t;

typedef struct {
    ble_connect_filter_policy filter_policy;
    bt_address peer_addr;/* For BLE_CONNECT_FILTER_ADDR only */
    ble_addr_type
    peer_addr_type;/* For BLE_CONNECT_FILTER_ADDR only. Set to BLE_ADDR_ANONYMOUS if unknown. */
    bool use_default_params;/* If TRUE, the following parameters are ignored. */
    ble_phy_type init_phy;
    uint16_t scan_interval;
    uint16_t scan_window;
    uint16_t connection_interval_min;
    uint16_t connection_interval_max;
    uint16_t connection_latency;
    uint16_t supervision_timeout;
    uint16_t min_ce_length;
    uint16_t max_ce_length;
} ble_connect_params_t;

typedef struct {
    int scan_interval;
    int scan_window;
    ble_phy_type scan_phy;
} scan_params_t;

typedef struct {
    bt_address bd_addr;           // remote device addr
    uint8_t length;            // length of the adv_data_mask
    uint8_t adv_data_mask[1];  // only reported to service layer if adv_data contains adv_data_mask
} ble_scan_filter_t;

typedef struct {
    bt_address remote_addr;
    bt_device_type device_type;
    int8_t rssi;
    ble_addr_type addr_type;
    ble_event_type evt_type;
    uint8_t length;
    char adv_data[1];
} scan_result_t;

typedef struct {
    ble_event_type adv_type;
    bt_address peer_addr;/* For BLE_ADV_DIRECT_IND only */
    ble_addr_type peer_addr_type;/* For BLE_ADV_DIRECT_IND only */
    bt_address own_addr;/* Set if own_addr_type is BLE_ADDR_TYPE_RANDOM. Ignored otherwise */
    ble_addr_type  own_addr_type;/* One of BLE_ADDR_TYPE_PUBLIC, BLE_ADDR_TYPE_RANDOM and BLE_ADDR_TYPE_UNKNOWN */
    uint32_t interval;
    int8_t tx_power; /* *Range:-20~10 */
    ble_adv_channel channel_map;
    ble_advertising_filter_policy filter_policy;
} ble_adv_params_t;


typedef struct {
    uint8_t adv_id;                   // advertising id specified by upper layer
    ble_adv_params_t params;  // advertising parameters
    uint16_t adv_length;
    char *adv_data;  // advertising data
    uint16_t scan_rsp_length;
    char *scan_rsp_data;  // scan response data
    int duration;         // duration for sending BLE ADV
} advertise_param_t;

typedef struct {
    uint32_t id; /* For the server application, this shall be assigned and managed by the application to identify each element uniquely.
                  The id of an INCLUDED_SERVICE shall be the same as that of the PRIMARY_SERVICE or SECONDARY_SERVICE being included. */
    /* For the client application, this is the attribute handle returned from service discovery procedure. */
    bt_uuid_t uuid;
    gatt_element_type type;
    uint32_t properties; /* bit masks, characteristic properties - for characteristic type only */
    uint32_t permissions; /* bit masks, attribute permissions - for all types */
} gatt_element_t;

typedef struct {
    uint32_t request_id;
    gatt_status status;
    uint16_t length; /* value length */
    uint8_t value[0];
} gatt_response_t;


typedef struct {
    bt_address remote_addr;
    uint32_t cod;
    bool min_16_digit;
    char bt_name[BT_DEV_NAME_MAX_SIZE];
} pin_request_data_t;

typedef struct {
    bt_address remote_addr;
    uint32_t cod;
    gap_spp_type ssp_type;
    uint32_t pass_key;
    char bt_name[BT_DEV_NAME_MAX_SIZE];
} ssp_request_data_t;


typedef uint32_t ACL_DISCONNECTED_REASON;

typedef struct {
    bt_address remote_addr;       // Remote BT address
    ble_addr_type addr_type;
    bt_status status;  //
    bt_acl_state state;
    ACL_DISCONNECTED_REASON reasonCode;
} acl_state_params_t;


typedef struct {
    uint8_t evt_code;  // HCI event code
    uint8_t length;    // length of the params
    char *params;    // parameters
} hci_event_t;


typedef struct {
    bt_uuid_t uuid;
    uint16_t profile_version;       /* Profile version of this service. 0x100 by default. */
    uint8_t server_channel;         /* Server channel if this service is RFCOMM based. */
    uint32_t features;              /* Supportedfeatures attribute value of this service. It is profile dependent. */
} br_service_t;

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
} hid_info_t;


typedef struct {
    const char *name;
    const char *description;
    const char *provider;
    hid_info_t hids_info;
} bt_hidd_sdp_settings_t;

typedef struct {
    uint8_t service_type;
    uint32_t token_rate;
    uint32_t token_bucket_size;
    uint32_t peak_bandwidth;
    uint32_t access_latency;
    uint32_t delay_variation;
} bt_hidd_qos_settings_t;

/*******************************************************************************
 *
 * GATT server connection state changed callback
 * @param       remote_addr     - Remote address
 * @param       state           - connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_connection_state_changed_callback)(bt_address remote_addr,
        profile_connection_state state);

/*******************************************************************************
 *
 * Indicates whether a local service has been added successfully
 * @param       status   - gatt status
 * @param       elements - added elements. It shall be the same buffer as the first
 *                         parameter of service_adapter_server_add_elements.
 * @param       size     - elements size
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_elements_added_callback)(gatt_status status,
        gatt_element_t *elements,
        uint16_t size);

/*******************************************************************************
 *
 * Indicates whether a local service has been removed successfully
 * @param       status   - gatt status
 * @param       elements - Full elements of the removed service.
 *                         It shall be the same buffer as the first parameter of
 *                         service_adapter_server_add_elements.
 * @param       size     - elements size
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_elements_removed_callback)(gatt_status status,
        gatt_element_t *elements,
        uint16_t size);

/*******************************************************************************
 *
 * PHY read callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_phy_read_callback)(bt_address remote_addr, ble_phy_type tx_phy,
        ble_phy_type rx_phy);

/*******************************************************************************
 *
 * PHY changed callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_phy_update_callback)(bt_address remote_addr, ble_phy_type tx_phy,
        ble_phy_type rx_phy, gatt_status status);

/*******************************************************************************
 *
 * A remote client has requested to read a local characteristic or descriptor
 * @param       remote_addr         - Remote address
 * @param       request_id          - request id
 * @param       element             - characteristic or descriptor
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_received_element_read_request_callback)(bt_address remote_addr,
        uint32_t request_id,
        gatt_element_t *element);

/*******************************************************************************
 *
 * A remote client has requested to write a local characteristic or descriptor
 * @param       remote_addr         - Remote address
 * @param       request_id          - request id
 * @param       element             - characteristic or descriptor
 * @param       value               - buffer, keeps valid until service_adapter_server_send_response is called.
 * @param       offset              - offset of the value to write
 * @param       length              - buffer length
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_received_element_write_request_callback)(bt_address remote_addr,
        uint32_t request_id,
        gatt_element_t *element, uint8_t *value,
        uint16_t offset, uint16_t length);

/*******************************************************************************
 *
 * mtu changed callback
 * @param       remote_addr         - Remote address
 * @param       mtu                 - The new (ATT_MTU-3) value negotiated.
 *                                    The default mtu value is (23-3).
 *                                    3 is the ATT PDU header size.
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_mtu_changed_callback)(bt_address remote_addr, uint32_t mtu);

/*******************************************************************************
 *
 * a notification or indication has been sent to a remote device
 * @param       remote_addr         - Remote address
 * @param       status              - GATT status
 * @return      void
 *
 ******************************************************************************/
typedef void (*server_notification_sent_callback)(bt_address remote_addr,
        gatt_status status);

/*******************************************************************************
 *
 * GATT client connection state changed callback
 * @param       remote_addr     - Remote address
 * @param       state           - connection state
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_connection_state_changed_callback)(bt_address remote_addr,
        profile_connection_state state);

/*******************************************************************************
 *
 * the list of remote services, characteristics and descriptors
 * for the remote device have been updated
 * @param       remote_addr     - Remote address
 * @param       elements        - full list of elements. A NULL pointer means
 *                              end of the services discovery procedure.
 * @param       size            - elements size
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_service_discovered_callback)(bt_address remote_addr,
        gatt_element_t *elements,
        uint16_t size);

/*******************************************************************************
 *
 * Callback reporting the result of a characteristic or descriptor read operation
 * @param       remote_addr     - Remote address
 * @param       element         - characteristic or descriptor
 * @param       value           - buffer
 * @param       length          - buffer length
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_element_read_callback)(bt_address remote_addr,
        gatt_element_t *element, uint8_t *value,
        uint16_t length, gatt_status status);

/*******************************************************************************
 *
 * Callback reporting the result of a characteristic or descriptor write operation
 * @param       remote_addr     - Remote address
 * @param       element         - characteristic or descriptor
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_element_written_callback)(bt_address remote_addr,
        gatt_element_t *element,
        gatt_status status);

/*******************************************************************************
 *
 * Callback triggered as a result of a remote characteristic notification
 * @param       remote_addr     - Remote address
 * @param       element         - characteristic
 * @param       value           - buffer
 * @param       length          - buffer length
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_element_changed_callback)(bt_address remote_addr,
        gatt_element_t *element,
        uint8_t *value, uint16_t length);

/*******************************************************************************
 *
 * Callback reporting the RSSI for a remote device connection
 * @param       remote_addr     - Remote address
 * @param       rssi            - rssi
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_remote_rssi_read_callback)(bt_address remote_addr, int32_t rssi,
        gatt_status status);

/*******************************************************************************
 *
 * PHY read callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_phy_read_callback)(bt_address remote_addr, ble_phy_type tx_phy,
        ble_phy_type rx_phy);

/*******************************************************************************
 *
 * PHY changed callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_phy_update_callback)(bt_address remote_addr, ble_phy_type tx_phy,
        ble_phy_type rx_phy, gatt_status status);

/*******************************************************************************
 *
 * mtu changed callback
 * @param       remote_addr     - Remote address
 * @param       mtu             - The new (ATT_MTU-3) value negotiated.
 *                                The default mtu value is (23-3).
 *                                3 is the ATT PDU header size.
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*client_mtu_changed_callback)(bt_address remote_addr, uint32_t mtu,
        gatt_status status);

/* * Stack Gatt Server callback structure */
typedef struct {
    /* * set to sizeof(GATT_SERVER_CALLBACKS_S) */
    uint8_t size;
    server_connection_state_changed_callback gatt_server_connection_state_changed_cb;
    server_elements_added_callback gatt_server_elements_added_cb;
    server_elements_removed_callback gatt_server_elements_removed_cb;
    server_phy_read_callback gatt_server_phy_read_cb;
    server_phy_update_callback gatt_server_phy_update_cb;
    server_received_element_read_request_callback gatt_server_received_element_read_request_cb;
    server_received_element_write_request_callback gatt_server_received_element_write_request_cb;
    server_mtu_changed_callback gatt_server_mtu_changed_cb;
    server_notification_sent_callback gatt_server_notification_sent_cb;
} stack_gatt_server_callbacks;

/* * Stack Gatt Client callback structure */
typedef struct {
    /* * set to sizeof(GATT_CLIENT_CALLBACKS_S) */
    uint8_t size;
    client_connection_state_changed_callback gatt_client_connection_state_changed_cb;
    client_service_discovered_callback gatt_client_service_discovered_cb;
    client_element_read_callback gatt_client_element_read_cb;
    client_element_written_callback gatt_client_element_written_cb;
    client_element_changed_callback gatt_client_element_changed_cb;
    client_remote_rssi_read_callback gatt_client_remote_rssi_read_cb;
    client_phy_read_callback gatt_client_phy_read_cb;
    client_phy_update_callback gatt_client_phy_update_cb;
    client_mtu_changed_callback gatt_client_mtu_changed_cb;
} stack_gatt_client_callbacks;

#endif