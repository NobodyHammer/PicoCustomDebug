#ifndef APPLICATION_TIMER_H
#define APPLICATION_TIMER_H

#include <stdbool.h>

/* Application module: timer (module nibble 0x4). Delays between two ticks of a sequence.
 * Each command takes one 4-byte big-endian value and pulses the trigger
 * according to the position byte of the command. */

#define TIMER_CMD_DELAY_US 0x1u   /* delay in microseconds */
#define TIMER_CMD_DELAY_MS 0x2u   /* delay in milliseconds */

/* Registers the timer handlers in the dispatcher. Call after dispatcher_init(). */
bool timer_register(void);

#endif /* APPLICATION_TIMER_H */
