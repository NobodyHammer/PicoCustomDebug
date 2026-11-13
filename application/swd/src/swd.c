#include <stdbool.h>
#include <stdint.h>

#include "hardware/clocks.h"
#include "pico/stdlib.h"

#include "application/dispatcher.h"
#include "application/swd.h"
#include "generic/cmd.h"
#include "generic/conf.h"
#include "generic/trigger.h"

/* ---- Protocol constants (ADIv5) ---- */
#define DP_IDCODE     0x0u
#define DP_ABORT      0x0u
#define DP_CTRLSTAT   0x4u
#define DP_SELECT     0x8u
#define DP_RDBUFF     0xCu

#define AP_CSW        0x0u
#define AP_TAR        0x4u
#define AP_DRW        0xCu

#define CTRLSTAT_CSYSPWRUPREQ   (1u << 30)
#define CTRLSTAT_CDBGPWRUPREQ   (1u << 28)
#define CTRLSTAT_CSYSPWRUPACK   (1u << 31)
#define CTRLSTAT_CDBGPWRUPACK   (1u << 29)

/* MEM-AP CSW: 32-bit access size. Value to check on hardware (DbgSwEnable and HPROT bits). */
#define AP_CSW_32BIT            0x23000002u

#define ABORT_CLEAR_ALL         0x1Eu

#define ACK_OK                  0x1u
#define ACK_WAIT                0x2u

typedef enum { T_OK, T_WAIT, T_FAULT } transfer_result_t;

/* Cached hardware configuration */
static uint32_t half_cycles;     /* busy-wait cycles per clock half period, 0 = no delay */
static uint8_t ap_sel;          /* debug AP selected in conf */
static bool select_valid;       /* DP SELECT already written for ap_sel */
static bool connected;

static uint32_t half_cycles_for(uint32_t freq_hz)
{
    if (freq_hz == 0) {
        return 0;
    }
    return clock_get_hz(clk_sys) / (2u * freq_hz);
}

/* Reloads what changed in conf since the last command, consuming the dirty flags */
static void reload(void)
{
    /* A protocol change ends the session: the target must be connected again */
    if (conf_is_dirty(CONF_DIRTY_PROTOCOL_SWD)) {
        connected = false;
        select_valid = false;
        conf_clear_dirty(CONF_DIRTY_PROTOCOL_SWD);
    }
    if (conf_is_dirty(CONF_DIRTY_SWD)) {
        half_cycles = half_cycles_for(conf_get_swd_freq_hz());
        conf_clear_dirty(CONF_DIRTY_SWD);
    }
    if (conf_is_dirty(CONF_DIRTY_DEBUG_AP_SWD)) {
        ap_sel = conf_get_debug_ap();
        select_valid = false;
        conf_clear_dirty(CONF_DIRTY_DEBUG_AP_SWD);
    }
}

/* ---- Bit-level primitives ---- */

static void half_clock(void)
{
    if (half_cycles != 0) {
        busy_wait_at_least_cycles(half_cycles);
    }
}

static void clk_set(bool level)
{
    gpio_put(CONF_GPIO_SWD_CLK, level);
}

static void dio_drive(bool level)
{
    gpio_set_dir(CONF_GPIO_SWD_DIO, GPIO_OUT);
    gpio_put(CONF_GPIO_SWD_DIO, level);
}

static void dio_release(void)
{
    gpio_set_dir(CONF_GPIO_SWD_DIO, GPIO_IN);
}

/* SWCLK idles high (as the ST-LINK does): each cycle is a low half-period followed by a high one.
 * The clock ends each cycle high, so the line rests at its idle level between transfers. */

/* One clock cycle without data (turnaround) */
static void clock_cycle(void)
{
    clk_set(false);
    half_clock();
    clk_set(true);
    half_clock();
}

/* Host drives one bit: DIO changes while the clock is high, target samples on the rising edge */
static void write_bit(bool bit)
{
    dio_drive(bit);
    clk_set(false);
    half_clock();
    clk_set(true);
    half_clock();
}

/* Host samples one bit on the rising edge; the target changes it after the falling edge */
static bool read_bit(void)
{
    dio_release();
    clk_set(false);
    half_clock();
    bool bit = gpio_get(CONF_GPIO_SWD_DIO);   /* sampled just before the rising edge */
    clk_set(true);
    half_clock();
    return bit;
}

static void write_bits(uint32_t value, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        write_bit((value >> i) & 1u);
    }
}

static uint32_t read_bits(uint8_t count)
{
    uint32_t value = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (read_bit()) {
            value |= 1u << i;
        }
    }
    return value;
}

/* Even parity over the data word: the parity bit is 1 when the data has an odd number of ones,
 * so that the total number of ones (data + parity) is even */
static bool parity_bit(uint32_t value)
{
    return (__builtin_popcount(value) & 1u) != 0;
}

/* Idle: DIO low for a few cycles */
static void idle(void)
{
    dio_drive(false);
    for (uint8_t i = 0; i < 8; i++) {
        clock_cycle();
    }
}

/* ---- Transfers ---- */

/* One request/response. For a read, *data receives the value; for a write, *data is sent. */
static transfer_result_t transfer_once(bool apndp, bool rnw, uint8_t addr, uint32_t *data)
{
    uint32_t a2 = (addr >> 2) & 1u;
    uint32_t a3 = (addr >> 3) & 1u;
    uint32_t ones = (uint32_t)apndp + (uint32_t)rnw + a2 + a3;
    /* ADIv5 request parity is even: the parity bit is the XOR of the four fields */
    uint32_t parity = ones & 1u;

    /* Request: start, APnDP, RnW, A[2:3], parity, stop (0), park (1) */
    uint32_t request = 1u
                     | ((uint32_t)apndp << 1)
                     | ((uint32_t)rnw << 2)
                     | (a2 << 3)
                     | (a3 << 4)
                     | (parity << 5)
                     | (1u << 7);

    write_bits(request, 8);
    dio_release();
    clock_cycle();                 /* turnaround */
    uint32_t ack = read_bits(3);

    if (ack == ACK_OK) {
        if (rnw) {
            uint32_t value = read_bits(32);
            bool received_parity = read_bit();
            dio_release();
            clock_cycle();         /* turnaround: target released the line */
            if (received_parity != parity_bit(value)) {
                return T_FAULT;
            }
            *data = value;
        } else {
            dio_release();
            clock_cycle();         /* turnaround: host drives the data after it */
            write_bits(*data, 32);
            write_bit(parity_bit(*data));
        }
        idle();
        return T_OK;
    }

    dio_release();
    clock_cycle();
    idle();
    return (ack == ACK_WAIT) ? T_WAIT : T_FAULT;
}

/* One attempt. On a WAIT or FAULT, the sticky errors are cleared and the call fails:
 * retrying is up to the caller. */
static bool transfer(bool apndp, bool rnw, uint8_t addr, uint32_t *data)
{
    transfer_result_t result = transfer_once(apndp, rnw, addr, data);

    if (result == T_OK) {
        return true;
    }
    uint32_t abort = ABORT_CLEAR_ALL;
    transfer_once(false, false, DP_ABORT, &abort);
    return false;
}

/* ---- Session ---- */

static void line_reset(void)
{
    dio_drive(true);
    for (uint8_t i = 0; i < 56; i++) {
        clock_cycle();
    }
}

static bool ensure_select(void)
{
    uint32_t select;

    if (select_valid) {
        return true;
    }
    select = (uint32_t)ap_sel << 24;
    if (!transfer(false, false, DP_SELECT, &select)) {
        return false;
    }
    select_valid = true;
    return true;
}

static bool ap_write(uint8_t addr, uint32_t value)
{
    return ensure_select() && transfer(true, false, addr, &value);
}

static bool ap_read(uint8_t addr, uint32_t *value)
{
    /* AP reads are posted: the value comes with the next DP RDBUFF read */
    return ensure_select() && transfer(true, true, addr, value);
}

/* Reply: IDCODE (4 bytes) on success. One attempt: the power-up is checked once. */
static cmd_output_t swd_connect(void)
{
    uint32_t idcode = 0;
    uint32_t value;
    uint32_t status = 0;

    /* JTAG-to-SWD: line reset, 0xE79E (LSB first), line reset, idle */
    line_reset();
    write_bits(0xE79Eu, 16);
    line_reset();
    idle();

    /* Clear any sticky error left by a previous session before the first access.
     * The result is ignored: a failed clear must not prevent the IDCODE read. */
    {
        uint32_t abort = ABORT_CLEAR_ALL;
        transfer_once(false, false, DP_ABORT, &abort);
    }

    if (!transfer(false, true, DP_IDCODE, &idcode)) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }

    /* Power up debug and system domains, then check the acknowledge once */
    value = CTRLSTAT_CDBGPWRUPREQ | CTRLSTAT_CSYSPWRUPREQ;
    if (!transfer(false, false, DP_CTRLSTAT, &value)) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    if (!transfer(false, true, DP_CTRLSTAT, &status)) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    if ((status & (CTRLSTAT_CDBGPWRUPACK | CTRLSTAT_CSYSPWRUPACK))
        != (CTRLSTAT_CDBGPWRUPACK | CTRLSTAT_CSYSPWRUPACK)) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }

    select_valid = false;
    if (!ensure_select()) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }

    value = AP_CSW_32BIT;
    if (!ap_write(AP_CSW, value)) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    return cmd_value_reply(idcode, 4);
}

/* Reply: 32-bit value on success */
static cmd_output_t mem_read32(uint32_t address, uint8_t positions)
{
    uint32_t value = 0;
    uint32_t dummy;
    bool ok;

    trigger_if(positions, TRIGGER_ADDRESS_BEFORE);
    ok = ap_write(AP_TAR, address);
    trigger_if(positions, TRIGGER_ADDRESS_AFTER);
    ok = ok && ap_read(AP_DRW, &dummy) && transfer(false, true, DP_RDBUFF, &value);
    if (!ok) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    return cmd_value_reply(value, 4);
}

/* Reply: status only */
static cmd_output_t mem_write32(uint32_t address, uint32_t value, uint8_t positions)
{
    bool ok;

    trigger_if(positions, TRIGGER_ADDRESS_BEFORE);
    ok = ap_write(AP_TAR, address);
    trigger_if(positions, TRIGGER_ADDRESS_AFTER);
    if (ok) {
        trigger_if(positions, TRIGGER_VALUE_BEFORE);
        ok = ap_write(AP_DRW, value);
        trigger_if(positions, TRIGGER_VALUE_AFTER);
    }
    return cmd_status_reply(ok ? CMD_STATUS_OK : CMD_STATUS_FAIL);
}

/* ---- Command handlers ---- */

static cmd_output_t connect_handler(const cmd_input_t *in)
{
    if (conf_get_debug_protocol() != CONF_PROTOCOL_SWD) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    cmd_output_t out;

    if (in->data_count != 0) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }

    reload();
    trigger_before(in->trigger);
    out = swd_connect();
    trigger_after(in->trigger);

    connected = (out.status == CMD_STATUS_OK);
    return out;
}

static cmd_output_t read_handler(const cmd_input_t *in)
{
    if (conf_get_debug_protocol() != CONF_PROTOCOL_SWD) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    uint32_t address;
    uint8_t status;
    cmd_output_t out;

    if (!connected) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    status = cmd_parse_value(in, 4, &address);
    if (status != CMD_STATUS_OK) {
        return cmd_status_reply(status);
    }

    reload();
    trigger_before(in->trigger);
    out = mem_read32(address, in->trigger);
    trigger_after(in->trigger);

    return out;
}

static cmd_output_t write_handler(const cmd_input_t *in)
{
    if (conf_get_debug_protocol() != CONF_PROTOCOL_SWD) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    uint32_t address;
    uint32_t value;
    cmd_output_t out;

    if (!connected) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    if (in->data_count == 0) {
        return cmd_status_reply(CMD_STATUS_MISSING_PARAM);
    }
    if (in->data_count != 2 || in->data[0].len != 4 || in->data[1].len != 4) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }

    address = ((uint32_t)in->data[0].buf[0] << 24) | ((uint32_t)in->data[0].buf[1] << 16)
            | ((uint32_t)in->data[0].buf[2] << 8) | in->data[0].buf[3];
    value = ((uint32_t)in->data[1].buf[0] << 24) | ((uint32_t)in->data[1].buf[1] << 16)
          | ((uint32_t)in->data[1].buf[2] << 8) | in->data[1].buf[3];

    reload();
    trigger_before(in->trigger);
    out = mem_write32(address, value, in->trigger);
    trigger_after(in->trigger);

    return out;
}

bool swd_register(void)
{
    half_cycles = half_cycles_for(conf_get_swd_freq_hz());
    ap_sel = conf_get_debug_ap();
    select_valid = false;
    connected = false;
    conf_clear_dirty(CONF_DIRTY_SWD | CONF_DIRTY_DEBUG_AP_SWD | CONF_DIRTY_PROTOCOL_SWD);

    return dispatcher_register(CMD_MAKE(CMD_MODULE_SWD, SWD_CMD_CONNECT), connect_handler)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_SWD, SWD_CMD_READ), read_handler)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_SWD, SWD_CMD_WRITE), write_handler);
}
