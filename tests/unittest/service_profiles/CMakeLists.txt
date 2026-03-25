############################################################################
#  Copyright (C) 2026 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

if(CONFIG_CM_SERVICE_PROFILES_TEST)
  nuttx_add_application(
    NAME cmocka_service_profiles_test
    PRIORITY ${CONFIG_CM_SERVICE_PROFILES_TEST_PRIORITY}
    STACKSIZE ${CONFIG_CM_SERVICE_PROFILES_TEST_STACKSIZE}
    MODULE ${CONFIG_CM_SERVICE_PROFILES_TEST}
    SRCS
      ${CMAKE_CURRENT_LIST_DIR}/cm_service_profiles_entry.c
      ${CMAKE_CURRENT_LIST_DIR}/src/test_a2dp_codec_sbc.c
      ${CMAKE_CURRENT_LIST_DIR}/src/test_a2dp_codec_aac.c
      ${CMAKE_CURRENT_LIST_DIR}/src/test_a2dp_event.c
      ${CMAKE_CURRENT_LIST_DIR}/src/test_avrcp_msg.c
      ${CMAKE_CURRENT_LIST_DIR}/src/test_hfp_hf_event.c
      ${CMAKE_CURRENT_LIST_DIR}/src/test_hfp_ag_event.c
      ${CMAKE_CURRENT_LIST_DIR}/src/test_gatt_event.c
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/a2dp/a2dp_event.c
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/avrcp/avrcp_msg.c
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/hfp_hf/hfp_hf_event.c
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/hfp_ag/hfp_ag_event.c
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/gatt/gattc_event.c
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/gatt/gatts_event.c
      ${CMAKE_CURRENT_LIST_DIR}/src/wrap_a2dp_codec_aac.c
    INCLUDE_DIRECTORIES
      ${CMAKE_CURRENT_LIST_DIR}/include
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/framework/include
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/framework/common
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/common
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/src
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/a2dp
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/a2dp/codec
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/vendor
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/avrcp
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/hfp_hf
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/hfp_ag
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/include
      ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/gatt
  )
endif()
