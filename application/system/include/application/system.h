#ifndef APPLICATION_SYSTEM_H
#define APPLICATION_SYSTEM_H

#include <stdbool.h>

/* Application module: system (module nibble 0x0). Identity of the probe. */

/* Command number (low nibble) within the system module */
#define SYSTEM_CMD_INFO 0x1u

/* Registers the system handlers in the dispatcher. Call after dispatcher_init(). */
bool system_register(void);

#endif /* APPLICATION_SYSTEM_H */
