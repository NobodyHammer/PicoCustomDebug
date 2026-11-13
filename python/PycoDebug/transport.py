"""Byte transports. A transport sends one request frame and returns one reply frame."""

from .errors import ProbeError, ProtocolError, TransportTimeout
from .protocol import MAX_PAYLOAD, SYNC


def read_frame(read) -> bytes:
    """Reads one frame using read(n) -> bytes. An empty result means timeout.

    Bytes before SYNC are discarded, so line noise does not break the next frame.
    """

    def read_exact(n: int) -> bytes:
        data = b""
        while len(data) < n:
            chunk = read(n - len(data))
            if not chunk:
                raise TransportTimeout("no reply from probe")
            data += chunk
        return data

    while read_exact(1)[0] != SYNC:
        pass

    length = read_exact(1)[0]
    if length > MAX_PAYLOAD:
        raise ProtocolError("reply LEN %d exceeds maximum %d" % (length, MAX_PAYLOAD))

    return bytes([SYNC, length]) + read_exact(length + 2)


# USB IDs of the Raspberry Pi Pico SDK default USB CDC (pico_stdio_usb):
# 0x0009 on RP2350 (Pico 2), 0x000A on RP2040 (Pico)
DEFAULT_VID = 0x2E8A
DEFAULT_PIDS = (0x0009, 0x000A)


def find_port(vid: int = DEFAULT_VID, pids=DEFAULT_PIDS, ports=None) -> str:
    """Returns the serial port of the single probe matching vid and one of pids.

    ports: iterable of objects with device, vid and pid (defaults to the system ports).
    """
    if ports is None:
        from serial.tools import list_ports

        ports = list_ports.comports()

    matches = [port.device for port in ports if port.vid == vid and port.pid in pids]
    if not matches:
        raise ProbeError("no DebugProbe found (USB %04X, pids %s)"
                         % (vid, ", ".join("%04X" % p for p in pids)))
    if len(matches) > 1:
        raise ProbeError("several DebugProbe found (%s), pass the port explicitly"
                         % ", ".join(matches))
    return matches[0]


class SerialTransport:
    """USB CDC (or UART) serial link. pyserial is imported here so the rest of the package
    works without it."""

    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 1.0):
        import serial

        self._serial = serial.Serial(port, baudrate=baudrate, timeout=timeout)

    def exchange(self, frame: bytes) -> bytes:
        self._serial.reset_input_buffer()
        self._serial.write(frame)
        self._serial.flush()
        return read_frame(self._serial.read)

    def close(self) -> None:
        self._serial.close()
