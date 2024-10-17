#
# Copyright (C) 2024 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#

if(CONFIG_BLUETOOTH)

  # Source Directories
  set(BLUETOOTH_DIR ${CMAKE_CURRENT_SOURCE_DIR})

  # Flags
  set(CFLAGS)
  set(CSRCS)
  set(INCDIR)

  # Sources
  file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/common/*.c)
  list(APPEND CSRCS ${APPEND_FILES})

  if(CONFIG_BLUETOOTH_FRAMEWORK)
    if(CONFIG_BLUETOOTH_FRAMEWORK_LOCAL)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/api/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
        file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/framework/api/bt_lea*)
        list(REMOVE_ITEM CSRCS ${EXLUDE_FILES})
      endif()

    elseif(CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/api/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
        file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/framework/api/bt_lea*)
        list(REMOVE_ITEM CSRCS ${EXLUDE_FILES})
      endif()

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/ipc/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/ipc/socket/src/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/socket/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      if (CONFIG_BLUETOOTH_FRAMEWORK_ASYNC)
        file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/socket/async/*.c)
        list(APPEND CSRCS ${APPEND_FILES})
      endif()

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/ipc/socket/include)

    else()
      # Other IPC source files

    endif()
  endif()

  list(
    APPEND
    CSRCS
    ${BLUETOOTH_DIR}/service/src/connection_manager.c
    ${BLUETOOTH_DIR}/service/src/manager_service.c
    ${BLUETOOTH_DIR}/service/common/index_allocator.c)

  if(CONFIG_BLUETOOTH_STORAGE_PROPERTY_SUPPORT)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/service/common/storage_property.c)
  else()
    list(APPEND CSRCS ${BLUETOOTH_DIR}/service/common/storage.c)
  endif()

  if(CONFIG_BLUETOOTH_SERVICE)
    list(
      APPEND
      CSRCS
      ${BLUETOOTH_DIR}/service/common/bt_time.c
      ${BLUETOOTH_DIR}/service/common/service_loop.c
      ${BLUETOOTH_DIR}/service/src/adapter_service.c
      ${BLUETOOTH_DIR}/service/src/adapter_state.c
      ${BLUETOOTH_DIR}/service/src/btservice.c
      ${BLUETOOTH_DIR}/service/src/device.c
      ${BLUETOOTH_DIR}/service/src/power_manager.c
      ${BLUETOOTH_DIR}/service/vendor/bt_vendor.c
      ${BLUETOOTH_DIR}/service/src/hci_parser.c)

    if(CONFIG_BLUETOOTH_BLE_ADV)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/src/advertising.c)
    endif()

    if(CONFIG_BLUETOOTH_BLE_SCAN)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/src/scan_manager.c
           ${BLUETOOTH_DIR}/service/src/scan_record.c
           ${BLUETOOTH_DIR}/service/src/scan_filter.c)
    endif()

    if(CONFIG_BLUETOOTH_L2CAP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/src/l2cap_service.c)
    endif()

    if(CONFIG_LE_DLF_SUPPORT)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/src/connectino_manager_dlf.c)
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/stacks/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    if(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET
       OR CONFIG_BLUETOOTH_STACK_LE_BLUELET)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/stacks/bluelet/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE OR CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_debug_interface.c)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_zblue.c)
      if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_adapter_classic_interface.c)
      endif()
    endif()

    if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
      file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/service/stacks/bluelet/sal_lea_*)
      list(REMOVE_ITEM CSRCS ${EXLUDE_FILES})
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/system/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    if(NOT CONFIG_BLUETOOTH_A2DP)
      list(REMOVE_ITEM CSRCS
           ${BLUETOOTH_DIR}/service/profiles/system/bt_player.c)
    endif()

    if(NOT
       (CONFIG_BLUETOOTH_A2DP
        OR CONFIG_BLUETOOTH_HFP_AG
        OR CONFIG_BLUETOOTH_HFP_HF
        OR CONFIG_BLUETOOTH_BLE_AUDIO))
      list(REMOVE_ITEM CSRCS
           ${BLUETOOTH_DIR}/service/profiles/system/media_system.c)
    endif()

    file(GLOB APPEND_FILES
         ${BLUETOOTH_DIR}/service/profiles/audio_interface/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    if(CONFIG_BLUETOOTH_GATT)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/gatt/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_A2DP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/codec/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/a2dp)

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/a2dp/codec)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SOURCE)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/source/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/sink/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_TARGET OR CONFIG_BLUETOOTH_AVRCP_CONTROL)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/avrcp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/avrcp)
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_TARGET)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/avrcp/target/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/avrcp/control/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HFP_HF)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hfp_hf/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HFP_AG)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hfp_ag/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_SPP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/spp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HID_DEVICE)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hid/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_PAN)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/pan/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT OR CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/audio_ipc/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/codec/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/server/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CCP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/ccp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/mcp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICS)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/vmics/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/client/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCS)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/mcs/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_TBS)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/tbs/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICP)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/vmicp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/utils/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/vhal/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    list(APPEND INCDIR ${BLUETOOTH_DIR}/service/vhal)
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/utils.c)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/log.c)

    if(CONFIG_BLUETOOTH_BLE_ADV)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/adv.c)
    endif()

    if(CONFIG_BLUETOOTH_BLE_SCAN)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/scan.c)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/a2dp_sink.c)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SOURCE)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/a2dp_source.c)
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/avrcp_control.c)
    endif()

    if(CONFIG_BLUETOOTH_GATT)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/gatt_client.c)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/gatt_server.c)
    endif()

    if(CONFIG_BLUETOOTH_HFP_HF)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/hfp_hf.c)
    endif()

    if(CONFIG_BLUETOOTH_HFP_AG)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/hfp_ag.c)
    endif()

    if(CONFIG_BLUETOOTH_SPP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/spp.c)
    endif()

    if(CONFIG_BLUETOOTH_HID_DEVICE)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/hid_device.c)
    endif()

    if(CONFIG_BLUETOOTH_PAN)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/panu.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_server.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_mcp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CCP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_ccp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICS)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_vmics.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_client.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_vmicp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCS)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_mcs.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_TBS)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_tbs.c)
    endif()
  endif()

  list(APPEND INCDIR ${BLUETOOTH_DIR}/framework/include)

  if(CONFIG_LIB_DBUS_RPMSG_SERVER_CPUNAME OR CONFIG_OFONO)
    list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/dbus/dbus)

    list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/glib/glib/glib)

    list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/glib/glib)

    list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/glib)

    list(APPEND INCDIR ${NUTTX_APPS_DIR}/frameworks/system/utils/gdbus)
  endif()

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/src)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/common)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/include)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/system)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/stacks)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/stacks/include)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/vendor)

  if(CONFIG_BLUETOOTH_SERVICE)
    if(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET
       OR CONFIG_BLUETOOTH_STACK_LE_BLUELET)
      list(
        APPEND INCDIR
        ${NUTTX_APPS_DIR}/external/bluelet/bluelet/src/samples/stack_adapter/inc
      )

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/stacks/bluelet/include)
      list(APPEND INCDIR ${NUTTX_APPS_DIR}/vendor/xiaomi/vela/bluelet/inc)
    endif()

    if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE OR CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
      list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/zblue/zblue/port/include/)
      list(APPEND INCDIR
           ${NUTTX_APPS_DIR}/external/zblue/zblue/port/include/kernel/include)
    endif()

    list(APPEND INCDIR ${BLUETOOTH_DIR}/service/ipc)
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    list(APPEND INCDIR ${BLUETOOTH_DIR}/tools)
  endif()

  if(CONFIG_ARCH_SIM)
    list(APPEND CFLAGS -O0)
  endif()

  list(APPEND CFLAGS -Wno-strict-prototypes)

  if(CONFIG_BLUETOOTH_FEATURE)
    set(FEATURE_TOP ${NUTTX_APPS_DIR}/frameworks/runtimes/feature)
    list(APPEND INCDIR ${FEATURE_TOP}/include)
    list(APPEND INCDIR
         ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/feature/include)

    list(APPEND CSRCS ${BLUETOOTH_DIR}/feature/src/system_bluetooth.c)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/feature/src/system_bluetooth_impl.c)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/feature/src/feature_bluetooth_util.c)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt.c)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_impl.c)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/feature/src/feature_bluetooth_callback.c)

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      list(APPEND CSRCS
           ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_a2dpsink.c)
      list(APPEND CSRCS
           ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_a2dpsink_impl.c)
    endif()
    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      list(APPEND CSRCS
           ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_avrcpcontrol.c)
      list(APPEND CSRCS
           ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_avrcpcontrol_impl.c)
    endif()
  endif()

  nuttx_add_library(libbluetooth STATIC)

  # Add Applications
  if(CONFIG_BLUETOOTH_SERVER)
    nuttx_add_application(
      NAME
      ${CONFIG_BLUETOOTH_SERVER_NAME}
      SRCS
      ${BLUETOOTH_DIR}/service/src/main.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      8192
      PRIORITY
      SCHED_PRIORITY_DEFAULT
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libbluetooth)
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    nuttx_add_application(
      NAME
      bttool
      SRCS
      ${BLUETOOTH_DIR}/tools/bt_tools.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      8192
      PRIORITY
      ${SCHED_PRIORITY_DEFAULT}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libbluetooth)

    nuttx_add_application(
      NAME
      adapter_test
      SRCS
      ${BLUETOOTH_DIR}/tests/adapter_test.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      8192
      PRIORITY
      ${SCHED_PRIORITY_DEFAULT}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libbluetooth)
  endif()

  if(CONFIG_BLUETOOTH_UPGRADE)
    nuttx_add_application(
        NAME
        bt_upgrade
        SRCS
        ${BLUETOOTH_DIR}/tools/storage_transform.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        STACKSIZE
        8192
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS
        ${CFLAGS}
        DEPENDS
        libbluetooth)
  endif()

  if(CONFIG_BLUETOOTH_FEATURE)
    include(nuttx_add_jidl)
    set(PY_SCRIPT ${FEATURE_TOP}/tools/jidl/jsongensource.py)
    set(BINARY_EXT_MODULES_DIR ${CMAKE_BINARY_DIR}/feature/modules/)
    set(JIDL_PATHS ${BLUETOOTH_DIR}/feature/jdil/bluetooth.jidl
                   ${BLUETOOTH_DIR}/feature/jdil/bluetooth_bt.jidl)

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      list(APPEND JIDL_PATHS
           ${BLUETOOTH_DIR}/feature/jdil/bluetooth_a2dpsink.jidl)
    endif()
    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      list(APPEND JIDL_PATHS
           ${BLUETOOTH_DIR}/feature/jdil/bluetooth_avrcpcontrol.jidl)
    endif()

    nuttx_add_jidl(
      TARGET
      libbluetooth
      JIDL_SCRIPT
      ${PY_SCRIPT}
      JIDL_OUT_DIR
      ${BINARY_EXT_MODULES_DIR}
      JIDLS
      ${JIDL_PATHS}
      OUT_SRC_EXT
      c)
  endif()

  # Add Dependson
  nuttx_add_dependencies(TARGET libbluetooth DEPENDS libuv)

  # Export Headers
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/include
             ${BLUETOOTH_DIR}/framework/include)

  # Library Configuration
  target_compile_options(libbluetooth PRIVATE ${CFLAGS})
  target_sources(libbluetooth PRIVATE ${CSRCS})
  target_include_directories(libbluetooth PRIVATE ${INCDIR})

endif()
