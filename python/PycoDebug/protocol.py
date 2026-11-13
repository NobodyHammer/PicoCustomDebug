"""Frame codec, mirror of generic/protocol and generic/cmd in the firmware.

Frame layout:
    [SYNC] [LEN] [PAYLOAD: LEN bytes] [CRC16 high] [CRC16 low]
CRC-16/CCITT-FALSE over LEN and PAYLOAD, big-endian.

Request payload:  [cmd] [trigger] [data_count] { [len] [len bytes] }
Response payload: [status] [data_count]        { [len] [len bytes] }
"""

from dataclasses import dataclass, field
from enum import IntEnum
from typing import List, Tuple

SYNC = 0xA5
CRC_INIT = 0xFFFF
MAX_DATA = 4
DATA_MAX_LEN = 4
MAX_PAYLOAD = 3 + MAX_DATA * (1 + DATA_MAX_LEN)
FRAME_OVERHEAD = 4


class Status(IntEnum):
    """Status codes, same values as CMD_STATUS_* in generic/cmd/include/generic/cmd.h."""
    OK = 0x47
    FAIL = 0xA1
    TIMEOUT = 0x1E
    BAD_COMMAND = 0x72
    MISSING_PARAM = 0x59
    UNEXPECTED_PARAM = 0xCC


def status_name(value: int) -> str:
    try:
        return Status(value).name
    except ValueError:
        return "0x%02X" % value


def crc16(data: bytes) -> int:
    """CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection, no xorout)."""
    crc = CRC_INIT
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if crc & 0x8000 else (crc << 1)
            crc &= 0xFFFF
    return crc


@dataclass
class Data:
    buf: bytes = b""  # 0..DATA_MAX_LEN bytes; len is len(buf)

    def __post_init__(self):
        if len(self.buf) > DATA_MAX_LEN:
            raise ValueError("data entry longer than %d bytes" % DATA_MAX_LEN)


@dataclass
class Input:
    """A request. cmd is the 16-bit command key (module << 8 | command), p1 selects a variant."""
    cmd: int
    p1: int = 0
    p2: int = 0
    trigger: int = 0
    data: List[Data] = field(default_factory=list)


@dataclass
class Output:
    status: int
    data: List[Data] = field(default_factory=list)


def _encode_entries(data: List[Data]) -> bytes:
    if len(data) > MAX_DATA:
        raise ValueError("more than %d data entries" % MAX_DATA)
    out = bytearray()
    for entry in data:
        out.append(len(entry.buf))
        out += entry.buf
    return bytes(out)


def _decode_entries(payload: bytes, pos: int, count: int) -> Tuple[List[Data], int]:
    entries = []
    for _ in range(count):
        if pos >= len(payload):
            raise ValueError("truncated data entry")
        n = payload[pos]
        pos += 1
        if n > DATA_MAX_LEN or pos + n > len(payload):
            raise ValueError("invalid data entry length")
        entries.append(Data(bytes(payload[pos:pos + n])))
        pos += n
    return entries, pos


def _wrap(payload: bytes) -> bytes:
    if len(payload) > MAX_PAYLOAD:
        raise ValueError("payload too long")
    body = bytes([len(payload)]) + payload
    crc = crc16(body)
    return bytes([SYNC]) + body + bytes([crc >> 8, crc & 0xFF])


def _unwrap(frame: bytes) -> bytes:
    if len(frame) < FRAME_OVERHEAD or frame[0] != SYNC:
        raise ValueError("bad SYNC or short frame")
    n = frame[1]
    if n > MAX_PAYLOAD or len(frame) != n + FRAME_OVERHEAD:
        raise ValueError("LEN does not match frame size")
    received = (frame[-2] << 8) | frame[-1]
    if crc16(frame[1:-2]) != received:
        raise ValueError("CRC mismatch")
    return frame[2:-2]


def encode_input(req: Input) -> bytes:
    if not 0 <= req.cmd <= 0xFFFF:
        raise ValueError("cmd must be a 16-bit command key")
    if not (0 <= req.p1 <= 0xFF and 0 <= req.p2 <= 0xFF and 0 <= req.trigger <= 0xFF):
        raise ValueError("p1, p2 and trigger must be bytes")
    header = bytes([req.cmd >> 8, req.cmd & 0xFF, req.p1, req.p2, req.trigger, len(req.data)])
    return _wrap(header + _encode_entries(req.data))


def decode_input(frame: bytes) -> Input:
    payload = _unwrap(frame)
    if len(payload) < 6:
        raise ValueError("request payload too short")
    count = payload[5]
    if count > MAX_DATA:
        raise ValueError("more than %d data entries" % MAX_DATA)
    data, pos = _decode_entries(payload, 6, count)
    if pos != len(payload):
        raise ValueError("trailing bytes in request")
    return Input(cmd=(payload[0] << 8) | payload[1], p1=payload[2], p2=payload[3],
                 trigger=payload[4], data=data)


def encode_output(resp: Output) -> bytes:
    if not 0 <= resp.status <= 0xFF:
        raise ValueError("status must be a byte")
    payload = bytes([resp.status, len(resp.data)]) + _encode_entries(resp.data)
    return _wrap(payload)


def decode_output(frame: bytes) -> Output:
    payload = _unwrap(frame)
    if len(payload) < 2:
        raise ValueError("response payload too short")
    count = payload[1]
    if count > MAX_DATA:
        raise ValueError("more than %d data entries" % MAX_DATA)
    data, pos = _decode_entries(payload, 2, count)
    if pos != len(payload):
        raise ValueError("trailing bytes in response")
    return Output(status=payload[0], data=data)
