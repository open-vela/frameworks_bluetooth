############################################################################
# frameworks/bluetooth/tools/gdb/btdiag_init.py
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
import os
import gdb

# Get the current directory path, which is where btinit.py is located
base_dir = os.path.dirname(os.path.abspath(__file__))

# Source gdbinit.py from the relative path
nuttx_gdbinit_path = os.path.abspath(
    os.path.join(base_dir, "../../../../nuttx/tools/gdb/gdbinit.py")
)

# Add the directory containing 'nuttxgdb' to sys.path
nuttx_gdb_module_path = os.path.abspath(
    os.path.join(base_dir, "../../../../nuttx/tools/gdb")
)
if nuttx_gdb_module_path not in sys.path:
    sys.path.insert(0, nuttx_gdb_module_path)

if os.path.exists(nuttx_gdbinit_path):
    gdb.execute(f"source {nuttx_gdbinit_path}")
    gdb.write(f"Sourced GDB init file from: {nuttx_gdbinit_path}\n")
else:
    gdb.write(f"GDB init file not found at: {nuttx_gdbinit_path}\n")

# List of modules to be registered
modules_to_register = [
    "service.btsocket",  # Example path: frameworks/bluetooth/tools/gdb/service/btsocket.py
    "service.btdev",
    "stack.btstack",
    "driver.btsnoop",
    "utlis.bttimeval",
]

# Import each module to register commands
for module_name in modules_to_register:
    module_path = os.path.join(base_dir, *module_name.split(".")) + ".py"

    if os.path.exists(module_path):
        try:
            gdb.execute(f"source {module_path}")
            gdb.write(f"Sourced GDB command module: {module_path}\n")
        except Exception as e:
            gdb.write(f"Failed to source module {module_path}: {e}\n")
    else:
        gdb.write(f"Module not found: {module_path}\n")
