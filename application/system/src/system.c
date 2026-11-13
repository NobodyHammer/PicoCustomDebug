#include <string.h>

#include "application/dispatcher.h"
#include "application/system.h"
#include "generic/cmd.h"

/* Short device name, exactly 4 bytes: one data entry carries at most 4 bytes */
static const uint8_t device_name[4] = {'D', 'B', 'G', 'P'};

/* Reply to SYSTEM_CMD_INFO:
 *   data[0] = device name (4 ASCII bytes)
 *   data[1] = board version (major, minor, patch)
 *   data[2] = firmware version (major, minor, patch) */
static cmd_output_t info_handler(const cmd_input_t *in)
{
    cmd_output_t out;

    memset(&out, 0, sizeof out);

    if (in->data_count != 0) {
        out.status = CMD_STATUS_UNEXPECTED_PARAM;
        return out;
    }

    memcpy(out.data[0].buf, device_name, sizeof device_name);
    out.data[0].len = sizeof device_name;

    out.data[1].buf[0] = BOARD_VERSION_MAJOR;
    out.data[1].buf[1] = BOARD_VERSION_MINOR;
    out.data[1].buf[2] = BOARD_VERSION_PATCH;
    out.data[1].len = 3;

    out.data[2].buf[0] = FW_VERSION_MAJOR;
    out.data[2].buf[1] = FW_VERSION_MINOR;
    out.data[2].buf[2] = FW_VERSION_PATCH;
    out.data[2].len = 3;

    out.data_count = 3;
    out.status = CMD_STATUS_OK;
    return out;
}

bool system_register(void)
{
    return dispatcher_register(CMD_MAKE(CMD_MODULE_SYSTEM, SYSTEM_CMD_INFO), info_handler);
}
