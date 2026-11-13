"""Sends 20 trigger pulses, one every 100 ms, to check the trigger output on the scope.

Usage (from the python/ folder):
    python3 examples/trigger_pulses.py
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import Probe, ProbeError, TriggerPosition  # noqa: E402


def main() -> int:
    try:
        probe = Probe.connect()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    try:
        for _ in range(20):
            probe.delay_us(10, trigger=TriggerPosition.BEFORE)
            probe.delay_ms(100)
        print("20 trigger pulses sent (width %d us)" % probe.trigger_width_us)
    finally:
        probe.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
