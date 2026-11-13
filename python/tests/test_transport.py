import io
import unittest

from PycoDebug import protocol
from PycoDebug.errors import ProtocolError, TransportTimeout
from PycoDebug.transport import read_frame

REPLY = bytes.fromhex("A5 02 47 00 36 A7")


class ReadFrame(unittest.TestCase):
    def test_reads_one_frame(self):
        self.assertEqual(read_frame(io.BytesIO(REPLY).read), REPLY)

    def test_skips_noise_before_sync(self):
        stream = io.BytesIO(bytes.fromhex("00 FF 13 42") + REPLY)
        self.assertEqual(read_frame(stream.read), REPLY)

    def test_truncated_frame_times_out(self):
        with self.assertRaises(TransportTimeout):
            read_frame(io.BytesIO(REPLY[:-1]).read)

    def test_empty_stream_times_out(self):
        with self.assertRaises(TransportTimeout):
            read_frame(io.BytesIO(b"").read)

    def test_length_above_maximum_is_rejected(self):
        with self.assertRaises(ProtocolError):
            read_frame(io.BytesIO(bytes([protocol.SYNC, 0xFF]) + bytes(300)).read)


if __name__ == "__main__":
    unittest.main()
