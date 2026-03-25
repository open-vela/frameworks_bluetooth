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

if(CONFIG_CM_SERVICE_UTILS_TEST)
  set(SERVICE_UTILS_INCDIR ${CMAKE_CURRENT_LIST_DIR}/include
                            ${CMAKE_CURRENT_LIST_DIR}/src
                            ${CMAKE_CURRENT_LIST_DIR}/../../../../framework/include
                            ${CMAKE_CURRENT_LIST_DIR}/../../../../framework/common
                            ${CMAKE_CURRENT_LIST_DIR}/../../../../service/common
                            ${CMAKE_CURRENT_LIST_DIR}/../../../../service/src
                            ${CMAKE_CURRENT_LIST_DIR}/../../../../service
                            ${CMAKE_CURRENT_LIST_DIR}/../../../../service/utils)
  file(GLOB SERVICE_UTILS_CSRC ${CMAKE_CURRENT_LIST_DIR}/src/*.c)
  nuttx_add_application(
    NAME
    cmocka_service_utils_test
    PRIORITY
    ${CONFIG_CM_SERVICE_UTILS_TEST_PRIORITY}
    STACKSIZE
    ${CONFIG_CM_SERVICE_UTILS_TEST_STACKSIZE}
    MODULE
    ${CONFIG_CM_SERVICE_UTILS_TEST}
    SRCS
    ${CMAKE_CURRENT_LIST_DIR}/cm_service_utils_entry.c
    ${SERVICE_UTILS_CSRC}
    INCLUDE_DIRECTORIES
    ${SERVICE_UTILS_INCDIR})
endif()
