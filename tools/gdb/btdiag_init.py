############################################################################
# frameworks/connectivity/bluetooth/tools/gdb/btdiag_init.py
#
# Copyright (C) 2024 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################
import sys

sys.dont_write_bytecode = True  # Prevent __pycache__ generation

import os
import gdb
import importlib.util

base_dir = os.path.dirname(os.path.abspath(__file__))

nxgdb_dir = os.path.abspath(
    os.path.join(base_dir, "../../../../../nuttx/tools/pynuttx")
)
if nxgdb_dir not in sys.path:
    sys.path.insert(0, nxgdb_dir)

if base_dir not in sys.path:
    sys.path.insert(0, base_dir)

gdbinit_path = os.path.join(nxgdb_dir, "gdbinit.py")
if os.path.exists(gdbinit_path):
    try:
        with open(gdbinit_path, "rb") as f:
            code = compile(f.read(), gdbinit_path, "exec")
            exec(code, globals(), globals())
        gdb.write(f"Imported GDB init module from: {gdbinit_path}\n")
    except Exception as e:
        gdb.write(f"Failed to import GDB init module: {e}\n")
else:
    gdb.write(f"GDB init file not found at: {gdbinit_path}\n")

modules_to_register = [
    "service.btsocket",
    "service.btdev",
    "stack.btstack",
    "driver.btsnoop",
    "utlis.bttimeval",
]


def import_module_from_path(module_path):
    module_name = os.path.splitext(os.path.basename(module_path))[0]
    try:
        spec = importlib.util.spec_from_file_location(module_name, module_path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        gdb.write(f"Imported GDB command module: {module_path}\n")
    except Exception as e:
        gdb.write(f"Failed to import module {module_path}: {e}\n")


for module_name in modules_to_register:
    module_path = os.path.join(base_dir, *module_name.split(".")) + ".py"
    if os.path.exists(module_path):
        import_module_from_path(module_path)
    else:
        gdb.write(f"Module not found: {module_path}\n")


# Register the bthelp command in GDB
class BtHelpCommand(gdb.Command):
    """Custom command to show help information for Bluetooth-related GDB commands."""

    def __init__(self):
        super(BtHelpCommand, self).__init__("bthelp", gdb.COMMAND_SUPPORT)

    def invoke(self, arg, from_tty):
        # Iterate through the modules and execute the help command for each
        for module_name in modules_to_register:
            command_name = module_name.split(".")[
                -1
            ]  # Extract the command name from module name
            try:
                # Print a divider for each command
                gdb.write(f"\n{'=' * 40}\n")
                gdb.write(f"Help for command: {command_name}\n")
                gdb.write(f"{'=' * 40}\n")

                # Execute the help command for each registered module
                gdb.execute(f"{command_name} -h")
            except gdb.error as e:
                gdb.write(f"Failed to execute help for {command_name}: {e}\n")


# Instantiate the bthelp command
BtHelpCommand()
