"""Command-line access to the probe, for use from a terminal.

Run from the python/ folder, for example:
    python3 examples/probe_cli.py info
    python3 examples/probe_cli.py swd-connect --trigger before
    python3 examples/probe_cli.py swd-read 0x20000000
    python3 examples/probe_cli.py gpio toggle ext0
    python3 examples/probe_cli.py swd-freq 100000
    python3 examples/probe_cli.py protocol swd
    python3 examples/probe_cli.py delay-ms 10
"""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from PycoDebug import DebugProtocol, GpioPin, Probe, ProbeError, SerialTransport, TriggerPosition  # noqa: E402


def parse_int(text: str) -> int:
    return int(text, 0)  # accepts decimal and 0x... hexadecimal


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="DebugProbe command-line tool")
    parser.add_argument("--port", help="serial port (default: auto-detect)")
    sub = parser.add_subparsers(dest="command", required=True)

    trigger_help = ("trigger position: none, before, after, both, address_before, address_after, "
                    "value_before, value_after (address/value: read and write only)")

    sub.add_parser("info", help="system information")

    p = sub.add_parser("swd-connect", help="connect to the target over SWD, print IDCODE")
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("swd-read", help="read a 32-bit word over SWD")
    p.add_argument("address", type=parse_int)
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("swd-write", help="write a 32-bit word over SWD")
    p.add_argument("address", type=parse_int)
    p.add_argument("value", type=parse_int)
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("jtag-connect", help="connect to the target over JTAG, print IDCODE")
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("jtag-read", help="read a 32-bit word over JTAG")
    p.add_argument("address", type=parse_int)
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("jtag-write", help="write a 32-bit word over JTAG")
    p.add_argument("address", type=parse_int)
    p.add_argument("value", type=parse_int)
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("gpio", help="drive a GPIO")
    p.add_argument("action", choices=["set", "reset", "toggle"])
    p.add_argument("pin", choices=[pin.name.lower() for pin in GpioPin])
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("delay-us", help="wait in microseconds")
    p.add_argument("value", type=parse_int)
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("delay-ms", help="wait in milliseconds")
    p.add_argument("value", type=parse_int)
    p.add_argument("--trigger", default="none", help=trigger_help)

    p = sub.add_parser("swd-freq", help="set (or show) the SWD clock frequency in Hz")
    p.add_argument("value", type=parse_int, nargs="?")

    p = sub.add_parser("jtag-freq", help="set (or show) the JTAG clock frequency in Hz")
    p.add_argument("value", type=parse_int, nargs="?")

    p = sub.add_parser("protocol", help="show (or set) the debug protocol")
    p.add_argument("value", choices=["swd", "jtag"], nargs="?")

    p = sub.add_parser("trigger-width", help="set (or show) the trigger pulse width in us")
    p.add_argument("value", type=parse_int, nargs="?")

    return parser


def trigger_of(name: str) -> int:
    return TriggerPosition[name.upper()]


def run(probe: Probe, args: argparse.Namespace) -> None:
    cmd = args.command
    trig = trigger_of(args.trigger) if hasattr(args, "trigger") else TriggerPosition.NONE

    if cmd == "info":
        info = probe.system_info()
        print("name=%s board=%s firmware=%s" % (info.name, info.board_version, info.firmware_version))
    elif cmd == "swd-connect":
        print("IDCODE: 0x%08X" % probe.swd_connect(trigger=trig))
    elif cmd == "swd-read":
        print("0x%08X" % probe.swd_read(args.address, trigger=trig))
    elif cmd == "swd-write":
        probe.swd_write(args.address, args.value, trigger=trig)
        print("ok")
    elif cmd == "jtag-connect":
        print("IDCODE: 0x%08X" % probe.jtag_connect(trigger=trig))
    elif cmd == "jtag-read":
        print("0x%08X" % probe.jtag_read(args.address, trigger=trig))
    elif cmd == "jtag-write":
        probe.jtag_write(args.address, args.value, trigger=trig)
        print("ok")
    elif cmd == "gpio":
        pin = GpioPin[args.pin.upper()]
        {"set": probe.gpio_set, "reset": probe.gpio_reset, "toggle": probe.gpio_toggle}[args.action](pin, trigger=trig)
        print("ok")
    elif cmd == "delay-us":
        probe.delay_us(args.value, trigger=trig)
        print("ok")
    elif cmd == "delay-ms":
        probe.delay_ms(args.value, trigger=trig)
        print("ok")
    elif cmd == "swd-freq":
        if args.value is None:
            print(probe.swd_freq_hz)
        else:
            probe.swd_freq_hz = args.value
            print("ok")
    elif cmd == "jtag-freq":
        if args.value is None:
            print(probe.jtag_freq_hz)
        else:
            probe.jtag_freq_hz = args.value
            print("ok")
    elif cmd == "protocol":
        if args.value is None:
            print(probe.debug_protocol.name.lower())
        else:
            probe.debug_protocol = DebugProtocol[args.value.upper()]
            print("ok")
    elif cmd == "trigger-width":
        if args.value is None:
            print(probe.trigger_width_us)
        else:
            probe.trigger_width_us = args.value
            print("ok")


def main() -> int:
    args = build_parser().parse_args()
    try:
        probe = Probe(SerialTransport(args.port)) if args.port else Probe.connect()
        try:
            run(probe, args)
        finally:
            probe.close()
    except ProbeError as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 1
    except (ValueError, KeyError) as exc:
        print("error: %s" % exc, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
