import re
import unittest
from pathlib import Path

from PycoDebug import protocol as proto
from PycoDebug.protocol import Data, Input, Output

CMD_H = Path(__file__).resolve().parents[2] / "generic" / "cmd" / "include" / "generic" / "cmd.h"

# Reference frames produced by the C firmware (generic/protocol host test)
REQ_CMD = (0x1 << 8) | 0x3          # CMD_MAKE(CMD_MODULE_SWD, 0x3): module 0x01, command 0x03
# Request frame as the firmware encodes it: module, command, p1 = 0, trigger, data
REQ_FRAME = bytes.fromhex("A5 0D 01 03 00 00 01 02 04 01 02 03 04 01 AB 50 F5")
RESP_FRAME = bytes.fromhex("A5 02 47 00 36 A7")


def c_status_values():
    text = CMD_H.read_text()
    return {
        name: int(value, 16)
        for name, value in re.findall(r"#define CMD_STATUS_(\w+)\s+(0x[0-9A-Fa-f]+)u", text)
    }


class Crc(unittest.TestCase):
    def test_check_value(self):
        self.assertEqual(proto.crc16(b"123456789"), 0x29B1)


class Vectors(unittest.TestCase):
    def test_encode_request_matches_firmware(self):
        req = Input(cmd=REQ_CMD, trigger=0x01,
                    data=[Data(bytes([1, 2, 3, 4])), Data(bytes([0xAB]))])
        self.assertEqual(proto.encode_input(req), REQ_FRAME)

    def test_decode_request_from_firmware(self):
        req = proto.decode_input(REQ_FRAME)
        self.assertEqual(req.cmd, REQ_CMD)
        self.assertEqual(req.trigger, 0x01)
        self.assertEqual([d.buf for d in req.data], [bytes([1, 2, 3, 4]), bytes([0xAB])])

    def test_decode_response_from_firmware(self):
        resp = proto.decode_output(RESP_FRAME)
        self.assertEqual(resp.status, 0x47)
        self.assertEqual(resp.data, [])

    def test_encode_response_matches_firmware(self):
        self.assertEqual(proto.encode_output(Output(status=0x47)), RESP_FRAME)


class RoundTrip(unittest.TestCase):
    def test_full_response(self):
        resp = Output(status=0xA1, data=[Data(bytes([i] * 4)) for i in range(4)])
        frame = proto.encode_output(resp)
        self.assertEqual(len(frame), 26)
        self.assertEqual(proto.decode_output(frame), resp)


class Rejection(unittest.TestCase):
    def test_bad_crc(self):
        bad = REQ_FRAME[:-1] + bytes([REQ_FRAME[-1] ^ 0x01])
        with self.assertRaises(ValueError):
            proto.decode_input(bad)

    def test_bad_sync(self):
        with self.assertRaises(ValueError):
            proto.decode_input(bytes([0x00]) + REQ_FRAME[1:])

    def test_corrupted_payload(self):
        bad = bytearray(REQ_FRAME)
        bad[2] ^= 0x80
        with self.assertRaises(ValueError):
            proto.decode_input(bytes(bad))

    def test_truncated(self):
        with self.assertRaises(ValueError):
            proto.decode_input(REQ_FRAME[:-1])

    def test_too_many_data(self):
        with self.assertRaises(ValueError):
            proto.encode_input(Input(cmd=1, data=[Data(b"")] * 5))

    def test_data_too_long(self):
        with self.assertRaises(ValueError):
            Data(b"12345")


class StatusConsistency(unittest.TestCase):
    """Guards against drift between the firmware status codes and this file."""

    def test_status_codes_match_cmd_h(self):
        values = c_status_values()
        self.assertEqual(len(values), 6)
        self.assertEqual(values["OK"], 0x47)
        self.assertEqual(values["FAIL"], 0xA1)
        self.assertEqual(values["TIMEOUT"], 0x1E)
        self.assertEqual(values["BAD_COMMAND"], 0x72)
        self.assertEqual(values["MISSING_PARAM"], 0x59)
        self.assertEqual(values["UNEXPECTED_PARAM"], 0xCC)


if __name__ == "__main__":
    unittest.main()
