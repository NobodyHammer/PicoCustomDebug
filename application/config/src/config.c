#include "application/config.h"
#include "application/dispatcher.h"
#include "generic/cmd.h"
#include "generic/conf.h"

/* A get command takes no parameter */
static bool has_no_param(const cmd_input_t *in)
{
    return in->data_count == 0;
}

static cmd_output_t get_trigger_width(const cmd_input_t *in)
{
    if (!has_no_param(in)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }
    return cmd_value_reply(conf_get_trigger_width_us(), 4);
}

static cmd_output_t set_trigger_width(const cmd_input_t *in)
{
    uint32_t value;
    uint8_t status = cmd_parse_value(in, 4, &value);
    if (status == CMD_STATUS_OK) {
        conf_set_trigger_width_us(value);
    }
    return cmd_status_reply(status);
}

static cmd_output_t get_gpio_pulse_width(const cmd_input_t *in)
{
    if (!has_no_param(in)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }
    return cmd_value_reply(conf_get_gpio_pulse_width_us(), 4);
}

static cmd_output_t set_gpio_pulse_width(const cmd_input_t *in)
{
    uint32_t value;
    uint8_t status = cmd_parse_value(in, 4, &value);
    if (status == CMD_STATUS_OK) {
        conf_set_gpio_pulse_width_us(value);
    }
    return cmd_status_reply(status);
}

static cmd_output_t get_gpio_pulse_after(const cmd_input_t *in)
{
    if (!has_no_param(in)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }
    return cmd_value_reply(conf_get_gpio_pulse_after_us(), 4);
}

static cmd_output_t set_gpio_pulse_after(const cmd_input_t *in)
{
    uint32_t value;
    uint8_t status = cmd_parse_value(in, 4, &value);
    if (status == CMD_STATUS_OK) {
        conf_set_gpio_pulse_after_us(value);
    }
    return cmd_status_reply(status);
}

static cmd_output_t get_gpio_pulse_last(const cmd_input_t *in)
{
    if (!has_no_param(in)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }
    return cmd_value_reply(conf_get_gpio_pulse_last_us(), 4);
}

static cmd_output_t set_gpio_pulse_last(const cmd_input_t *in)
{
    uint32_t value;
    uint8_t status = cmd_parse_value(in, 4, &value);
    if (status == CMD_STATUS_OK) {
        conf_set_gpio_pulse_last_us(value);
    }
    return cmd_status_reply(status);
}

static cmd_output_t get_debug_ap(const cmd_input_t *in)
{
    if (!has_no_param(in)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }
    return cmd_value_reply(conf_get_debug_ap(), 1);
}

static cmd_output_t set_debug_ap(const cmd_input_t *in)
{
    uint32_t value;
    uint8_t status = cmd_parse_value(in, 1, &value);
    if (status == CMD_STATUS_OK) {
        conf_set_debug_ap((uint8_t)value);
    }
    return cmd_status_reply(status);
}

static cmd_output_t get_swd_freq(const cmd_input_t *in)
{
    if (!has_no_param(in)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }
    return cmd_value_reply(conf_get_swd_freq_hz(), 4);
}

static cmd_output_t set_swd_freq(const cmd_input_t *in)
{
    uint32_t value;
    uint8_t status = cmd_parse_value(in, 4, &value);
    if (status == CMD_STATUS_OK) {
        conf_set_swd_freq_hz(value);
    }
    return cmd_status_reply(status);
}

static cmd_output_t get_jtag_freq(const cmd_input_t *in)
{
    if (!has_no_param(in)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }
    return cmd_value_reply(conf_get_jtag_freq_hz(), 4);
}

static cmd_output_t set_jtag_freq(const cmd_input_t *in)
{
    uint32_t value;
    uint8_t status = cmd_parse_value(in, 4, &value);
    if (status == CMD_STATUS_OK) {
        conf_set_jtag_freq_hz(value);
    }
    return cmd_status_reply(status);
}

static cmd_output_t get_debug_protocol(const cmd_input_t *in)
{
    if (!has_no_param(in)) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }
    return cmd_value_reply(conf_get_debug_protocol(), 1);
}

static cmd_output_t set_debug_protocol(const cmd_input_t *in)
{
    uint32_t value;
    uint8_t status = cmd_parse_value(in, 1, &value);
    if (status != CMD_STATUS_OK) {
        return cmd_status_reply(status);
    }
    if (value != CONF_PROTOCOL_SWD && value != CONF_PROTOCOL_JTAG) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }

    conf_set_debug_protocol((uint8_t)value);
    conf_apply_pins();   /* the pins of the new protocol replace the previous ones */
    return cmd_status_reply(CMD_STATUS_OK);
}

bool config_register(void)
{
    return dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_GET_TRIGGER_WIDTH_US), get_trigger_width)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_SET_TRIGGER_WIDTH_US), set_trigger_width)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_GET_DEBUG_AP), get_debug_ap)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_SET_DEBUG_AP), set_debug_ap)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_GET_SWD_FREQ_HZ), get_swd_freq)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_SET_SWD_FREQ_HZ), set_swd_freq)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_GET_JTAG_FREQ_HZ), get_jtag_freq)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_SET_JTAG_FREQ_HZ), set_jtag_freq)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_GET_DEBUG_PROTOCOL), get_debug_protocol)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_SET_DEBUG_PROTOCOL), set_debug_protocol)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_GET_GPIO_PULSE_WIDTH_US), get_gpio_pulse_width)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_SET_GPIO_PULSE_WIDTH_US), set_gpio_pulse_width)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_GET_GPIO_PULSE_AFTER_US), get_gpio_pulse_after)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_SET_GPIO_PULSE_AFTER_US), set_gpio_pulse_after)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_GET_GPIO_PULSE_LAST_US), get_gpio_pulse_last)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_CONFIG, CONFIG_CMD_SET_GPIO_PULSE_LAST_US), set_gpio_pulse_last);
}
