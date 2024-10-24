############################################################################
# frameworks/bluetooth/tools/gdb/utlis/bttimeval.py
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


class BTTImevalCommand(gdb.Command):
    """Command to display timeval information for given structures."""

    def __init__(self):
        super(BTTImevalCommand, self).__init__("bttimeval", gdb.COMMAND_USER)
        self.setup_parser()

    def setup_parser(self):
        self.parser = argparse.ArgumentParser(description=self.__doc__)
        self.parser.add_argument(
            "-s",
            "--structures",
            nargs="+",
            required=True,
            help="Specify one or more structure names to process (e.g., 'service_message_callback').",
        )

    def invoke(self, args, from_tty):
        argv = gdb.string_to_argv(args)
        try:
            parsed_args = self.parser.parse_args(argv)
        except SystemExit:
            return

        for structure in parsed_args.structures:
            timeval_var_name = f"g_timeval_{structure}"
            self.process_timeval(timeval_var_name, structure)
            gdb.write("\n")

    def process_timeval(self, timeval_var_name, structure_name):

        timeval_var = gdb.parse_and_eval(timeval_var_name)

        if not timeval_var:
            gdb.write(f"{structure_name} is NULL or not available.\n")
            return

        use_microseconds = (
            utils.get_symbol_value("CONFIG_BLUETOOTH_DEBUG_TIME_UNIT_US") or 0
        )

        gdb.write(f"========= {structure_name} Timeval Information =========\n")
        if use_microseconds:
            gdb.write(
                f"max_timeval: {timeval_var['max_timeval']} us, pre_timeval: {timeval_var['last_timeval']} us\n"
            )
            gdb.write(
                f"entrystamp: {timeval_var['entry_time']} us, exitstamp: {timeval_var['exit_time']} us\n"
            )
        else:
            gdb.write(
                f"max_timeval: {timeval_var['max_timeval']} ms, pre_timeval: {timeval_var['last_timeval']} ms\n"
            )
            gdb.write(
                f"entrystamp: {timeval_var['entry_time']} ms, exitstamp: {timeval_var['exit_time']} ms\n"
            )
        gdb.write("--------- End of Timeval Information ---------\n")


# Register the command
BTTImevalCommand()
