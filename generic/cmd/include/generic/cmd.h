#ifndef GENERIC_CMD_H
#define GENERIC_CMD_H

#include <stdint.h>

/* Generic module: cmd. Types and status codes describing command inputs and outputs. */

/* Maximum number of data entries in one command input or output */
#define CMD_MAX_DATA      4u

/* Maximum payload size of one data entry, in bytes */
#define CMD_DATA_MAX_LEN  4u

/* ---- Command key ----
 * A command is identified by its module (CLA) and its command number (INS), as in ISO 7816.
 * The key is a 16-bit value: high byte = module, low byte = command. Each module can hold 256 commands.
 * Only application modules are addressable. */
#define CMD_MAKE(module, command)  ((uint16_t)((((uint16_t)(module) & 0xFFu) << 8) | ((uint16_t)(command) & 0xFFu)))
#define CMD_MODULE_OF(key)         ((uint8_t)(((key) >> 8) & 0xFFu))
#define CMD_COMMAND_OF(key)        ((uint8_t)((key) & 0xFFu))

/* Module identifiers (high nibble) */
#define CMD_MODULE_SYSTEM     0x0u
#define CMD_MODULE_SWD        0x1u
#define CMD_MODULE_JTAG       0x2u
#define CMD_MODULE_GPIO       0x3u
#define CMD_MODULE_TIMER      0x4u
#define CMD_MODULE_SEQUENCE   0x5u
#define CMD_MODULE_CONFIG     0x6u

/* Status codes returned in cmd_output_t.status (hardcoded protocol values).
 * Values are randomly chosen with a minimum Hamming distance of 4 between them,
 * so a single corrupted bit cannot turn one status into another. 0x00 and 0xFF are
 * avoided so that stuck lines or an empty buffer never read as a valid status. */
#define CMD_STATUS_OK               0x47u  /* Command executed successfully */
#define CMD_STATUS_FAIL             0xA1u  /* Command executed but failed */
#define CMD_STATUS_TIMEOUT          0x1Eu  /* Command did not complete in time */
#define CMD_STATUS_BAD_COMMAND      0x72u  /* Command byte is unknown */
#define CMD_STATUS_MISSING_PARAM    0x59u  /* A required data entry is missing */
#define CMD_STATUS_UNEXPECTED_PARAM 0xCCu  /* A data entry was not expected */

/* One data entry: a length followed by up to 4 bytes */
typedef struct {
    uint8_t len;                        /* Number of valid bytes in buf (0..CMD_DATA_MAX_LEN) */
    uint8_t buf[CMD_DATA_MAX_LEN];
} cmd_data_t;

/* Command input */
typedef struct {
    uint8_t module;                     /* Module (CLA), see CMD_MAKE */
    uint8_t cmd;                        /* Command number within the module (INS) */
    uint8_t p1;                         /* Parameter 1: selects a variant of the command, 0 = default */
    uint8_t p2;                         /* Parameter 2: refines the variant selected by p1, 0 = default */
    uint8_t trigger;                    /* Trigger byte */
    uint8_t data_count;                 /* Number of valid entries in data (0..CMD_MAX_DATA) */
    cmd_data_t data[CMD_MAX_DATA];
} cmd_input_t;

/* Command output */
typedef struct {
    uint8_t status;                     /* One of the CMD_STATUS_* values */
    uint8_t data_count;                 /* Number of valid entries in data (0..CMD_MAX_DATA) */
    cmd_data_t data[CMD_MAX_DATA];
} cmd_output_t;

/* ---- Helpers shared by the command handlers ---- */

/* Reply with a status and no data */
cmd_output_t cmd_status_reply(uint8_t status);

/* Reply with one value of size bytes (1..4), big-endian, in data[0] */
cmd_output_t cmd_value_reply(uint32_t value, uint8_t size);

/* Reads the single value of an input (data[0] must be size bytes, big-endian).
 * Returns CMD_STATUS_OK, CMD_STATUS_MISSING_PARAM or CMD_STATUS_UNEXPECTED_PARAM. */
uint8_t cmd_parse_value(const cmd_input_t *in, uint8_t size, uint32_t *value);

#endif /* GENERIC_CMD_H */
