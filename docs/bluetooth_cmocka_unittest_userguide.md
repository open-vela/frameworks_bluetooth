# Bluetooth Unit Test User Guide

## 1. Overview

The Bluetooth unit tests are based on the [cmocka](https://cmocka.org/) test framework and run on the openvela system. Test code is located at:

```
frameworks/connectivity/bluetooth/tests/unittest/
├── framework_common/    # Bluetooth framework common module tests (146 cases)
├── framework_api/       # Bluetooth framework API tests
├── service_device/      # Bluetooth service device module tests (48 cases)
└── Kconfig              # Top-level menu configuration
```

Directory structure for each test module:

```
[module]/
├── cm_[module]_entry.c      # Entry file, registers all test cases
├── include/
│   └── cm_[source].h        # Test function declaration header
├── src/
│   └── test_[source].c      # Test case implementation
├── Kconfig                  # Module Kconfig configuration
├── Makefile                 # Make build file
├── CMakeLists.txt           # CMake build file
└── Make.defs                # Make.defs reference
```

## 2. Dependencies

### 2.1 Kconfig Dependencies

The following configuration items must be enabled in defconfig:

| Config Item | Description | Required |
|-------------|-------------|----------|
| `CONFIG_TESTING_CMOCKA=y` | cmocka test framework | Yes |
| `CONFIG_ALLOW_MIT_COMPONENTS=y` | Allow MIT licensed components (cmocka dependency) | Yes |
| `CONFIG_CM_FRAMEWORK_COMMON_TEST=y` | framework_common test module | As needed |
| `CONFIG_CM_SERVICE_DEVICE_TEST=y` | service_device test module | As needed |
| `CONFIG_CM_FRAMEWORK_API_TEST=y` | framework_api test module | As needed |

All test modules have `depends on TESTING_CMOCKA`. If cmocka is not enabled, test module options will not appear in menuconfig.

### 2.2 Optional Stack Size Configuration

| Config Item | Default | Description |
|-------------|---------|-------------|
| `CONFIG_CM_FRAMEWORK_COMMON_TEST_STACKSIZE` | `8192` | framework_common test task stack size |
| `CONFIG_CM_SERVICE_DEVICE_TEST_STACKSIZE` | `16384` | service_device test task stack size |

### 2.3 Build Environment

- OpenVela source tree with `./build.sh` initialized
- Target board configuration (e.g., `vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap`)
- QEMU emulator (for goldfish targets) or actual hardware

## 3. Adding Bluetooth Unit Tests

### 3.1 Enabling Existing Test Modules

Edit the target board's defconfig file, for example:

```
vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig
```

Add the following configuration:

```ini
# Base dependencies
CONFIG_TESTING_CMOCKA=y
CONFIG_ALLOW_MIT_COMPONENTS=y

# Enable desired test modules
CONFIG_CM_FRAMEWORK_COMMON_TEST=y
CONFIG_CM_FRAMEWORK_COMMON_TEST_STACKSIZE=8192
CONFIG_CM_SERVICE_DEVICE_TEST=y
CONFIG_CM_SERVICE_DEVICE_TEST_STACKSIZE=16384
```

### 3.2 Adding a New Test Module

Example: adding a `service_foo` module.

#### Step 1: Create directory structure

```bash
mkdir -p frameworks/connectivity/bluetooth/tests/unittest/service_foo/include
mkdir -p frameworks/connectivity/bluetooth/tests/unittest/service_foo/src
```

#### Step 2: Create Kconfig

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

#### Step 3: Create Makefile

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

#### Step 4: Create CMakeLists.txt

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

#### Step 5: Create Make.defs

```makefile
# frameworks/connectivity/bluetooth/tests/unittest/service_foo/Make.defs

ifneq ($(CONFIG_CM_SERVICE_FOO_TEST),)
CONFIGURED_APPS += $(APPDIR)/frameworks/connectivity/bluetooth/tests/unittest/service_foo
endif
```

#### Step 6: Create test header file

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

#### Step 7: Create test source file

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
    return 0;
}

int test_foo_teardown(void** state)
{
    return 0;
}

void test_foo_normal(void** state)
{
    assert_true(1);
}

void test_foo_null_param(void** state)
{
    assert_true(1);
}
```

#### Step 8: Create entry file

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

#### Step 9: Register with the top-level build system

Add references to the new module in the following files:

`frameworks/connectivity/bluetooth/tests/unittest/Kconfig`:
```kconfig
source "$APPDIR/frameworks/connectivity/bluetooth/tests/unittest/service_foo/Kconfig"
```

`frameworks/connectivity/bluetooth/tests/unittest/CMakeLists.txt`:
```cmake
include(${CMAKE_CURRENT_LIST_DIR}/service_foo/CMakeLists.txt)
```

`frameworks/connectivity/bluetooth/tests/unittest/Make.defs` does not need modification (uses wildcard to auto-include subdirectories).

#### Step 10: Enable in defconfig

```ini
CONFIG_CM_SERVICE_FOO_TEST=y
```

### 3.3 Adding Test Cases to an Existing Module

To add new test source files to an existing module (e.g., `framework_common`):

1. Create `include/cm_[source].h` to declare test functions
2. Create `src/test_[source].c` to implement test cases
3. Add `CSRCS += src/test_[source].c` in `Makefile`
4. Include `cm_[source].h` in `cm_[module]_entry.c` and register cases in the test array

## 4. Build and Run

### 4.1 Build

```bash
# Full build
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j$(nproc)
```

For incremental builds, if you encounter link errors (`undefined reference to cmocka_xxx_main`), delete the `.built` files under the test module and rebuild:

```bash
rm -f frameworks/connectivity/bluetooth/tests/unittest/*/.built
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j$(nproc)
```

### 4.2 Start the Emulator

Run with the QEMU emulator (goldfish target):

```bash
./emulator.sh vela -no-window
```

Wait for the `openvela-ap>` prompt to appear, indicating the NuttX shell is ready.

### 4.3 Run Tests

Enter test commands in the NuttX shell:

```
openvela-ap> cmocka_framework_common_test
openvela-ap> cmocka_service_device_test
```

The test command name corresponds to the `PROGNAME` value in the Makefile.

### 4.4 Result Interpretation

cmocka output format:

```
[==========] module_tests: Running N test(s).
[ RUN      ] test_xxx_normal
[       OK ] test_xxx_normal
...
[==========] module_tests: N test(s) run.
[  PASSED  ] N test(s).
```

- `[  PASSED  ]` with no `[  FAILED  ]`: all tests passed
- `[  FAILED  ]` present: some test cases failed, investigate based on the output

### 4.5 Automated Execution with tmux (Recommended)

```bash
# Install tmux (if not installed)
which tmux || apt-get install -y tmux

# Start emulator
tmux new-session -d -s nuttx './emulator.sh vela -no-window'

# Wait for startup then send test command
sleep 15
tmux send-keys -t nuttx 'cmocka_framework_common_test' Enter

# Wait for execution then view output
sleep 10
tmux capture-pane -t nuttx -p -S -200

# Run next test
tmux send-keys -t nuttx 'cmocka_service_device_test' Enter

# Close when done
tmux kill-session -t nuttx
```

## 5. Existing Test Modules

| Module | Command | Cases | Test Content |
|--------|---------|-------|--------------|
| framework_common | `cmocka_framework_common_test` | 146 | bt_addr, bt_uuid, bt_list, bt_hash, bt_time, callbacks_list, state_machine, index_allocator, hci_parser, scan_record, scan_filter, advertiser_data |
| service_device | `cmocka_service_device_test` | 48 | device create/delete, transport, address, name, device class, UUIDs, appearance, RSSI, alias, connection state, ACL handle, bond state, link key, LE PHY, flags, SMP key |
| framework_api | `cmocka_framework_api_test` | — | Framework API layer tests (enable separately) |

## 6. FAQs

### Q1: Build error `undefined reference to cmocka_xxx_test_main`

During incremental builds, the NuttX build system may skip recompiling the test module due to the `.built` marker file.

Solution:

```bash
rm -f frameworks/connectivity/bluetooth/tests/unittest/*/.built
```

Then rebuild.

### Q2: Bluetooth test options not visible in menuconfig

Ensure the following prerequisite configurations are enabled:

```ini
CONFIG_TESTING_CMOCKA=y
CONFIG_ALLOW_MIT_COMPONENTS=y
```

Bluetooth test modules declare `depends on TESTING_CMOCKA`. If cmocka is not enabled, test options will not be displayed.

### Q3: Test command shows `command not found` in NuttX shell

Possible causes:
1. Corresponding `CONFIG_CM_xxx_TEST=y` not enabled in defconfig
2. Firmware not reflashed / emulator not restarted after build
3. Command name misspelled; use the `help` command to list registered commands

Verify command is compiled into firmware:

```bash
strings out/<config>/nuttx | grep cmocka_
```

### Q4: Test case crashes or signal exception during execution

Common causes:
- Insufficient stack size: increase `CONFIG_CM_xxx_TEST_STACKSIZE` (e.g., from default to `8192` or `16384`)
- Uninitialized pointer access in tests: check that the setup function initializes correctly
- Incorrect cmocka header inclusion order: must be included in the following order

```c
// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on
```

### Q5: How to run only a specific module's tests?

Each test module is an independent NuttX command. Simply execute the corresponding command in the shell. Individual test case selection is not supported at the command-line level. To skip certain cases, modify the test array in the entry file.

### Q6: New test module compiles successfully but command is not registered

Check that `Make.defs` correctly adds the module to `CONFIGURED_APPS`. The top-level `Make.defs` uses wildcard to auto-include subdirectories, but if the subdirectory's `Make.defs` has an incorrect `ifneq` condition, the module will not be included in the build.

### Q7: Emulator hangs after startup, `openvela-ap>` prompt does not appear

- Ensure the correct emulator command is used: `./emulator.sh vela -no-window`
- Check for lingering processes occupying ports: `pkill -f emulator` and retry
- Check emulator log output for ERROR messages
