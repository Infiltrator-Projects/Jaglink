// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327_session.h
 * @brief Transport-backed ELM327 command session engine.
 */
#ifndef JAGLINK_ELM327_SESSION_H
#define JAGLINK_ELM327_SESSION_H

#include "jaglink/elm327.h"
#include "jaglink/transport.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JAGLINK_ELM327_SESSION_IDLE = 0,
    JAGLINK_ELM327_SESSION_WAITING,
    JAGLINK_ELM327_SESSION_COMPLETE,
    JAGLINK_ELM327_SESSION_TIMED_OUT,
    JAGLINK_ELM327_SESSION_CANCELLED,
    JAGLINK_ELM327_SESSION_FAILED
} JaglinkElm327SessionStatus;

typedef enum {
    JAGLINK_ELM327_SESSION_OP_OK = 0,
    JAGLINK_ELM327_SESSION_OP_INVALID_ARGUMENT,
    JAGLINK_ELM327_SESSION_OP_BUSY,
    JAGLINK_ELM327_SESSION_OP_NOT_CONNECTED,
    JAGLINK_ELM327_SESSION_OP_NEEDS_RESYNC,
    JAGLINK_ELM327_SESSION_OP_DEADLINE_OVERFLOW,
    JAGLINK_ELM327_SESSION_OP_TRANSPORT_ERROR
} JaglinkElm327SessionOpResult;

struct JaglinkElm327Session;
typedef struct JaglinkElm327Session JaglinkElm327Session;

typedef void (*JaglinkElm327SessionEventFn)(
    void *context,
    const JaglinkElm327Session *session);

struct JaglinkElm327Session {
    JaglinkTransport transport;
    JaglinkElm327Parser parser;
    JaglinkElm327Response response;
    JaglinkElm327SessionStatus status;
    JaglinkElm327Result elm_result;
    JaglinkTransportStatus transport_status;
    uint64_t deadline_ms;
    uint64_t sequence;
    size_t unexpected_input_bytes;
    bool needs_resync;
    bool callback_active;
    JaglinkElm327SessionEventFn event;
    void *event_context;
};

/**
 * Initialise a session and install its transport receiver callback.
 *
 * The transport object is copied, but its context and provider-owned resources
 * must remain valid until `jaglink_elm327_session_deinit()` is called.
 */
bool jaglink_elm327_session_init(JaglinkElm327Session *session,
                                const JaglinkTransport *transport,
                                JaglinkElm327SessionEventFn event,
                                void *event_context);

/**
 * Detach the transport receiver callback and invalidate the session.
 *
 * Call this before the session storage or transport provider is destroyed and
 * only after any synchronous session event callback has returned.
 */
void jaglink_elm327_session_deinit(JaglinkElm327Session *session);

/** Connect the underlying transport. A successful reconnect re-synchronises. */
JaglinkTransportStatus jaglink_elm327_session_connect(
    JaglinkElm327Session *session);

/** Disconnect the underlying transport and cancel any outstanding command. */
void jaglink_elm327_session_disconnect(JaglinkElm327Session *session);

/** Return whether the underlying provider currently reports connected. */
bool jaglink_elm327_session_is_connected(
    const JaglinkElm327Session *session);

/**
 * Begin one ELM327 command.
 *
 * Exactly one command may be outstanding. `now_ms` must use a monotonic
 * caller-owned clock. Timeout zero is invalid. The session is single-threaded;
 * callers must serialize access, and a completion callback may observe state
 * but must defer starting the next command until the callback returns.
 */
JaglinkElm327SessionOpResult jaglink_elm327_session_begin(
    JaglinkElm327Session *session,
    const char *command,
    uint64_t now_ms,
    uint64_t timeout_ms);

/** Apply timeout processing using the same monotonic clock supplied to begin. */
JaglinkElm327SessionStatus jaglink_elm327_session_tick(
    JaglinkElm327Session *session,
    uint64_t now_ms);

/**
 * Cancel an outstanding command without sending an adapter-side abort byte.
 *
 * Cancellation requires transport re-synchronisation before another command,
 * preventing late bytes from a cancelled request contaminating the next one.
 */
bool jaglink_elm327_session_cancel(JaglinkElm327Session *session);

/**
 * Explicitly clear the re-synchronisation guard after the caller has drained
 * to a known prompt or otherwise restored the ELM command boundary.
 */
void jaglink_elm327_session_mark_resynchronized(
    JaglinkElm327Session *session);

/** Return the completed response, or NULL unless status is COMPLETE. */
const JaglinkElm327Response *jaglink_elm327_session_response(
    const JaglinkElm327Session *session);

/** Return a stable string for an operation result. */
const char *jaglink_elm327_session_op_result_name(
    JaglinkElm327SessionOpResult result);

#ifdef __cplusplus
}
#endif

#endif
