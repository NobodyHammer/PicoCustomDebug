#include <string.h>

#include "generic/cmd.h"

cmd_output_t cmd_status_reply(uint8_t status)
{
    cmd_output_t out;
    memset(&out, 0, sizeof out);
    out.status = status;
    return out;
}

cmd_output_t cmd_value_reply(uint32_t value, uint8_t size)
{
    cmd_output_t out;
    memset(&out, 0, sizeof out);
    out.status = CMD_STATUS_OK;
    out.data_count = 1;
    out.data[0].len = size;
    for (uint8_t i = 0; i < size; i++) {
        out.data[0].buf[size - 1u - i] = (uint8_t)(value >> (8u * i));
    }
    return out;
}

uint8_t cmd_parse_value(const cmd_input_t *in, uint8_t size, uint32_t *value)
{
    if (in->data_count == 0) {
        return CMD_STATUS_MISSING_PARAM;
    }
    if (in->data_count != 1 || in->data[0].len != size) {
        return CMD_STATUS_UNEXPECTED_PARAM;
    }

    *value = 0;
    for (uint8_t i = 0; i < size; i++) {
        *value = (*value << 8) | in->data[0].buf[i];
    }
    return CMD_STATUS_OK;
}
