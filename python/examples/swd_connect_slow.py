"""Connects over SWD at 10 kHz, with reset and a trigger marker, for scope captures.

The SWD clock is restored to 1 MHz at the end, even if the connection fails.

Usage (from the python/ folder):
    python3 examples/swd_connect_slow.py
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import GpioPin, Probe, ProbeError, TriggerPosition  # noqa: E402

SLOW_HZ = 10000
NORMAL_HZ = 1000000


def main() -> int:
    try:
        probe = Probe.connect()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    try:
        probe.swd_freq_hz = SLOW_HZ
        probe.gpio_reset(GpioPin.RST)
        probe.delay_ms(10)
        probe.gpio_set(GpioPin.RST)
        probe.delay_ms(50)
        idcode = probe.swd_connect(trigger=TriggerPosition.BEFORE)
        print("IDCODE: 0x%08X" % idcode)
        return 0
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    finally:
        probe.swd_freq_hz = NORMAL_HZ
        probe.close()


if __name__ == "__main__":
    sys.exit(main())
