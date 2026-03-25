# 蓝牙单元测试 User Guide

## 1. 概述

蓝牙单元测试基于 [cmocka](https://cmocka.org/) 测试框架，运行在 OpenVela/NuttX 系统上。测试代码位于：

```
frameworks/connectivity/bluetooth/tests/unittest/
├── framework_common/    # 蓝牙 framework 公共模块测试（146 个用例）
├── framework_api/       # 蓝牙 framework API 测试
├── service_device/      # 蓝牙 service device 模块测试（48 个用例）
└── Kconfig              # 顶层菜单配置
```

每个测试模块的目录结构：

```
[module]/
├── cm_[module]_entry.c      # 入口文件，注册所有测试用例
├── include/
│   └── cm_[source].h        # 测试函数声明头文件
├── src/
│   └── test_[source].c      # 测试用例实现
├── Kconfig                  # 模块 Kconfig 配置
├── Makefile                 # Make 构建文件
├── CMakeLists.txt           # CMake 构建文件
└── Make.defs                # Make.defs 引用
```

## 2. 依赖条件

### 2.1 Kconfig 依赖

以下配置项必须在 defconfig 中启用：

| 配置项 | 说明 | 必须 |
|--------|------|------|
| `CONFIG_TESTING_CMOCKA=y` | cmocka 测试框架 | 是 |
| `CONFIG_ALLOW_MIT_COMPONENTS=y` | 允许 MIT 许可组件（cmocka 依赖） | 是 |
| `CONFIG_CM_FRAMEWORK_COMMON_TEST=y` | framework_common 测试模块 | 按需 |
| `CONFIG_CM_SERVICE_DEVICE_TEST=y` | service_device 测试模块 | 按需 |
| `CONFIG_CM_FRAMEWORK_API_TEST=y` | framework_api 测试模块 | 按需 |

各测试模块均 `depends on TESTING_CMOCKA`，如果未启用 cmocka，测试模块选项不会出现在 menuconfig 中。

### 2.2 可选栈大小配置

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| `CONFIG_CM_FRAMEWORK_COMMON_TEST_STACKSIZE` | `8192` | framework_common 测试任务栈大小 |
| `CONFIG_CM_SERVICE_DEVICE_TEST_STACKSIZE` | `16384` | service_device 测试任务栈大小 |

### 2.3 构建环境

- OpenVela 源码树已完成 `./build.sh` 初始化
- 目标板配置（如 `vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap`）
- QEMU 模拟器（用于 goldfish 目标）或实际硬件

## 3. 添加蓝牙单元测试

### 3.1 启用已有测试模块

编辑目标板的 defconfig 文件，例如：

```
vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig
```

添加以下配置：

```ini
# 基础依赖
CONFIG_TESTING_CMOCKA=y
CONFIG_ALLOW_MIT_COMPONENTS=y

# 启用需要的测试模块
CONFIG_CM_FRAMEWORK_COMMON_TEST=y
CONFIG_CM_FRAMEWORK_COMMON_TEST_STACKSIZE=8192
CONFIG_CM_SERVICE_DEVICE_TEST=y
CONFIG_CM_SERVICE_DEVICE_TEST_STACKSIZE=16384
```

### 3.2 新增测试模块

以新增 `service_foo` 模块为例：

#### Step 1: 创建目录结构

```bash
mkdir -p frameworks/connectivity/bluetooth/tests/unittest/service_foo/include
mkdir -p frameworks/connectivity/bluetooth/tests/unittest/service_foo/src
```

#### Step 2: 创建 Kconfig

```kconfig
# frameworks/connectivity/bluetooth/tests/unittest/service_foo/Kconfig

config CM_SERVICE_FOO_TEST
    tristate "vela auto tests service_foo"
    default n
    depends on TESTING_CMOCKA
    ---help---
        Enable auto tests for the vela service foo module

if CM_SERVICE_FOO_TEST

config CM_SERVICE_FOO_TEST_PRIORITY
    int "Task priority"
    default 100

config CM_SERVICE_FOO_TEST_STACKSIZE
    int "Stack size"
    default DEFAULT_TASK_STACKSIZE

endif
```

#### Step 3: 创建 Makefile

```makefile
# frameworks/connectivity/bluetooth/tests/unittest/service_foo/Makefile

include $(APPDIR)/Make.defs

PROGNAME  = cmocka_service_foo_test
PRIORITY  = $(CONFIG_CM_SERVICE_FOO_TEST_PRIORITY)
STACKSIZE = $(CONFIG_CM_SERVICE_FOO_TEST_STACKSIZE)
MODULE    = $(CONFIG_CM_SERVICE_FOO_TEST)

MAINSRC = $(CURDIR)/cm_service_foo_entry.c

CSRCS  += src/test_foo.c

CFLAGS += -I$(CURDIR)/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/src
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service

include $(APPDIR)/Application.mk
```

#### Step 4: 创建 CMakeLists.txt

```cmake
# frameworks/connectivity/bluetooth/tests/unittest/service_foo/CMakeLists.txt

if(CONFIG_CM_SERVICE_FOO_TEST)
  nuttx_add_application(
    NAME cmocka_service_foo_test
    PRIORITY ${CONFIG_CM_SERVICE_FOO_TEST_PRIORITY}
    STACKSIZE ${CONFIG_CM_SERVICE_FOO_TEST_STACKSIZE}
    MODULE ${CONFIG_CM_SERVICE_FOO_TEST}
    SRCS cm_service_foo_entry.c src/test_foo.c
    INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/include
  )
endif()
```

#### Step 5: 创建 Make.defs

```makefile
# frameworks/connectivity/bluetooth/tests/unittest/service_foo/Make.defs

ifneq ($(CONFIG_CM_SERVICE_FOO_TEST),)
CONFIGURED_APPS += $(APPDIR)/frameworks/connectivity/bluetooth/tests/unittest/service_foo
endif
```

#### Step 6: 创建测试头文件

```c
/* frameworks/connectivity/bluetooth/tests/unittest/service_foo/include/cm_foo.h */

#ifndef CM_FOO_H
#define CM_FOO_H

int test_foo_setup(void** state);
int test_foo_teardown(void** state);

void test_foo_normal(void** state);
void test_foo_null_param(void** state);

#endif /* CM_FOO_H */
```

#### Step 7: 创建测试源文件

```c
/* frameworks/connectivity/bluetooth/tests/unittest/service_foo/src/test_foo.c */

// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on

#include "cm_foo.h"

int test_foo_setup(void** state)
{
    /* 初始化测试环境 */
    return 0;
}

int test_foo_teardown(void** state)
{
    /* 清理测试环境 */
    return 0;
}

void test_foo_normal(void** state)
{
    /* 正常路径测试 */
    assert_true(1);
}

void test_foo_null_param(void** state)
{
    /* 空参数测试 */
    assert_true(1);
}
```

#### Step 8: 创建入口文件

```c
/* frameworks/connectivity/bluetooth/tests/unittest/service_foo/cm_service_foo_entry.c */

// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on

#include "cm_foo.h"

int cmocka_service_foo_test_main(int argc, FAR char* argv[])
{
    const struct CMUnitTest service_foo_tests[] =
    {
        cmocka_unit_test_setup_teardown(test_foo_normal,
            test_foo_setup, test_foo_teardown),
        cmocka_unit_test_setup_teardown(test_foo_null_param,
            test_foo_setup, test_foo_teardown),
    };

    return cmocka_run_group_tests(service_foo_tests, NULL, NULL);
}
```

#### Step 9: 注册到上层构建系统

在以下文件中添加新模块的引用：

`frameworks/connectivity/bluetooth/tests/unittest/Kconfig`：
```kconfig
source "$APPDIR/frameworks/connectivity/bluetooth/tests/unittest/service_foo/Kconfig"
```

`frameworks/connectivity/bluetooth/tests/unittest/CMakeLists.txt`：
```cmake
include(${CMAKE_CURRENT_LIST_DIR}/service_foo/CMakeLists.txt)
```

`frameworks/connectivity/bluetooth/tests/unittest/Make.defs` 无需修改（已使用 wildcard 自动包含）。

#### Step 10: 在 defconfig 中启用

```ini
CONFIG_CM_SERVICE_FOO_TEST=y
```

### 3.3 向已有模块新增测试用例

如果要向已有模块（如 `framework_common`）添加新的测试源文件：

1. 创建 `include/cm_[source].h` 声明测试函数
2. 创建 `src/test_[source].c` 实现测试用例
3. 在 `Makefile` 中添加 `CSRCS += src/test_[source].c`
4. 在 `cm_[module]_entry.c` 中 `#include "cm_[source].h"` 并在测试数组中注册用例

## 4. 编译运行

### 4.1 编译

```bash
# 完整编译
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j$(nproc)
```

如果是增量编译且遇到链接错误（`undefined reference to cmocka_xxx_main`），删除测试模块下的 `.built` 文件后重新编译：

```bash
rm -f frameworks/connectivity/bluetooth/tests/unittest/*/.built
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j$(nproc)
```

### 4.2 启动模拟器

使用 QEMU 模拟器运行（goldfish 目标）：

```bash
./emulator.sh vela -no-window
```

等待出现 `openvela-ap>` 提示符，表示 NuttX shell 已就绪。

### 4.3 执行测试

在 NuttX shell 中输入测试命令：

```
openvela-ap> cmocka_framework_common_test
openvela-ap> cmocka_service_device_test
```

测试命令名称即 Makefile 中的 `PROGNAME` 值。

### 4.4 结果判断

cmocka 输出格式：

```
[==========] module_tests: Running N test(s).
[ RUN      ] test_xxx_normal
[       OK ] test_xxx_normal
...
[==========] module_tests: N test(s) run.
[  PASSED  ] N test(s).
```

- `[  PASSED  ]` 且无 `[  FAILED  ]`：全部通过
- 出现 `[  FAILED  ]`：存在失败用例，需根据输出定位问题

### 4.5 使用 tmux 自动化运行（推荐）

```bash
# 安装 tmux（如未安装）
which tmux || apt-get install -y tmux

# 启动模拟器
tmux new-session -d -s nuttx './emulator.sh vela -no-window'

# 等待启动完成后发送测试命令
sleep 15
tmux send-keys -t nuttx 'cmocka_framework_common_test' Enter

# 等待执行完成后查看输出
sleep 10
tmux capture-pane -t nuttx -p -S -200

# 执行下一个测试
tmux send-keys -t nuttx 'cmocka_service_device_test' Enter

# 结束后关闭
tmux kill-session -t nuttx
```

## 5. 现有测试模块一览

| 模块 | 命令名 | 用例数 | 测试内容 |
|------|--------|--------|----------|
| framework_common | `cmocka_framework_common_test` | 146 | bt_addr, bt_uuid, bt_list, bt_hash, bt_time, callbacks_list, state_machine, index_allocator, hci_parser, scan_record, scan_filter, advertiser_data |
| service_device | `cmocka_service_device_test` | 48 | device create/delete, transport, address, name, device class, UUIDs, appearance, RSSI, alias, connection state, ACL handle, bond state, link key, LE PHY, flags, SMP key |
| framework_api | `cmocka_framework_api_test` | — | framework API 层测试（需单独启用） |

## 6. FAQs

### Q1: 编译报错 `undefined reference to cmocka_xxx_test_main`

增量编译时，NuttX 构建系统可能因为 `.built` 标记文件存在而跳过测试模块的重新编译。

解决方法：

```bash
rm -f frameworks/connectivity/bluetooth/tests/unittest/*/.built
```

然后重新编译。

### Q2: menuconfig 中看不到蓝牙测试选项

确认以下前置配置已启用：

```ini
CONFIG_TESTING_CMOCKA=y
CONFIG_ALLOW_MIT_COMPONENTS=y
```

蓝牙测试模块的 Kconfig 声明了 `depends on TESTING_CMOCKA`，如果 cmocka 未启用，测试选项不会显示。

### Q3: 测试命令在 NuttX shell 中提示 `command not found`

可能原因：
1. defconfig 中未启用对应的 `CONFIG_CM_xxx_TEST=y`
2. 编译后未重新烧录/启动模拟器
3. 命令名拼写错误，可通过 `help` 命令查看已注册的命令列表

验证命令是否编入固件：

```bash
strings out/<config>/nuttx | grep cmocka_
```

### Q4: 测试用例执行时 crash 或 signal 异常

常见原因：
- 栈大小不足：增大 `CONFIG_CM_xxx_TEST_STACKSIZE`（如从默认值改为 `8192` 或 `16384`）
- 测试中访问了未初始化的指针：检查 setup 函数是否正确初始化
- cmocka 头文件包含顺序错误：必须按以下顺序包含

```c
// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on
```

### Q5: 如何只运行某个模块的测试？

每个测试模块是独立的 NuttX 命令，直接在 shell 中执行对应命令即可。无法在命令行级别选择单个用例，如需跳过某些用例，需修改入口文件中的测试数组。

### Q6: 新增测试模块后编译正常但命令未注册

检查 `Make.defs` 是否正确将模块加入 `CONFIGURED_APPS`。顶层 `Make.defs` 使用 wildcard 自动包含子目录，但如果子目录的 `Make.defs` 中 `ifneq` 条件不正确，模块不会被加入构建。

### Q7: 模拟器启动后卡住，没有出现 `openvela-ap>` 提示符

- 确认使用了正确的模拟器命令：`./emulator.sh vela -no-window`
- 检查是否有残留进程占用端口：`pkill -f emulator` 后重试
- 查看模拟器日志输出中是否有 ERROR 信息
