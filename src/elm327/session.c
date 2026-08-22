// SPDX-License-Identifier: GPL-3.0-or-later
/** @file session.c @brief JAGLINK ABI wrappers for LINK's ELM327 session engine. */
#include "jaglink/elm327_session.h"

bool jaglink_elm327_session_init(JaglinkElm327Session *session,
                                 const JaglinkTransport *transport,
                                 JaglinkElm327SessionEventFn event,
                                 void *event_context)
{
    return link_elm327_session_init(session, transport, event, event_context);
}

void jaglink_elm327_session_deinit(JaglinkElm327Session *session)
{
    link_elm327_session_deinit(session);
}

JaglinkTransportStatus jaglink_elm327_session_connect(JaglinkElm327Session *session)
{
    return link_elm327_session_connect(session);
}

void jaglink_elm327_session_disconnect(JaglinkElm327Session *session)
{
    link_elm327_session_disconnect(session);
}

bool jaglink_elm327_session_is_connected(const JaglinkElm327Session *session)
{
    return link_elm327_session_is_connected(session);
}

JaglinkElm327SessionOpResult jaglink_elm327_session_begin(
    JaglinkElm327Session *session,
    const char *command,
    uint64_t now_ms,
    uint64_t timeout_ms)
{
    return link_elm327_session_begin(session, command, now_ms, timeout_ms);
}

JaglinkElm327SessionStatus jaglink_elm327_session_tick(
    JaglinkElm327Session *session,
    uint64_t now_ms)
{
    return link_elm327_session_tick(session, now_ms);
}

bool jaglink_elm327_session_cancel(JaglinkElm327Session *session)
{
    return link_elm327_session_cancel(session);
}

void jaglink_elm327_session_mark_resynchronized(JaglinkElm327Session *session)
{
    link_elm327_session_mark_resynchronized(session);
}

const JaglinkElm327Response *jaglink_elm327_session_response(
    const JaglinkElm327Session *session)
{
    return link_elm327_session_response(session);
}

const char *jaglink_elm327_session_op_result_name(
    JaglinkElm327SessionOpResult result)
{
    return link_elm327_session_op_result_name(result);
}
