#ifndef GENERIC_PROTOCOL_H
#define GENERIC_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "generic/cmd.h"

/* Generic module: protocol. Frame format carrying cmd_input_t and cmd_output_t over a byte stream
 * (USB CDC). The PC side (python/) implements the same format.
 *
 * Frame layout (all multi-byte fields big-endian):
 *   [SYNC] [LEN] [PAYLOAD: LEN bytes] [CRC16 high] [CRC16 low]
 *
 *   LEN is the payload length only (0..PROTO_MAX_PAYLOAD).
 *   CRC16 is CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection, no xorout),
 *   computed over LEN and PAYLOAD (SYNC is not covered).
 *
 * Request payload:  [module] [cmd] [p1] [p2] [trigger] [data_count] { [len] [len bytes] } x data_count
 * Response payload: [status] [data_count]        { [len] [len bytes] } x data_count
 */

#define PROTO_SYNC              0xA5u
#define PROTO_CRC_INIT          0xFFFFu

/* Largest payload: 6 request header bytes + CMD_MAX_DATA entries of (1 + CMD_DATA_MAX_LEN) bytes */
#define PROTO_REQUEST_HEADER    6u
#define PROTO_MAX_PAYLOAD       (PROTO_REQUEST_HEADER + CMD_MAX_DATA * (1u + CMD_DATA_MAX_LEN))

/* Frame overhead: SYNC + LEN + CRC16 */
#define PROTO_FRAME_OVERHEAD    4u

/* Largest complete frame */
#define PROTO_FRAME_MAX         (PROTO_MAX_PAYLOAD + PROTO_FRAME_OVERHEAD)

/* CRC-16/CCITT-FALSE over data */
uint16_t proto_crc16(const uint8_t *data, size_t len);

/* Encodes a request into frame. Returns the frame length, or 0 if the input is invalid or cap is too small. */
size_t proto_encode_input(const cmd_input_t *in, uint8_t *frame, size_t cap);

/* Decodes a request frame. Returns false if the frame is malformed or its CRC does not match. */
bool proto_decode_input(const uint8_t *frame, size_t len, cmd_input_t *in);

/* Encodes a response into frame. Returns the frame length, or 0 if the output is invalid or cap is too small. */
size_t proto_encode_output(const cmd_output_t *out, uint8_t *frame, size_t cap);

/* Decodes a response frame. Returns false if the frame is malformed or its CRC does not match. */
bool proto_decode_output(const uint8_t *frame, size_t len, cmd_output_t *out);

#endif /* GENERIC_PROTOCOL_H */
