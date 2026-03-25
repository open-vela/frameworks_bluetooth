/****************************************************************************
 * tests/unittest/service_profiles/include/cm_hfp_ag_event.h
 ****************************************************************************/

#ifndef __TESTING_CM_SERVICE_PROFILES_HFP_AG_EVENT_H
#define __TESTING_CM_SERVICE_PROFILES_HFP_AG_EVENT_H

int test_hfp_ag_event_setup(FAR void** state);
int test_hfp_ag_event_teardown(FAR void** state);

void test_hfp_ag_msg_new_normal(FAR void** state);
void test_hfp_ag_msg_new_null_addr(FAR void** state);
void test_hfp_ag_event_new_ext_with_data(FAR void** state);
void test_hfp_ag_event_new_ext_zero_size(FAR void** state);

#endif
