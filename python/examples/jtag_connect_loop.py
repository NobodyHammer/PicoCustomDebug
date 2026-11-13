"""Repeats the JTAG connection sequence in a loop, to debug it with an oscilloscope.

Each iteration: select JTAG, connect (trigger before the connection), wait, repeat.
Stop with Ctrl+C. The protocol is left on JTAG at the end.

Usage (from the python/ folder):
    python3 examples/jtag_connect_loop.py [--port /dev/ttyACM0] [--count 10] [--delay-ms 500]

--count 0 (default) loops until Ctrl+C.
"""

import argparse
import sys
import time
from pathlib import Path

# Allows running the script from any folder without installing the package
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import DebugProtocol, Probe, ProbeError, SerialTransport, TriggerPosition  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description="Loop the JTAG connection for scope debugging")
    parser.add_argument("--port", help="serial port of the probe (default: auto-detect)")
    parser.add_argument("--count", type=int, default=0,
                        help="number of connections to attempt (0 = until Ctrl+C)")
    parser.add_argument("--delay-ms", type=int, default=500,
                        help="pause between two connections, in ms (default: 500)")
    args = parser.parse_args()

    try:
        transport = SerialTransport(args.port) if args.port else None
        probe = Probe(transport) if transport else Probe.connect()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1

    try:
        probe.debug_protocol = DebugProtocol.JTAG
        iteration = 0
        while args.count == 0 or iteration < args.count:
            iteration += 1
            try:
                idcode = probe.jtag_connect(trigger=TriggerPosition.BEFORE)
                result = "IDCODE 0x%08X" % idcode
            except ProbeError as exc:
                result = "FAIL: %s" % exc
            print("[%4d] %s  %s" % (iteration, time.strftime("%H:%M:%S"), result))
            time.sleep(args.delay_ms / 1000.0)
    except KeyboardInterrupt:
        print("\nstopped after %d attempt(s)" % iteration)
    finally:
        probe.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
