// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/elm327_probe.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

static JaglinkElm327Response response_with(JaglinkElm327Result result,
                                          const char *text)
{
    JaglinkElm327Response response;
    memset(&response, 0, sizeof(response));
    response.result = result;
    response.prompt_seen = true;
    if (text != NULL) {
        size_t length = strlen(text);
        if (length >= sizeof(response.text)) {
            length = sizeof(response.text) - 1U;
        }
        memcpy(response.text, text, length);
        response.text[length] = '\0';
        response.length = length;
        response.line_count = length == 0U ? 0U : 1U;
    }
    return response;
}

static void test_supported_probe(void)
{
    JaglinkElm327ProbeState state;
    JaglinkElm327Response response;

    jaglink_elm327_probe_begin(&state);
    check(strcmp(jaglink_elm327_probe_command(&state), "AT@1") == 0,
          "probe begins with device description");

    response = response_with(JAGLINK_ELM327_RESULT_OK,
                             "OBDII to RS232 Interpreter");
    check(jaglink_elm327_probe_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_OK,
          "device description accepted");
    check(state.device_description_supported,
          "device description capability recorded");
    check(strcmp(state.device_description,
                 "OBDII to RS232 Interpreter") == 0,
          "device description captured");

    check(strcmp(jaglink_elm327_probe_command(&state), "ATDP") == 0,
          "probe requests protocol description");
    response = response_with(JAGLINK_ELM327_RESULT_OK,
                             "AUTO, ISO 15765-4 (CAN 11/500)");
    check(jaglink_elm327_probe_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_OK,
          "protocol description accepted");

    check(strcmp(jaglink_elm327_probe_command(&state), "ATDPN") == 0,
          "probe requests numeric protocol");
    response = response_with(JAGLINK_ELM327_RESULT_OK, "A6");
    check(jaglink_elm327_probe_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_OK,
          "automatic protocol number accepted");
    check(state.stage == JAGLINK_ELM327_PROBE_COMPLETE,
          "probe completes");
    check(state.protocol_was_automatic && state.protocol_number == 6U,
          "automatic protocol metadata parsed");
    check(strcmp(state.protocol_description,
                 "AUTO, ISO 15765-4 (CAN 11/500)") == 0,
          "protocol description retained");
}

static void test_optional_description(void)
{
    JaglinkElm327ProbeState state;
    JaglinkElm327Response response;

    jaglink_elm327_probe_begin(&state);
    response = response_with(JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND, NULL);
    check(jaglink_elm327_probe_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_OK,
          "unsupported AT@1 is tolerated");
    check(!state.device_description_supported,
          "unsupported optional capability recorded");

    response = response_with(JAGLINK_ELM327_RESULT_OK, "ISO 15765-4 CAN");
    check(jaglink_elm327_probe_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_OK,
          "required ATDP still accepted");
    response = response_with(JAGLINK_ELM327_RESULT_OK, "6");
    check(jaglink_elm327_probe_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_OK,
          "manual protocol number accepted");
    check(!state.protocol_was_automatic && state.protocol_number == 6U,
          "manual protocol metadata parsed");
}

static void test_manual_protocol_a(void)
{
    JaglinkElm327ProbeState state;
    JaglinkElm327Response response;

    jaglink_elm327_probe_begin(&state);
    response = response_with(JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND, NULL);
    (void)jaglink_elm327_probe_accept(&state, &response);
    response = response_with(JAGLINK_ELM327_RESULT_OK, "SAE J1939");
    (void)jaglink_elm327_probe_accept(&state, &response);
    response = response_with(JAGLINK_ELM327_RESULT_OK, "A");
    check(jaglink_elm327_probe_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_OK,
          "single A is a manual hexadecimal protocol number");
    check(!state.protocol_was_automatic && state.protocol_number == 0x0aU,
          "manual protocol A is not confused with automatic prefix");
}

static void test_malformed_protocol_number(void)
{
    JaglinkElm327ProbeState state;
    JaglinkElm327Response response;

    jaglink_elm327_probe_begin(&state);
    response = response_with(JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND, NULL);
    (void)jaglink_elm327_probe_accept(&state, &response);
    response = response_with(JAGLINK_ELM327_RESULT_OK, "AUTO");
    (void)jaglink_elm327_probe_accept(&state, &response);
    response = response_with(JAGLINK_ELM327_RESULT_OK, "AXX");
    check(jaglink_elm327_probe_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE,
          "malformed protocol number rejected");
    check(state.stage == JAGLINK_ELM327_PROBE_FAILED,
          "malformed required capability fails probe");
}

int main(void)
{
    test_supported_probe();
    test_optional_description();
    test_manual_protocol_a();
    test_malformed_protocol_number();

    if (failures != 0) {
        fprintf(stderr, "%d ELM327 probe test(s) failed\n", failures);
        return 1;
    }
    puts("ELM327 probe tests passed");
    return 0;
}
