#include <stdio.h>

#include "application/link.h"
#include "generic/protocol.h"

bool link_read_frame(uint8_t *frame, size_t cap, size_t *len)
{
    int c;
    size_t payload_len;

    /* Discard anything before SYNC */
    do {
        c = getchar();
    } while (c != PROTO_SYNC);

    c = getchar();
    if (c == EOF) {
        return false;
    }
    payload_len = (size_t)c;
    if (payload_len > PROTO_MAX_PAYLOAD || payload_len + PROTO_FRAME_OVERHEAD > cap) {
        return false;
    }

    frame[0] = PROTO_SYNC;
    frame[1] = (uint8_t)payload_len;

    /* Payload followed by the two CRC bytes */
    for (size_t i = 0; i < payload_len + 2u; i++) {
        c = getchar();
        if (c == EOF) {
            return false;
        }
        frame[2u + i] = (uint8_t)c;
    }

    *len = payload_len + PROTO_FRAME_OVERHEAD;
    return true;
}

void link_write_frame(const uint8_t *frame, size_t len)
{
    fwrite(frame, 1, len, stdout);
    fflush(stdout);
}
