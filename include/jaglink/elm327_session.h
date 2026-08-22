// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327_session.h
 * @brief JAGLINK compatibility facade for LINK's ELM327 session engine.
 */
#ifndef JAGLINK_ELM327_SESSION_H
#define JAGLINK_ELM327_SESSION_H

#include "link/elm327_session.h"
#include "jaglink/elm327.h"
#include "jaglink/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_ELM327_SESSION_IDLE LINK_ELM327_SESSION_IDLE
#define JAGLINK_ELM327_SESSION_WAITING LINK_ELM327_SESSION_WAITING
#define JAGLINK_ELM327_SESSION_COMPLETE LINK_ELM327_SESSION_COMPLETE
#define JAGLINK_ELM327_SESSION_TIMED_OUT LINK_ELM327_SESSION_TIMED_OUT
#define JAGLINK_ELM327_SESSION_CANCELLED LINK_ELM327_SESSION_CANCELLED
#define JAGLINK_ELM327_SESSION_FAILED LINK_ELM327_SESSION_FAILED
#define JAGLINK_ELM327_SESSION_OP_OK LINK_ELM327_SESSION_OP_OK
#define JAGLINK_ELM327_SESSION_OP_INVALID_ARGUMENT LINK_ELM327_SESSION_OP_INVALID_ARGUMENT
#define JAGLINK_ELM327_SESSION_OP_BUSY LINK_ELM327_SESSION_OP_BUSY
#define JAGLINK_ELM327_SESSION_OP_NOT_CONNECTED LINK_ELM327_SESSION_OP_NOT_CONNECTED
#define JAGLINK_ELM327_SESSION_OP_NEEDS_RESYNC LINK_ELM327_SESSION_OP_NEEDS_RESYNC
#define JAGLINK_ELM327_SESSION_OP_DEADLINE_OVERFLOW LINK_ELM327_SESSION_OP_DEADLINE_OVERFLOW
#define JAGLINK_ELM327_SESSION_OP_TRANSPORT_ERROR LINK_ELM327_SESSION_OP_TRANSPORT_ERROR

typedef LinkElm327SessionStatus JaglinkElm327SessionStatus;
typedef LinkElm327SessionOpResult JaglinkElm327SessionOpResult;
typedef LinkElm327Session JaglinkElm327Session;
typedef LinkElm327SessionEventFn JaglinkElm327SessionEventFn;

bool jaglink_elm327_session_init(JaglinkElm327Session *session,
                                 const JaglinkTransport *transport,
                                 JaglinkElm327SessionEventFn event,
                                 void *event_context);
void jaglink_elm327_session_deinit(JaglinkElm327Session *session);
JaglinkTransportStatus jaglink_elm327_session_connect(JaglinkElm327Session *session);
void jaglink_elm327_session_disconnect(JaglinkElm327Session *session);
bool jaglink_elm327_session_is_connected(const JaglinkElm327Session *session);
JaglinkElm327SessionOpResult jaglink_elm327_session_begin(
    JaglinkElm327Session *session,
    const char *command,
    uint64_t now_ms,
    uint64_t timeout_ms);
JaglinkElm327SessionStatus jaglink_elm327_session_tick(
    JaglinkElm327Session *session,
    uint64_t now_ms);
bool jaglink_elm327_session_cancel(JaglinkElm327Session *session);
void jaglink_elm327_session_mark_resynchronized(JaglinkElm327Session *session);
const JaglinkElm327Response *jaglink_elm327_session_response(
    const JaglinkElm327Session *session);
const char *jaglink_elm327_session_op_result_name(
    JaglinkElm327SessionOpResult result);

#ifdef __cplusplus
}
#endif

#endif
