############################################################################
# frameworks/bluetooth/tools/gdb/driver/btsnoop.py
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
from nuttxgdb import fs
import struct


class BTSnoopCommand(gdb.Command):
    """Command to capture Bluetooth snoop data and save it to a specified file."""

    def __init__(self):
        # Register the command in GDB with the name "btsnoop"
        super(BTSnoopCommand, self).__init__("btsnoop", gdb.COMMAND_DATA)
        self.setup_parser()

    def setup_parser(self):
        # Use argparse to parse command line arguments
        self.parser = argparse.ArgumentParser(description=self.__doc__)
        self.parser.add_argument(
            "-p",
            "--path",
            required=False,
            default="/dev/ttyBT0",
            help="Device path, e.g., /dev/ttyBT0. , /dev/ttyBLE0",
        )
        self.parser.add_argument(
            "-f",
            "--file",
            required=False,
            default="snoop_circlebuffer_default.log",
            help="Output log file path, e.g., /snoop_circlebuffer_01.log.",
        )
        self.parser.add_argument(
            "-a",
            "--all",
            action="store_true",
            help="Dump the entire buffer content.",
            required=False,
            default=False,
        )

    def invoke(self, args, from_tty):
        # Parse command line arguments
        argv = gdb.string_to_argv(args)
        try:
            parsed_args = self.parser.parse_args(argv)
        except SystemExit:
            return

        device_path = parsed_args.path
        output_path = parsed_args.file
        dump_all = parsed_args.all

        # Find inode by device path
        inode = self.get_inode_by_path(device_path)
        if inode is None:
            gdb.write("Error: Inode for path '{}' not found.\n".format(device_path))
            return

        # Get device pointer (dev)
        dev_pointer = inode["i_private"]
        uart_bth4_type = utils.lookup_type("struct uart_bth4_s").pointer()
        dev = dev_pointer.cast(uart_bth4_type)

        if dev is None:
            gdb.write("Error: Device pointer (dev) is NULL or unable to cast.\n")
            return

        circbuf = dev["circbuf"]
        self.dump_circbuf_to_snoop_file(circbuf, output_path, dump_all)

    def get_inode_by_path(self, path):
        """Helper function to get the inode based on the given device path."""
        return next((node for node, p in fs.foreach_inode() if path == p), None)

    def get_header_length(self, tlv_type):
        if tlv_type == 2:
            return utils.lookup_type("struct bt_hci_acl_hdr_s").sizeof
        elif tlv_type == 4:
            return utils.lookup_type("struct bt_hci_evt_hdr_s").sizeof
        elif tlv_type == 5:
            return utils.lookup_type("struct bt_hci_iso_hdr_s").sizeof
        return None

    def get_data_length(self, tlv_type, header):
        if tlv_type == 2:
            _, data_len = struct.unpack("<HH", header[1:5])
        elif tlv_type == 4:
            _, data_len = struct.unpack("BB", header[1:3])
        elif tlv_type == 5:
            _, data_len = struct.unpack("<HH", header[1:5])
        else:
            data_len = 0
        return data_len

    def circbuf_size(self, circbuf):
        """Return size of the circular buffer."""
        return int(circbuf["size"])

    def circbuf_used(self, circbuf):
        """Return the used bytes of the circular buffer."""
        return int(circbuf["head"]) - int(circbuf["tail"])

    def circbuf_read(self, circbuf, pos, bytes_to_read):
        """Get data from a specified position in the circular buffer without removing."""
        base = circbuf["base"]
        size = self.circbuf_size(circbuf)
        head = int(circbuf["head"])
        offset = pos % size
        len1 = min(
            bytes_to_read, size - offset
        )  # First read length (until end of buffer)
        len2 = bytes_to_read - len1  # Remaining read length (from start of buffer)

        # Read the memory in two parts if necessary (if we wrap around the buffer)
        if head > size:
            data = gdb.selected_inferior().read_memory(base + offset, len1).tobytes()
            if len2 > 0:
                print(">size")
                data += gdb.selected_inferior().read_memory(base, len2).tobytes()
        else:
            if len2 > 0:
                data = gdb.selected_inferior().read_memory(base, len2).tobytes()
                data += (
                    gdb.selected_inferior().read_memory(base + offset, len1).tobytes()
                )
            else:
                data = (
                    gdb.selected_inferior().read_memory(base + offset, len1).tobytes()
                )

        return data

    def dump_circbuf_to_snoop_file(self, circbuf, file_path, dump_all=False):
        """Dump all data from the circular buffer to a Bluetooth snoop log file."""
        base = circbuf["base"]
        size = self.circbuf_size(circbuf)
        head = int(circbuf["head"])
        tail = int(circbuf["tail"])

        if base == 0 or size == 0:
            gdb.write("Error: Circular buffer is not initialized.\n")
            return

        if not dump_all and self.circbuf_used(circbuf) <= 0:
            gdb.write("All the cache buffer has been read.\n")
            return
        elif dump_all and head == 0:
            gdb.write("All historical buffers are empty.\n")
            return

        try:
            with open(file_path, "wb") as f:
                file_header = struct.pack(">8sII", b"btsnoop\0", 1, 1002)
                f.write(file_header)
                pos = tail
                end_pos = head if not dump_all else tail + size

                while pos < end_pos:
                    data = self.circbuf_read(circbuf, pos, 1)
                    if not data or len(data) == 0:
                        # gdb.write("Error: Failed to read circular buffer at position {}.\n".format(pos))
                        pos += 1
                        continue

                    tlv_type = data[0]
                    hdr_len = self.get_header_length(tlv_type)
                    if hdr_len is None:
                        # gdb.write("Error: Unknown TLV type {} at position {}.\n".format(tlv_type, pos))
                        pos += 1
                        continue

                    header = self.circbuf_read(circbuf, pos, 1 + hdr_len)
                    if len(header) < 1 + hdr_len:
                        # gdb.write("Error: Incomplete header at position {}.\n".format(pos))
                        pos += 1
                        continue

                    data_len = self.get_data_length(tlv_type, header)
                    total_length = 1 + hdr_len + data_len
                    packet_data = self.circbuf_read(circbuf, pos, total_length)

                    if total_length > len(packet_data):
                        # gdb.write("Error: Incomplete packet at position {}.\n".format(pos))
                        # Skip to the next packet if the current packet is incomplete.
                        pos += total_length
                        continue

                    packet_header = struct.pack(
                        ">IIIIQ", total_length, total_length, 1, 0, 0
                    )
                    f.write(packet_header)
                    f.write(packet_data)

                    pos += total_length

            gdb.write(
                "Successfully dumped circular buffer to bt snoop log '{}'.\n".format(
                    file_path
                )
            )

        except Exception as e:
            gdb.write("Error: Failed to write to file '{}'. {}\n".format(file_path, e))


# Register the command
BTSnoopCommand()
