"""Pulses GPIO pins with gpio_pulse, to check the sequence on the scope.

Sequence sent to the probe: trigger (if --trigger has BEFORE), pins low for --width-us,
pins high, --after-us, trigger (if --trigger has AFTER), then --last-us only with --last-delay.

Usage (from the python/ folder):
    python3 examples/gpio_pulse.py --pins RST TRST --width-us 100 --after-us 500 \
        --last-us 300 --last-delay --trigger after --count 5

--count 0 (default) sends one pulse. The probe settings are written before the pulses.
"""

import argparse
import sys
import time
from pathlib import Path

# Allows running the script from any folder without installing the package
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import GpioPinMask, Probe, ProbeError, SerialTransport, TriggerPosition  # noqa: E402

TRIGGERS = {
    "none": TriggerPosition.NONE,
    "before": TriggerPosition.BEFORE,
    "after": TriggerPosition.AFTER,
    "both": TriggerPosition.BOTH,
}


def main() -> int:
    parser = argparse.ArgumentParser(description="Send GPIO pulses with a configurable sequence")
    parser.add_argument("--port", help="serial port of the probe (default: auto-detect)")
    parser.add_argument("--pins", nargs="+", default=["RST"], choices=[pin.name for pin in GpioPinMask],
                        help="pins pulsed together (default: RST)")
    parser.add_argument("--width-us", type=int, default=10, help="time the pins stay low, in us (default: 10)")
    parser.add_argument("--after-us", type=int, default=10, help="wait after the pins go high, in us (default: 10)")
    parser.add_argument("--last-us", type=int, default=10, help="last wait, used with --last-delay, in us (default: 10)")
    parser.add_argument("--last-delay", action="store_true",
                        help="run the last wait, with the 'after' trigger just before it")
    parser.add_argument("--trigger", choices=sorted(TRIGGERS), default="none",
                        help="trigger pulses: before (falling edge), after (see --last-delay), both, none")
    parser.add_argument("--count", type=int, default=1, help="number of pulses (default: 1, 0 = until Ctrl+C)")
    parser.add_argument("--interval-ms", type=int, default=100, help="pause between pulses, in ms (default: 100)")
    args = parser.parse_args()

    mask = GpioPinMask(0)
    for name in args.pins:
        mask |= GpioPinMask[name]

    try:
        transport = SerialTransport(args.port) if args.port else None
        probe = Probe(transport) if transport else Probe.connect()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1

    pulses = 0
    try:
        probe.gpio_pulse_width_us = args.width_us
        probe.gpio_pulse_after_us = args.after_us
        probe.gpio_pulse_last_us = args.last_us
        print("pins=%s width=%d us after=%d us last=%d us (%s) trigger=%s" % (
            "+".join(args.pins), args.width_us, args.after_us, args.last_us,
            "run" if args.last_delay else "not run", args.trigger))

        while args.count == 0 or pulses < args.count:
            probe.gpio_pulse(mask, trigger=TRIGGERS[args.trigger], last_delay=args.last_delay)
            pulses += 1
            print("[%4d] %s  pulse" % (pulses, time.strftime("%H:%M:%S")))
            if args.count == 0 or pulses < args.count:
                time.sleep(args.interval_ms / 1000)
    except KeyboardInterrupt:
        print("\nstopped after %d pulse(s)" % pulses)
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    finally:
        probe.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
