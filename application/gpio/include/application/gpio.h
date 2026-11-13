#ifndef APPLICATION_GPIO_H
#define APPLICATION_GPIO_H

#include <stdbool.h>
#include <stdint.h>

/* Application module: gpio (module nibble 0x3). Set, reset and toggle of the RST and EXT pins.
 * set, reset and toggle take one data byte: the pin selector below.
 * pulse takes no data: P1 is a pin mask (GPIO_PULSE_*). P2 is 0 (no last delay) or 1 (last delay
 * from generic/conf, with the trigger just before it). Other P2 values are refused. */

#define GPIO_CMD_SET    0x1u
#define GPIO_CMD_RESET  0x2u
#define GPIO_CMD_TOGGLE 0x3u
#define GPIO_CMD_PULSE  0x4u   /* P1 = pin mask (GPIO_PULSE_*), no data */

#define GPIO_SEL_RST    0x0u   /* CONF_GPIO_RST */
#define GPIO_SEL_EXT0   0x1u   /* CONF_GPIO_EXT0 */
#define GPIO_SEL_EXT1   0x2u   /* CONF_GPIO_EXT1 */
#define GPIO_SEL_EXT2   0x3u   /* CONF_GPIO_EXT2 */
#define GPIO_SEL_TRST   0x5u   /* CONF_GPIO_JTAG_TRST, active low (output in JTAG mode only) */

/* Pulse pin mask, in P1 of GPIO_CMD_PULSE: one bit per pin, all pulsed together.
 * Timing comes from generic/conf: gpio_pulse_width_us (low) and gpio_pulse_after_us (after high). */
#define GPIO_PULSE_RST   (1u << 0)
#define GPIO_PULSE_EXT0  (1u << 1)
#define GPIO_PULSE_EXT1  (1u << 2)
#define GPIO_PULSE_EXT2  (1u << 3)
#define GPIO_PULSE_TRST  (1u << 4)
#define GPIO_PULSE_MASK_ALL (GPIO_PULSE_RST | GPIO_PULSE_EXT0 | GPIO_PULSE_EXT1 | GPIO_PULSE_EXT2 | GPIO_PULSE_TRST)

/* Configures the RST and EXT pins as outputs and registers the handlers.
 * Call after dispatcher_init(). */
bool gpio_register(void);

#endif /* APPLICATION_GPIO_H */
