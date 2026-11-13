#ifndef GENERIC_TRIGGER_H
#define GENERIC_TRIGGER_H

#include <stdint.h>

/* Generic module: trigger. Pulse on CONF_GPIO_TRIGGER around a command.
 * The position comes from the trigger byte of the command input (bit mask). */

#define TRIGGER_BEFORE (1u << 0)   /* pulse before the command runs */
#define TRIGGER_AFTER  (1u << 1)   /* pulse after the command ran */
#define TRIGGER_ADDRESS_BEFORE (1u << 2)   /* read/write: pulse just before the address is sent (TAR) */
#define TRIGGER_ADDRESS_AFTER  (1u << 3)   /* read/write: pulse just after the address is sent */
#define TRIGGER_VALUE_BEFORE   (1u << 4)   /* write: pulse just before the value is sent (DRW) */
#define TRIGGER_VALUE_AFTER    (1u << 5)   /* write: pulse just after the value is sent */

/* Configures the trigger pin as output, low */
void trigger_init(void);

/* Pulses the trigger if TRIGGER_BEFORE is set in positions */
void trigger_before(uint8_t positions);

/* Pulses the trigger if TRIGGER_AFTER is set in positions */
void trigger_after(uint8_t positions);

/* Pulses the trigger if the given position bit (one of the TRIGGER_* above) is set in positions */
void trigger_if(uint8_t positions, uint8_t position);

#endif /* GENERIC_TRIGGER_H */
