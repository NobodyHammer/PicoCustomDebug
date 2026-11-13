"""Connects to the target over SWD and prints its IDCODE.

Usage (from the python/ folder):
    python3 examples/swd_idcode.py [--port /dev/ttyACM0]

Without --port, the single DebugProbe on USB is found automatically.
"""

import argparse
import sys
from pathlib import Path

# Allows running the script from any folder without installing the package
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import GpioPin, Probe, ProbeError, SerialTransport, TriggerPosition  # noqa: E402


def decode_idcode(idcode: int) -> str:
    """Splits an ARM DP IDCODE into its fields (ADIv5)."""
    version = (idcode >> 28) & 0xF
    part = (idcode >> 12) & 0xFFFF
    designer = (idcode >> 1) & 0x7FF
    return "version=%d part=0x%04X designer=0x%03X" % (version, part, designer)


def main() -> int:
    parser = argparse.ArgumentParser(description="Read the SWD IDCODE of the target")
    parser.add_argument("--port", help="serial port of the probe (default: auto-detect)")
    parser.add_argument("--trigger", action="store_true",
                        help="pulse the trigger before the connection")
    parser.add_argument("--connect-under-reset", action="store_true",
                        help="hold RST low during the connection (target held in reset)")
    parser.add_argument("--reset", action="store_true",
                        help="assert reset (RST low) for 10 ms, release it (RST high), then wait 50 ms before connecting")
    args = parser.parse_args()

    trigger = TriggerPosition.BEFORE if args.trigger else TriggerPosition.NONE

    try:
        transport = SerialTransport(args.port) if args.port else None
        probe = Probe(transport) if transport else Probe.connect()
        if args.reset:
            # RST is assumed wired to the active-low NRST of the target:
            # low asserts the reset, high releases it before connecting
            probe.gpio_reset(GpioPin.RST)
            probe.delay_ms(10)
            probe.gpio_set(GpioPin.RST)
            probe.delay_ms(50)
        try:
            if args.connect_under_reset:
                probe.gpio_reset(GpioPin.RST)
                probe.delay_ms(10)
            idcode = probe.swd_connect(trigger=trigger)
        finally:
            # Always release RST, even when the connection fails
            probe.gpio_set(GpioPin.RST)
            probe.close()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1

    print("IDCODE: 0x%08X (%s)" % (idcode, decode_idcode(idcode)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
