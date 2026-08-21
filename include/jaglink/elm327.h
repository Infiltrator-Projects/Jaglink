// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327.h
 * @brief Portable ELM327 command and response engine.
 *
 * The ELM327 layer owns serial-style command framing, response accumulation,
 * prompt detection, normalisation, adapter-status classification and the
 * deterministic initialisation sequence. It does not know about BLE, Wi-Fi,
 * sockets or any other concrete transport.
 */
#ifndef JAGLINK_ELM327_H
#define JAGLINK_ELM327_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_ELM327_MAX_COMMAND 64U
#define JAGLINK_ELM327_MAX_RESPONSE 4096U
#define JAGLINK_ELM327_MAX_ADAPTER_ID 96U

typedef enum {
    JAGLINK_ELM327_RESULT_OK = 0,
    JAGLINK_ELM327_RESULT_MORE_DATA,
    JAGLINK_ELM327_RESULT_INVALID_ARGUMENT,
    JAGLINK_ELM327_RESULT_COMMAND_TOO_LONG,
    JAGLINK_ELM327_RESULT_RESPONSE_TOO_LONG,
    JAGLINK_ELM327_RESULT_NO_DATA,
    JAGLINK_ELM327_RESULT_STOPPED,
    JAGLINK_ELM327_RESULT_UNABLE_TO_CONNECT,
    JAGLINK_ELM327_RESULT_BUS_INIT_ERROR,
    JAGLINK_ELM327_RESULT_CAN_ERROR,
    JAGLINK_ELM327_RESULT_BUFFER_FULL,
    JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND,
    JAGLINK_ELM327_RESULT_ADAPTER_ERROR,
    JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE
} JaglinkElm327Result;

typedef struct {
    JaglinkElm327Result result;
    bool prompt_seen;
    bool echo_removed;
    bool searching_seen;
    bool ok_seen;
    size_t line_count;
    size_t length;
    char text[JAGLINK_ELM327_MAX_RESPONSE];
} JaglinkElm327Response;

typedef struct {
    char command[JAGLINK_ELM327_MAX_COMMAND];
    uint8_t raw[JAGLINK_ELM327_MAX_RESPONSE];
    size_t raw_length;
    bool prompt_seen;
    bool overflowed;
} JaglinkElm327Parser;

typedef enum {
    JAGLINK_ELM327_INIT_RESET = 0,
    JAGLINK_ELM327_INIT_ECHO_OFF,
    JAGLINK_ELM327_INIT_LINEFEEDS_OFF,
    JAGLINK_ELM327_INIT_SPACES_OFF,
    JAGLINK_ELM327_INIT_HEADERS_OFF,
    JAGLINK_ELM327_INIT_PROTOCOL_AUTO,
    JAGLINK_ELM327_INIT_IDENTIFY,
    JAGLINK_ELM327_INIT_COMPLETE,
    JAGLINK_ELM327_INIT_FAILED
} JaglinkElm327InitStage;

typedef struct {
    JaglinkElm327InitStage stage;
    JaglinkElm327Result failure;
    char adapter_id[JAGLINK_ELM327_MAX_ADAPTER_ID];
} JaglinkElm327InitState;

/** Return a stable human-readable name for an ELM327 result. */
const char *jaglink_elm327_result_name(JaglinkElm327Result result);

/**
 * Validate and frame an ELM327/OBD command for transmission.
 *
 * Leading/trailing ASCII whitespace is removed and exactly one carriage
 * return is appended. Embedded CR/LF bytes, the ELM prompt character and
 * non-printable bytes are rejected.
 */
JaglinkElm327Result jaglink_elm327_build_command(const char *command,
                                                uint8_t *buffer,
                                                size_t buffer_size,
                                                size_t *written);

/** Initialise a response accumulator for one outstanding command. */
JaglinkElm327Result jaglink_elm327_parser_begin(JaglinkElm327Parser *parser,
                                               const char *command);

/**
 * Feed arbitrarily fragmented transport bytes into the parser.
 *
 * Returns MORE_DATA until a '>' prompt is seen, OK when a complete response
 * has been accumulated, or RESPONSE_TOO_LONG if the bounded buffer overflows.
 * `consumed` reports bytes consumed from this fragment; bytes after the prompt
 * remain the caller's responsibility and are never silently discarded.
 */
JaglinkElm327Result jaglink_elm327_parser_feed(JaglinkElm327Parser *parser,
                                              const uint8_t *data,
                                              size_t size,
                                              size_t *consumed);

/** Normalise and classify a complete accumulated ELM327 response. */
JaglinkElm327Result jaglink_elm327_parser_finish(const JaglinkElm327Parser *parser,
                                                JaglinkElm327Response *response);

/** Initialise the deterministic adapter setup state machine. */
void jaglink_elm327_init_begin(JaglinkElm327InitState *state);

/** Return the command required by the current initialisation stage. */
const char *jaglink_elm327_init_command(const JaglinkElm327InitState *state);

/**
 * Advance initialisation after receiving a parsed response.
 *
 * Reset/identity responses may contain text; configuration steps require OK.
 * Any classified adapter error moves the state to FAILED.
 */
JaglinkElm327Result jaglink_elm327_init_accept(JaglinkElm327InitState *state,
                                              const JaglinkElm327Response *response);

#ifdef __cplusplus
}
#endif

#endif
