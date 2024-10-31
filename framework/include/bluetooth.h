/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef _BT_BLUETOOTH_H__
#define _BT_BLUETOOTH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <uv.h>

#include "bt_addr.h"
#include "bt_config.h"
#include "bt_list.h"
#include "bt_profile.h"
#include "bt_status.h"
#include "bt_uuid.h"
#include "callbacks_list.h"
#include "uv_thread_loop.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

#if defined(CONFIG_CPU_BIT64) // CONFIG_CPU_BIT64
#define PTR uint64_t
#define PRTx PRIx64
#if !defined(INT2PTR)
#define INT2PTR(pt) (pt)(uint64_t) // For example, INT2PTR(pt)(int): (int)=>uint64_t=>(pt)/pointer type
#endif
#if !defined(PTR2INT)
#define PTR2INT(it) (it)(uint64_t) // For example, PTR2INT(it)(pointer): (pointer)=>uint64_t=>(it)/int type
#endif
#else // CONFIG_CPU_BIT32 and others
#define PTR uint32_t
#define PRTx PRIx32
#if !defined(INT2PTR)
#define INT2PTR(pt) (pt)(uint32_t) // For example, INT2PTR(pt)(int): (int)=>uint32_t=>(pt)/pointer type
#endif
#if !defined(PTR2INT)
#define PTR2INT(it) (it)(uint32_t) // For example, PTR2INT(it)(pointer): (pointer)=>uint32_t=>(it)/int type
#endif
#endif // End of else

#define PRIMARY_ADAPTER 0

typedef uint8_t bt_controller_id_t;

typedef enum {
    BT_IO_CAPABILITY_DISPLAYONLY = 0,
    BT_IO_CAPABILITY_DISPLAYYESNO,
    BT_IO_CAPABILITY_KEYBOARDONLY,
    BT_IO_CAPABILITY_NOINPUTNOOUTPUT,
    BT_IO_CAPABILITY_KEYBOARDDISPLAY,
    BT_IO_CAPABILITY_UNKNOW = 0xFF
} bt_io_capability_t;

#ifndef ANDROID_LIBBLUETOOTH
/*
 * hardware/libhardware/include/hardware/bluetooth.h
 * This definition was copied from Android 13.
 * DON NOT MODIFY IT !!!
 */
typedef enum {
    BT_SCAN_MODE_NONE,
    BT_SCAN_MODE_CONNECTABLE,
    BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE
} bt_scan_mode_t;
#endif

// Rename the definition
#define BT_BR_SCAN_MODE_NONE BT_SCAN_MODE_NONE
#define BT_BR_SCAN_MODE_CONNECTABLE BT_SCAN_MODE_CONNECTABLE
#define BT_BR_SCAN_MODE_CONNECTABLE_DISCOVERABLE BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE

typedef enum {
    BT_BR_SCAN_TYPE_STANDARD,
    BT_BR_SCAN_TYPE_INTERLACED,
    BT_BR_SCAN_TYPE_UNKNOWN = 0xFF
} bt_scan_type_t;

#ifndef ANDROID_LIBBLUETOOTH
/*
 * hardware/libhardware/include/hardware/bluetooth.h
 * This definition was copied from Android 13.
 * DON NOT MODIFY IT !!!
 */
typedef enum {
    BT_DISCOVERY_STOPPED,
    BT_DISCOVERY_STARTED
} bt_discovery_state_t;
#endif

// Rename the definition
#define BT_DISCOVERY_STATE_STOPPED BT_DISCOVERY_STOPPED
#define BT_DISCOVERY_STATE_STARTED BT_DISCOVERY_STARTED

typedef enum {
    BT_BR_FILTER_CLEAR_ALL,
    BT_BR_FILTER_INQUIRY_RESULT,
    BT_BR_FILTER_CONNECTION_SETUP
} bt_filter_type_t;

/* * HCI Event Filter - Filter Condition Type */
typedef enum {
    BT_BR_FILTER_CONDITION_ALL_DEVICES,
    BT_BR_FILTER_CONDITION_DEVICE_CLASS,
    BT_BR_FILTER_CONDITION_BDADDR,
    BT_BR_FILTER_CONDITION_RSSI,
    BT_BR_FILTER_CONDITION_UUIDS
} bt_filter_condition_type_t;

typedef enum {
    BT_LINK_ROLE_MASTER,
    BT_LINK_ROLE_SLAVE,
    BT_LINK_ROLE_UNKNOWN
} bt_link_role_t;

typedef enum {
    BT_LINK_MODE_ACTIVE,
    BT_LINK_MODE_SNIFF,
    BT_LINK_MODE_UNKNOWN,
} bt_link_mode_t;

typedef enum {
    BT_BR_LINK_POLICY_DISABLE_ALL,
    BT_BR_LINK_POLICY_ENABLE_ROLE_SWITCH,
    BT_BR_LINK_POLICY_ENABLE_SNIFF,
    BT_BR_LINK_POLICY_ENABLE_ROLE_SWITCH_AND_SNIFF
} bt_link_policy_t;

typedef enum {
    PAIR_TYPE_PASSKEY_CONFIRMATION = 0,
    PAIR_TYPE_PASSKEY_ENTRY,
    PAIR_TYPE_CONSENT,
    PAIR_TYPE_PASSKEY_NOTIFICATION,
    PAIR_TYPE_PIN_CODE,
} bt_pair_type_t;

typedef enum {
    BT_TRANSPORT_BLE,
    BT_TRANSPORT_BREDR
} bt_transport_t;

#ifndef ANDROID_LIBBLUETOOTH
/*
 * hardware/libhardware/include/hardware/bluetooth.h
 * This definition was copied from Android 13.
 * DON NOT MODIFY IT !!!
 */
typedef enum {
    BT_DEVICE_DEVTYPE_BREDR = 0x1,
    BT_DEVICE_DEVTYPE_BLE,
    BT_DEVICE_DEVTYPE_DUAL
} bt_device_type_t;
#endif

// Rename the definition
#define BT_DEVICE_TYPE_BREDR BT_DEVICE_DEVTYPE_BREDR
#define BT_DEVICE_TYPE_BLE BT_DEVICE_DEVTYPE_BLE
#define BT_DEVICE_TYPE_DUAL BT_DEVICE_DEVTYPE_DUAL

// Add new definition
#define BT_DEVICE_TYPE_UNKNOW 0xFF

typedef enum {
    BT_LE_ADDR_TYPE_PUBLIC,
    BT_LE_ADDR_TYPE_RANDOM,
    BT_LE_ADDR_TYPE_PUBLIC_ID,
    BT_LE_ADDR_TYPE_RANDOM_ID,
    BT_LE_ADDR_TYPE_ANONYMOUS,
    BT_LE_ADDR_TYPE_UNKNOWN = 0xFF
} ble_addr_type_t;

typedef enum {
    BT_ADDR_TYPE_BREDR,
    BT_ADDR_TYPE_UNKNOWN = 0xFF
} bt_addr_type_t;

/* * BLE PHY type */
typedef enum {
    BT_LE_1M_PHY,
    BT_LE_2M_PHY,
    BT_LE_CODED_PHY
} ble_phy_type_t;

typedef enum {
    BT_LE_CONNECT_FILTER_POLICY_ADDR,
    BT_LE_CONNECT_FILTER_POLICY_WHITE_LIST
} ble_connect_filter_policy_t;

typedef enum {
    EM_LE_LOW_LATENCY,
    EM_LE_HIGH_TPUT,
    EM_LE_LOW_POWER,
} bt_enhanced_mode_t;

typedef uint8_t bt_128key_t[16];

typedef struct {
    uint8_t hash[16];
    uint8_t rand[16];
} bt_oob_data_t;

#define COD_SERVICE_BITS(c) (c & 0xFFE000) /* The major service classes field */
#define COD_DEVICE_MAJOR_BITS(c) (c & 0x001F00) /* The major device classes field */
#define COD_DEVICE_CLASS_BITS(c) (c & 0x001FFC) /* The device classes field, including major and minor */

#define COD_SERVICE_LDM 0x002000 /* Limited Discoverable Mode */
#define COD_SERVICE_POSITION 0x010000 /* Positioning (Location identification) */
#define COD_SERVICE_NETWORK 0x020000 /* Networking (LAN, Ad hoc, ...) */
#define COD_SERVICE_RENDERING 0x040000 /* Rending (Printing, Speaker, ...) */
#define COD_SERVICE_CAPTURING 0x080000 /* Capturing (Scanner, Microphone, ...) */
#define COD_SERVICE_OBJECT 0x100000 /* Object Transfer (v-Inbox, v-Folder, ...) */
#define COD_SERVICE_AUDIO 0x200000 /* Audio (Speaker, Microphone, Headset service, ...) */
#define COD_SERVICE_TELEPHONY 0x400000 /* Telephony (Cordless telephony, Modem, Headset service, ...) */
#define COD_SERVICE_INFORMATION 0x800000 /* Information (WEB-server, WAP-server, ...) */

/* * Major Device Classes bit mask */
#define COD_DEVICE_MISCELLANEOUS 0x000000 /* Major Device Class - Miscellaneous */
#define COD_DEVICE_COMPUTER 0x000100 /* Major Device Class - Computer (desktop, notebook, PDA, organizers, ...) */
#define COD_DEVICE_PHONE 0x000200 /* Major Device Class - Phone (cellular, cordless, payphone, modem, ...) */
#define COD_DEVICE_LAP 0x000300 /* Major Device Class - LAN/Network Access Point */
#define COD_DEVICE_AV 0x000400 /* Major Device Class - Audio/Video (headset, speaker, stereo, video display, vcr...) */
#define COD_DEVICE_PERIPHERAL 0x000500 /* Major Device Class - Peripheral (mouse, joystick, keyboards, ...) */
#define COD_DEVICE_IMAGING 0x000600 /* Major Device Class - Imaging (printing, scanner, camera, display, ...) */
#define COD_DEVICE_WEARABLE 0x000700 /* Major Device Class - Wearable */
#define COD_DEVICE_TOY 0x000800 /* Major Device Class - Toy */
#define COD_DEVICE_HEALTH 0x000900 /* Major Device Class - Health */
#define COD_DEVICE_UNCLASSIFIED 0x001F00 /* Major Device Class - Uncategorized, specific device code not specified */

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

/* * Headset Device Class */
#define IS_HEADSET(cod) ((COD_SERVICE_BITS(cod) & COD_SERVICE_AUDIO) && COD_DEVICE_MAJOR_BITS(cod) == COD_DEVICE_AV)

#ifdef CONFIG_DEV_NAME_MAX_LEN
#define BT_DEV_NAME_MAX_LEN (CONFIG_DEV_NAME_MAX_LEN)
#else
#define BT_DEV_NAME_MAX_LEN (64)
#endif

#define BLUETOOTH_COMPANY_ID_XIAOMI 0x038F
#define BLUETOOTH_COMPANY_ID_GOOGLE 0x00E0

#define BT_LOC_NAME_MAX_LEN BT_DEV_NAME_MAX_LEN
#define BT_REM_NAME_MAX_LEN BT_DEV_NAME_MAX_LEN

#define BT_UUID_MAX_NUM (32)
#define BT_UUID_128_LEN (16)

typedef struct {
    bt_address_t addr;
    int8_t rssi;
    int8_t pad[1];
    uint32_t cod;
    char name[BT_REM_NAME_MAX_LEN + 1];
} bt_discovery_result_t;

/* * HCI Set Event Filter Command param */
typedef struct {
    bt_filter_condition_type_t condition_type;
    uint32_t condition_cod;
    bt_address_t condition_bdaddr;
} bt_discovery_filter_t;

// BLE connect parameter
typedef struct {
    uint8_t filter_policy; /* ble_connect_filter_policy_t */
    uint8_t use_default_params; /* boolean, If TRUE, the following parameters are ignored. */
    uint8_t init_phy; /* ble_phy_type_t */
    uint8_t pad[1];
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
    uint8_t enable; /* enable sniff mode, boolean */
    uint8_t idle_time; /* Idle time in seconds before entering sniff mode */
    uint16_t sniff_max_interval; /* sniff maximum interval */
    uint16_t sniff_min_interval; /* sniff minimum interval */
    uint16_t sniff_attempt; /* sniff attempt */
    uint16_t sniff_timeout; /* sniff timeout */
} bt_auto_sniff_params_t;

typedef struct {
    uint8_t evt_code; /* HCI event code */
    uint8_t length; /* length of the params */
    char params[0]; /* parameters */
} bt_hci_event_t;

/* Possible 2.4G channel band width (MHz) */
#define AFH_WIFI_BANDWIDTH_20 20
#define AFH_WIFI_BANDWIDTH_22 22
#define AFH_WIFI_BANDWIDTH_40 40

/* Possible 2.4G none Bluetooth radio channel central frequency (MHz) */
#define AFH_WIFI_CENTRAL_FREQUENCY_CH1 2412
#define AFH_WIFI_CENTRAL_FREQUENCY_STEP 5
#define AFH_WIFI_CHANNEL_TO_FREQ(ch) \
    (AFH_WIFI_CENTRAL_FREQUENCY_CH1 + ((ch - 1) * AFH_WIFI_CENTRAL_FREQUENCY_STEP))

enum {
    BLUETOOTH_SYSTEM,
    BLUETOOTH_USER,
};

typedef struct bt_instance bt_instance_t;

typedef bool (*bt_allocator_t)(void** data, uint32_t size);

typedef void (*bt_hci_event_callback_t)(bt_hci_event_t* hci_event, void* context);

typedef void (*bt_ipc_connected_cb_t)(bt_instance_t* ins, void* user_data);
typedef void (*bt_ipc_disconnected_cb_t)(bt_instance_t* ins, void* user_data, int status);

typedef struct bt_instance {
    uint32_t app_id;
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_BINDER_IPC
    void* manager_proxy;
    void* adapter_proxy;
    void* a2dp_proxy;
    void* a2dp_sink_proxy;
    void* hfp_hf_proxy;
    void* hfp_ag_proxy;
    void* gatt_proxy;
    void* spp_proxy;
    void* pan_proxy;
    void* hidd_proxy;
    void* gattc_proxy;
    void* gatts_proxy;
    void* lea_server_proxy;
#endif

#ifdef CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC
    void* poll;
    uv_mutex_t mutex;
    uv_sem_t message_processed;
    int peer_fd;
    uv_loop_t* client_loop;
    uv_loop_t* external_loop;
    uv_async_t* external_async;
    int offset;
    void* packet;
    void* cpacket;
    struct list_node msg_queue;
    void* context;
    uv_mutex_t lock;

    bt_ipc_disconnected_cb_t disconnected;
    callbacks_list_t* adapter_callbacks;
    callbacks_list_t* a2dp_sink_callbacks;
    callbacks_list_t* a2dp_source_callbacks;
    callbacks_list_t* avrcp_target_callbacks;
    callbacks_list_t* avrcp_control_callbacks;
    void* adapter_cookie;
    void* a2dp_sink_cookie;
    void* a2dp_source_cookie;
    void* avrcp_target_cookie;
    void* avrcp_control_cookie;

    callbacks_list_t* hfp_ag_callbacks;
    callbacks_list_t* hfp_hf_callbacks;
    callbacks_list_t* panu_callbacks;
    callbacks_list_t* spp_callbacks;
    callbacks_list_t* hidd_callbacks;
    callbacks_list_t* l2cap_callbacks;
    void* hfp_ag_cookie;
    void* hfp_hf_cookie;
    void* panu_cookie;
    void* spp_cookie;
    void* hidd_cookie;
    void* l2cap_cookie;

    bt_list_t* gattc_remote_list;
    bt_list_t* gatts_remote_list;
    void* priv;
#endif
} bt_instance_t;

/**
 * @brief Create bluetooth client instance
 *
 * @return bt_instance_t* - ins on success, NULL on failure.
 */
bt_instance_t* BTSYMBOLS(bluetooth_create_instance)(void);

/**
 * @brief Get bluetooth client instance, If it does not exist, an instance is created
 *
 * @return bt_instance_t* - ins if exist or create success, NULL, if create fail.
 */
bt_instance_t* BTSYMBOLS(bluetooth_get_instance)(void);

/**
 * @brief Find bluetooth client instance
 *
 * @return bt_instance_t* - ins if exist, NULL otherwise.
 */
bt_instance_t* BTSYMBOLS(bluetooth_find_instance)(pid_t pid);

/**
 * @brief Get profile proxy
 *
 * @param ins - bluetooth client instance.
 * @param id - profile ID.
 * @return void* - profile proxy.
 */
void* BTSYMBOLS(bluetooth_get_proxy)(bt_instance_t* ins, enum profile_id id);

/**
 * @brief Delete client instance
 *
 * @param ins - bluetooth client instance.
 */
void BTSYMBOLS(bluetooth_delete_instance)(bt_instance_t* ins);

/**
 * @brief Start profile service
 *
 * @param ins - bluetooth client instance.
 * @param id - profile ID.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bluetooth_start_service)(bt_instance_t* ins, enum profile_id id);

/**
 * @brief Stop profile service
 *
 * @param ins - bluetooth client instance.
 * @param id -profile ID.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bluetooth_stop_service)(bt_instance_t* ins, enum profile_id id);

bool BTSYMBOLS(bluetooth_set_external_uv)(bt_instance_t* ins, uv_loop_t* ext_loop);

/*
    Async instance
*/

/**
 * @brief Create bluetooth async client instance
 *
 * @param loop uv_loop_t
 * @param connected client instance connected callback
 * @param disconnected client instance disconnected callback
 * @return bt_instance_t* - ins on success, NULL on failure.
 */
bt_instance_t* bluetooth_create_async_instance(uv_loop_t* loop, bt_ipc_connected_cb_t connected, bt_ipc_disconnected_cb_t disconnected, void* user_data);

/**
 * @brief Delete bluetooth async client instance
 *
 * @param ins bt_instance_t*
 */
void bluetooth_delete_async_instance(bt_instance_t* ins);

#ifdef __cplusplus
}
#endif

#endif /* _BT_BLUETOOTH_H__ */
