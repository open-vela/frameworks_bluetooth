

#pragma once

#include <stddef.h>
#include "log.h"
/****************************************************************************
 * Included Files
 ****************************************************************************/
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/** Bluetooth profile name */
#define BT_PROFILE_GAP "gap"
#define BT_PROFILE_HANDSFREE_AG "handsfree_ag"
#define BT_PROFILE_HANDSFREE_HF "handsfree_hf"
#define BT_PROFILE_ADVANCED_AUDIO_SOURCE "a2dp_source"
#define BT_PROFILE_ADVANCED_AUDIO_SINK "a2dp_sink"
#define BT_PROFILE_HIDHOST "hidhost"
#define BT_PROFILE_HIDDEV "hiddev"
#define BT_PROFILE_GATT "gatt"
#define BT_PROFILE_AV_RC_TARGET "avrcp_target"
#define BT_PROFILE_AV_RC_CTRL "avrcp_ctrl"
#define BT_PROFILE_SPP "spp"
#define BT_PROFILE_LE_AUDIO "le_audio"

/** 
 * @brief Bluetooth address length 
 */
#define BT_ADDR_LENGTH (6)        /*!< define the address length*/
#define BD_NAME_MAX_SIZE (20)

/** Bluetooth address type*/
typedef uint8_t bt_address[BT_ADDR_LENGTH];

/** 
 * Result code of bluetooth manager
 */
typedef enum { 
    BT_RESULT_STATE_ALLREADY_ON = -7,
    BT_RESULT_STATE_ALLREADY_OFF = -6,
    BT_RESULT_STATE_NOT_ON = -5,
    BT_RESULT_ALLOC_BUFFER_FAILED = -4,
    BT_RESULT_CALLBACK_ALREADY_EXSIT = -3, 
    BT_RESULT_PARAMETER_ERROR = -2, 
    BT_RESULT_FAILED = -1,
    BT_RESULT_SUCCESS = 0, 
    BT_RESULT_WAITING_FOR_INIT_STATUS_CHANGED = 1, 
    BT_RESULT_ENABLE_ALLREADY_ON_GOING = 2,
 } bt_result_code;

/** State of bluetooth manager*/
 typedef enum { 
    BT_MANAGER_STATE_OFF = 0, 
    BT_MANAGER_STATE_TURNING_OFF, 
    BT_MANAGER_STATE_TURNING_ON, 
    BT_MANAGER_STATE_ON 
} bt_manager_bt_state;

/** State of bluetooth manager*/
 typedef enum { 
    BT_STATE_INITED = 0, 
    BT_STATE_INITING, 
    BT_STATE_NOT_INITED
} bt_init_state;

/** State of bluetooth manager*/
 typedef enum { 
    STATE_BLE_OFF = 0, 
    STATE_BLE_TURNING_OFF, 
    STATE_BLE_TURNING_ON, 
    STATE_BLE_ON 
} bt_manager_ble_state;

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
    BT_PROFILE_GATT_ID,
    BT_PROFILE_AV_RC_TARGET_ID,
    BT_PROFILE_AV_RC_CTRL_ID,
    BT_PROFILE_SPP_ID,
    BT_PROFILE_LE_AUDIO_ID, 
    BT_PROFILE_MAX_ID,
} bt_profile_id;

/** Bluetooth connection state*/
 typedef enum { 
    STATE_DISCONNECTED = 0, 
    STATE_DISCONNECTING, 
    STATE_CONNECTING, 
    STATE_CONNECTED, 
} bt_connection_state;

/* * Bluetooth discovery state */
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

/* * Bluetooth Scan Mode */
typedef enum { 
    BT_SCAN_MODE_NONE = 0, 
    BT_SCAN_MODE_CONNECTABLE, 
    BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE 
} bt_scan_mode;

/** Bluetooth link mode */
typedef enum { 
    BT_LINK_MODE_ACTIVE = 0, 
    BT_LINK_MODE_SNIFF 
} bt_link_mode;

// Type of the event created by the ctroller when a command is completed
typedef enum {
	HCI_COMMAND_COMPLETED_BY_NONE = 0, /* None of the following complete event is created for this command */
	HCI_COMMAND_COMPLETED_BY_COMMAND_COMPLETE_EVENT, /* HCI Command Complete event completes this command */
	HCI_COMMAND_COMPLETED_BY_VENDOR_SPECIFIC_EVENT, /* A HCI Vendor Specific event completes this command */
	HCI_COMMAND_COMPLETED_EVENT_TYPE_END /* End of definition, new event type shall be added before it */
} bt_service_hci_command_complete_event;

/** ssp type data */
typedef enum {
    SPP_TYPE_PASSKEY_CONFIRMATION = 0,
    SPP_TYPE_PASSKEY_ENTRY,
    SPP_TYPE_CONSENT,
    SPP_TYPE_PASSKEY_NOTIFICATION
} ssp_type;

/** ssp request data */
typedef struct {
    bt_address remote_addr;
    uint32_t cod;
    ssp_type ssp_type;
    uint32_t pass_key;
    char bt_name[BD_NAME_MAX_SIZE];
} bt_ssp_request_data_t;

// details of HCI event from controller
typedef struct {
    uint8_t evt_code;  // HCI event code
    uint8_t length;    // length of the params
    char params[0];    // parameters
} bt_hci_event_t;

/**
 * HCI event callback (only for the raw HCI command sent by upper layer)
 * @param[in] hci_event, include evt_code and parameters
 * @return   void
 */
typedef void (*hci_event_callback)(bt_hci_event_t *hci_event);

// HCI command struct
typedef struct {
    uint8_t ogf;                // OpCode Group Field
    uint16_t ocf;               // OpCode Command Field
    hci_event_callback cb;  // callback
    uint8_t length;             // length of the params
    char params[1];             // parameters
} bt_hci_command_t;


typedef enum {
     BT_DEVTYPE_BREDR, 
     BT_DEVTYPE_BLE, 
     BT_DEVTYPE_DUAL, 
}bt_device_type;

/* * BLE address type */
typedef enum {
    BLE_ADDRESS_PUBLIC,
    BLE_ADDRESS_RANDOM,
    BLE_ADDRESS_PUBLIC_ID,
    BLE_ADDRESS_RANDOM_ID,
    BLE_ADDRESS_ANONYMOUS,
}ble_address_type;

typedef struct
{
    bt_address addr;
    bt_device_type device_type;
    ble_address_type addr_type;
    int rssi;
}bt_device_t;

typedef void (*btm_process_func) (void * handle, char * context, size_t data_size);

typedef void (*bt_manager_state_changed_callback)(bt_manager_bt_state state);

typedef void (*bt_manager_ble_state_changed_callback)(bt_manager_ble_state state);

typedef void (*init_status_changed_callback)(bt_result_code status);

typedef void (*bt_connection_state_changed_callback)(bt_device_t device, bt_connection_state state);

/**
 * BR/EDR device found callback, invoked in response to btStartDiscovery()
 * @param[in] addr - newly found device
 * @return   void
 */
typedef void (*device_found_callback)(bt_device_t device);
/**
 * BR/EDR device's name updated callback, invoked in response to btStartDiscovery()/btGetRemoteName()
 * @param[in] bt_name - remote device name
 * @param[in] length  - buffer length
 * @return   void
 */
typedef void (*received_remote_name_callback)(bt_address bd_addr, char *btName, uint8_t length);

/**
 * @name: discoveryStateChangedCallback
 * Discovery state change callback, invoked in response to btSetScanMode()
 * @param[in] state - newly state
 * @return   void
 */
typedef void (*discovery_state_changed_callback)(bt_discovery_state state);

/**
 * SSP pairing reqeust callback - Just Works & Numeric Comparison
 * @param[in] request_data - request data from callback
 * @return   void
 */
typedef void (*ssp_request_callback)(bt_ssp_request_data_t *request_data);

/**
 * Bonding state change callback - invoked in response to btCreateBond(), service_adapter_cancel_bond(),
 * service_adapter_remove_bond()
 * @param[in] remoteAddr - remote BT address
 * @param[in] state - bond state
 * @return   void
 */
typedef void (*bond_state_changed_callback)(bt_device_t device, bt_bond_state state);

/**
 * Get local name callback - invoked in response to btGetLocalName()
 * service_adapter_remove_bond()
 * @param[in] bt_name - local name
 * @param[in] maxLen - lenth
 * @return   void
 */
typedef void (*local_name_callback)(char *bt_name, uint8_t length);

typedef void (*local_address_callback)(bt_device_t device);

typedef void (*local_device_class_callback)(uint32_t device_class);

typedef void (*connected_state_callback)(bt_device_t device, bt_connection_state state);

typedef void (*get_bonded_device_list_callback)(bt_address*bonded_device_list, uint8_t umber);

typedef void (*connected_device_list_callback)(bt_address*connected_device_list, uint8_t umber);

typedef struct {
    /** set to sizeof(bt_callbacks_t) */
    size_t size;
    bt_manager_state_changed_callback bt_manager_state_changed_callback_cb;
    init_status_changed_callback init_status_changed_callback_cb;
    bt_connection_state_changed_callback   bt_connection_state_changed_callback_cb;
    device_found_callback device_found_callback_cb;
    received_remote_name_callback received_remote_name_callback_cb;
    discovery_state_changed_callback discovery_state_changed_callback_cb;
    ssp_request_callback ssp_request_callback_cb;
    bond_state_changed_callback bond_state_changed_callback_cb;
    hci_event_callback hciEventCallbackCb;
    get_bonded_device_list_callback get_bonded_device_list_callback_cb;
    local_name_callback local_name_callback_cb;
    local_device_class_callback local_device_class_callback_cb;
    connected_state_callback connected_state_callback_cb;
    connected_device_list_callback connected_device_list_callback_cb;
    local_address_callback local_address_callback_cb;
} bt_callbacks_t;

typedef struct{
    size_t size;
    bt_callbacks_t *callback;
    //TODO, handle is for  RPC in the feature
    void * client_hanlde;      
    //hanlde id created in service
    int  app_id;
    btm_process_func manager_func_list[BT_PROFILE_MAX_ID];
}manager_context_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/


/**
 *@fn  btManagerInit
 *
 *  Description:
 *  Initlize BT manager 
 *
 *@param [in] void
 *@return  @bt_result_code_t if success, returned = BT_RESULT_SUCCESS, 
 *   others are failed.
 *
 ****************************************************************************/
bt_result_code bt_manager_init(void *p);

/*! \fn int btManagerDeinit(void)
    \brief Opens a file descriptor.
 
    \param none.
*/

bt_result_code bt_manager_deinit(void * handle);
bt_result_code  bt_register_callback(void * handle, bt_callbacks_t *bt_callbacks_cb);
bt_result_code bt_manager_enable(void * handle);
bt_result_code bt_manager_disable(void * handle);
bool bt_manager_is_enable(void * handle);
bt_manager_bt_state bt_get_state(void * handle);
bt_manager_ble_state ble_get_state(void * handle);
void * bt_get_profile_interface(void * handle, const bt_profile_id profile_id);

/*Local property*/
bt_result_code bt_set_local_address(void * handle, bt_device_t device);
bt_result_code bt_get_local_address(void * handle);
bt_result_code bt_set_local_io_capability(void * handle, bt_io_capability io_capability);
bt_result_code bt_set_local_name(void * handle, char *bt_name, uint8_t len);
bt_result_code bt_get_local_name(void * handle);
bt_result_code bt_set_local_device_class(void * handle, uint32_t class_of_device);
bt_result_code bt_get_local_device_class(void * handle);

/*Remote device*/
bt_result_code bt_get_remote_name(void * handle, bt_device_t device);
bt_result_code bt_get_connection_state(void * handle, bt_device_t device);

/*Bond*/
bt_bond_state bt_get_bond_state(void * handle, bt_device_t device);
bt_result_code bt_reply_pair_request(void * handle, bt_device_t device, bool accept);
bt_result_code bt_create_bond(void * handle, bt_device_t device);
bt_result_code bt_cancel_bond(void * handle, bt_device_t device);
bt_result_code bt_remove_bond(void * handle, bt_device_t device);
bt_result_code bt_get_bonded_devices(void * handle);

/*Connection*/
bt_result_code bt_connect_all(void * handle, bt_device_t device);
bt_result_code bt_disonnect_all(void * handle, bt_device_t device);
bt_result_code bt_get_connected_devices(void * handle);

/*Discovery*/
bt_result_code bt_set_scan_mode(void * handle, bt_scan_mode scanMode, bool bondable);
//bt_result_code btGapSetLinkMode(bt_address remote_addr, BTLinkMode link_mode);
bt_result_code bt_start_discovery(void * handle, uint32_t timeout);
bt_result_code bt_stop_discovery(void * handle);

/*VSC command*/
bt_result_code bt_send_hci_command_v1(void * handle, bt_hci_command_t *command, bt_service_hci_command_complete_event event_type);

/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
bt_result_code bt_send_hci_command(void * handle, uint8_t *hci_cmd_packet, hci_event_callback cb);

bt_result_code send_to_service(void * handle, bt_profile_id profile_id, void* profile_buffer, int profile_size);

bt_result_code  register_process_func_to_manager(void* handle, bt_profile_id profile_id, 
                                                btm_process_func func);

bt_result_code unregister_process_func_to_manager(void* handle, bt_profile_id profile_id);

typedef struct {
    size_t size;

    bt_result_code (*init)(void* handle, const bt_callbacks_t* callbacks);
    bt_result_code (*enable)(void* handle);
    bt_result_code (*disable)(void* handle);
    void (*cleanup)(void* handle);

    const void* (*get_profile_interface)(const char* profile_id);
} btm_interface_t;

btm_interface_t* get_bt_manager_interface();

#define BT_CBACK(P_CB, P_CBACK, ...)                           \
    do {                                                       \
        if ((P_CB) && (P_CB)->P_CBACK) {                       \
            BT_LOGD("%s: BT %s->%s", __func__, #P_CB, #P_CBACK); \
            (P_CB)->P_CBACK(__VA_ARGS__);                      \
        } else {                                               \
            BT_LOGE("%s Callback is NULL", __func__);            \
        }                                                      \
    } while (0)