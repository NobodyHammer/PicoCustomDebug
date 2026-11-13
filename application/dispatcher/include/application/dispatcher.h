#ifndef APPLICATION_DISPATCHER_H
#define APPLICATION_DISPATCHER_H

#include <stdbool.h>
#include <stdint.h>

#include "generic/cmd.h"

/* Application module: dispatcher. Routes a command (module, command) to the handler registered for it.
 * The handler receives p1 in its input and selects the variant of the command from it.
 * Handlers are provided by the application modules. */

/* Maximum number of registered handlers */
#define DISPATCHER_MAX_HANDLERS 64u

/* Handler: reads the command input and returns the complete reply.
 * The command decides its own status (out.status) and fills its data entries. */
typedef cmd_output_t (*dispatcher_handler_t)(const cmd_input_t *in);

/* Clears the handler table. Call before registering handlers. */
void dispatcher_init(void);

/* Registers a handler for a command key (see CMD_MAKE).
 * Returns false if the handler is NULL, the command is already registered,
 * or the table is full. Registration is expected at init time only. */
bool dispatcher_register(uint16_t cmd, dispatcher_handler_t handler);

/* Executes the command described by in and stores the reply in out.
 * The dispatcher builds a reply itself only when it cannot run the command:
 *  - Unknown command (module, command):                       CMD_STATUS_BAD_COMMAND
 *  - Malformed input (data_count or a data len out of range): CMD_STATUS_UNEXPECTED_PARAM
 * Otherwise the reply is the one returned by the handler. */
void dispatcher_dispatch(const cmd_input_t *in, cmd_output_t *out);

#endif /* APPLICATION_DISPATCHER_H */
