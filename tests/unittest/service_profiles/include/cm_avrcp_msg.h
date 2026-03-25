/****************************************************************************
 * tests/unittest/service_profiles/include/cm_avrcp_msg.h
 ****************************************************************************/

#ifndef __TESTING_CM_SERVICE_PROFILES_AVRCP_MSG_H
#define __TESTING_CM_SERVICE_PROFILES_AVRCP_MSG_H

int test_avrcp_msg_setup(FAR void** state);
int test_avrcp_msg_teardown(FAR void** state);

void test_avrcp_msg_new_normal(FAR void** state);
void test_avrcp_msg_new_null_addr(FAR void** state);
void test_avrcp_msg_destroy_with_attrs(FAR void** state);
void test_avrcp_msg_destroy_no_attrs(FAR void** state);

#endif
