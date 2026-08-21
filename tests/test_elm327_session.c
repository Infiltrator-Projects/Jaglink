// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/elm327_session.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    bool connected;
    bool fail_write;
    bool respond_during_write;
    char last_write[128];
    size_t last_write_size;
    JaglinkTransportReceiveFn receiver;
    void *receiver_context;
} MockTransport;

typedef struct {
    JaglinkElm327Session *session;
    JaglinkElm327SessionOpResult nested_result;
    unsigned int calls;
} ReentrantEventContext;

static int failures = 0;
static unsigned int event_count = 0U;

static void check(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

static JaglinkTransportStatus mock_connect(void *context)
{
    MockTransport *mock = context;
    mock->connected = true;
    return JAGLINK_TRANSPORT_OK;
}

static void mock_disconnect(void *context)
{
    MockTransport *mock = context;
    mock->connected = false;
}

static bool mock_is_connected(void *context)
{
    const MockTransport *mock = context;
    return mock->connected;
}

static JaglinkTransportStatus mock_write(void *context,
                                        const uint8_t *data,
                                        size_t size)
{
    MockTransport *mock = context;
    size_t copy_size = size;

    if (mock->fail_write) {
        return JAGLINK_TRANSPORT_IO_ERROR;
    }
    if (copy_size >= sizeof(mock->last_write)) {
        copy_size = sizeof(mock->last_write) - 1U;
    }
    memcpy(mock->last_write, data, copy_size);
    mock->last_write[copy_size] = '\0';
    mock->last_write_size = size;

    if (mock->respond_during_write && mock->receiver != NULL) {
        static const uint8_t response[] = "010C\r41 0C 1A F8\r>";
        mock->receiver(mock->receiver_context, response,
                       sizeof(response) - 1U);
    }
    return JAGLINK_TRANSPORT_OK;
}

static void mock_set_receiver(void *context,
                              JaglinkTransportReceiveFn receiver,
                              void *receiver_context)
{
    MockTransport *mock = context;
    mock->receiver = receiver;
    mock->receiver_context = receiver_context;
}

static JaglinkTransport mock_transport_interface(MockTransport *mock)
{
    JaglinkTransport transport = {
        .struct_size = sizeof(JaglinkTransport),
        .abi_version = JAGLINK_TRANSPORT_ABI,
        .context = mock,
        .connect = mock_connect,
        .disconnect = mock_disconnect,
        .is_connected = mock_is_connected,
        .write = mock_write,
        .set_receiver = mock_set_receiver
    };
    return transport;
}

static void mock_emit(MockTransport *mock, const char *text)
{
    if (mock->receiver != NULL) {
        mock->receiver(mock->receiver_context,
                       (const uint8_t *)text, strlen(text));
    }
}

static void on_session_event(void *context,
                             const JaglinkElm327Session *session)
{
    unsigned int *last_sequence = context;
    event_count++;
    *last_sequence = (unsigned int)session->sequence;
}

static void on_reentrant_event(void *context,
                               const JaglinkElm327Session *session)
{
    ReentrantEventContext *event_context = context;
    (void)session;
    event_context->calls++;
    event_context->nested_result = jaglink_elm327_session_begin(
        event_context->session, "010D", 1U, 100U);
}

static void test_fragmented_execution(void)
{
    MockTransport mock = {0};
    JaglinkTransport transport = mock_transport_interface(&mock);
    JaglinkElm327Session session;
    const JaglinkElm327Response *response;
    unsigned int last_sequence = 0U;

    check(jaglink_elm327_session_init(&session, &transport,
                                     on_session_event, &last_sequence),
          "session initialises");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "mock transport connects");
    check(jaglink_elm327_session_begin(&session, "010C", 1000U, 500U) ==
              JAGLINK_ELM327_SESSION_OP_OK,
          "command begins");
    check(session.status == JAGLINK_ELM327_SESSION_WAITING,
          "command remains waiting before response");
    check(mock.last_write_size == 5U &&
              memcmp(mock.last_write, "010C\r", 5U) == 0,
          "session writes exact command frame");
    check(jaglink_elm327_session_begin(&session, "010D", 1001U, 500U) ==
              JAGLINK_ELM327_SESSION_OP_BUSY,
          "second command rejected while first outstanding");

    mock_emit(&mock, "010C\r41 0C ");
    check(session.status == JAGLINK_ELM327_SESSION_WAITING,
          "fragment does not complete without prompt");
    mock_emit(&mock, "1A F8\r>tail");
    check(session.status == JAGLINK_ELM327_SESSION_COMPLETE,
          "prompt completes outstanding command");
    response = jaglink_elm327_session_response(&session);
    check(response != NULL &&
              strcmp(response->text, "41 0C 1A F8") == 0,
          "fragmented response normalised");
    check(session.unexpected_input_bytes == 4U,
          "bytes after prompt are accounted for");
    check(session.needs_resync,
          "non-whitespace bytes after prompt require resynchronisation");
    check(jaglink_elm327_session_begin(&session, "010D", 1002U, 500U) ==
              JAGLINK_ELM327_SESSION_OP_NEEDS_RESYNC,
          "post-prompt contamination blocks the next command");
    check(event_count == 1U && last_sequence == 1U,
          "completion event delivered once");
}

static void test_whitespace_after_prompt_is_harmless(void)
{
    MockTransport mock = {0};
    JaglinkTransport transport = mock_transport_interface(&mock);
    JaglinkElm327Session session;

    check(jaglink_elm327_session_init(&session, &transport, NULL, NULL),
          "whitespace session initialises");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "whitespace transport connects");
    check(jaglink_elm327_session_begin(&session, "ATI", 0U, 100U) ==
              JAGLINK_ELM327_SESSION_OP_OK,
          "whitespace command begins");
    mock_emit(&mock, "ATI\rELM327 v1.5\r>\r\n");
    check(session.status == JAGLINK_ELM327_SESSION_COMPLETE,
          "whitespace response completes");
    check(!session.needs_resync,
          "trailing line endings do not force resynchronisation");
}

static void test_synchronous_provider_callback(void)
{
    MockTransport mock = {0};
    JaglinkTransport transport = mock_transport_interface(&mock);
    JaglinkElm327Session session;

    mock.respond_during_write = true;
    check(jaglink_elm327_session_init(&session, &transport, NULL, NULL),
          "sync callback session initialises");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "sync callback transport connects");
    check(jaglink_elm327_session_begin(&session, "010C", 0U, 1000U) ==
              JAGLINK_ELM327_SESSION_OP_OK,
          "begin tolerates synchronous receive from write");
    check(session.status == JAGLINK_ELM327_SESSION_COMPLETE,
          "synchronous receive completes safely");
}

static void test_completion_callback_cannot_reenter(void)
{
    MockTransport mock = {0};
    JaglinkTransport transport = mock_transport_interface(&mock);
    JaglinkElm327Session session;
    ReentrantEventContext event_context = {0};

    mock.respond_during_write = true;
    event_context.session = &session;
    check(jaglink_elm327_session_init(&session, &transport,
                                     on_reentrant_event, &event_context),
          "reentrant session initialises");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "reentrant transport connects");
    check(jaglink_elm327_session_begin(&session, "010C", 0U, 100U) ==
              JAGLINK_ELM327_SESSION_OP_OK,
          "outer command starts");
    check(event_context.calls == 1U,
          "completion callback invoked once");
    check(event_context.nested_result == JAGLINK_ELM327_SESSION_OP_BUSY,
          "completion callback cannot start next command reentrantly");
    check(session.status == JAGLINK_ELM327_SESSION_COMPLETE,
          "reentrant attempt does not disturb completed response");
}

static void test_timeout_and_resync_guard(void)
{
    MockTransport mock = {0};
    JaglinkTransport transport = mock_transport_interface(&mock);
    JaglinkElm327Session session;

    check(jaglink_elm327_session_init(&session, &transport, NULL, NULL),
          "timeout session initialises");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "timeout transport connects");
    check(jaglink_elm327_session_begin(&session, "0100", 100U, 50U) ==
              JAGLINK_ELM327_SESSION_OP_OK,
          "timeout test command begins");
    check(jaglink_elm327_session_tick(&session, 149U) ==
              JAGLINK_ELM327_SESSION_WAITING,
          "deadline not triggered early");
    check(jaglink_elm327_session_tick(&session, 150U) ==
              JAGLINK_ELM327_SESSION_TIMED_OUT,
          "deadline triggers exactly");
    check(session.needs_resync, "timeout marks session unsynchronised");
    check(jaglink_elm327_session_begin(&session, "ATI", 151U, 50U) ==
              JAGLINK_ELM327_SESSION_OP_NEEDS_RESYNC,
          "new command blocked until explicit resync");

    jaglink_elm327_session_mark_resynchronized(&session);
    check(!session.needs_resync &&
              session.status == JAGLINK_ELM327_SESSION_IDLE,
          "explicit resync restores idle state");
    check(jaglink_elm327_session_begin(&session, "ATI", 151U, 50U) ==
              JAGLINK_ELM327_SESSION_OP_OK,
          "command accepted after resync");
}

static void test_disconnect_reconnect_and_deinit(void)
{
    MockTransport mock = {0};
    JaglinkTransport transport = mock_transport_interface(&mock);
    JaglinkElm327Session session;

    check(jaglink_elm327_session_init(&session, &transport, NULL, NULL),
          "lifetime session initialises");
    check(mock.receiver != NULL && mock.receiver_context == &session,
          "initialisation installs receiver");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "lifetime transport connects");
    jaglink_elm327_session_disconnect(&session);
    check(mock.receiver == NULL && mock.receiver_context == NULL,
          "disconnect detaches receiver");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "session reconnects after detach");
    check(mock.receiver != NULL && mock.receiver_context == &session,
          "reconnect reinstalls receiver");
    jaglink_elm327_session_deinit(&session);
    check(mock.receiver == NULL && mock.receiver_context == NULL,
          "deinit detaches receiver before storage dies");
}

static void test_cancel_and_write_failure(void)
{
    MockTransport mock = {0};
    JaglinkTransport transport = mock_transport_interface(&mock);
    JaglinkElm327Session session;

    check(jaglink_elm327_session_init(&session, &transport, NULL, NULL),
          "cancel session initialises");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "cancel transport connects");
    check(jaglink_elm327_session_begin(&session, "ATI", 0U, 100U) ==
              JAGLINK_ELM327_SESSION_OP_OK,
          "cancel test command begins");
    check(jaglink_elm327_session_cancel(&session), "outstanding command cancels");
    check(session.status == JAGLINK_ELM327_SESSION_CANCELLED &&
              session.needs_resync,
          "cancel requires resync");

    jaglink_elm327_session_mark_resynchronized(&session);
    mock.fail_write = true;
    check(jaglink_elm327_session_begin(&session, "ATI", 0U, 100U) ==
              JAGLINK_ELM327_SESSION_OP_TRANSPORT_ERROR,
          "write failure returned to caller");
    check(session.status == JAGLINK_ELM327_SESSION_FAILED &&
              session.transport_status == JAGLINK_TRANSPORT_IO_ERROR &&
              session.needs_resync,
          "write failure records transport state and resync requirement");
}

static void test_deadline_overflow(void)
{
    MockTransport mock = {0};
    JaglinkTransport transport = mock_transport_interface(&mock);
    JaglinkElm327Session session;

    check(jaglink_elm327_session_init(&session, &transport, NULL, NULL),
          "overflow session initialises");
    check(jaglink_elm327_session_connect(&session) == JAGLINK_TRANSPORT_OK,
          "overflow transport connects");
    check(jaglink_elm327_session_begin(&session, "ATI", UINT64_MAX - 2U, 10U) ==
              JAGLINK_ELM327_SESSION_OP_DEADLINE_OVERFLOW,
          "deadline arithmetic never wraps");
}

int main(void)
{
    test_fragmented_execution();
    test_whitespace_after_prompt_is_harmless();
    test_synchronous_provider_callback();
    test_completion_callback_cannot_reenter();
    test_timeout_and_resync_guard();
    test_disconnect_reconnect_and_deinit();
    test_cancel_and_write_failure();
    test_deadline_overflow();

    if (failures != 0) {
        fprintf(stderr, "%d ELM327 session test(s) failed\n", failures);
        return 1;
    }
    puts("ELM327 session tests passed");
    return 0;
}
