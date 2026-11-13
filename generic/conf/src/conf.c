#include "generic/conf.h"

/* Pins shared by every protocol */
static const pin_config_t common_pins[] = {
    { CONF_GPIO_TRIGGER, PIN_OUT, PIN_PULL_NONE, false },
    { CONF_GPIO_RST,     PIN_OUT, PIN_PULL_NONE, true },    /* released (high) */
    { CONF_GPIO_EXT0,    PIN_OUT, PIN_PULL_NONE, false },
    { CONF_GPIO_EXT1,    PIN_OUT, PIN_PULL_NONE, false },
    { CONF_GPIO_EXT2,    PIN_OUT, PIN_PULL_NONE, false },
};

/* SWD: SWCLK is an output idling high, SWDIO is an input at rest with a pull-up
 * (the driver switches its direction during each transfer) */
static const pin_config_t swd_pins[] = {
    { CONF_GPIO_SWD_CLK, PIN_OUT, PIN_PULL_NONE, true },
    { CONF_GPIO_SWD_DIO, PIN_IN,  PIN_PULL_UP,   false },
};

/* JTAG: TCK, TMS and TDI are outputs, TDO is an input. TMS, TDI and TDO have a pull-up,
 * as on a JTAG target: the lines idle high when no target drives them. */
static const pin_config_t jtag_pins[] = {
    { CONF_GPIO_JTAG_TCK, PIN_OUT, PIN_PULL_NONE, true },   /* TCK idles high, as for SWCLK */
    { CONF_GPIO_JTAG_TMS, PIN_OUT, PIN_PULL_UP,   true },
    { CONF_GPIO_JTAG_TDI, PIN_OUT, PIN_PULL_UP,   false },
    { CONF_GPIO_JTAG_TDO, PIN_IN,  PIN_PULL_UP,   false },
    { CONF_GPIO_JTAG_TRST, PIN_OUT, PIN_PULL_NONE, true },  /* TRST released (high) */
};

/* Private state: only reachable through the accessors below */
static struct {
    uint32_t trigger_width_us;
    uint32_t gpio_pulse_width_us;
    uint32_t gpio_pulse_after_us;
    uint32_t gpio_pulse_last_us;
    uint8_t debug_ap;
    uint8_t debug_protocol;
    uint32_t swd_freq_hz;
    uint32_t jtag_freq_hz;
    conf_dirty_t dirty;
} conf = {
    .trigger_width_us = CONF_DEFAULT_TRIGGER_WIDTH_US,
    .gpio_pulse_width_us = CONF_DEFAULT_GPIO_PULSE_WIDTH_US,
    .gpio_pulse_after_us = CONF_DEFAULT_GPIO_PULSE_AFTER_US,
    .gpio_pulse_last_us = CONF_DEFAULT_GPIO_PULSE_LAST_US,
    .debug_ap = CONF_DEFAULT_DEBUG_AP,
    .debug_protocol = CONF_DEFAULT_PROTOCOL,
    .swd_freq_hz = CONF_DEFAULT_SWD_FREQ_HZ,
    .jtag_freq_hz = CONF_DEFAULT_JTAG_FREQ_HZ,
    .dirty = 0,
};

void conf_apply_pins(void)
{
    pin_apply_table(common_pins, sizeof common_pins / sizeof common_pins[0]);
    if (conf.debug_protocol == CONF_PROTOCOL_JTAG) {
        pin_apply_table(jtag_pins, sizeof jtag_pins / sizeof jtag_pins[0]);
    } else {
        pin_apply_table(swd_pins, sizeof swd_pins / sizeof swd_pins[0]);
    }
}

uint32_t conf_get_trigger_width_us(void)
{
    return conf.trigger_width_us;
}

void conf_set_trigger_width_us(uint32_t width_us)
{
    if (width_us != conf.trigger_width_us) {
        conf.trigger_width_us = width_us;
        conf.dirty |= CONF_DIRTY_TRIGGER;
    }
}

uint32_t conf_get_gpio_pulse_width_us(void)
{
    return conf.gpio_pulse_width_us;
}

void conf_set_gpio_pulse_width_us(uint32_t width_us)
{
    conf.gpio_pulse_width_us = width_us;
}

uint32_t conf_get_gpio_pulse_after_us(void)
{
    return conf.gpio_pulse_after_us;
}

void conf_set_gpio_pulse_after_us(uint32_t after_us)
{
    conf.gpio_pulse_after_us = after_us;
}

uint32_t conf_get_gpio_pulse_last_us(void)
{
    return conf.gpio_pulse_last_us;
}

void conf_set_gpio_pulse_last_us(uint32_t last_us)
{
    conf.gpio_pulse_last_us = last_us;
}

uint8_t conf_get_debug_ap(void)
{
    return conf.debug_ap;
}

void conf_set_debug_ap(uint8_t ap)
{
    if (ap != conf.debug_ap) {
        conf.debug_ap = ap;
        conf.dirty |= CONF_DIRTY_DEBUG_AP;
    }
}

uint8_t conf_get_debug_protocol(void)
{
    return conf.debug_protocol;
}

void conf_set_debug_protocol(uint8_t protocol)
{
    if (protocol != conf.debug_protocol) {
        conf.debug_protocol = protocol;
        conf.dirty |= CONF_DIRTY_PROTOCOL;
    }
}

uint32_t conf_get_swd_freq_hz(void)
{
    return conf.swd_freq_hz;
}

void conf_set_swd_freq_hz(uint32_t freq_hz)
{
    if (freq_hz != conf.swd_freq_hz) {
        conf.swd_freq_hz = freq_hz;
        conf.dirty |= CONF_DIRTY_SWD;
    }
}

uint32_t conf_get_jtag_freq_hz(void)
{
    return conf.jtag_freq_hz;
}

void conf_set_jtag_freq_hz(uint32_t freq_hz)
{
    if (freq_hz != conf.jtag_freq_hz) {
        conf.jtag_freq_hz = freq_hz;
        conf.dirty |= CONF_DIRTY_JTAG;
    }
}

bool conf_is_dirty(conf_dirty_t mask)
{
    return (conf.dirty & mask) != 0;
}

void conf_clear_dirty(conf_dirty_t mask)
{
    conf.dirty &= ~mask;
}
