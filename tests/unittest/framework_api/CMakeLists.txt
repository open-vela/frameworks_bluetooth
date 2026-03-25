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

if(CONFIG_CM_FRAMEWORK_API_TEST)
  set(FRAMEWORK_API_INCDIR ${CMAKE_CURRENT_LIST_DIR}/include
                           ${CMAKE_CURRENT_LIST_DIR}/src
                           ${CMAKE_CURRENT_LIST_DIR}/../../../framework/include
                           ${CMAKE_CURRENT_LIST_DIR}/../../../framework/common
                           ${CMAKE_CURRENT_LIST_DIR}/../../../service/src
                           ${CMAKE_CURRENT_LIST_DIR}/../../../service/common
                           ${CMAKE_CURRENT_LIST_DIR}/../../../service
                           ${CMAKE_CURRENT_LIST_DIR}/../../../service/profiles
                           ${CMAKE_CURRENT_LIST_DIR}/../../../service/profiles/include)
  file(GLOB FRAMEWORK_API_CSRC ${CMAKE_CURRENT_LIST_DIR}/src/*.c)
  nuttx_add_application(
    NAME
    cmocka_framework_api_test
    PRIORITY
    ${CONFIG_CM_FRAMEWORK_API_TEST_PRIORITY}
    STACKSIZE
    ${CONFIG_CM_FRAMEWORK_API_TEST_STACKSIZE}
    MODULE
    ${CONFIG_CM_FRAMEWORK_API_TEST}
    SRCS
    ${CMAKE_CURRENT_LIST_DIR}/cm_framework_api_entry.c
    ${FRAMEWORK_API_CSRC}
    INCLUDE_DIRECTORIES
    ${FRAMEWORK_API_INCDIR})
endif()
