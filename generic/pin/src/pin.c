#include "pico/stdlib.h"

#include "generic/pin.h"

void pin_apply_table(const pin_config_t *table, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        const pin_config_t *pin = &table[i];

        gpio_init(pin->gpio);
        gpio_put(pin->gpio, pin->level);
        gpio_set_pulls(pin->gpio, pin->pull == PIN_PULL_UP, pin->pull == PIN_PULL_DOWN);
        gpio_set_dir(pin->gpio, pin->dir == PIN_OUT ? GPIO_OUT : GPIO_IN);
    }
}
