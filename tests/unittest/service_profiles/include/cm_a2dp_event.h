/****************************************************************************
 * tests/unittest/service_profiles/include/cm_a2dp_event.h
 ****************************************************************************/

#ifndef __TESTING_CM_SERVICE_PROFILES_A2DP_EVENT_H
#define __TESTING_CM_SERVICE_PROFILES_A2DP_EVENT_H

int test_a2dp_event_setup(FAR void** state);
int test_a2dp_event_teardown(FAR void** state);

void test_a2dp_event_new_normal(FAR void** state);
void test_a2dp_event_new_null_addr(FAR void** state);
void test_a2dp_event_new_ext_with_data(FAR void** state);
void test_a2dp_event_new_ext_zero_size(FAR void** state);

#endif
