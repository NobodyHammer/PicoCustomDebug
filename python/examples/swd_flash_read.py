"""Connects over SWD and reads the first words of the target flash.

The connection is retried a few times: a parity error on the IDCODE is
reported as a failure, so a single attempt is not always enough.

Usage (from the python/ folder):
    python3 examples/swd_flash_read.py [--port /dev/ttyACM0] [--address 0x08000000]
                                       [--words 4] [--retries 10]
"""

import argparse
import sys
from pathlib import Path

# Allows running the script from any folder without installing the package
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import DebugProtocol, Probe, ProbeError, SerialTransport, TriggerPosition  # noqa: E402

FLASH_BASE = 0x08000000  # STM32 flash start address


def parse_int(text: str) -> int:
    return int(text, 0)


def main() -> int:
    parser = argparse.ArgumentParser(description="Read the first words of the target flash over SWD")
    parser.add_argument("--port", help="serial port of the probe (default: auto-detect)")
    parser.add_argument("--address", type=parse_int, default=FLASH_BASE,
                        help="first address to read (default: 0x08000000)")
    parser.add_argument("--words", type=int, default=4, help="number of 32-bit words to read")
    parser.add_argument("--retries", type=int, default=10, help="connection attempts before giving up")
    args = parser.parse_args()

    try:
        transport = SerialTransport(args.port) if args.port else None
        probe = Probe(transport) if transport else Probe.connect()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1

    try:
        probe.debug_protocol = DebugProtocol.SWD
        idcode = None
        for attempt in range(1, args.retries + 1):
            try:
                idcode = probe.swd_connect(trigger=TriggerPosition.BEFORE)
                print("connected on attempt %d, IDCODE 0x%08X" % (attempt, idcode))
                break
            except ProbeError as exc:
                print("attempt %d failed: %s" % (attempt, exc))
        if idcode is None:
            print("error: no connection after %d attempt(s)" % args.retries, file=sys.stderr)
            return 1

        for i in range(args.words):
            address = args.address + 4 * i
            value = probe.swd_read(address)
            print("0x%08X: 0x%08X" % (address, value))
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    finally:
        probe.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
