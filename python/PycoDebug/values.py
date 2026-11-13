"""Values given by the user: an integer, or a hexadecimal string.

A hexadecimal string may carry the 0x prefix or not: "0x2001FF00" and "2001FF00" are the same value.
Both forms are checked against the size of the field, so an out-of-range value is refused rather than
truncated.
"""

import re
from typing import Union

Value = Union[int, str]

_HEX_DIGITS = re.compile(r"[0-9a-fA-F]+")


def to_int(value: Value, size: int, name: str = "value") -> int:
    """Returns the integer for value, checked to fit in size bytes (unsigned)."""
    if isinstance(value, bool):
        raise TypeError("%s: a boolean is not a value" % name)
    if isinstance(value, int):
        number = int(value)
    elif isinstance(value, str):
        text = value.strip()
        if text[:2].lower() == "0x":
            text = text[2:]
        if not _HEX_DIGITS.fullmatch(text):
            raise ValueError("%s: %r is not a hexadecimal string" % (name, value))
        number = int(text, 16)
    else:
        raise TypeError("%s: expected an int or a hexadecimal string, got %s" % (name, type(value).__name__))

    if not 0 <= number < (1 << (8 * size)):
        raise ValueError("%s: %r does not fit in %d byte(s)" % (name, value, size))
    return number


def to_bytes(value: Value, size: int, name: str = "value") -> bytes:
    """Returns value as size bytes, big-endian."""
    return to_int(value, size, name).to_bytes(size, "big")
