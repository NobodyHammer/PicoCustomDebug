"""High-level entry point: Probe sends commands to the probe.

Config settings are properties. Each application function will get its own method here.
"""

from __future__ import annotations

import threading
from dataclasses import dataclass
from typing import Iterable, Optional, Protocol, Union

from . import protocol
from .commands import (ConfigCommand, DebugProtocol, GpioCommand, GpioPin, GpioPinMask, JtagCommand,
                       Module, SwdCommand, SystemCommand, TimerCommand, TriggerPosition, command_key)
from .errors import CommandError, ProtocolError
from .values import Value, to_bytes


@dataclass(frozen=True)
class SystemInfo:
    name: str
    board_version: str
    firmware_version: str


def _version(buf: bytes) -> str:
    if len(buf) != 3:
        raise ProtocolError("version field must be 3 bytes")
    return "%d.%d.%d" % tuple(buf)


def _u32(value: Value) -> bytes:
    return to_bytes(value, 4)


def _u8(value: Value) -> bytes:
    return to_bytes(value, 1)


class Transport(Protocol):
    """What Probe needs from a transport: one request/reply exchange, and an optional close."""

    def exchange(self, frame: bytes) -> bytes: ...

    def close(self) -> None: ...


class Probe:
    """Access to the probe."""

    def __init__(self, transport: Transport) -> None:
        self._transport = transport
        # One command in flight at a time: the probe answers each command before the next one is sent
        self._lock = threading.Lock()

    @classmethod
    def connect(cls, port: Optional[str] = None) -> "Probe":
        """Opens the probe. If port is None, the single DebugProbe on USB is found automatically."""
        from .transport import SerialTransport, find_port

        return cls(SerialTransport(port or find_port()))

    @property
    def debug_protocol(self) -> DebugProtocol:
        """Debug protocol in use. Setting it reconfigures the GPIOs of the probe."""
        return DebugProtocol(self._get_value(ConfigCommand.GET_DEBUG_PROTOCOL))

    @debug_protocol.setter
    def debug_protocol(self, value: DebugProtocol) -> None:
        self._set_value(ConfigCommand.SET_DEBUG_PROTOCOL, int(value), 1)

    # ---- System ----

    def system_info(self) -> SystemInfo:
        """Name, board version and firmware version of the probe."""
        resp = self.request(Module.SYSTEM, SystemCommand.INFO)
        if len(resp.data) != 3:
            raise ProtocolError("system info reply must carry 3 data entries")
        return SystemInfo(
            name=resp.data[0].buf.decode("ascii", errors="replace"),
            board_version=_version(resp.data[1].buf),
            firmware_version=_version(resp.data[2].buf),
        )

    # ---- Config properties ----

    @property
    def trigger_width_us(self) -> int:
        return self._get_value(ConfigCommand.GET_TRIGGER_WIDTH_US)

    @trigger_width_us.setter
    def trigger_width_us(self, value: int) -> None:
        self._set_value(ConfigCommand.SET_TRIGGER_WIDTH_US, value, 4)

    @property
    def debug_ap(self) -> int:
        return self._get_value(ConfigCommand.GET_DEBUG_AP)

    @debug_ap.setter
    def debug_ap(self, value: int) -> None:
        self._set_value(ConfigCommand.SET_DEBUG_AP, value, 1)

    @property
    def swd_freq_hz(self) -> int:
        return self._get_value(ConfigCommand.GET_SWD_FREQ_HZ)

    @swd_freq_hz.setter
    def swd_freq_hz(self, value: int) -> None:
        self._set_value(ConfigCommand.SET_SWD_FREQ_HZ, value, 4)

    @property
    def jtag_freq_hz(self) -> int:
        return self._get_value(ConfigCommand.GET_JTAG_FREQ_HZ)

    @jtag_freq_hz.setter
    def jtag_freq_hz(self, value: int) -> None:
        self._set_value(ConfigCommand.SET_JTAG_FREQ_HZ, value, 4)

    @property
    def gpio_pulse_width_us(self) -> int:
        """Time the pulsed GPIOs stay low, in microseconds."""
        return self._get_value(ConfigCommand.GET_GPIO_PULSE_WIDTH_US)

    @gpio_pulse_width_us.setter
    def gpio_pulse_width_us(self, value: int) -> None:
        self._set_value(ConfigCommand.SET_GPIO_PULSE_WIDTH_US, value, 4)

    @property
    def gpio_pulse_after_us(self) -> int:
        """Wait after the pulsed GPIOs go back high, in microseconds."""
        return self._get_value(ConfigCommand.GET_GPIO_PULSE_AFTER_US)

    @gpio_pulse_after_us.setter
    def gpio_pulse_after_us(self, value: int) -> None:
        self._set_value(ConfigCommand.SET_GPIO_PULSE_AFTER_US, value, 4)

    @property
    def gpio_pulse_last_us(self) -> int:
        """Last wait of a GPIO pulse, before the closing trigger, in microseconds."""
        return self._get_value(ConfigCommand.GET_GPIO_PULSE_LAST_US)

    @gpio_pulse_last_us.setter
    def gpio_pulse_last_us(self, value: int) -> None:
        self._set_value(ConfigCommand.SET_GPIO_PULSE_LAST_US, value, 4)

    def _get_value(self, command: ConfigCommand) -> int:
        resp = self.request(Module.CONFIG, command)
        if not resp.data:
            raise ProtocolError("config reply carries no value")
        return int.from_bytes(resp.data[0].buf, "big")

    def _set_value(self, command: ConfigCommand, value: int, size: int) -> None:
        self.request(Module.CONFIG, command, data=[value.to_bytes(size, "big")])

    # ---- SWD (module 0x1) ----

    def swd_connect(self, trigger: int = TriggerPosition.NONE) -> int:
        """Connects to the target over SWD and selects the debug AP. Returns the IDCODE."""
        return self._value(Module.SWD, SwdCommand.CONNECT, trigger)

    def swd_read(self, address: Value, trigger: int = TriggerPosition.NONE) -> int:
        """Reads a 32-bit word of the target through the MEM-AP."""
        return self._value(Module.SWD, SwdCommand.READ, trigger, [_u32(address)])

    def swd_write(self, address: Value, value: Value, trigger: int = TriggerPosition.NONE) -> None:
        """Writes a 32-bit word of the target through the MEM-AP."""
        self.request(Module.SWD, SwdCommand.WRITE, trigger, [_u32(address), _u32(value)])

    # ---- JTAG (module 0x2) ----

    def jtag_connect(self, trigger: int = TriggerPosition.NONE) -> int:
        """Resets the TAP, reads the IDCODE and selects the debug AP. Returns the IDCODE."""
        return self._value(Module.JTAG, JtagCommand.CONNECT, trigger)

    def jtag_read(self, address: Value, trigger: int = TriggerPosition.NONE) -> int:
        """Reads a 32-bit word of the target through the MEM-AP."""
        return self._value(Module.JTAG, JtagCommand.READ, trigger, [_u32(address)])

    def jtag_write(self, address: Value, value: Value, trigger: int = TriggerPosition.NONE) -> None:
        """Writes a 32-bit word of the target through the MEM-AP."""
        self.request(Module.JTAG, JtagCommand.WRITE, trigger, [_u32(address), _u32(value)])

    # ---- GPIO (module 0x3) ----

    def gpio_set(self, pin: Union[GpioPin, int], trigger: int = TriggerPosition.NONE) -> None:
        self.request(Module.GPIO, GpioCommand.SET, trigger, [_u8(pin)])

    def gpio_reset(self, pin: Union[GpioPin, int], trigger: int = TriggerPosition.NONE) -> None:
        self.request(Module.GPIO, GpioCommand.RESET, trigger, [_u8(pin)])

    def gpio_toggle(self, pin: Union[GpioPin, int], trigger: int = TriggerPosition.NONE) -> None:
        self.request(Module.GPIO, GpioCommand.TOGGLE, trigger, [_u8(pin)])

    def gpio_pulse(self, pins: Union[GpioPinMask, int], trigger: int = TriggerPosition.NONE,
                   last_delay: bool = False) -> None:
        """Pulls the given pins low together, waits gpio_pulse_width_us, releases them high,
        then waits gpio_pulse_after_us.

        The trigger 'before' pulses before the falling edge, the trigger 'after' pulses after the
        after-delay. With last_delay=True, the 'after' trigger pulses just before the gpio_pulse_last_us
        delay, which then runs. With last_delay=False, that delay is not taken into account."""
        mask = int(pins)
        if not 0x01 <= mask <= 0x1F:
            raise ValueError("pins must be a non-empty combination of GpioPinMask")
        self.request(Module.GPIO, GpioCommand.PULSE, trigger, p1=mask, p2=1 if last_delay else 0)

    # ---- Timer (module 0x4) ----

    def delay_us(self, microseconds: int, trigger: int = TriggerPosition.NONE) -> None:
        self.request(Module.TIMER, TimerCommand.DELAY_US, trigger, [_u32(microseconds)])

    def delay_ms(self, milliseconds: int, trigger: int = TriggerPosition.NONE) -> None:
        self.request(Module.TIMER, TimerCommand.DELAY_MS, trigger, [_u32(milliseconds)])

    def _value(self, module: Union[Module, int], command: int, trigger: int,
               data: Iterable[bytes] = ()) -> int:
        resp = self.request(module, command, trigger, data)
        if not resp.data:
            raise ProtocolError("reply carries no value")
        return int.from_bytes(resp.data[0].buf, "big")

    # ---- Generic access ----

    def request(self, module: Union[Module, int], command: int, trigger: int = 0,
                data: Iterable[bytes] = (), p1: int = 0, p2: int = 0) -> protocol.Output:
        """Sends one command and returns the reply.

        Blocks until the reply arrives. Concurrent calls are serialized.
        Raises CommandError if the probe reports a non-OK status,
        ProtocolError if the reply is malformed, and TransportTimeout if there is no reply.
        """
        req = protocol.Input(
            cmd=command_key(module, command),
            p1=p1,
            p2=p2,
            trigger=trigger,
            data=[protocol.Data(bytes(d)) for d in data],
        )
        frame = protocol.encode_input(req)
        with self._lock:
            reply = self._transport.exchange(frame)

        try:
            resp = protocol.decode_output(reply)
        except ValueError as exc:
            raise ProtocolError(str(exc)) from exc

        if resp.status != protocol.Status.OK:
            raise CommandError(resp.status, resp.data)
        return resp

    def close(self) -> None:
        close = getattr(self._transport, "close", None)
        if close is not None:
            close()
