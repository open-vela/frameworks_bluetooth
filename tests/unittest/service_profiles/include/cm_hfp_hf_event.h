/****************************************************************************
 * tests/unittest/service_profiles/include/cm_hfp_hf_event.h
 ****************************************************************************/

#ifndef __TESTING_CM_SERVICE_PROFILES_HFP_HF_EVENT_H
#define __TESTING_CM_SERVICE_PROFILES_HFP_HF_EVENT_H

int test_hfp_hf_event_setup(FAR void** state);
int test_hfp_hf_event_teardown(FAR void** state);

void test_hfp_hf_msg_new_normal(FAR void** state);
void test_hfp_hf_msg_new_null_addr(FAR void** state);
void test_hfp_hf_msg_new_ext_with_data(FAR void** state);
void test_hfp_hf_msg_destroy_null(FAR void** state);

#endif
