import re
import unittest
from pathlib import Path

from PycoDebug import Module, Probe, ProtocolError, TransportTimeout, command_key
from PycoDebug.commands import (ConfigCommand, DebugProtocol, GpioCommand, GpioPin, GpioPinMask,
                                 JtagCommand, SwdCommand, SystemCommand, TimerCommand, TriggerPosition)
from PycoDebug import protocol
from PycoDebug.errors import CommandError, ProbeError
from PycoDebug.protocol import Data, Output, Status

CMD_H = Path(__file__).resolve().parents[2] / "generic" / "cmd" / "include" / "generic" / "cmd.h"


class FakeDevice:
    """Stands in for the firmware: decodes each request and answers with handler()."""

    def __init__(self, handler):
        self.handler = handler
        self.requests = []

    def exchange(self, frame):
        req = protocol.decode_input(frame)
        self.requests.append(req)
        return protocol.encode_output(self.handler(req))


class RequestPath(unittest.TestCase):
    def test_builds_command_byte_trigger_and_data(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        Probe(device).request(Module.SWD, 0x3, trigger=1, data=[b"\x01\x02"])
        req = device.requests[0]
        self.assertEqual(req.cmd, 0x0103)
        self.assertEqual(req.p1, 0)
        self.assertEqual(req.trigger, 1)
        self.assertEqual([d.buf for d in req.data], [b"\x01\x02"])

    def test_p1_is_sent_as_given(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        Probe(device).request(Module.SWD, 0x3, p1=2)
        self.assertEqual(device.requests[0].p1, 2)

    def test_returns_reply_data(self):
        device = FakeDevice(lambda req: Output(Status.OK, data=[Data(b"\x2A")]))
        resp = Probe(device).request(Module.CONFIG, 0x0)
        self.assertEqual(resp.data[0].buf, b"\x2A")

    def test_non_ok_status_raises_command_error(self):
        device = FakeDevice(lambda req: Output(Status.BAD_COMMAND))
        with self.assertRaises(CommandError) as ctx:
            Probe(device).request(Module.JTAG, 0x1)
        self.assertEqual(ctx.exception.status, Status.BAD_COMMAND)
        self.assertIn("BAD_COMMAND", str(ctx.exception))

    def test_malformed_reply_raises_protocol_error(self):
        class Garbage:
            def exchange(self, frame):
                return bytes([protocol.SYNC, 0x01, 0x47, 0x00, 0x00, 0x00])

        with self.assertRaises(ProtocolError):
            Probe(Garbage()).request(Module.SWD, 0x0)

    def test_timeout_is_a_probe_error(self):
        class Silent:
            def exchange(self, frame):
                raise TransportTimeout("no reply from probe")

        with self.assertRaises(TransportTimeout):
            Probe(Silent()).request(Module.SWD, 0x0)
        self.assertTrue(issubclass(TransportTimeout, ProbeError))


class Addressing(unittest.TestCase):
    def test_module_zero_is_system(self):
        self.assertEqual(command_key(Module.SYSTEM, SystemCommand.INFO), 0x01)

    def test_command_must_fit_a_byte(self):
        with self.assertRaises(ValueError):
            Probe(FakeDevice(lambda req: Output(Status.OK))).request(Module.SWD, 256)

    def test_modules_match_cmd_h(self):
        text = CMD_H.read_text()
        header = {
            name: int(value, 16)
            for name, value in re.findall(r"#define CMD_MODULE_(\w+)\s+(0x[0-9A-Fa-f]+)u", text)
        }
        for module in Module:
            self.assertEqual(header[module.name], int(module))


class ConfigProperties(unittest.TestCase):
    def test_setter_sends_big_endian_value(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        probe = Probe(device)
        probe.trigger_width_us = 0x01020304
        req = device.requests[0]
        self.assertEqual(req.cmd, command_key(Module.CONFIG, ConfigCommand.SET_TRIGGER_WIDTH_US))
        self.assertEqual(req.data[0].buf, b"\x01\x02\x03\x04")

    def test_debug_ap_setter_sends_one_byte(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        Probe(device).debug_ap = 2
        self.assertEqual(device.requests[0].data[0].buf, b"\x02")

    def test_getter_parses_reply(self):
        device = FakeDevice(lambda req: Output(Status.OK, data=[Data(b"\x00\x0F\x42\x40")]))
        self.assertEqual(Probe(device).swd_freq_hz, 1000000)
        self.assertEqual(device.requests[0].cmd, command_key(Module.CONFIG, ConfigCommand.GET_SWD_FREQ_HZ))

    def test_getter_without_value_raises(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        with self.assertRaises(ProtocolError):
            _ = Probe(device).jtag_freq_hz

    def test_non_ok_status_raises(self):
        device = FakeDevice(lambda req: Output(Status.FAIL))
        with self.assertRaises(CommandError):
            Probe(device).debug_ap = 1


class SystemInfoTests(unittest.TestCase):
    def test_system_info_parses_reply(self):
        device = FakeDevice(lambda req: Output(Status.OK, data=[
            Data(b"DBGP"), Data(bytes([1, 0, 0])), Data(bytes([0, 1, 0])),
        ]))
        info = Probe(device).system_info()
        self.assertEqual(info.name, "DBGP")
        self.assertEqual(info.board_version, "1.0.0")
        self.assertEqual(info.firmware_version, "0.1.0")
        self.assertEqual(device.requests[0].cmd, 0x01)

    def test_system_info_rejects_missing_entries(self):
        device = FakeDevice(lambda req: Output(Status.OK, data=[Data(b"DBGP")]))
        with self.assertRaises(ProtocolError):
            Probe(device).system_info()


class FindPort(unittest.TestCase):
    def ports(self, *entries):
        from types import SimpleNamespace
        return [SimpleNamespace(device=d, vid=v, pid=p) for d, v, p in entries]

    def test_finds_the_probe(self):
        from PycoDebug.transport import find_port
        ports = self.ports(("/dev/ttyS0", None, None), ("/dev/ttyACM0", 0x2E8A, 0x0009))
        self.assertEqual(find_port(ports=ports), "/dev/ttyACM0")

    def test_no_probe(self):
        from PycoDebug.transport import find_port
        with self.assertRaises(ProbeError):
            find_port(ports=self.ports(("/dev/ttyS0", None, None)))

    def test_several_probes_are_ambiguous(self):
        from PycoDebug.transport import find_port
        ports = self.ports(("/dev/ttyACM0", 0x2E8A, 0x0009), ("/dev/ttyACM1", 0x2E8A, 0x0009))
        with self.assertRaises(ProbeError):
            find_port(ports=ports)


class FunctionMethods(unittest.TestCase):
    def test_swd_read_sends_address_and_returns_value(self):
        device = FakeDevice(lambda req: Output(Status.OK, data=[Data(b"\xDE\xAD\xBE\xEF")]))
        value = Probe(device).swd_read(0x20000000, trigger=TriggerPosition.BOTH)
        req = device.requests[0]
        self.assertEqual(req.cmd, command_key(Module.SWD, SwdCommand.READ))
        self.assertEqual(req.trigger, 0x3)
        self.assertEqual(req.data[0].buf, b"\x20\x00\x00\x00")
        self.assertEqual(value, 0xDEADBEEF)

    def test_swd_write_sends_address_and_value(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        Probe(device).swd_write(0x08, 0x01020304, trigger=TriggerPosition.BEFORE)
        req = device.requests[0]
        self.assertEqual(req.cmd, command_key(Module.SWD, SwdCommand.WRITE))
        self.assertEqual([d.buf for d in req.data], [b"\x00\x00\x00\x08", b"\x01\x02\x03\x04"])

    def test_jtag_connect_returns_idcode(self):
        device = FakeDevice(lambda req: Output(Status.OK, data=[Data(b"\x4B\xA0\x04\x77")]))
        self.assertEqual(Probe(device).jtag_connect(), 0x4BA00477)
        self.assertEqual(device.requests[0].cmd, command_key(Module.JTAG, JtagCommand.CONNECT))

    def test_gpio_toggle_sends_pin_selector(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        Probe(device).gpio_toggle(GpioPin.EXT2)
        req = device.requests[0]
        self.assertEqual(req.cmd, command_key(Module.GPIO, GpioCommand.TOGGLE))
        self.assertEqual(req.data[0].buf, b"\x03")

    def test_gpio_pulse_sends_pin_mask_in_p1(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        Probe(device).gpio_pulse(GpioPinMask.RST | GpioPinMask.TRST, trigger=TriggerPosition.BEFORE)
        req = device.requests[0]
        self.assertEqual(req.cmd, command_key(Module.GPIO, GpioCommand.PULSE))
        self.assertEqual(req.p1, 0x11)
        self.assertEqual(req.trigger, TriggerPosition.BEFORE)
        self.assertEqual(req.data, [])

    def test_gpio_pulse_sends_last_delay_flag_in_p2(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        probe = Probe(device)
        probe.gpio_pulse(GpioPinMask.EXT0)
        probe.gpio_pulse(GpioPinMask.EXT0, last_delay=True)
        self.assertEqual(device.requests[0].p2, 0)
        self.assertEqual(device.requests[1].p2, 1)

    def test_gpio_pulse_rejects_empty_or_unknown_mask(self):
        probe = Probe(FakeDevice(lambda req: Output(Status.OK)))
        with self.assertRaises(ValueError):
            probe.gpio_pulse(0)
        with self.assertRaises(ValueError):
            probe.gpio_pulse(0x20)

    def test_gpio_pulse_timing_settings_are_big_endian(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        probe = Probe(device)
        probe.gpio_pulse_width_us = 0x0102
        probe.gpio_pulse_after_us = 0x0304
        probe.gpio_pulse_last_us = 0x0506
        self.assertEqual(device.requests[0].cmd, command_key(Module.CONFIG, ConfigCommand.SET_GPIO_PULSE_WIDTH_US))
        self.assertEqual(device.requests[0].data[0].buf, b"\x00\x00\x01\x02")
        self.assertEqual(device.requests[1].cmd, command_key(Module.CONFIG, ConfigCommand.SET_GPIO_PULSE_AFTER_US))
        self.assertEqual(device.requests[1].data[0].buf, b"\x00\x00\x03\x04")
        self.assertEqual(device.requests[2].cmd, command_key(Module.CONFIG, ConfigCommand.SET_GPIO_PULSE_LAST_US))
        self.assertEqual(device.requests[2].data[0].buf, b"\x00\x00\x05\x06")

    def test_delays_send_big_endian_values(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        probe = Probe(device)
        probe.delay_us(500, trigger=TriggerPosition.AFTER)
        probe.delay_ms(2)
        self.assertEqual(device.requests[0].cmd, command_key(Module.TIMER, TimerCommand.DELAY_US))
        self.assertEqual(device.requests[0].data[0].buf, b"\x00\x00\x01\xF4")
        self.assertEqual(device.requests[0].trigger, 0x2)
        self.assertEqual(device.requests[1].cmd, command_key(Module.TIMER, TimerCommand.DELAY_MS))

    def test_out_of_range_address_is_rejected(self):
        with self.assertRaises(ValueError):
            Probe(FakeDevice(lambda req: Output(Status.OK))).swd_read(1 << 32)

    def test_command_error_propagates(self):
        device = FakeDevice(lambda req: Output(Status.FAIL))
        with self.assertRaises(CommandError):
            Probe(device).swd_connect()


class ErrorDetails(unittest.TestCase):
    def test_failure_data_is_kept_in_the_error(self):
        device = FakeDevice(lambda req: Output(Status.FAIL, data=[Data(bytes([1, 0xFF]))]))
        with self.assertRaises(CommandError) as ctx:
            Probe(device).swd_connect()
        self.assertEqual(ctx.exception.data, [bytes([1, 0xFF])])
        self.assertIn("01ff", str(ctx.exception))


class DebugProtocolProperty(unittest.TestCase):
    def test_default_read_back(self):
        device = FakeDevice(lambda req: Output(Status.OK, data=[Data(b"\x00")]))
        self.assertEqual(Probe(device).debug_protocol, DebugProtocol.SWD)

    def test_setter_sends_one_byte(self):
        device = FakeDevice(lambda req: Output(Status.OK))
        Probe(device).debug_protocol = DebugProtocol.JTAG
        req = device.requests[0]
        self.assertEqual(req.cmd, command_key(Module.CONFIG, ConfigCommand.SET_DEBUG_PROTOCOL))
        self.assertEqual(req.data[0].buf, b"\x01")


if __name__ == "__main__":
    unittest.main()
