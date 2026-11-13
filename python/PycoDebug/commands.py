"""Command byte layout: high nibble selects the module, low nibble the command.

Mirrors CMD_MAKE and CMD_MODULE_* in generic/cmd/include/generic/cmd.h.
"""

from enum import IntEnum, IntFlag


class Module(IntEnum):
    SYSTEM = 0x0
    SWD = 0x1
    JTAG = 0x2
    GPIO = 0x3
    TIMER = 0x4
    SEQUENCE = 0x5
    CONFIG = 0x6


def command_key(module: int, command: int) -> int:
    """16-bit command key: module (CLA) in the high byte, command (INS) in the low byte."""
    module = int(module)
    if not 0x0 <= module <= 0xFF:
        raise ValueError("module must be in 0x00..0xFF")
    if not 0x0 <= command <= 0xFF:
        raise ValueError("command must be in 0x00..0xFF")
    return (module << 8) | command


class ConfigCommand(IntEnum):
    """Commands of the config module. Numbers are proposals, to be validated.

    Values are big-endian in the single data entry: 1 byte for debug AP, 4 bytes otherwise.
    Getters return the value in the reply's data[0].
    """
    GET_TRIGGER_WIDTH_US = 0x0
    SET_TRIGGER_WIDTH_US = 0x1
    GET_DEBUG_AP = 0x2
    SET_DEBUG_AP = 0x3
    GET_SWD_FREQ_HZ = 0x4
    SET_SWD_FREQ_HZ = 0x5
    GET_JTAG_FREQ_HZ = 0x6
    SET_JTAG_FREQ_HZ = 0x7
    GET_DEBUG_PROTOCOL = 0x8
    SET_DEBUG_PROTOCOL = 0x9
    GET_GPIO_PULSE_WIDTH_US = 0xA
    SET_GPIO_PULSE_WIDTH_US = 0xB
    GET_GPIO_PULSE_AFTER_US = 0xC
    SET_GPIO_PULSE_AFTER_US = 0xD
    GET_GPIO_PULSE_LAST_US = 0xE
    SET_GPIO_PULSE_LAST_US = 0xF


class SystemCommand(IntEnum):
    """Commands of the system module (module 0x0)."""
    INFO = 0x1


class SwdCommand(IntEnum):
    CONNECT = 0x1
    READ = 0x2
    WRITE = 0x3


class JtagCommand(IntEnum):
    CONNECT = 0x1
    READ = 0x2
    WRITE = 0x3


class GpioCommand(IntEnum):
    SET = 0x1
    RESET = 0x2
    TOGGLE = 0x3
    PULSE = 0x4   # P1 is a GpioPinMask


class GpioPin(IntEnum):
    """Pin selector of the gpio module. EXT0..EXT2 are the firmware names of the EXT pins.
    TRST is the JTAG TRST line (active low), an output only in JTAG mode."""
    RST = 0x0
    EXT0 = 0x1
    EXT1 = 0x2
    EXT2 = 0x3
    TRST = 0x5


class GpioPinMask(IntFlag):
    """Pins pulsed together by GpioCommand.PULSE, sent in P1: one bit per pin."""
    RST = 1 << 0
    EXT0 = 1 << 1
    EXT1 = 1 << 2
    EXT2 = 1 << 3
    TRST = 1 << 4


class TimerCommand(IntEnum):
    DELAY_US = 0x1
    DELAY_MS = 0x2


class TriggerPosition(IntEnum):
    """Bit mask in the trigger byte of a command: where the trigger pulses."""
    NONE = 0x0
    BEFORE = 0x1
    AFTER = 0x2
    BOTH = 0x3
    ADDRESS_BEFORE = 0x04   # read/write: pulse just before the address is sent
    ADDRESS_AFTER = 0x08    # read/write: pulse just after the address is sent
    VALUE_BEFORE = 0x10     # write: pulse just before the value is sent
    VALUE_AFTER = 0x20      # write: pulse just after the value is sent


class DebugProtocol(IntEnum):
    """Debug protocol selected in the probe. Changing it reconfigures the GPIOs."""
    SWD = 0x0
    JTAG = 0x1
