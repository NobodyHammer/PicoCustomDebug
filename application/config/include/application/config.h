#ifndef APPLICATION_CONFIG_H
#define APPLICATION_CONFIG_H

#include <stdbool.h>

/* Application module: config (module nibble 0x6). Get/set of the global settings.
 * Setting a value raises the matching dirty flag in generic/conf. Values are big-endian:
 * 4 bytes for trigger width and frequencies, 1 byte for the debug AP. */

/* Command numbers (low nibble), same values as ConfigCommand in python/PycoDebug/commands.py */
#define CONFIG_CMD_GET_TRIGGER_WIDTH_US 0x0u
#define CONFIG_CMD_SET_TRIGGER_WIDTH_US 0x1u
#define CONFIG_CMD_GET_DEBUG_AP         0x2u
#define CONFIG_CMD_SET_DEBUG_AP         0x3u
#define CONFIG_CMD_GET_SWD_FREQ_HZ      0x4u
#define CONFIG_CMD_SET_SWD_FREQ_HZ      0x5u
#define CONFIG_CMD_GET_JTAG_FREQ_HZ     0x6u
#define CONFIG_CMD_SET_JTAG_FREQ_HZ     0x7u
#define CONFIG_CMD_GET_DEBUG_PROTOCOL   0x8u
#define CONFIG_CMD_SET_DEBUG_PROTOCOL   0x9u
#define CONFIG_CMD_GET_GPIO_PULSE_WIDTH_US 0xAu
#define CONFIG_CMD_SET_GPIO_PULSE_WIDTH_US 0xBu
#define CONFIG_CMD_GET_GPIO_PULSE_AFTER_US 0xCu
#define CONFIG_CMD_SET_GPIO_PULSE_AFTER_US 0xDu
#define CONFIG_CMD_GET_GPIO_PULSE_LAST_US  0xEu
#define CONFIG_CMD_SET_GPIO_PULSE_LAST_US  0xFu

/* Registers the config handlers in the dispatcher. Call after dispatcher_init(). */
bool config_register(void);

#endif /* APPLICATION_CONFIG_H */
