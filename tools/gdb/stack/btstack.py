############################################################################
# frameworks/bluetooth/tools/gdb/stack/btstack.py
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
import gdb
import argparse
from nuttxgdb import utils

ADPT_GATT_REQ_TYPE = utils.enum("ADPT_GATT_REQ_TYPE")


class BTStackCommand(gdb.Command):
    """Command to interact with the Bluetooth stack and display information."""

    def __init__(self):
        # Register the command in GDB with the name "btstack"
        super(BTStackCommand, self).__init__("btstack", gdb.COMMAND_USER)
        self.setup_parser()

    def setup_parser(self):
        # Use argparse to parse command line arguments
        self.parser = argparse.ArgumentParser(description=self.__doc__)
        self.parser.add_argument(
            "-t",
            "--type",
            required=True,
            choices=["gatt"],
            help="Specify the type of stack to display, e.g., 'gatt'.",
        )

    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        try:
            parsed_args = self.parser.parse_args(argv)
        except SystemExit:
            return
        if parsed_args.type == "gatt":
            self.process_gatt_requests()

    def process_gatt_requests(self):
        # Get the GATT request list head
        s_adpt_inst = gdb.parse_and_eval("s_adpt_inst")
        req = s_adpt_inst["gatt_req_list"]["head"]
        adpt_gatt_client_req_type = utils.lookup_type(
            "ADPT_GATT_CLIENT_REQ_S"
        ).pointer()
        count = 0
        # Loop through each request in the list
        while req:
            if not req:
                gdb.write("No more requests in the GATT request list.\n")
                return
            # Print the request type
            req_s = req.cast(adpt_gatt_client_req_type)
            _req_type = ADPT_GATT_REQ_TYPE(int(req_s["req_type"])).name
            count += 1
            gdb.write(f"Pending request Type: {_req_type}, no. {count}\n")
            # Get the next request in the list using the SNEXT equivalent
            next_req = self.get_next_request(req)
            req = next_req

    def get_next_request(self, req):
        """Python implementation of the SNEXT macro to get the next node."""
        try:
            # Evaluate the pointer arithmetic similar to SNEXT macro
            next_req_address = gdb.parse_and_eval(f"(*((void **){req}) - 1)")
            return next_req_address
        except gdb.error as e:
            gdb.write(f"Error getting next request: {e}\n")
            return None


BTStackCommand()
