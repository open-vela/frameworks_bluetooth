#ifndef __BTS_AVRCP_TARGET__H__
#define __BTS_AVRCP_TARGET__H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>

#include "btm_avrcp.h"
#include "bts_gap_service.h"

bt_result_code avrcp_target_service_start(void);
void avrcp_target_service_stop(void);
const avrcp_tg_interface_t *get_avrcp_tg_service_interface(void);
#endif
