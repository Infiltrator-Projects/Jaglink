// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327.c
 * @brief Portable ELM327 command framing, response parsing and initialisation.
 */
#include "jaglink/elm327.h"

#include "infiltratr/core.h"

#include <string.h>

static bool elm327_ascii_space(unsigned char value)
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\n' ||
           value == '\v' || value == '\f';
}

static size_t elm327_canonicalise(const char *source, char *destination,
                                  size_t destination_size)
{
    size_t written = 0U;

    if (destination == NULL || destination_size == 0U) {
        return 0U;
    }
    destination[0] = '\0';
    if (source == NULL) {
        return 0U;
    }

    while (*source != '\0') {
        unsigned char value = (unsigned char)*source++;
        if (elm327_ascii_space(value)) {
            continue;
        }
        if (written + 1U >= destination_size) {
            destination[0] = '\0';
            return 0U;
        }
        if (value >= (unsigned char)'a' && value <= (unsigned char)'z') {
            value = (unsigned char)(value - (unsigned char)'a' + (unsigned char)'A');
        }
        destination[written++] = (char)value;
    }
    destination[written] = '\0';
    return written;
}

static bool elm327_canonical_equal(const char *left, const char *right)
{
    char left_key[JAGLINK_ELM327_MAX_COMMAND];
    char right_key[JAGLINK_ELM327_MAX_COMMAND];
    size_t left_length;
    size_t right_length;

    left_length = elm327_canonicalise(left, left_key, sizeof(left_key));
    right_length = elm327_canonicalise(right, right_key, sizeof(right_key));
    return left_length > 0U && left_length == right_length &&
           infiltratr_string_equal(left_key, right_key);
}

static JaglinkElm327Result elm327_classify_line(const char *line)
{
    char key[128];

    if (line == NULL || elm327_canonicalise(line, key, sizeof(key)) == 0U) {
        return JAGLINK_ELM327_RESULT_OK;
    }

    if (infiltratr_string_equal(key, "NODATA")) {
        return JAGLINK_ELM327_RESULT_NO_DATA;
    }
    if (infiltratr_string_equal(key, "STOPPED")) {
        return JAGLINK_ELM327_RESULT_STOPPED;
    }
    if (infiltratr_string_equal(key, "UNABLETOCONNECT")) {
        return JAGLINK_ELM327_RESULT_UNABLE_TO_CONNECT;
    }
    if (infiltratr_string_equal(key, "BUSINIT:ERROR") ||
        infiltratr_string_equal(key, "BUSINIT...ERROR")) {
        return JAGLINK_ELM327_RESULT_BUS_INIT_ERROR;
    }
    if (infiltratr_string_equal(key, "CANERROR")) {
        return JAGLINK_ELM327_RESULT_CAN_ERROR;
    }
    if (infiltratr_string_equal(key, "BUFFERFULL")) {
        return JAGLINK_ELM327_RESULT_BUFFER_FULL;
    }
    if (infiltratr_string_equal(key, "?")) {
        return JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND;
    }
    if (infiltratr_string_equal(key, "ERROR")) {
        return JAGLINK_ELM327_RESULT_ADAPTER_ERROR;
    }
    return JAGLINK_ELM327_RESULT_OK;
}

static bool elm327_line_is_searching(const char *line)
{
    char key[128];

    if (line == NULL || elm327_canonicalise(line, key, sizeof(key)) == 0U) {
        return false;
    }
    return infiltratr_string_starts_with(key, "SEARCHING...");
}

static bool elm327_response_append(JaglinkElm327Response *response,
                                   const char *line)
{
    size_t line_length;
    size_t separator;
    size_t available;

    if (response == NULL || line == NULL) {
        return false;
    }

    line_length = strlen(line);
    separator = response->length == 0U ? 0U : 1U;
    if (response->length >= sizeof(response->text)) {
        return false;
    }
    available = sizeof(response->text) - response->length - 1U;
    if (separator > available || line_length > available - separator) {
        return false;
    }

    if (separator != 0U) {
        response->text[response->length++] = '\n';
    }
    memcpy(response->text + response->length, line, line_length);
    response->length += line_length;
    response->text[response->length] = '\0';
    response->line_count++;
    return true;
}

const char *jaglink_elm327_result_name(JaglinkElm327Result result)
{
    switch (result) {
    case JAGLINK_ELM327_RESULT_OK:
        return "ok";
    case JAGLINK_ELM327_RESULT_MORE_DATA:
        return "more-data";
    case JAGLINK_ELM327_RESULT_INVALID_ARGUMENT:
        return "invalid-argument";
    case JAGLINK_ELM327_RESULT_COMMAND_TOO_LONG:
        return "command-too-long";
    case JAGLINK_ELM327_RESULT_RESPONSE_TOO_LONG:
        return "response-too-long";
    case JAGLINK_ELM327_RESULT_NO_DATA:
        return "no-data";
    case JAGLINK_ELM327_RESULT_STOPPED:
        return "stopped";
    case JAGLINK_ELM327_RESULT_UNABLE_TO_CONNECT:
        return "unable-to-connect";
    case JAGLINK_ELM327_RESULT_BUS_INIT_ERROR:
        return "bus-init-error";
    case JAGLINK_ELM327_RESULT_CAN_ERROR:
        return "can-error";
    case JAGLINK_ELM327_RESULT_BUFFER_FULL:
        return "buffer-full";
    case JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND:
        return "unsupported-command";
    case JAGLINK_ELM327_RESULT_ADAPTER_ERROR:
        return "adapter-error";
    case JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE:
        return "malformed-response";
    }
    return "unknown";
}

JaglinkElm327Result jaglink_elm327_build_command(const char *command,
                                                uint8_t *buffer,
                                                size_t buffer_size,
                                                size_t *written)
{
    char work[JAGLINK_ELM327_MAX_COMMAND];
    size_t input_length;
    size_t command_length;
    size_t index;

    if (written != NULL) {
        *written = 0U;
    }
    if (command == NULL || buffer == NULL || written == NULL || buffer_size == 0U) {
        return JAGLINK_ELM327_RESULT_INVALID_ARGUMENT;
    }

    input_length = strlen(command);
    if (input_length >= sizeof(work)) {
        return JAGLINK_ELM327_RESULT_COMMAND_TOO_LONG;
    }
    infiltratr_copy_string(work, sizeof(work), command);
    infiltratr_trim(work);
    command_length = strlen(work);
    if (command_length == 0U) {
        return JAGLINK_ELM327_RESULT_INVALID_ARGUMENT;
    }

    for (index = 0U; index < command_length; ++index) {
        unsigned char value = (unsigned char)work[index];
        if (value < 0x20U || value > 0x7eU || value == '>') {
            return JAGLINK_ELM327_RESULT_INVALID_ARGUMENT;
        }
    }

    if (command_length + 1U > buffer_size) {
        return JAGLINK_ELM327_RESULT_COMMAND_TOO_LONG;
    }
    memcpy(buffer, work, command_length);
    buffer[command_length] = '\r';
    *written = command_length + 1U;
    return JAGLINK_ELM327_RESULT_OK;
}

JaglinkElm327Result jaglink_elm327_parser_begin(JaglinkElm327Parser *parser,
                                               const char *command)
{
    uint8_t framed[JAGLINK_ELM327_MAX_COMMAND + 1U];
    size_t framed_size;
    JaglinkElm327Result result;

    if (parser == NULL || command == NULL) {
        return JAGLINK_ELM327_RESULT_INVALID_ARGUMENT;
    }

    memset(parser, 0, sizeof(*parser));
    result = jaglink_elm327_build_command(command, framed, sizeof(framed),
                                         &framed_size);
    if (result != JAGLINK_ELM327_RESULT_OK) {
        return result;
    }
    if (framed_size <= 1U || framed_size > sizeof(parser->command)) {
        return JAGLINK_ELM327_RESULT_COMMAND_TOO_LONG;
    }
    memcpy(parser->command, framed, framed_size - 1U);
    parser->command[framed_size - 1U] = '\0';
    return JAGLINK_ELM327_RESULT_OK;
}

JaglinkElm327Result jaglink_elm327_parser_feed(JaglinkElm327Parser *parser,
                                              const uint8_t *data,
                                              size_t size,
                                              size_t *consumed)
{
    size_t index;

    if (consumed != NULL) {
        *consumed = 0U;
    }
    if (parser == NULL || consumed == NULL || (data == NULL && size != 0U)) {
        return JAGLINK_ELM327_RESULT_INVALID_ARGUMENT;
    }
    if (parser->overflowed) {
        return JAGLINK_ELM327_RESULT_RESPONSE_TOO_LONG;
    }
    if (parser->prompt_seen) {
        return JAGLINK_ELM327_RESULT_OK;
    }

    for (index = 0U; index < size; ++index) {
        uint8_t value = data[index];
        *consumed = index + 1U;
        if (value == (uint8_t)'>') {
            parser->prompt_seen = true;
            return JAGLINK_ELM327_RESULT_OK;
        }
        if (parser->raw_length >= sizeof(parser->raw)) {
            parser->overflowed = true;
            return JAGLINK_ELM327_RESULT_RESPONSE_TOO_LONG;
        }
        parser->raw[parser->raw_length++] = value;
    }
    return JAGLINK_ELM327_RESULT_MORE_DATA;
}

JaglinkElm327Result jaglink_elm327_parser_finish(const JaglinkElm327Parser *parser,
                                                JaglinkElm327Response *response)
{
    size_t position = 0U;
    JaglinkElm327Result classified = JAGLINK_ELM327_RESULT_OK;

    if (parser == NULL || response == NULL) {
        return JAGLINK_ELM327_RESULT_INVALID_ARGUMENT;
    }
    memset(response, 0, sizeof(*response));
    if (parser->overflowed) {
        response->result = JAGLINK_ELM327_RESULT_RESPONSE_TOO_LONG;
        return response->result;
    }
    if (!parser->prompt_seen) {
        response->result = JAGLINK_ELM327_RESULT_MORE_DATA;
        return response->result;
    }
    response->prompt_seen = true;

    while (position < parser->raw_length) {
        char line[JAGLINK_ELM327_MAX_RESPONSE];
        size_t line_length = 0U;
        JaglinkElm327Result line_result;

        while (position < parser->raw_length &&
               (parser->raw[position] == (uint8_t)'\r' ||
                parser->raw[position] == (uint8_t)'\n')) {
            position++;
        }
        while (position < parser->raw_length &&
               parser->raw[position] != (uint8_t)'\r' &&
               parser->raw[position] != (uint8_t)'\n') {
            if (line_length + 1U >= sizeof(line)) {
                response->result = JAGLINK_ELM327_RESULT_RESPONSE_TOO_LONG;
                return response->result;
            }
            line[line_length++] = (char)parser->raw[position++];
        }
        line[line_length] = '\0';
        infiltratr_trim(line);
        if (line[0] == '\0') {
            continue;
        }

        if (elm327_canonical_equal(line, parser->command)) {
            response->echo_removed = true;
            continue;
        }
        if (elm327_line_is_searching(line)) {
            response->searching_seen = true;
            continue;
        }
        if (elm327_canonical_equal(line, "OK")) {
            response->ok_seen = true;
            continue;
        }

        line_result = elm327_classify_line(line);
        if (line_result != JAGLINK_ELM327_RESULT_OK) {
            if (classified == JAGLINK_ELM327_RESULT_OK) {
                classified = line_result;
            }
            continue;
        }
        if (!elm327_response_append(response, line)) {
            response->result = JAGLINK_ELM327_RESULT_RESPONSE_TOO_LONG;
            return response->result;
        }
    }

    if (classified != JAGLINK_ELM327_RESULT_OK) {
        response->result = classified;
    } else if (response->line_count == 0U && !response->ok_seen) {
        response->result = JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE;
    } else {
        response->result = JAGLINK_ELM327_RESULT_OK;
    }
    return response->result;
}

void jaglink_elm327_init_begin(JaglinkElm327InitState *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->stage = JAGLINK_ELM327_INIT_RESET;
    state->failure = JAGLINK_ELM327_RESULT_OK;
}

const char *jaglink_elm327_init_command(const JaglinkElm327InitState *state)
{
    if (state == NULL) {
        return NULL;
    }
    switch (state->stage) {
    case JAGLINK_ELM327_INIT_RESET:
        return "ATZ";
    case JAGLINK_ELM327_INIT_ECHO_OFF:
        return "ATE0";
    case JAGLINK_ELM327_INIT_LINEFEEDS_OFF:
        return "ATL0";
    case JAGLINK_ELM327_INIT_SPACES_OFF:
        return "ATS0";
    case JAGLINK_ELM327_INIT_HEADERS_OFF:
        return "ATH0";
    case JAGLINK_ELM327_INIT_PROTOCOL_AUTO:
        return "ATSP0";
    case JAGLINK_ELM327_INIT_IDENTIFY:
        return "ATI";
    case JAGLINK_ELM327_INIT_COMPLETE:
    case JAGLINK_ELM327_INIT_FAILED:
        return NULL;
    }
    return NULL;
}

JaglinkElm327Result jaglink_elm327_init_accept(JaglinkElm327InitState *state,
                                              const JaglinkElm327Response *response)
{
    bool configuration_stage;

    if (state == NULL || response == NULL ||
        state->stage == JAGLINK_ELM327_INIT_COMPLETE ||
        state->stage == JAGLINK_ELM327_INIT_FAILED) {
        return JAGLINK_ELM327_RESULT_INVALID_ARGUMENT;
    }
    if (response->result != JAGLINK_ELM327_RESULT_OK) {
        state->failure = response->result;
        state->stage = JAGLINK_ELM327_INIT_FAILED;
        return response->result;
    }

    configuration_stage = state->stage >= JAGLINK_ELM327_INIT_ECHO_OFF &&
                          state->stage <= JAGLINK_ELM327_INIT_PROTOCOL_AUTO;
    if (configuration_stage && !response->ok_seen) {
        state->failure = JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE;
        state->stage = JAGLINK_ELM327_INIT_FAILED;
        return state->failure;
    }

    if (state->stage == JAGLINK_ELM327_INIT_IDENTIFY) {
        if (response->length == 0U) {
            state->failure = JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE;
            state->stage = JAGLINK_ELM327_INIT_FAILED;
            return state->failure;
        }
        infiltratr_copy_string(state->adapter_id, sizeof(state->adapter_id),
                               response->text);
        state->stage = JAGLINK_ELM327_INIT_COMPLETE;
        return JAGLINK_ELM327_RESULT_OK;
    }

    state->stage = (JaglinkElm327InitStage)((unsigned int)state->stage + 1U);
    return JAGLINK_ELM327_RESULT_OK;
}
