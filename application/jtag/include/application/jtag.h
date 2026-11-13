#ifndef APPLICATION_JTAG_H
#define APPLICATION_JTAG_H

#include <stdbool.h>

/* Application module: jtag (module nibble 0x2). JTAG bit-banged on CONF_GPIO_JTAG_TCK/TMS/TDI/TDO
 * (TDI driven by the probe, TDO read from the target).
 *
 *   JTAG_CMD_CONNECT: TAP reset, IDCODE read, DP power-up, select of the debug AP.
 *                     Reply: IDCODE (4 bytes) in data[0].
 *   JTAG_CMD_READ:    data[0] = target address (4 bytes). Reply: 32-bit value in data[0].
 *   JTAG_CMD_WRITE:   data[0] = target address (4 bytes), data[1] = value (4 bytes).
 *
 * Access to the DP/AP uses the ARM DPACC (IR 0xA) and APACC (IR 0xB) registers, with a 9-bit IR scan
 * (CoreSight instruction + 5 bits of BYPASS) and 36-clock DR frames (35 bits + 1 BYPASS bit).
 * Each command makes one attempt: a failure is reported with CMD_STATUS_FAIL and retries are up to the host.
 * Clock frequency and AP come from conf and are reloaded when their dirty flags are raised. */

#define JTAG_CMD_CONNECT 0x1u
#define JTAG_CMD_READ    0x2u
#define JTAG_CMD_WRITE   0x3u

/* Configures the JTAG pins and registers the handlers. Call after dispatcher_init(). */
bool jtag_register(void);

#endif /* APPLICATION_JTAG_H */
