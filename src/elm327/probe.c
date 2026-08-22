// SPDX-License-Identifier: GPL-3.0-or-later
/** @file probe.c @brief JAGLINK ABI wrappers for LINK's ELM327 capability probe. */
#include "jaglink/elm327_probe.h"

void jaglink_elm327_probe_begin(JaglinkElm327ProbeState *state)
{
    link_elm327_probe_begin(state);
}

const char *jaglink_elm327_probe_command(const JaglinkElm327ProbeState *state)
{
    return link_elm327_probe_command(state);
}

JaglinkElm327Result jaglink_elm327_probe_accept(
    JaglinkElm327ProbeState *state,
    const JaglinkElm327Response *response)
{
    return link_elm327_probe_accept(state, response);
}
