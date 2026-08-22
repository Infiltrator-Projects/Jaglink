// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327_probe.h
 * @brief JAGLINK compatibility facade for LINK's ELM327 capability probe.
 */
#ifndef JAGLINK_ELM327_PROBE_H
#define JAGLINK_ELM327_PROBE_H

#include "link/elm327_probe.h"
#include "jaglink/elm327.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_ELM327_MAX_DEVICE_DESCRIPTION LINK_ELM327_MAX_DEVICE_DESCRIPTION
#define JAGLINK_ELM327_MAX_PROTOCOL_DESCRIPTION LINK_ELM327_MAX_PROTOCOL_DESCRIPTION
#define JAGLINK_ELM327_PROBE_DEVICE_DESCRIPTION LINK_ELM327_PROBE_DEVICE_DESCRIPTION
#define JAGLINK_ELM327_PROBE_PROTOCOL_DESCRIPTION LINK_ELM327_PROBE_PROTOCOL_DESCRIPTION
#define JAGLINK_ELM327_PROBE_PROTOCOL_NUMBER LINK_ELM327_PROBE_PROTOCOL_NUMBER
#define JAGLINK_ELM327_PROBE_COMPLETE LINK_ELM327_PROBE_COMPLETE
#define JAGLINK_ELM327_PROBE_FAILED LINK_ELM327_PROBE_FAILED

typedef LinkElm327ProbeStage JaglinkElm327ProbeStage;
typedef LinkElm327ProbeState JaglinkElm327ProbeState;

void jaglink_elm327_probe_begin(JaglinkElm327ProbeState *state);
const char *jaglink_elm327_probe_command(const JaglinkElm327ProbeState *state);
JaglinkElm327Result jaglink_elm327_probe_accept(
    JaglinkElm327ProbeState *state,
    const JaglinkElm327Response *response);

#ifdef __cplusplus
}
#endif

#endif
