#ifndef GENERIC_PIN_H
#define GENERIC_PIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Generic module: pin. Setup of the GPIOs from a table of configurations.
 * Shared pins (SWD and JTAG use the same GPIO) get one entry in the table. */

typedef enum {
    PIN_IN = 0,
    PIN_OUT = 1,
} pin_dir_t;

typedef enum {
    PIN_PULL_NONE = 0,
    PIN_PULL_UP = 1,
    PIN_PULL_DOWN = 2,
} pin_pull_t;

typedef struct {
    uint8_t gpio;
    pin_dir_t dir;
    pin_pull_t pull;
    bool level;   /* output level applied at setup (ignored for inputs) */
} pin_config_t;

/* Configures every pin of the table: level, pull, direction */
void pin_apply_table(const pin_config_t *table, size_t count);

#endif /* GENERIC_PIN_H */
