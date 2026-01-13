/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef _CS_RAS_TEST_H_
#define _CS_RAS_TEST_H_

#include "bt_addr.h"
#include "cs_ras_gatts.h"

#define RAS_TEST_SUCCESS (0)
#define RAS_TEST_FAIL (1)

#define RAS_TESTCASE_REAL_TIME_NOTIFY_VALID_RANG_DATA_001 (0)
#define RAS_TESTCASE_REAL_TIME_INDICATE_VALID_RANG_DATA_002 (1)
#define RAS_TESTCASE_ON_DEMAND_NOTIFY_VALID_RANG_DATA_003 (3)
#define RAS_TESTCASE_ON_DEMAND_INDICATE_VALID_RANG_DATA_004 (4)
#define RAS_TESTCASE_ON_DEMAND_OVERWRITE_VALID_RANG_DATA_1_005 (5)
#define RAS_TESTCASE_ON_DEMAND_OVERWRITE_VALID_RANG_DATA_2_006 (6)
#define RAS_TESTCASE_ON_DEMAND_OVERWRITE_VALID_RANG_DATA_3_007 (7)
#define RAS_TESTCASE_ON_DEMAND_OVERWRITE_VALID_RANG_DATA_4_008 (8)
#define RAS_TESTCASE_ON_DEMAND_OVERWRITE_VALID_RANG_DATA_5_009 (9)
#define RAS_TESTCASE_ON_DEMAND_WRITE_RANG_DATA_TIMEOUT_010 (10)

typedef uint16_t ras_testcase_t;

int bt_gatt_notify_cb_test(ras_attr_notify_t attr, bt_address_t* addr, uint8_t* value, uint16_t len);

int bt_gatt_attr_read_test(ras_attr_notify_t attr, bt_address_t* addr, uint8_t* value, uint16_t len);

int bt_gatt_notify_test(ras_attr_notify_t attr, bt_address_t* addr, uint8_t* value, uint16_t len);

int bt_gatt_indicate_test(ras_attr_notify_t attr, bt_address_t* addr, uint8_t* value, uint16_t len);

int cs_ras_subevent_recv_test(void* data, uint16_t len);
#endif /* _CS_RAS_TEST_H_ */