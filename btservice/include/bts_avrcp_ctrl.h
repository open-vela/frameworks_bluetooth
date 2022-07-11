#ifndef __BTS_AVRCP_H__
#define __BTS_AVRCP_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include "btm_avrcp_ctrl.h"
#include "bts_gap_service.h"

bt_result_code avrcp_ctrl_service_start(void);
void avrcp_ctrl_service_stop(void);
const avrc_ctrl_interface_t* get_avrcp_ctrl_service_interface(void);

#endif