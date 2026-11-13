#ifndef APPLICATION_SWD_H
#define APPLICATION_SWD_H

#include <stdbool.h>

/* Application module: swd (module nibble 0x1). SWD bit-banged on CONF_GPIO_SWD_CLK / CONF_GPIO_SWD_DIO.
 *
 *   SWD_CMD_CONNECT: line reset, JTAG-to-SWD switch, DP power-up, select of the debug AP.
 *                    Reply: IDCODE (4 bytes) in data[0].
 *   SWD_CMD_READ:    data[0] = target address (4 bytes). Reply: 32-bit value in data[0].
 *   SWD_CMD_WRITE:   data[0] = target address (4 bytes), data[1] = value (4 bytes).
 *
 * Read and write go through the MEM-AP of the debug AP selected in conf.
 * Clock frequency and AP come from conf and are reloaded when their dirty flags are raised. */

#define SWD_CMD_CONNECT 0x1u
#define SWD_CMD_READ    0x2u
#define SWD_CMD_WRITE   0x3u

/* Configures the SWD pins and registers the handlers. Call after dispatcher_init(). */
bool swd_register(void);

#endif /* APPLICATION_SWD_H */
