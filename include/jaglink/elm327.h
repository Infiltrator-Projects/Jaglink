// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327.h
 * @brief JAGLINK compatibility facade for LINK's shared ELM327 engine.
 */
#ifndef JAGLINK_ELM327_H
#define JAGLINK_ELM327_H

#include "link/elm327.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_ELM327_MAX_COMMAND LINK_ELM327_MAX_COMMAND
#define JAGLINK_ELM327_MAX_RESPONSE LINK_ELM327_MAX_RESPONSE
#define JAGLINK_ELM327_MAX_ADAPTER_ID LINK_ELM327_MAX_ADAPTER_ID

#define JAGLINK_ELM327_RESULT_OK LINK_ELM327_RESULT_OK
#define JAGLINK_ELM327_RESULT_MORE_DATA LINK_ELM327_RESULT_MORE_DATA
#define JAGLINK_ELM327_RESULT_INVALID_ARGUMENT LINK_ELM327_RESULT_INVALID_ARGUMENT
#define JAGLINK_ELM327_RESULT_COMMAND_TOO_LONG LINK_ELM327_RESULT_COMMAND_TOO_LONG
#define JAGLINK_ELM327_RESULT_RESPONSE_TOO_LONG LINK_ELM327_RESULT_RESPONSE_TOO_LONG
#define JAGLINK_ELM327_RESULT_NO_DATA LINK_ELM327_RESULT_NO_DATA
#define JAGLINK_ELM327_RESULT_STOPPED LINK_ELM327_RESULT_STOPPED
#define JAGLINK_ELM327_RESULT_UNABLE_TO_CONNECT LINK_ELM327_RESULT_UNABLE_TO_CONNECT
#define JAGLINK_ELM327_RESULT_BUS_INIT_ERROR LINK_ELM327_RESULT_BUS_INIT_ERROR
#define JAGLINK_ELM327_RESULT_CAN_ERROR LINK_ELM327_RESULT_CAN_ERROR
#define JAGLINK_ELM327_RESULT_BUFFER_FULL LINK_ELM327_RESULT_BUFFER_FULL
#define JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND LINK_ELM327_RESULT_UNSUPPORTED_COMMAND
#define JAGLINK_ELM327_RESULT_ADAPTER_ERROR LINK_ELM327_RESULT_ADAPTER_ERROR
#define JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE LINK_ELM327_RESULT_MALFORMED_RESPONSE

typedef LinkElm327Result JaglinkElm327Result;
typedef LinkElm327Response JaglinkElm327Response;
typedef LinkElm327Parser JaglinkElm327Parser;

#define JAGLINK_ELM327_INIT_RESET LINK_ELM327_INIT_RESET
#define JAGLINK_ELM327_INIT_ECHO_OFF LINK_ELM327_INIT_ECHO_OFF
#define JAGLINK_ELM327_INIT_LINEFEEDS_OFF LINK_ELM327_INIT_LINEFEEDS_OFF
#define JAGLINK_ELM327_INIT_SPACES_OFF LINK_ELM327_INIT_SPACES_OFF
#define JAGLINK_ELM327_INIT_HEADERS_OFF LINK_ELM327_INIT_HEADERS_OFF
#define JAGLINK_ELM327_INIT_PROTOCOL_AUTO LINK_ELM327_INIT_PROTOCOL_AUTO
#define JAGLINK_ELM327_INIT_IDENTIFY LINK_ELM327_INIT_IDENTIFY
#define JAGLINK_ELM327_INIT_COMPLETE LINK_ELM327_INIT_COMPLETE
#define JAGLINK_ELM327_INIT_FAILED LINK_ELM327_INIT_FAILED

typedef LinkElm327InitStage JaglinkElm327InitStage;
typedef LinkElm327InitState JaglinkElm327InitState;

const char *jaglink_elm327_result_name(JaglinkElm327Result result);
JaglinkElm327Result jaglink_elm327_build_command(const char *command,
                                                 uint8_t *buffer,
                                                 size_t buffer_size,
                                                 size_t *written);
JaglinkElm327Result jaglink_elm327_parser_begin(JaglinkElm327Parser *parser,
                                                const char *command);
JaglinkElm327Result jaglink_elm327_parser_feed(JaglinkElm327Parser *parser,
                                               const uint8_t *data,
                                               size_t size,
                                               size_t *consumed);
JaglinkElm327Result jaglink_elm327_parser_finish(const JaglinkElm327Parser *parser,
                                                 JaglinkElm327Response *response);
void jaglink_elm327_init_begin(JaglinkElm327InitState *state);
const char *jaglink_elm327_init_command(const JaglinkElm327InitState *state);
JaglinkElm327Result jaglink_elm327_init_accept(JaglinkElm327InitState *state,
                                               const JaglinkElm327Response *response);

#ifdef __cplusplus
}
#endif

#endif
