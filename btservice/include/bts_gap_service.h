#ifndef __BTS_GAP_SERVICE__H__
#define __BTS_GAP_SERVICE__H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "btm_gap.h"
#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

BT_RESULT_CODE gap_service_init(void);
btm_gap_interface_t* get_gap_service_instance(void);

#endif