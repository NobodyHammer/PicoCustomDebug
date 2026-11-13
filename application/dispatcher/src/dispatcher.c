#include <stddef.h>

#include "application/dispatcher.h"

typedef struct {
    uint16_t cmd;
    dispatcher_handler_t handler;
} dispatcher_entry_t;

/* Private state: only reachable through the functions in dispatcher.h */
static struct {
    dispatcher_entry_t entries[DISPATCHER_MAX_HANDLERS];
    uint8_t count;
} table;

void dispatcher_init(void)
{
    table.count = 0;
}

bool dispatcher_register(uint16_t cmd, dispatcher_handler_t handler)
{
    if (handler == NULL || table.count >= DISPATCHER_MAX_HANDLERS) {
        return false;
    }

    for (uint8_t i = 0; i < table.count; i++) {
        if (table.entries[i].cmd == cmd) {
            return false;
        }
    }

    table.entries[table.count].cmd = cmd;
    table.entries[table.count].handler = handler;
    table.count++;
    return true;
}

static dispatcher_handler_t find_handler(uint16_t cmd)
{
    for (uint8_t i = 0; i < table.count; i++) {
        if (table.entries[i].cmd == cmd) {
            return table.entries[i].handler;
        }
    }
    return NULL;
}

static bool input_is_valid(const cmd_input_t *in)
{
    if (in->data_count > CMD_MAX_DATA) {
        return false;
    }
    for (uint8_t i = 0; i < in->data_count; i++) {
        if (in->data[i].len > CMD_DATA_MAX_LEN) {
            return false;
        }
    }
    return true;
}

void dispatcher_dispatch(const cmd_input_t *in, cmd_output_t *out)
{
    dispatcher_handler_t handler = find_handler(CMD_MAKE(in->module, in->cmd));

    if (handler == NULL) {
        /* Unknown command: the dispatcher builds the reply itself */
        *out = (cmd_output_t){ .status = CMD_STATUS_BAD_COMMAND };
        return;
    }

    if (!input_is_valid(in)) {
        *out = (cmd_output_t){ .status = CMD_STATUS_UNEXPECTED_PARAM };
        return;
    }

    /* Known command: the reply is the one built by its handler */
    *out = handler(in);
}
