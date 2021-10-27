/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_tws.h
Abstract:
    Bluetooth Stack TWS adapter interfaces.
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_TWS_H
#define STACK_ADAPTER_TWS_H

#include "stack_adapter_common.h"

typedef enum {
    SERVICE_TWS_INFO_TYPE_GROUP,
    SERVICE_TWS_INFO_TYPE_ACL_LINK,
    SERVICE_TWS_INFO_TYPE_L2CAP_BASIC_CHANNEL,
    SERVICE_TWS_INFO_TYPE_L2CAP_ERTM_CHANNEL,
    SERVICE_TWS_INFO_TYPE_RFCOMM_DLCI0,
    SERVICE_TWS_INFO_TYPE_RFCOMM_DLCIX,
    SERVICE_TWS_INFO_TYPE_PROFILE,
    SERVICE_TWS_INFO_TYPE_REMOVE_ACL_LINK,
    SERVICE_TWS_INFO_TYPE_REMOVE_PROFILE_GENERIC,
    SERVICE_TWS_INFO_TYPE_REMOVE_SPP,
    /* Following shall be redefined by the application */
    SERVICE_TWS_INFO_TYPE_APP_0,
    SERVICE_TWS_INFO_TYPE_APP_1,
    SERVICE_TWS_INFO_TYPE_APP_2,
    SERVICE_TWS_INFO_TYPE_APP_3,
    SERVICE_TWS_INFO_TYPE_APP_4,
    SERVICE_TWS_INFO_TYPE_APP_5,
} SERVICE_TWS_INFO_TYPE_S;

/* * TWS Information Packet Header */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_S */
} SERVICE_TWS_PACKET_HEADER_S;

/* * TWS Information Profile Header */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_S */
    /* Profile Common */
    uint16_t uuid_16;/* A2DP SRC (0x110A) or A2DP SNK (0x110B) */
    uint16_t acl_handle;
    uint16_t pc_reserved;
} SERVICE_TWS_PROFILE_HEADER_S;

/* * TWS Information Group - multiple items followed, e.g. Full AVRCP connection information:
 SERVICE_TWS_GROUP_S + SERVICE_TWS_ACL_LINK_S + SERVICE_TWS_L2CAP_BASIC_CHANNEL_S + SERVICE_TWS_AVRCP_S.
 Group items can be removed or added arbitrarily. Group items can be sent over the air using separate
 packets and segamented at the receiving side. The receiving side can reconstruct the same connection(s)
 as an atomic operation after all group items got. */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_GROUP_START */
    /* Group Information */
    uint16_t group_size;/* Total Size, in bytes, of the whole group, including itself */
} SERVICE_TWS_GROUP_S;

/* * ACL link */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_ACL_LINK */
    /* Address */
    BD_ADDR remote_addr;
    uint32_t cod;
    /* Following SHALL be the same as HCI_PS_LinkInfoStru */
    uint16_t acl_handle;
    uint16_t sco_handle;
    uint8_t link_mask;
    uint8_t link_mode;
    uint16_t sec_mask;
    uint8_t io_capability;
    uint8_t link_key_type;
    uint8_t link_key[16];
    uint8_t lmp_features[3][8];
    uint16_t reserved;/* For alignment only, may be extended in the future */
    /* Following SHALL be the same as L2CAP_PS_LinkInfoStru */
    uint8_t last_transid[2];
    uint8_t mask;
    uint8_t info_ctrl;
    uint8_t info_extfea[4];
    uint8_t info_fixcid[8];
} SERVICE_TWS_ACL_LINK_S;

/* * L2CAP Basic Mode channel */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_L2CAP_BASIC_CHANNEL */
    /* L2CAP_PS_ChannelInfoStru */
    uint16_t acl_handle;
    uint16_t psm;
    uint16_t local_cid;
    uint16_t remote_cid;
    uint16_t mask;
    uint8_t status;
    uint8_t trans_id;
    uint16_t local_mtu;
    uint16_t remote_mtu;
    uint16_t reserved;/* For alignment only, may be extended in the future */
} SERVICE_TWS_L2CAP_BASIC_CHANNEL_S;

#define SERVICE_MAX_A2DP_CODEC_CAP_SIZE 22
/* * A2DP Profile Information */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_PROFILE */
    /* Profile Common */
    uint16_t uuid_16;/* A2DP SRC (0x110A) or A2DP SNK (0x110B) */
    uint16_t acl_handle;
    uint16_t pc_reserved;
    /* Channel IDs */
    uint16_t signal_lcid;
    uint16_t stream_lcid;
    /* Following SHALL be the same as AVDTP_PS_StreamInfoStru */
    uint8_t status;
    uint8_t mask;
    uint8_t local_seid;
    uint8_t remote_seid;
    uint8_t media_type;
    uint8_t sep_cap_length;
    uint8_t sep_cap[SERVICE_MAX_A2DP_CODEC_CAP_SIZE];
} SERVICE_TWS_A2DP_S;

#define SERVICE_MAX_AVRCP_EVENTS_REGISTERED     14
/* * AVRCP profile information */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_PROFILE */
    /* Profile Common */
    uint16_t uuid_16;/* AVRCP CT (0x110E) */
    uint16_t acl_handle;
    uint16_t pc_reserved;
    /* Channel IDs */
    uint16_t control_cid;
    uint16_t reserved;
    /* Following SHALL be the same as AVRCP_PS_ControlInfoStru */
    uint32_t notification_event[SERVICE_MAX_AVRCP_EVENTS_REGISTERED];
    uint32_t playback_interval;
} SERVICE_TWS_AVRCP_S;

/* * RFCOMM DLCI0 */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_RFCOMM_DLCI0 */
    /* Following SHALL be the same as RFCOMM_PS_DLCI0InfoStru */
    uint16_t acl_handle;
    uint16_t psm;
    uint16_t local_cid;
    uint16_t remote_cid;
    uint16_t chnl_mask;
    uint8_t chnl_status;
    uint8_t trans_id;
    uint16_t local_mtu;
    uint16_t remote_mtu;
    uint8_t dlci0_status;
    uint8_t dlci0_mask;
} SERVICE_TWS_RFCOMM_DLCI0_S;

/* * RFCOMM DLCIx */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_RFCOMM_DLCIX */
    /* Channel ID */
    uint16_t local_cid;
    /* Following SHALL be the same as RFCOMM_PS_DLCIxInfoStru */
    uint16_t credit[3];
    uint16_t mask;
    uint16_t mfs;
    uint8_t ddlci;
    uint8_t status;
} SERVICE_TWS_RFCOMM_DLCIX_S;

/* * SPP profile information */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_PROFILE */
    /* Profile Common */
    uint16_t uuid_16;/* SPP (0x1101) */
    uint16_t acl_handle;
    uint16_t pc_reserved;
    /* Adapter Instance Information */
    BT_UUID_T private_uuid;
    uint32_t rfcomm_handle;
    uint16_t connect_port;
    uint8_t fc_mask;
    uint8_t pigyback_credits;
} SERVICE_TWS_SPP_S;

/* * HFP profile information */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_PROFILE */
    /* Profile Common */
    uint16_t uuid_16;/* HFP HF (0x111E) or HFP AG (0x111F) or HEP HES (0x1108) or HEP AG (0x1112) */
    uint16_t acl_handle;
    uint16_t pc_reserved;
    /* HFP_PS_ConnectionInfoStru */
    uint32_t rfcomm_handle;
    uint32_t remote_features;
    uint32_t current_features;
    uint32_t available_codecs;
    uint16_t status;
    uint16_t esco_max_latency;
    uint8_t esco_retrans_effort;
    uint8_t server_channel;
    uint8_t role;
    uint8_t chld_supportmask;
    uint8_t cind_num;
    uint8_t indicator_map[7];/* MAX_HFP_INDICATOR_COUNT */
    uint8_t indicator_map_flag[7];
    uint8_t hf_indicator_num;
    uint16_t hf_indicator_list[4];/* MAX_HFINDICATOR_COUNT */
    uint8_t hf_indicator_flag[4];
    uint8_t procedure_mask;
    uint8_t select_codec;
    /* Adapter Instance Information */
    uint16_t state_flag;
    uint8_t call_status;
    uint8_t state_reported;
    uint16_t reserved;
} SERVICE_TWS_HFP_HF_S;

/* * Remove ACL link */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_REMOVE_ACL_LINK */
    /* Link Information */
    BD_ADDR remote_addr;
} SERVICE_TWS_REMOVE_ACL_LINK_S;

/* * Remove Profile */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_REMOVE_PROFILE_GENERIC */
    /* Profile Common */
    uint16_t uuid_16;/* Service UUID (except for SPP) */
    /* Profile Information */
    BD_ADDR remote_addr;
    uint16_t reserved;
} SERVICE_TWS_REMOVE_PROFILE_GENERIC_S;

/* * Remove Profile */
typedef struct {
    /* Common header */
    uint8_t size;/* size of this structure */
    uint8_t type;/* SERVICE_TWS_INFO_TYPE_REMOVE_SPP */
    /* Profile Common */
    uint16_t uuid_16;/* SPP (0x1101) */
    /* Profile Information */
    uint32_t rfcomm_handle;
} SERVICE_TWS_REMOVE_SPP_S;

/**
 * TWS information got callback. If a connection is represents by multiple items,
 * Each item is reported using a separated callback function call.
 * @param[in]   remote_addr  - Remote address
 * @param[in]   tws_info     - TWS information returned. NULL means no more items.
 * @return      void
 */
typedef void (*tws_info_get_callback)(BD_ADDR remote_addr, SERVICE_TWS_PACKET_HEADER_S *tws_info);

/* * TWS related API */

/**
 * Return connection information of all profiles.
 * @param[in] remote_addr - Remote BT address
 * @param[in] cbk - Callback function to return the connection information.
 * @return  connection instance
 */
SERVICE_BT_STATUS service_adapter_tws_sync_device(BD_ADDR remote_addr, tws_info_get_callback cbk);

/**
 * Clone connection instance.
 * @param[in] tws_info - Information of the instance to clone
 * @return  Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_tws_clone(SERVICE_TWS_PACKET_HEADER_S *tws_info);

#endif
