#include <stdint.h>

#include "pico/time.h"

#include "application/dispatcher.h"
#include "application/timer.h"
#include "generic/cmd.h"
#include "generic/trigger.h"

static cmd_output_t delay_us_handler(const cmd_input_t *in)
{
    uint32_t us;
    uint8_t status = cmd_parse_value(in, 4, &us);
    if (status != CMD_STATUS_OK) {
        return cmd_status_reply(status);
    }

    trigger_before(in->trigger);
    busy_wait_us(us);
    trigger_after(in->trigger);

    return cmd_status_reply(CMD_STATUS_OK);
}

static cmd_output_t delay_ms_handler(const cmd_input_t *in)
{
    uint32_t ms;
    uint8_t status = cmd_parse_value(in, 4, &ms);
    if (status != CMD_STATUS_OK) {
        return cmd_status_reply(status);
    }

    trigger_before(in->trigger);
    for (uint32_t i = 0; i < ms; i++) {
        busy_wait_us(1000);
    }
    trigger_after(in->trigger);

    return cmd_status_reply(CMD_STATUS_OK);
}

bool timer_register(void)
{
    return dispatcher_register(CMD_MAKE(CMD_MODULE_TIMER, TIMER_CMD_DELAY_US), delay_us_handler)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_TIMER, TIMER_CMD_DELAY_MS), delay_ms_handler);
}
