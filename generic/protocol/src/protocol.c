#include <string.h>

#include "generic/protocol.h"

uint16_t proto_crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = PROTO_CRC_INIT;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/* Writes count entries into out. *written receives the number of bytes used. */
static bool encode_entries(const cmd_data_t *data, uint8_t count,
                           uint8_t *out, size_t cap, size_t *written)
{
    size_t pos = 0;

    for (uint8_t i = 0; i < count; i++) {
        if (data[i].len > CMD_DATA_MAX_LEN || pos + 1u + data[i].len > cap) {
            return false;
        }
        out[pos++] = data[i].len;
        memcpy(&out[pos], data[i].buf, data[i].len);
        pos += data[i].len;
    }

    *written = pos;
    return true;
}

/* Reads count entries from payload[*pos ..< len]. Advances *pos. */
static bool decode_entries(const uint8_t *payload, size_t len, size_t *pos,
                           uint8_t count, cmd_data_t *data)
{
    for (uint8_t i = 0; i < count; i++) {
        if (*pos >= len) {
            return false;
        }
        uint8_t entry_len = payload[(*pos)++];
        if (entry_len > CMD_DATA_MAX_LEN || *pos + entry_len > len) {
            return false;
        }
        data[i].len = entry_len;
        memcpy(data[i].buf, &payload[*pos], entry_len);
        *pos += entry_len;
    }
    return true;
}

/* Frame is already filled from index 2 with payload_len bytes. Adds SYNC, LEN and CRC. */
static size_t finish_frame(uint8_t *frame, size_t cap, size_t payload_len)
{
    uint16_t crc;

    if (payload_len > PROTO_MAX_PAYLOAD || payload_len + PROTO_FRAME_OVERHEAD > cap) {
        return 0;
    }

    frame[0] = PROTO_SYNC;
    frame[1] = (uint8_t)payload_len;
    crc = proto_crc16(&frame[1], payload_len + 1u);
    frame[payload_len + 2u] = (uint8_t)(crc >> 8);
    frame[payload_len + 3u] = (uint8_t)(crc & 0xFFu);
    return payload_len + PROTO_FRAME_OVERHEAD;
}

/* Checks SYNC, LEN and CRC. On success, *payload and *payload_len point into frame. */
static bool unwrap_frame(const uint8_t *frame, size_t len,
                         const uint8_t **payload, size_t *payload_len)
{
    size_t n;
    uint16_t received;

    if (len < PROTO_FRAME_OVERHEAD || frame[0] != PROTO_SYNC) {
        return false;
    }

    n = frame[1];
    if (n > PROTO_MAX_PAYLOAD || len != n + PROTO_FRAME_OVERHEAD) {
        return false;
    }

    received = (uint16_t)(((uint16_t)frame[len - 2u] << 8) | frame[len - 1u]);
    if (proto_crc16(&frame[1], n + 1u) != received) {
        return false;
    }

    *payload = &frame[2];
    *payload_len = n;
    return true;
}

size_t proto_encode_input(const cmd_input_t *in, uint8_t *frame, size_t cap)
{
    uint8_t *p;
    size_t entries_len = 0;

    if (in->data_count > CMD_MAX_DATA || cap < PROTO_FRAME_OVERHEAD + PROTO_REQUEST_HEADER) {
        return 0;
    }

    p = &frame[2];
    p[0] = in->module;
    p[1] = in->cmd;
    p[2] = in->p1;
    p[3] = in->p2;
    p[4] = in->trigger;
    p[5] = in->data_count;

    if (!encode_entries(in->data, in->data_count, &p[PROTO_REQUEST_HEADER],
                        cap - PROTO_FRAME_OVERHEAD - PROTO_REQUEST_HEADER, &entries_len)) {
        return 0;
    }

    return finish_frame(frame, cap, PROTO_REQUEST_HEADER + entries_len);
}

bool proto_decode_input(const uint8_t *frame, size_t len, cmd_input_t *in)
{
    const uint8_t *p;
    size_t n;
    size_t pos = PROTO_REQUEST_HEADER;
    cmd_input_t tmp;

    if (!unwrap_frame(frame, len, &p, &n) || n < PROTO_REQUEST_HEADER) {
        return false;
    }

    memset(&tmp, 0, sizeof tmp);
    tmp.module = p[0];
    tmp.cmd = p[1];
    tmp.p1 = p[2];
    tmp.p2 = p[3];
    tmp.trigger = p[4];
    tmp.data_count = p[5];

    if (tmp.data_count > CMD_MAX_DATA ||
        !decode_entries(p, n, &pos, tmp.data_count, tmp.data) ||
        pos != n) {
        return false;
    }

    *in = tmp;
    return true;
}

size_t proto_encode_output(const cmd_output_t *out, uint8_t *frame, size_t cap)
{
    uint8_t *p;
    size_t entries_len = 0;

    if (out->data_count > CMD_MAX_DATA || cap < PROTO_FRAME_OVERHEAD + 2u) {
        return 0;
    }

    p = &frame[2];
    p[0] = out->status;
    p[1] = out->data_count;

    if (!encode_entries(out->data, out->data_count, &p[2],
                        cap - PROTO_FRAME_OVERHEAD - 2u, &entries_len)) {
        return 0;
    }

    return finish_frame(frame, cap, 2u + entries_len);
}

bool proto_decode_output(const uint8_t *frame, size_t len, cmd_output_t *out)
{
    const uint8_t *p;
    size_t n;
    size_t pos = 2;
    cmd_output_t tmp;

    if (!unwrap_frame(frame, len, &p, &n) || n < 2u) {
        return false;
    }

    memset(&tmp, 0, sizeof tmp);
    tmp.status = p[0];
    tmp.data_count = p[1];

    if (tmp.data_count > CMD_MAX_DATA ||
        !decode_entries(p, n, &pos, tmp.data_count, tmp.data) ||
        pos != n) {
        return false;
    }

    *out = tmp;
    return true;
}
