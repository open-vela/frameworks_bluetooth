############################################################################
# frameworks/bluetooth/tools/gdb/service/btsocket.py
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
from collections import defaultdict

# Initialize enum values globally
BT_MESSAGE_TYPE = utils.enum("bt_message_type_t")


class BTSocketCommand(gdb.Command):
    """Command to traverse socket lists and print IPC server/client msg_queue lengths."""

    def __init__(self):
        super(BTSocketCommand, self).__init__("btsocket", gdb.COMMAND_USER)
        self.setup_parser()

    def setup_parser(self):
        self.parser = argparse.ArgumentParser(description=self.__doc__)
        self.parser.add_argument(
            "-s",
            "--server",
            action="store_true",
            help="Traverse g_instances_list for server.",
        )
        self.parser.add_argument(
            "-c", "--client", action="store_true", help="Traverse g_bt_ins for client."
        )
        self.parser.add_argument(
            "-d",
            "--detail",
            action="store_true",
            help="Show detailed packet information.",
        )
        # Added more choices for packet types, like 'choices=["scan", "gatt", "state"]', easily expandable
        self.parser.add_argument(
            "-t",
            "--type",
            choices=["scan"],
            help="Specify the packet type to process (e.g., 'scan', 'gatt', 'state').",
        )

    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        try:
            parsed_args = self.parser.parse_args(argv)
        except SystemExit:
            return
        if parsed_args.server:
            self.traverse_server(parsed_args.detail, parsed_args.type)
        elif parsed_args.client:
            self.traverse_client(parsed_args.detail, parsed_args.type)
        else:
            gdb.write("Please specify either -s (server) or -c (client).\n")

    def traverse_server(self, detail, packet_type):
        gdb.write(
            "Traversing IPC server's msg queue and outputting bt_message_packet_t code...\n"
        )
        gdb.write("\n")

        g_instances_list = gdb.parse_and_eval("g_instances_list")
        if not g_instances_list:
            gdb.write("g_instances_list is NULL\n")
            return

        bt_instance_type = utils.lookup_type("struct bt_instance").pointer()
        instance_counter = 0
        for list_node in lists.NxList(
            g_instances_list["list"], "struct _bt_list_node", "node", reverse=True
        ):
            remote_ins = list_node["data"].cast(bt_instance_type)
            if not remote_ins:
                continue
            instance_counter += 1
            packet_count = 0
            addr_packet_count = defaultdict(int) if packet_type == "scan" else None

            for packet_cache_node in lists.NxList(
                remote_ins["msg_queue"], "bt_packet_cache_t", "node", reverse=True
            ):
                packet = packet_cache_node["packet"]
                if not packet:
                    continue
                code_value = int(packet["code"])
                code_name = BT_MESSAGE_TYPE(code_value).name
                packet_count += 1

                if detail:
                    gdb.write(f"NO. {packet_count} {code_name} ({code_value})\n")

                # Handle specific code_name operations if packet_type is provided
                if packet_type == "scan" and "LE_ON_SCAN_RESULT" in code_name:
                    self.handle_packet_by_code_name(
                        code_name, packet, addr_packet_count, detail
                    )

            gdb.write("--------- End of msg queue ---------\n")
            gdb.write(
                f"msg_queue length for remote_ins {instance_counter} is {packet_count}\n"
            )

            # Write address packet counts only if packet_type is "scan"
            if packet_type == "scan" and addr_packet_count:
                gdb.write(
                    "--------- Show key-value info of addr when packet type is scan ---------\n"
                )
                for addr, count in addr_packet_count.items():
                    gdb.write(f"Address {addr} has {count} packets.\n")

    def traverse_client(self, detail, packet_type):
        gdb.write(
            "Traversing IPC client's msg queue and outputting bt_message_packet_t code...\n"
        )
        gdb.write("\n")

        g_bt_ins = gdb.parse_and_eval("g_bt_ins")

        if not g_bt_ins:
            gdb.write("g_bt_ins is NULL\n")
            return

        packet_count = 0
        addr_packet_count = defaultdict(int) if packet_type == "scan" else None
        for packet_cache_node in lists.NxList(
            g_bt_ins["msg_queue"], "bt_packet_cache_t", "node", reverse=True
        ):
            packet = packet_cache_node["packet"]
            if not packet:
                continue
            code_value = int(packet["code"])
            code_name = BT_MESSAGE_TYPE(code_value).name
            packet_count += 1

            if detail:
                gdb.write(f"NO. {packet_count} {code_name} ({code_value})\n")

            # Handle specific code_name operations if packet_type is provided
            if packet_type == "scan" and "LE_ON_SCAN_RESULT" in code_name:
                self.handle_packet_by_code_name(
                    code_name, packet, addr_packet_count, detail
                )

        gdb.write("--------- End of msg queue ---------\n")
        gdb.write(f"msg_queue length for local_ins is {packet_count}\n")

        # Write address packet counts only if packet_type is "scan"
        if packet_type == "scan" and addr_packet_count:
            gdb.write(
                "--------- Show key-value info of addr when packet type is scan ---------\n"
            )
            for addr, count in addr_packet_count.items():
                gdb.write(f"Address {addr} has {count} packets.\n")

    def handle_packet_by_code_name(self, code_name, packet, addr_packet_count, detail):
        """Handle packet based on the code_name. Extend this function to handle more packet types."""
        if "LE_ON_SCAN_RESULT" in code_name and addr_packet_count is not None:
            addr_array = packet["scan_cb"]["_on_scan_result_cb"]["result"]["addr"][
                "addr"
            ]
            address_str = ":".join(
                f"{int(byte):02x}"
                for byte in utils.ArrayIterator(addr_array, maxlen=6, reverse=True)
            )
            addr_packet_count[address_str] += 1
            if detail:
                gdb.write(f"Address: {address_str.upper()} (Type: {code_name})\n")

        # Future handling for other packet types can be added here
        elif "LE_ON_GATT_EVENT" in code_name:
            # Add GATT specific operations here
            if detail:
                gdb.write(f"GATT Event Detected (Type: {code_name})\n")


# Register the command
BTSocketCommand()
