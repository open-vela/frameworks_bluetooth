#ifndef __BTS_AVRCP_H__
#define __BTS_AVRCP_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>

#include "btm_avrcp.h"
#include "btm_avrcp_ctrl.h"
#include "bts_gap_service.h"
#include "stack_adapter_common.h"

typedef enum {
    CONNECTION_STATE_CHANGED,
    PASSTHROUHT_CMD_RSP,
    GET_CAPABILITY_RSP,
    REGISTER_NOTIFICATION_RSP,
    GET_ELEMENT_ATTRIBUTES_RSP,
    GET_PLAY_STATUS_RSP
} avrcp_event_t;

typedef struct {
    avrcp_passthr_cmd_t cmd;
    avrcp_key_state_t state;
    uint8_t rsp;
} avrcp_passthr_rsp_t;

typedef struct {
    play_status_t status;
    uint32_t song_len;
    uint32_t song_pos;
} avrcp_play_status_t;

typedef struct {
    uint8_t cap_count;
    uint8_t *capabilities;
} avrcp_capabilities_t;

typedef struct {
    SERVICE_AVRCP_NOTIFICATION_EVENT event;
    uint32_t value;
} avrcp_notification_t;

typedef struct avrcp_ctrl_msg_ {
    bt_address addr;
    avrcp_event_t msg_id;
    union {
        avrcp_connection_state_t conn_state;
        avrcp_passthr_rsp_t passthr_rsp;
        avrcp_play_status_t play_status;
        avrcp_capabilities_t cap;
        avrcp_notification_t notification;
    } data;
} avrcp_ctrl_msg_t;

typedef struct {
    bt_address addr;
    uint8_t role;
} avrcp_device_t;

bt_result_code bts_avrcp_ctrl_init(const avrc_ctrl_callbacks_t *cbs);
bt_result_code bts_avrcp_send_passthrough_cmd(bt_address addr, avrcp_passthr_cmd_t cmd, avrcp_key_state_t state);
bt_result_code bts_avrcp_get_play_status(bt_address addr);
void bts_avrcp_ctrl_cleanup(void);
bt_result_code avrcp_ctrl_service_start(void);
void avrcp_ctrl_service_stop(void);
const avrc_ctrl_interface_t* get_avrcp_ctrl_service_interface(void);

#endif