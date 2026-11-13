#include "pico/stdlib.h"

#include "generic/conf.h"
#include "generic/trigger.h"

/* Cached pulse width, reloaded from conf when CONF_DIRTY_TRIGGER is raised */
static uint32_t width_us;

static void pulse(void)
{
    if (conf_is_dirty(CONF_DIRTY_TRIGGER)) {
        width_us = conf_get_trigger_width_us();
        conf_clear_dirty(CONF_DIRTY_TRIGGER);
    }

    gpio_put(CONF_GPIO_TRIGGER, 1);
    busy_wait_us(width_us);
    gpio_put(CONF_GPIO_TRIGGER, 0);
}

void trigger_init(void)
{
    width_us = conf_get_trigger_width_us();
    conf_clear_dirty(CONF_DIRTY_TRIGGER);
}

void trigger_before(uint8_t positions)
{
    if (positions & TRIGGER_BEFORE) {
        pulse();
    }
}

void trigger_after(uint8_t positions)
{
    if (positions & TRIGGER_AFTER) {
        pulse();
    }
}

void trigger_if(uint8_t positions, uint8_t position)
{
    if (positions & position) {
        pulse();
    }
}
