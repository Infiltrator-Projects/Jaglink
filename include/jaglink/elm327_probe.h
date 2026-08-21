// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327_probe.h
 * @brief Adapter and active-protocol capability probing for ELM327 devices.
 */
#ifndef JAGLINK_ELM327_PROBE_H
#define JAGLINK_ELM327_PROBE_H

#include "jaglink/elm327.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_ELM327_MAX_DEVICE_DESCRIPTION 96U
#define JAGLINK_ELM327_MAX_PROTOCOL_DESCRIPTION 128U

typedef enum {
    JAGLINK_ELM327_PROBE_DEVICE_DESCRIPTION = 0,
    JAGLINK_ELM327_PROBE_PROTOCOL_DESCRIPTION,
    JAGLINK_ELM327_PROBE_PROTOCOL_NUMBER,
    JAGLINK_ELM327_PROBE_COMPLETE,
    JAGLINK_ELM327_PROBE_FAILED
} JaglinkElm327ProbeStage;

typedef struct {
    JaglinkElm327ProbeStage stage;
    JaglinkElm327Result failure;
    bool device_description_supported;
    bool protocol_was_automatic;
    uint8_t protocol_number;
    char device_description[JAGLINK_ELM327_MAX_DEVICE_DESCRIPTION];
    char protocol_description[JAGLINK_ELM327_MAX_PROTOCOL_DESCRIPTION];
} JaglinkElm327ProbeState;

/** Begin the deterministic AT@1 -> ATDP -> ATDPN probe sequence. */
void jaglink_elm327_probe_begin(JaglinkElm327ProbeState *state);

/** Return the command required by the current probe stage. */
const char *jaglink_elm327_probe_command(
    const JaglinkElm327ProbeState *state);

/**
 * Accept the parsed response for the current probe stage.
 *
 * AT@1 is optional because compatible adapters may not implement it. ATDP and
 * ATDPN are required to establish the currently selected OBD protocol.
 */
JaglinkElm327Result jaglink_elm327_probe_accept(
    JaglinkElm327ProbeState *state,
    const JaglinkElm327Response *response);

#ifdef __cplusplus
}
#endif

#endif
