# PycoDebug

Python API for the PicoDebugProbe firmware: a Raspberry Pi Pico 2 that drives SWD or JTAG
towards a target, and can trigger a scope on each command.

## Installation

From this folder:

```bash
pip install .
```

For development, install in editable mode so that code changes apply directly:

```bash
pip install -e .
```

Dependency: `pyserial` (installed with the package).

The examples in `examples/` run without installation, from this folder:

```bash
python3 examples/probe_cli.py info
```

## Opening the probe

```python
from PycoDebug import Probe

probe = Probe.connect()                  # the single DebugProbe on USB
probe = Probe.connect("/dev/ttyACM0")    # or a given serial port
...
probe.close()
```

`Probe.connect()` raises `ProbeError` when no probe is found. Always call `close()` when done,
for example in a `try ... finally`.

## Choosing the protocol

The probe drives either SWD or JTAG. Changing the protocol reconfigures the GPIOs.

```python
from PycoDebug import DebugProtocol

probe.debug_protocol = DebugProtocol.JTAG
probe.debug_protocol = DebugProtocol.SWD
```

**Warning**: the target keeps the debug mode of its last session. After an SWD session, JTAG
may not respond until the target is reset. Reset the target before a JTAG connection when it was
last used in SWD.

## Values: int or hexadecimal string

Any 32-bit value (address, data, `expected`, `mask`) can be given as an integer or as a hexadecimal
string. The `0x` prefix is optional:

```python
probe.jtag_read(0x08000000)
probe.jtag_read("0x08000000")
probe.jtag_read("08000000")     # same value
```

A value that does not fit in the field is refused with `ValueError`, not truncated. Decimal strings
are refused: `"100"` is read as hexadecimal `0x100`.

## Triggers

Each command takes a `trigger` argument, a `TriggerPosition` telling where the trigger pulses
on the scope:

| Value | Meaning |
|---|---|
| `TriggerPosition.NONE` | no pulse (default) |
| `TriggerPosition.BEFORE` | pulse before the command |
| `TriggerPosition.AFTER` | pulse after the command |
| `TriggerPosition.BOTH` | pulse before and after |
| `TriggerPosition.ADDRESS_BEFORE` | read/write: pulse just before the address is sent |
| `TriggerPosition.ADDRESS_AFTER` | read/write: pulse just after the address is sent |
| `TriggerPosition.VALUE_BEFORE` | write: pulse just before the value is sent |
| `TriggerPosition.VALUE_AFTER` | write: pulse just after the value is sent |

The address and value positions are read by the read and write commands only; the other commands ignore them. The positions can be combined with `|`, for example `TriggerPosition.ADDRESS_BEFORE | TriggerPosition.VALUE_AFTER`. `probe_cli.py` takes a single position per command.

The pulse width is set with `probe.trigger_width_us` (microseconds).

## Direct commands

Each call sends one command and waits for its reply. A failure raises an exception: nothing is
retried by the library, so a failed connection is reported on the first attempt.

**SWD**

```python
idcode = probe.swd_connect(trigger=TriggerPosition.BEFORE)   # returns the IDCODE
value = probe.swd_read(0x20000000)
probe.swd_write(0x20000000, 0xDEADBEEF)
```

**JTAG**

```python
idcode = probe.jtag_connect()        # TAP reset, IDCODE, power-up, AP select
value = probe.jtag_read("0x08000000")
probe.jtag_write(0x2001FF00, 0xDEADBEEF)
```

**GPIO**: the pins are `GpioPin.RST`, `GpioPin.EXT0` to `GpioPin.EXT2`, and `GpioPin.TRST`
(JTAG TRST, active low, an output only in JTAG mode).

```python
from PycoDebug import GpioPin

probe.gpio_reset(GpioPin.RST)   # low
probe.gpio_set(GpioPin.RST)     # high
probe.gpio_toggle(GpioPin.EXT0)
```

**GPIO pulse**: `gpio_pulse` takes a `GpioPinMask` (one bit per pin, several pins can be combined).
The pins go low together, stay low for `gpio_pulse_width_us`, go back high, then the probe waits
`gpio_pulse_after_us`. The trigger `BEFORE` pulses before the falling edge; the trigger `AFTER`
pulses after the after-delay. With `last_delay=True`, `AFTER` pulses just before the
`gpio_pulse_last_us` delay, and that delay runs. The three times are probe settings, like `trigger_width_us`.

```python
from PycoDebug import GpioPinMask, TriggerPosition

probe.gpio_pulse_width_us = 100   # RST and TRST low for 100 us
probe.gpio_pulse_after_us = 500   # then wait 500 us after they go high
probe.gpio_pulse(GpioPinMask.RST | GpioPinMask.TRST, trigger=TriggerPosition.BEFORE)
```

**Timer**

```python
probe.delay_us(10)
probe.delay_ms(50)
```

**Settings**: `swd_freq_hz`, `jtag_freq_hz`, `debug_ap`, `trigger_width_us`, `gpio_pulse_width_us`,
`gpio_pulse_after_us` are properties.
Reading them asks the probe; assigning them changes the probe setting.

```python
probe.swd_freq_hz = 100000
print(probe.jtag_freq_hz)
print(probe.system_info())      # name, board_version, firmware_version
```

## Errors

| Exception | When |
|---|---|
| `ProbeError` | base class of the errors below |
| `CommandError` | the probe answered with a non-OK status; `.status` and `.data` are set |
| `ProtocolError` | the reply is malformed |
| `TransportTimeout` | no reply from the probe |
| `ValueError` | a value does not fit, or a hexadecimal string is invalid |

```python
from PycoDebug import CommandError, ProbeError

try:
    idcode = probe.jtag_connect()
except CommandError as exc:
    print("probe reported", exc.status, exc.data)
except ProbeError as exc:
    print("error:", exc)
```

## Low-level access

`probe.request(module, command, trigger=0, data=(), p1=0, p2=0)` sends any command of the protocol.
Each command is identified by its module (CLA, one byte) and its command number (INS, one byte).
`p1` selects a variant of the command and `p2` refines it: the command reads them and enables the matching behaviour.
The methods above send `p1 = 0` and `p2 = 0`, the default variant.

`Module` and the command enums are in `PycoDebug.commands`; `command_key(module, command)` gives the
16-bit key used in the frame.

## Files

| File | Content |
|---|---|
| `PycoDebug/probe.py` | `Probe`, the public API |
| `PycoDebug/values.py` | int or hexadecimal string conversion |
| `PycoDebug/commands.py` | enums: modules, commands, `GpioPin`, `TriggerPosition`, `DebugProtocol` |
| `PycoDebug/transport.py` | serial link to the probe |
| `examples/` | scripts for the terminal and for scope tests |
| `tests/` | unit tests: `python3 -m pytest tests` |
