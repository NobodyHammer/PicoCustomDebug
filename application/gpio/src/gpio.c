#include "pico/stdlib.h"

#include "application/dispatcher.h"
#include "application/gpio.h"
#include "generic/cmd.h"
#include "generic/conf.h"
#include "generic/trigger.h"

/* Maps a selector to a GPIO number, or returns false for an unknown selector */
static bool pin_of(uint32_t selector, uint8_t *pin)
{
    switch (selector) {
    case GPIO_SEL_RST:  *pin = CONF_GPIO_RST;  return true;
    case GPIO_SEL_EXT0: *pin = CONF_GPIO_EXT0; return true;
    case GPIO_SEL_EXT1: *pin = CONF_GPIO_EXT1; return true;
    case GPIO_SEL_EXT2: *pin = CONF_GPIO_EXT2; return true;
    case GPIO_SEL_TRST: *pin = CONF_GPIO_JTAG_TRST; return true;
    default:            return false;
    }
}

/* Reads the selector, applies action to its pin inside the trigger pulses */
static cmd_output_t run_on_pin(const cmd_input_t *in, void (*action)(uint8_t pin))
{
    uint32_t selector;
    uint8_t pin;

    uint8_t status = cmd_parse_value(in, 1, &selector);
    if (status != CMD_STATUS_OK) {
        return cmd_status_reply(status);
    }
    if (!pin_of(selector, &pin)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }

    trigger_before(in->trigger);
    action(pin);
    trigger_after(in->trigger);

    return cmd_status_reply(CMD_STATUS_OK);
}

static void do_set(uint8_t pin)    { gpio_put(pin, 1); }
static void do_reset(uint8_t pin)  { gpio_put(pin, 0); }
static void do_toggle(uint8_t pin) { gpio_put(pin, !gpio_get_out_level(pin)); }

static cmd_output_t set_handler(const cmd_input_t *in)    { return run_on_pin(in, do_set); }
static cmd_output_t reset_handler(const cmd_input_t *in)  { return run_on_pin(in, do_reset); }
static cmd_output_t toggle_handler(const cmd_input_t *in) { return run_on_pin(in, do_toggle); }

/* Pin of each bit of the pulse mask, in bit order */
static const struct {
    uint8_t bit;
    uint8_t pin;
} pulse_pins[] = {
    { GPIO_PULSE_RST,  CONF_GPIO_RST },
    { GPIO_PULSE_EXT0, CONF_GPIO_EXT0 },
    { GPIO_PULSE_EXT1, CONF_GPIO_EXT1 },
    { GPIO_PULSE_EXT2, CONF_GPIO_EXT2 },
    { GPIO_PULSE_TRST, CONF_GPIO_JTAG_TRST },
};

/* Drives every pin of the mask to level (0 or 1) */
static void drive_mask(uint8_t mask, bool level)
{
    for (size_t i = 0; i < sizeof pulse_pins / sizeof pulse_pins[0]; i++) {
        if (mask & pulse_pins[i].bit) {
            gpio_put(pulse_pins[i].pin, level);
        }
    }
}

/* Pins of the mask go low together, stay low for the configured width, go back high,
 * then wait the configured after-delay.
 * P2 = 0: the trigger after pulses here, at the end of the command.
 * P2 = 1: the trigger after pulses just before the configured last delay, which then runs. */
static cmd_output_t pulse_handler(const cmd_input_t *in)
{
    uint8_t mask = in->p1;

    if (mask == 0 || (mask & ~GPIO_PULSE_MASK_ALL) || in->p2 > 1 || in->data_count != 0) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }

    trigger_before(in->trigger);
    drive_mask(mask, false);
    sleep_us(conf_get_gpio_pulse_width_us());
    drive_mask(mask, true);
    sleep_us(conf_get_gpio_pulse_after_us());
    trigger_after(in->trigger);
    if (in->p2 == 1) {
        sleep_us(conf_get_gpio_pulse_last_us());
    }

    return cmd_status_reply(CMD_STATUS_OK);
}

bool gpio_register(void)
{
    return dispatcher_register(CMD_MAKE(CMD_MODULE_GPIO, GPIO_CMD_SET), set_handler)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_GPIO, GPIO_CMD_RESET), reset_handler)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_GPIO, GPIO_CMD_TOGGLE), toggle_handler)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_GPIO, GPIO_CMD_PULSE), pulse_handler);
}
