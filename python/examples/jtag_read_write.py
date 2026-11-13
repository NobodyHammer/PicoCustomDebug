"""Connects over JTAG, then reads or writes one 32-bit word of the target memory (MEM-AP).

Only JTAG is used: do not mix this script with SWD sessions on the same target
without resetting it, the target stays in the protocol it was last connected with.

Usage (from the python/ folder):
    python3 examples/jtag_read_write.py read  0x2001FF00 [--port /dev/ttyACM0]
    python3 examples/jtag_read_write.py write 0x2001FF00 0xDEADBEEF [--port /dev/ttyACM0]

With write, the word is read back afterwards and the script reports a mismatch.
"""

import argparse
import sys
from pathlib import Path

# Allows running the script from any folder without installing the package
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import DebugProtocol, Probe, ProbeError, SerialTransport, TriggerPosition  # noqa: E402


def parse_int(text: str) -> int:
    return int(text, 0)


def main() -> int:
    parser = argparse.ArgumentParser(description="Read or write one word of the target memory over JTAG")
    parser.add_argument("--port", help="serial port of the probe (default: auto-detect)")
    sub = parser.add_subparsers(dest="action", required=True)

    read = sub.add_parser("read", help="read one 32-bit word")
    read.add_argument("address", type=parse_int, help="address to read, e.g. 0x2001FF00")

    write = sub.add_parser("write", help="write one 32-bit word, then read it back")
    write.add_argument("address", type=parse_int, help="address to write, e.g. 0x2001FF00")
    write.add_argument("value", type=parse_int, help="32-bit value to write, e.g. 0xDEADBEEF")

    args = parser.parse_args()

    try:
        transport = SerialTransport(args.port) if args.port else None
        probe = Probe(transport) if transport else Probe.connect()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1

    try:
        probe.debug_protocol = DebugProtocol.JTAG
        idcode = probe.jtag_connect(trigger=TriggerPosition.BEFORE)
        print("connected over JTAG, IDCODE 0x%08X" % idcode)

        if args.action == "read":
            value = probe.jtag_read(args.address)
            print("0x%08X: 0x%08X" % (args.address, value))
        else:
            probe.jtag_write(args.address, args.value)
            readback = probe.jtag_read(args.address)
            print("0x%08X: wrote 0x%08X, read 0x%08X" % (args.address, args.value, readback))
            if readback != args.value:
                print("error: read-back mismatch", file=sys.stderr)
                return 1
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    finally:
        probe.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
