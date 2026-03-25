/****************************************************************************
 * tests/unittest/service_profiles/include/cm_gatt_event.h
 ****************************************************************************/

#ifndef __TESTING_CMOCKA_SERVICE_PROFILES_CM_GATT_EVENT_H
#define __TESTING_CMOCKA_SERVICE_PROFILES_CM_GATT_EVENT_H

int test_gatt_event_setup(FAR void** state);
int test_gatt_event_teardown(FAR void** state);

void test_gatts_msg_new_normal(FAR void** state);
void test_gatts_msg_new_with_payload(FAR void** state);
void test_gatts_op_new_normal(FAR void** state);

void test_gattc_msg_new_normal(FAR void** state);
void test_gattc_msg_new_with_payload(FAR void** state);
void test_gattc_op_new_normal(FAR void** state);

#endif
