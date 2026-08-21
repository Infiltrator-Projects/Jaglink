// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file probe.c
 * @brief Deterministic ELM327 adapter/protocol capability probing.
 */
#include "jaglink/elm327_probe.h"

#include "infiltratr/core.h"

#include <stddef.h>
#include <string.h>

static bool elm327_probe_parse_protocol_number(
    const char *text,
    bool *automatic,
    uint8_t *protocol_number)
{
    char compact[16];
    size_t input_index = 0U;
    size_t output_index = 0U;
    uint64_t parsed;

    if (text == NULL || automatic == NULL || protocol_number == NULL) {
        return false;
    }

    while (text[input_index] != '\0') {
        unsigned char value = (unsigned char)text[input_index++];
        if (value == ' ' || value == '\t' || value == '\r' || value == '\n') {
            continue;
        }
        if (output_index + 1U >= sizeof(compact)) {
            return false;
        }
        if (value >= (unsigned char)'a' && value <= (unsigned char)'f') {
            value = (unsigned char)(value - (unsigned char)'a' +
                                    (unsigned char)'A');
        }
        compact[output_index++] = (char)value;
    }
    compact[output_index] = '\0';

    if (output_index == 0U) {
        return false;
    }

    *automatic = false;
    if (compact[0] == 'A' && output_index > 1U) {
        *automatic = true;
        memmove(compact, compact + 1, output_index);
        output_index--;
    }

    if (!infiltratr_parse_u64_range(compact, 16U, 0U, 0xffU, &parsed)) {
        return false;
    }
    *protocol_number = (uint8_t)parsed;
    return true;
}

void jaglink_elm327_probe_begin(JaglinkElm327ProbeState *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->stage = JAGLINK_ELM327_PROBE_DEVICE_DESCRIPTION;
    state->failure = JAGLINK_ELM327_RESULT_OK;
}

const char *jaglink_elm327_probe_command(
    const JaglinkElm327ProbeState *state)
{
    if (state == NULL) {
        return NULL;
    }

    switch (state->stage) {
    case JAGLINK_ELM327_PROBE_DEVICE_DESCRIPTION:
        return "AT@1";
    case JAGLINK_ELM327_PROBE_PROTOCOL_DESCRIPTION:
        return "ATDP";
    case JAGLINK_ELM327_PROBE_PROTOCOL_NUMBER:
        return "ATDPN";
    case JAGLINK_ELM327_PROBE_COMPLETE:
    case JAGLINK_ELM327_PROBE_FAILED:
        return NULL;
    }
    return NULL;
}

JaglinkElm327Result jaglink_elm327_probe_accept(
    JaglinkElm327ProbeState *state,
    const JaglinkElm327Response *response)
{
    if (state == NULL || response == NULL ||
        state->stage == JAGLINK_ELM327_PROBE_COMPLETE ||
        state->stage == JAGLINK_ELM327_PROBE_FAILED) {
        return JAGLINK_ELM327_RESULT_INVALID_ARGUMENT;
    }

    if (state->stage == JAGLINK_ELM327_PROBE_DEVICE_DESCRIPTION) {
        if (response->result == JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND) {
            state->device_description_supported = false;
            state->stage = JAGLINK_ELM327_PROBE_PROTOCOL_DESCRIPTION;
            return JAGLINK_ELM327_RESULT_OK;
        }
        if (response->result != JAGLINK_ELM327_RESULT_OK ||
            response->length == 0U) {
            state->failure = response->result == JAGLINK_ELM327_RESULT_OK
                                 ? JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE
                                 : response->result;
            state->stage = JAGLINK_ELM327_PROBE_FAILED;
            return state->failure;
        }

        infiltratr_copy_string(state->device_description,
                               sizeof(state->device_description),
                               response->text);
        state->device_description_supported = true;
        state->stage = JAGLINK_ELM327_PROBE_PROTOCOL_DESCRIPTION;
        return JAGLINK_ELM327_RESULT_OK;
    }

    if (response->result != JAGLINK_ELM327_RESULT_OK ||
        response->length == 0U) {
        state->failure = response->result == JAGLINK_ELM327_RESULT_OK
                             ? JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE
                             : response->result;
        state->stage = JAGLINK_ELM327_PROBE_FAILED;
        return state->failure;
    }

    if (state->stage == JAGLINK_ELM327_PROBE_PROTOCOL_DESCRIPTION) {
        infiltratr_copy_string(state->protocol_description,
                               sizeof(state->protocol_description),
                               response->text);
        state->stage = JAGLINK_ELM327_PROBE_PROTOCOL_NUMBER;
        return JAGLINK_ELM327_RESULT_OK;
    }

    if (state->stage == JAGLINK_ELM327_PROBE_PROTOCOL_NUMBER) {
        if (!elm327_probe_parse_protocol_number(
                response->text,
                &state->protocol_was_automatic,
                &state->protocol_number)) {
            state->failure = JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE;
            state->stage = JAGLINK_ELM327_PROBE_FAILED;
            return state->failure;
        }

        state->stage = JAGLINK_ELM327_PROBE_COMPLETE;
        return JAGLINK_ELM327_RESULT_OK;
    }

    state->failure = JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE;
    state->stage = JAGLINK_ELM327_PROBE_FAILED;
    return state->failure;
}
