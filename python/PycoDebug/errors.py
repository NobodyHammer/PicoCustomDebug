"""Exceptions raised by the DebugProbe Python API."""

from . import protocol


class ProbeError(Exception):
    """Base class for all DebugProbe errors."""


class TransportTimeout(ProbeError):
    """No (complete) reply was received from the probe."""


class ProtocolError(ProbeError):
    """A reply was received but is malformed or fails its CRC check."""


class CommandError(ProbeError):
    """The probe executed the command and reported a non-OK status."""

    def __init__(self, status: int, data=()):
        self.status = status
        self.data = [entry.buf for entry in data]
        message = "probe reported status %s" % protocol.status_name(status)
        if self.data:
            message += " (data: %s)" % ", ".join(entry.hex() for entry in self.data)
        super().__init__(message)
