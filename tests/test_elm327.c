// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/elm327.h"

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

static JaglinkElm327Response parse_complete(const char *command,
                                           const char *wire)
{
    JaglinkElm327Parser parser;
    JaglinkElm327Response response;
    size_t consumed = 0U;
    JaglinkElm327Result result;

    memset(&response, 0, sizeof(response));
    result = jaglink_elm327_parser_begin(&parser, command);
    check(result == JAGLINK_ELM327_RESULT_OK, "parser begin");
    result = jaglink_elm327_parser_feed(&parser, (const uint8_t *)wire,
                                       strlen(wire), &consumed);
    check(result == JAGLINK_ELM327_RESULT_OK, "complete response finds prompt");
    check(consumed <= strlen(wire), "consumed is bounded");
    (void)jaglink_elm327_parser_finish(&parser, &response);
    return response;
}

static void test_command_framing(void)
{
    uint8_t bytes[16];
    size_t written = 0U;
    JaglinkElm327Result result;

    result = jaglink_elm327_build_command("  atz  ", bytes, sizeof(bytes), &written);
    check(result == JAGLINK_ELM327_RESULT_OK, "command framing succeeds");
    check(written == 4U, "command framing writes exact size");
    check(memcmp(bytes, "atz\r", 4U) == 0, "command framing trims and appends CR");

    result = jaglink_elm327_build_command("ATZ\nATI", bytes, sizeof(bytes), &written);
    check(result == JAGLINK_ELM327_RESULT_INVALID_ARGUMENT,
          "embedded newline is rejected");
}

static void test_fragmented_response(void)
{
    JaglinkElm327Parser parser;
    JaglinkElm327Response response;
    size_t consumed = 0U;
    JaglinkElm327Result result;

    check(jaglink_elm327_parser_begin(&parser, "010C") == JAGLINK_ELM327_RESULT_OK,
          "fragment parser begin");
    result = jaglink_elm327_parser_feed(&parser, (const uint8_t *)"010C\r41 0C ",
                                       strlen("010C\r41 0C "), &consumed);
    check(result == JAGLINK_ELM327_RESULT_MORE_DATA, "fragment needs more data");
    result = jaglink_elm327_parser_feed(&parser, (const uint8_t *)"1A F8\r>tail",
                                       strlen("1A F8\r>tail"), &consumed);
    check(result == JAGLINK_ELM327_RESULT_OK, "second fragment completes");
    check(consumed == 7U, "parser stops consuming at prompt");
    result = jaglink_elm327_parser_finish(&parser, &response);
    check(result == JAGLINK_ELM327_RESULT_OK, "fragmented response parses");
    check(response.echo_removed, "echo is removed");
    check(response.prompt_seen, "prompt is recorded");
    check(response.line_count == 1U, "one payload line remains");
    check(strcmp(response.text, "41 0C 1A F8") == 0, "payload is normalised");
}

static void test_status_classification(void)
{
    JaglinkElm327Response response;

    response = parse_complete("0100", "0100\rSEARCHING...\rNO DATA\r>");
    check(response.result == JAGLINK_ELM327_RESULT_NO_DATA, "NO DATA classified");
    check(response.searching_seen, "SEARCHING marker retained as metadata");
    check(response.echo_removed, "status response echo removed");

    response = parse_complete("ATFOO", "ATFOO\r?\r>");
    check(response.result == JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND,
          "question mark classified as unsupported command");

    response = parse_complete("0100", "UNABLE TO CONNECT\r>");
    check(response.result == JAGLINK_ELM327_RESULT_UNABLE_TO_CONNECT,
          "unable-to-connect classified");

    response = parse_complete("0100", ">");
    check(response.result == JAGLINK_ELM327_RESULT_MALFORMED_RESPONSE,
          "empty prompt response is malformed");
}

static void test_probe_style_identity_responses(void)
{
    JaglinkElm327Response response;

    response = parse_complete("ATI", "ATI\rELM327 v1.5\r>");
    check(response.result == JAGLINK_ELM327_RESULT_OK,
          "CR-only ATI identity is accepted by C parser");
    check(response.echo_removed, "CR-only ATI echo is removed");
    check(response.line_count == 1U, "ATI leaves exactly one identity line");
    check(strcmp(response.text, "ELM327 v1.5") == 0,
          "ATI identity text is preserved");

    response = parse_complete("ATI", "ATI\r?\r>");
    check(response.result == JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND,
          "CR-only ATI question mark is rejected");
    check(response.length == 0U,
          "unsupported ATI reply cannot masquerade as identity text");

    response = parse_complete("ATI", "ATI\rERROR\r>");
    check(response.result == JAGLINK_ELM327_RESULT_ADAPTER_ERROR,
          "CR-only ATI ERROR is rejected");
    check(response.length == 0U,
          "adapter error cannot masquerade as identity text");
}

static void test_initialisation(void)
{
    JaglinkElm327InitState state;
    JaglinkElm327Response response;
    const char *expected[] = {"ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP0", "ATI"};
    size_t index;

    jaglink_elm327_init_begin(&state);
    for (index = 0U; index < 7U; ++index) {
        check(strcmp(jaglink_elm327_init_command(&state), expected[index]) == 0,
              "initialisation command order");
        if (index == 0U) {
            response = parse_complete("ATZ", "ATZ\rELM327 v2.3\r>");
        } else if (index == 6U) {
            response = parse_complete("ATI", "ELM327 v2.3\r>");
        } else {
            response = parse_complete(expected[index], "OK\r>");
        }
        check(jaglink_elm327_init_accept(&state, &response) == JAGLINK_ELM327_RESULT_OK,
              "initialisation stage accepts response");
    }
    check(state.stage == JAGLINK_ELM327_INIT_COMPLETE, "initialisation completes");
    check(strcmp(state.adapter_id, "ELM327 v2.3") == 0, "adapter identity captured");
    check(jaglink_elm327_init_command(&state) == NULL, "complete state has no command");
}

static void test_initialisation_failure(void)
{
    JaglinkElm327InitState state;
    JaglinkElm327Response response;

    jaglink_elm327_init_begin(&state);
    response = parse_complete("ATZ", "ELM327 v2.3\r>");
    check(jaglink_elm327_init_accept(&state, &response) == JAGLINK_ELM327_RESULT_OK,
          "reset accepted before failure test");
    response = parse_complete("ATE0", "?\r>");
    check(jaglink_elm327_init_accept(&state, &response) ==
              JAGLINK_ELM327_RESULT_UNSUPPORTED_COMMAND,
          "init surfaces adapter error");
    check(state.stage == JAGLINK_ELM327_INIT_FAILED, "init enters failed state");
}

int main(void)
{
    test_command_framing();
    test_fragmented_response();
    test_status_classification();
    test_probe_style_identity_responses();
    test_initialisation();
    test_initialisation_failure();

    if (failures != 0) {
        fprintf(stderr, "%d ELM327 test(s) failed\n", failures);
        return 1;
    }
    puts("ELM327 tests passed");
    return 0;
}
