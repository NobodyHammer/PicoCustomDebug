#include <stdbool.h>
#include <stdio.h>

#include "pico/stdlib.h"

#include "application/config.h"
#include "application/dispatcher.h"
#include "application/gpio.h"
#include "application/jtag.h"
#include "application/link.h"
#include "application/swd.h"
#include "application/system.h"
#include "application/timer.h"
#include "generic/conf.h"
#include "generic/pin.h"
#include "generic/protocol.h"
#include "generic/trigger.h"

/* Registers every command handler. Returns false if the dispatcher table is full. */
static bool register_commands(void)
{
    return system_register()
        && config_register()
        && gpio_register()
        && timer_register()
        && swd_register()
        && jtag_register();
}

int main(void)
{
    uint8_t rx[PROTO_FRAME_MAX];
    uint8_t tx[PROTO_FRAME_MAX];

    stdio_init_all();

    conf_apply_pins();

    trigger_init();
    dispatcher_init();
    if (!register_commands()) {
        while (true) {
            sleep_ms(1000);
        }
    }

    while (true) {
        cmd_input_t in;
        cmd_output_t out;
        size_t rx_len = 0;

        if (!link_read_frame(rx, sizeof rx, &rx_len)) {
            continue;
        }

        /* Bad CRC or bad layout: no reply, the PC times out and retries */
        if (!proto_decode_input(rx, rx_len, &in)) {
            continue;
        }

        dispatcher_dispatch(&in, &out);

        size_t tx_len = proto_encode_output(&out, tx, sizeof tx);
        if (tx_len > 0) {
            link_write_frame(tx, tx_len);
        }
    }

    return 0;
}
