############################################################################
# frameworks/bluetooth/tools/gdb/service/btdev.py
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
import argparse
import gdb
from nuttxgdb import utils
from nuttxgdb import lists


# Initialize enum values globally
BOND_STATE_TYPE = utils.enum("bond_state_t")
CONNECTION_STATE_TYPE = utils.enum("connection_state_t")


class BTDevCommand(gdb.Command):
    """Command to traverse device lists and print detailed device information."""

    def __init__(self):
        super(BTDevCommand, self).__init__("btdev", gdb.COMMAND_USER)
        self.setup_parser()

    def setup_parser(self):
        self.parser = argparse.ArgumentParser(description=self.__doc__)
        self.parser.add_argument(
            "-t",
            "--type",
            choices=["le", "br"],
            required=True,
            help="Specify the device type to traverse (e.g., 'le' for BLE, 'br' for BR/EDR).",
        )

    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        try:
            parsed_args = self.parser.parse_args(argv)
        except SystemExit:
            return

        if parsed_args.type == "le":
            self.traverse_devices("le_devices")
        elif parsed_args.type == "br":
            self.traverse_devices("devices")

    def traverse_devices(self, device_type):
        list_name = "BLE" if device_type == "le_devices" else "BR/EDR"
        gdb.write(
            f"Traversing {list_name} device lists and outputting detailed info...\n"
        )
        gdb.write("\n")
        g_adapter_service = gdb.parse_and_eval("g_adapter_service")

        if not g_adapter_service:
            gdb.write("g_adapter_service is NULL\n")
            return

        bt_device_type = utils.lookup_type("struct bt_device").pointer()

        for list_node in lists.NxList(
            g_adapter_service[device_type]["list"],
            "struct _bt_list_node",
            "node",
            reverse=True,
        ):
            if not list_node["data"]:
                continue
            bt_device = list_node["data"].cast(bt_device_type)
            remote_device = bt_device["remote"]
            addr_array = remote_device["addr"]["addr"]
            address_str = ":".join(
                f"{int(byte):02x}"
                for byte in utils.ArrayIterator(addr_array, maxlen=6, reverse=True)
            )
            bond_state = BOND_STATE_TYPE(int(remote_device["bond_state"])).name
            connection_state = CONNECTION_STATE_TYPE(
                int(remote_device["connection_state"])
            ).name
            acl_handle = int(remote_device["acl_handle"])

            # Print detailed device information
            gdb.write(f"Device Name: {remote_device['name'].string()}\n")
            gdb.write(f"Alias: {remote_device['alias'].string()}\n")
            gdb.write(f"Bluetooth Address: {address_str.upper()}\n")
            gdb.write(f"ACL Handle: 0x{acl_handle:04x}\n")
            gdb.write(f"Connection State: {connection_state}\n")
            gdb.write(f"Bond State: {bond_state}\n")
            gdb.write("--------- End of Device Info ---------\n")
            gdb.write("\n")


# Register the command
BTDevCommand()
