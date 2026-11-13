"""Sends NRST pulses in a loop, with a trigger before each pulse, to check the reset line on the scope.

Each iteration: trigger (before the pulse), RST low for --low-ms, RST high for --high-ms, repeat.
RST is released when the script ends, even on Ctrl+C.

Usage (from the python/ folder):
    python3 examples/nrst_pulse_loop.py [--port /dev/ttyACM0] [--count 20] [--low-ms 10] [--high-ms 100]

--count 0 (default) loops until Ctrl+C.
"""

import argparse
import sys
import time
from pathlib import Path

# Allows running the script from any folder without installing the package
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import GpioPin, Probe, ProbeError, SerialTransport, TriggerPosition  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description="Send NRST pulses in a loop, with a trigger before each")
    parser.add_argument("--port", help="serial port of the probe (default: auto-detect)")
    parser.add_argument("--count", type=int, default=0, help="number of pulses (0 = until Ctrl+C)")
    parser.add_argument("--low-ms", type=int, default=10, help="time RST stays low, in ms (default: 10)")
    parser.add_argument("--high-ms", type=int, default=100, help="time RST stays high after, in ms (default: 100)")
    args = parser.parse_args()

    try:
        transport = SerialTransport(args.port) if args.port else None
        probe = Probe(transport) if transport else Probe.connect()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1

    pulses = 0
    try:
        while args.count == 0 or pulses < args.count:
            probe.gpio_reset(GpioPin.RST, trigger=TriggerPosition.BEFORE)
            probe.delay_ms(args.low_ms)
            probe.gpio_set(GpioPin.RST)
            probe.delay_ms(args.high_ms)
            pulses += 1
            print("[%4d] %s  NRST pulse" % (pulses, time.strftime("%H:%M:%S")))
    except KeyboardInterrupt:
        print("\nstopped after %d pulse(s)" % pulses)
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    finally:
        try:
            probe.gpio_set(GpioPin.RST)
        finally:
            probe.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
