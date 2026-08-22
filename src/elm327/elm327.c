// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327.c
 * @brief JAGLINK ABI wrappers for LINK's shared ELM327 core.
 */
#include "jaglink/elm327.h"

const char *jaglink_elm327_result_name(JaglinkElm327Result result)
{
    return link_elm327_result_name(result);
}

JaglinkElm327Result jaglink_elm327_build_command(const char *command,
                                                 uint8_t *buffer,
                                                 size_t buffer_size,
                                                 size_t *written)
{
    return link_elm327_build_command(command, buffer, buffer_size, written);
}

JaglinkElm327Result jaglink_elm327_parser_begin(JaglinkElm327Parser *parser,
                                                const char *command)
{
    return link_elm327_parser_begin(parser, command);
}

JaglinkElm327Result jaglink_elm327_parser_feed(JaglinkElm327Parser *parser,
                                               const uint8_t *data,
                                               size_t size,
                                               size_t *consumed)
{
    return link_elm327_parser_feed(parser, data, size, consumed);
}

JaglinkElm327Result jaglink_elm327_parser_finish(const JaglinkElm327Parser *parser,
                                                 JaglinkElm327Response *response)
{
    return link_elm327_parser_finish(parser, response);
}

void jaglink_elm327_init_begin(JaglinkElm327InitState *state)
{
    link_elm327_init_begin(state);
}

const char *jaglink_elm327_init_command(const JaglinkElm327InitState *state)
{
    return link_elm327_init_command(state);
}

JaglinkElm327Result jaglink_elm327_init_accept(JaglinkElm327InitState *state,
                                               const JaglinkElm327Response *response)
{
    return link_elm327_init_accept(state, response);
}
