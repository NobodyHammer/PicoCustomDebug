"""Restores the default settings: SWD and JTAG at 1 MHz, SWD protocol, 10 us trigger width.

Usage (from the python/ folder):
    python3 examples/restore_defaults.py
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import DebugProtocol, Probe, ProbeError  # noqa: E402


def main() -> int:
    try:
        probe = Probe.connect()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    try:
        probe.swd_freq_hz = 1000000
        probe.jtag_freq_hz = 1000000
        probe.debug_protocol = DebugProtocol.SWD
        probe.trigger_width_us = 10
        print("defaults restored: swd=%d Hz jtag=%d Hz protocol=%s trigger=%d us"
              % (probe.swd_freq_hz, probe.jtag_freq_hz, probe.debug_protocol.name,
                 probe.trigger_width_us))
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    finally:
        probe.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
