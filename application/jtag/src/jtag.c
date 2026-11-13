#include <stdbool.h>
#include <stdint.h>

#include "hardware/clocks.h"
#include "pico/stdlib.h"

#include "application/dispatcher.h"
#include "application/jtag.h"
#include "generic/cmd.h"
#include "generic/conf.h"
#include "generic/trigger.h"

/* ---- Protocol constants (ARM JTAG-DP, 4-bit IR) ---- */
#define IR_DPACC     0xAu
#define IR_APACC     0xBu
#define IR_IDCODE    0xEu

#define DP_ABORT     0x0u
#define DP_CTRLSTAT  0x4u
#define DP_SELECT    0x8u
#define DP_RDBUFF    0xCu

#define AP_CSW       0x0u
#define AP_TAR       0x4u
#define AP_DRW       0xCu

#define CTRLSTAT_CSYSPWRUPREQ   (1u << 30)
#define CTRLSTAT_CDBGPWRUPREQ   (1u << 28)
#define CTRLSTAT_CSYSPWRUPACK   (1u << 31)
#define CTRLSTAT_CDBGPWRUPACK   (1u << 29)

/* MEM-AP CSW: 32-bit access size. Value to check on hardware. */
#define AP_CSW_32BIT            0x23000002u

#define ABORT_CLEAR_ALL         0x1Eu

/* 35-bit DPACC/APACC response: ACK in bits 2..0, data in bits 34..3 */
#define ACK_OK                  0x2u

/* Cached hardware configuration */
static uint32_t half_cycles;   /* busy-wait cycles per TCK half period, 0 = no delay */
static uint8_t ap_sel;
static bool select_valid;
static bool connected;

static uint32_t half_cycles_for(uint32_t freq_hz)
{
    if (freq_hz == 0) {
        return 0;
    }
    return clock_get_hz(clk_sys) / (2u * freq_hz);
}

static void reload(void)
{
    /* A protocol change ends the session: the target must be connected again */
    if (conf_is_dirty(CONF_DIRTY_PROTOCOL_JTAG)) {
        connected = false;
        select_valid = false;
        conf_clear_dirty(CONF_DIRTY_PROTOCOL_JTAG);
    }
    if (conf_is_dirty(CONF_DIRTY_JTAG)) {
        half_cycles = half_cycles_for(conf_get_jtag_freq_hz());
        conf_clear_dirty(CONF_DIRTY_JTAG);
    }
    if (conf_is_dirty(CONF_DIRTY_DEBUG_AP_JTAG)) {
        ap_sel = conf_get_debug_ap();
        select_valid = false;
        conf_clear_dirty(CONF_DIRTY_DEBUG_AP_JTAG);
    }
}

static void half_clock(void)
{
    if (half_cycles != 0) {
        busy_wait_at_least_cycles(half_cycles);
    }
}

/* One TCK cycle. TCK idles high: it goes low, TDI and TMS are set while it is low,
 * TDO is sampled just before the rising edge, then TCK returns high. */
static bool clock_bit(bool tdi, bool tms)
{
    bool tdo;

    gpio_put(CONF_GPIO_JTAG_TCK, false);
    gpio_put(CONF_GPIO_JTAG_TDI, tdi);
    gpio_put(CONF_GPIO_JTAG_TMS, tms);
    half_clock();
    tdo = gpio_get(CONF_GPIO_JTAG_TDO);
    gpio_put(CONF_GPIO_JTAG_TCK, true);
    half_clock();
    return tdo;
}

/* From any state: five TMS=1 cycles reach Test-Logic-Reset, one TMS=0 goes to Run-Test/Idle */
static void tap_reset(void)
{
    for (uint8_t i = 0; i < 5; i++) {
        clock_bit(false, true);
    }
    clock_bit(false, false);
}

/* IR scan length of the reference sequence: 9 bits. The first 4 bits shifted (0xE, LSB first)
 * are the CoreSight JTAG-DP instruction, the following 5 bits are ones (BYPASS of the other TAP).
 * Matches the reference TDI: 0,1,1,1,1,1,1,1,1. */
#define JTAG_IR_SCAN_BITS   9u
#define JTAG_IR_BYPASS_LEN  5u

/* From Run-Test/Idle: loads ir first in the shift order, followed by BYPASS ones, back to Run-Test/Idle */
static void shift_ir(uint8_t ir)
{
    uint16_t value = (uint16_t)((((1u << JTAG_IR_BYPASS_LEN) - 1u) << 4) | ir);

    clock_bit(false, true);    /* Select-DR-Scan */
    clock_bit(false, true);    /* Select-IR-Scan */
    clock_bit(false, false);   /* Capture-IR */
    clock_bit(false, false);   /* Shift-IR */
    for (uint8_t i = 0; i < JTAG_IR_SCAN_BITS; i++) {
        bool last = (i == JTAG_IR_SCAN_BITS - 1u);
        clock_bit((value >> i) & 1u, last);
    }
    clock_bit(false, true);    /* Update-IR */
    clock_bit(false, false);   /* Run-Test/Idle */
}

/* From Run-Test/Idle: shifts count bits of value, LSB first, returns the bits captured from TDO */
static uint64_t shift_dr(uint64_t value, uint8_t count)
{
    uint64_t captured = 0;

    clock_bit(false, true);    /* Select-DR-Scan */
    clock_bit(false, false);   /* Capture-DR */
    clock_bit(false, false);   /* Shift-DR */
    for (uint8_t i = 0; i < count; i++) {
        bool last = (i == count - 1u);
        if (clock_bit((value >> i) & 1u, last)) {
            captured |= (uint64_t)1u << i;
        }
    }
    clock_bit(false, true);    /* Update-DR */
    clock_bit(false, false);   /* Run-Test/Idle */
    return captured;
}

/* DPACC/APACC frame: 35 bits. The BYPASS bit of the other TAP sits between TDI and the DP,
 * so one extra clock is needed to push the whole frame into the DP. That bit is sent as 0
 * and the response bit it returns is ignored. */
#define DP_FRAME_BITS       35u
#define JTAG_DR_BYPASS_BITS 1u

/* DPACC/APACC access. The response is the result of the previous access, so reads
 * need a following RDBUFF read to get their value. */
static uint64_t access(bool apndp, bool rnw, uint8_t addr, uint32_t data)
{
    uint64_t a_field = (addr >> 2) & 3u;
    uint64_t request = (uint64_t)(rnw ? 1u : 0u) | (a_field << 1) | ((uint64_t)data << 3);
    uint64_t response;

    shift_ir(apndp ? IR_APACC : IR_DPACC);
    response = shift_dr(request, DP_FRAME_BITS + JTAG_DR_BYPASS_BITS);
    return response & ((1ull << DP_FRAME_BITS) - 1u);
}

static bool dp_write(uint8_t addr, uint32_t value)
{
    access(false, false, addr, value);
    return true;
}

static bool dp_read(uint8_t addr, uint32_t *value)
{
    access(false, true, addr, 0);
    uint64_t response = access(false, true, DP_RDBUFF, 0);
    *value = (uint32_t)(response >> 3);
    return (response & 7u) == ACK_OK;
}

static bool ensure_select(void)
{
    if (select_valid) {
        return true;
    }
    dp_write(DP_SELECT, (uint32_t)ap_sel << 24);
    select_valid = true;
    return true;
}

static bool ap_write(uint8_t addr, uint32_t value)
{
    ensure_select();
    access(true, false, addr, value);
    return true;
}

/* Reply: IDCODE (4 bytes) on success. One attempt: the power-up is checked once. */
static cmd_output_t jtag_connect(void)
{
    uint32_t idcode;
    uint32_t status = 0;

    tap_reset();
    shift_ir(IR_IDCODE);
    idcode = (uint32_t)shift_dr(0, 32);

    /* Power up debug and system domains, then check the acknowledge once */
    dp_write(DP_CTRLSTAT, CTRLSTAT_CDBGPWRUPREQ | CTRLSTAT_CSYSPWRUPREQ);
    if (!dp_read(DP_CTRLSTAT, &status)) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    if ((status & (CTRLSTAT_CDBGPWRUPACK | CTRLSTAT_CSYSPWRUPACK))
        != (CTRLSTAT_CDBGPWRUPACK | CTRLSTAT_CSYSPWRUPACK)) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }

    select_valid = false;
    ensure_select();
    ap_write(AP_CSW, AP_CSW_32BIT);
    return cmd_value_reply(idcode, 4);
}

/* Reply: 32-bit value on success, FAIL when the access is not acknowledged */
static cmd_output_t jtag_mem_read32(uint32_t address, uint8_t positions)
{
    uint32_t value;

    trigger_if(positions, TRIGGER_ADDRESS_BEFORE);
    ap_write(AP_TAR, address);
    trigger_if(positions, TRIGGER_ADDRESS_AFTER);
    access(true, true, AP_DRW, 0);          /* posted read, value comes with RDBUFF */
    uint64_t response = access(false, true, DP_RDBUFF, 0);
    value = (uint32_t)(response >> 3);
    if ((response & 7u) != ACK_OK) {
        dp_write(DP_ABORT, ABORT_CLEAR_ALL);
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    return cmd_value_reply(value, 4);
}

/* Reply: status only */
static cmd_output_t jtag_mem_write32(uint32_t address, uint32_t value, uint8_t positions)
{
    bool ok;

    trigger_if(positions, TRIGGER_ADDRESS_BEFORE);
    ap_write(AP_TAR, address);
    trigger_if(positions, TRIGGER_ADDRESS_AFTER);
    trigger_if(positions, TRIGGER_VALUE_BEFORE);
    ok = ap_write(AP_DRW, value);
    trigger_if(positions, TRIGGER_VALUE_AFTER);
    return cmd_status_reply(ok ? CMD_STATUS_OK : CMD_STATUS_FAIL);
}

/* ---- Command handlers ---- */

static cmd_output_t connect_handler(const cmd_input_t *in)
{
    if (conf_get_debug_protocol() != CONF_PROTOCOL_JTAG) {
        return cmd_status_reply(CMD_STATUS_FAIL);
    }
    cmd_output_t out;

    if (in->data_count != 0) {
        return cmd_status_reply(CMD_STATUS_UNEXPECTED_PARAM);
    }

    reload();
    trigger_before(in->trigger);
    out = jtag_connect();
    trigger_after(in->trigger);

    connected = (out.status == CMD_STATUS_OK);
    return out;
}

static cmd_output_t read_handler(const cmd_input_t *in)
{
    if (conf_get_debug_protocol() != CONF_PROTOCOL_JTAG) {
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
    out = jtag_mem_read32(address, in->trigger);
    trigger_after(in->trigger);

    return out;
}

static cmd_output_t write_handler(const cmd_input_t *in)
{
    if (conf_get_debug_protocol() != CONF_PROTOCOL_JTAG) {
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
    out = jtag_mem_write32(address, value, in->trigger);
    trigger_after(in->trigger);

    return out;
}

bool jtag_register(void)
{
    half_cycles = half_cycles_for(conf_get_jtag_freq_hz());
    ap_sel = conf_get_debug_ap();
    select_valid = false;
    connected = false;
    conf_clear_dirty(CONF_DIRTY_JTAG | CONF_DIRTY_DEBUG_AP_JTAG | CONF_DIRTY_PROTOCOL_JTAG);

    return dispatcher_register(CMD_MAKE(CMD_MODULE_JTAG, JTAG_CMD_CONNECT), connect_handler)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_JTAG, JTAG_CMD_READ), read_handler)
        && dispatcher_register(CMD_MAKE(CMD_MODULE_JTAG, JTAG_CMD_WRITE), write_handler);
}
