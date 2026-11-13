#ifndef APPLICATION_LINK_H
#define APPLICATION_LINK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Application module: link. Frame exchange with the PC over USB CDC (stdio). */

/* Blocks until a SYNC byte and a complete frame have been read into frame.
 * Returns false if the LEN byte is invalid or does not fit in cap; the caller
 * then keeps waiting for the next SYNC. On success *len is the frame length. */
bool link_read_frame(uint8_t *frame, size_t cap, size_t *len);

/* Writes a complete frame to the PC and flushes it. */
void link_write_frame(const uint8_t *frame, size_t len);

#endif /* APPLICATION_LINK_H */
