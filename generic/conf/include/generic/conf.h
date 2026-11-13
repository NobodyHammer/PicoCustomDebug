#ifndef GENERIC_CONF_H
#define GENERIC_CONF_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "generic/pin.h"

/* Generic module: conf. Global project settings. */

/* ---- Default values, used until a setter changes them ---- */
#define CONF_DEFAULT_TRIGGER_WIDTH_US  10u        /* Trigger pulse width, in microseconds */
#define CONF_DEFAULT_GPIO_PULSE_WIDTH_US 10u      /* GPIO pulse: time the pins stay low, in microseconds */
#define CONF_DEFAULT_GPIO_PULSE_AFTER_US 10u      /* GPIO pulse: wait after the pins go back high, in microseconds */
#define CONF_DEFAULT_GPIO_PULSE_LAST_US  0u       /* GPIO pulse: last wait before the trigger, in microseconds */
#define CONF_DEFAULT_DEBUG_AP          0u         /* Access Port selected for debug */
#define CONF_DEFAULT_SWD_FREQ_HZ       1000000u   /* SWD clock, in Hz */
#define CONF_DEFAULT_JTAG_FREQ_HZ      1000000u   /* JTAG clock, in Hz */

/* ---- GPIO mapping (Raspberry Pi Pico 2 GPIO numbers) ----
 * Placeholder values: to be checked against the PCB. */
#define CONF_GPIO_TRIGGER     0u   /* Trigger output */

#define CONF_GPIO_SWD_CLK     CONF_GPIO_JTAG_TCK   /* SWD port: SWCLK, same pin as JTAG TCK */
#define CONF_GPIO_SWD_DIO     CONF_GPIO_JTAG_TMS   /* SWD port: SWDIO, same pin as JTAG TMS */

#define CONF_GPIO_JTAG_TCK    6u   /* JTAG port: TCK */
#define CONF_GPIO_JTAG_TMS    7u   /* JTAG port: TMS */
#define CONF_GPIO_JTAG_TDI    8u   /* Target TDI: driven by the probe */
#define CONF_GPIO_JTAG_TDO    9u   /* Target TDO: read by the probe */

#define CONF_GPIO_RST        10u   /* Target NRST, active low (wired to the target reset pin) */
#define CONF_GPIO_JTAG_TRST  12u   /* JTAG TRST: dedicated output, active low (pico pin 16) */

#define CONF_GPIO_EXT_COUNT   3u
#define CONF_GPIO_EXT0       16u   /* EXT general purpose GPIO 0 */
#define CONF_GPIO_EXT1       17u   /* EXT general purpose GPIO 1 */
#define CONF_GPIO_EXT2       18u   /* EXT general purpose GPIO 2 */

/* ---- Debug protocol ----
 * Selects SWD or JTAG. The GPIO directions and pulls of each protocol are applied by
 * conf_apply_pins(): called at boot and after every protocol change. */
#define CONF_PROTOCOL_SWD        0u
#define CONF_PROTOCOL_JTAG       1u
#define CONF_DEFAULT_PROTOCOL    CONF_PROTOCOL_SWD

/* Applies the GPIO configuration of the selected protocol and the common pins */
void conf_apply_pins(void);

/* ---- Accessors ---- */

/* Trigger pulse width, in microseconds */
uint32_t conf_get_trigger_width_us(void);
void conf_set_trigger_width_us(uint32_t width_us);

/* GPIO pulse: time the selected pins stay low, in microseconds */
uint32_t conf_get_gpio_pulse_width_us(void);
void conf_set_gpio_pulse_width_us(uint32_t width_us);

/* GPIO pulse: wait after the selected pins go back high, in microseconds */
uint32_t conf_get_gpio_pulse_after_us(void);
void conf_set_gpio_pulse_after_us(uint32_t after_us);

/* GPIO pulse: last wait, after the after-delay and before the closing trigger, in microseconds */
uint32_t conf_get_gpio_pulse_last_us(void);
void conf_set_gpio_pulse_last_us(uint32_t last_us);

/* Access Port (AP) index selected for debug */
uint8_t conf_get_debug_ap(void);
void conf_set_debug_ap(uint8_t ap);

/* SWD clock frequency, in Hz */
uint32_t conf_get_swd_freq_hz(void);
void conf_set_swd_freq_hz(uint32_t freq_hz);

/* Debug protocol: CONF_PROTOCOL_SWD or CONF_PROTOCOL_JTAG */
uint8_t conf_get_debug_protocol(void);
void conf_set_debug_protocol(uint8_t protocol);

/* JTAG clock frequency, in Hz */
uint32_t conf_get_jtag_freq_hz(void);
void conf_set_jtag_freq_hz(uint32_t freq_hz);

/* ---- Dirty flags ----
 * A setter raises the flag of the settings it changes (only if the value actually changes).
 * An application reloads its hardware configuration when its flag is raised, then clears it. */
typedef uint32_t conf_dirty_t;

#define CONF_DIRTY_TRIGGER  (1u << 0)   /* trigger width changed */
#define CONF_DIRTY_DEBUG_AP_SWD  (1u << 4)   /* debug AP changed, seen by SWD */
#define CONF_DIRTY_DEBUG_AP_JTAG (1u << 5)   /* debug AP changed, seen by JTAG */
#define CONF_DIRTY_DEBUG_AP      (CONF_DIRTY_DEBUG_AP_SWD | CONF_DIRTY_DEBUG_AP_JTAG)  /* set by the setter */
#define CONF_DIRTY_SWD      (1u << 2)   /* SWD frequency changed */
#define CONF_DIRTY_JTAG     (1u << 3)   /* JTAG frequency changed */
#define CONF_DIRTY_PROTOCOL_SWD  (1u << 6)   /* protocol changed, seen by SWD */
#define CONF_DIRTY_PROTOCOL_JTAG (1u << 7)   /* protocol changed, seen by JTAG */
#define CONF_DIRTY_PROTOCOL      (CONF_DIRTY_PROTOCOL_SWD | CONF_DIRTY_PROTOCOL_JTAG)

/* True if at least one flag of mask is raised */
bool conf_is_dirty(conf_dirty_t mask);

/* Clears the flags of mask */
void conf_clear_dirty(conf_dirty_t mask);

#endif /* GENERIC_CONF_H */
