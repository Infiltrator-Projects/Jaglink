// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327_can.h
 * @brief ELM327-managed ISO 15765 CAN diagnostic channel contracts.
 *
 * This path is for text-command adapters that perform ISO-TP formatting and
 * flow control internally. Raw CAN providers use jaglink/isotp.h instead.
 * UDS remains above both paths and consumes complete PDUs.
 */
#ifndef JAGLINK_ELM327_CAN_H
#define JAGLINK_ELM327_CAN_H

#include "jaglink/elm327.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum complete PDU that fits in one current ELM session command. */
#define JAGLINK_ELM327_CAN_MAX_REQUEST_PDU 31U

/**
 * One physical diagnostic endpoint pair.
 *
 * `extended_id == false` uses 11-bit CAN identifiers. `true` uses 29-bit CAN
 * identifiers. JAGLINK does not infer manufacturer addresses here; callers must
 * supply addresses from a validated discovery/profile source.
 */
typedef struct {
    uint32_t tx_can_id;
    uint32_t rx_can_id;
    bool extended_id;
} JaglinkElm327CanChannelConfig;

typedef enum {
    JAGLINK_ELM327_CAN_STAGE_SET_HEADER = 0,
    JAGLINK_ELM327_CAN_STAGE_SET_RECEIVE_ADDRESS,
    JAGLINK_ELM327_CAN_STAGE_ENABLE_AUTO_FORMATTING,
    JAGLINK_ELM327_CAN_STAGE_ENABLE_FLOW_CONTROL,
    JAGLINK_ELM327_CAN_STAGE_COMPLETE,
    JAGLINK_ELM327_CAN_STAGE_FAILED
} JaglinkElm327CanStage;

typedef enum {
    JAGLINK_ELM327_CAN_RESULT_OK = 0,
    JAGLINK_ELM327_CAN_RESULT_INVALID_ARGUMENT,
    JAGLINK_ELM327_CAN_RESULT_BUFFER_TOO_SMALL,
    JAGLINK_ELM327_CAN_RESULT_PDU_TOO_LARGE,
    JAGLINK_ELM327_CAN_RESULT_ELM_ERROR,
    JAGLINK_ELM327_CAN_RESULT_MALFORMED_RESPONSE,
    JAGLINK_ELM327_CAN_RESULT_UNEXPECTED_RESPONSE,
    JAGLINK_ELM327_CAN_RESULT_FAILED_STATE
} JaglinkElm327CanResult;

typedef struct {
    JaglinkElm327CanChannelConfig config;
    JaglinkElm327CanStage stage;
    JaglinkElm327CanResult failure;
    JaglinkElm327Result elm_failure;
} JaglinkElm327CanChannelState;

const char *jaglink_elm327_can_result_name(JaglinkElm327CanResult result);
const char *jaglink_elm327_can_stage_name(JaglinkElm327CanStage stage);

bool jaglink_elm327_can_channel_config_is_valid(
    const JaglinkElm327CanChannelConfig *config);

/** Initialise a configuration sequence from a validated caller-owned address. */
JaglinkElm327CanResult jaglink_elm327_can_channel_begin(
    JaglinkElm327CanChannelState *state,
    const JaglinkElm327CanChannelConfig *config);

/**
 * Build the AT command for the current configuration stage.
 *
 * Commands are `ATSH`, `ATCRA`, `ATCAF1`, then `ATCFC1`. The output buffer is
 * cleared on entry when possible. COMPLETE/FAILED states have no command.
 */
JaglinkElm327CanResult jaglink_elm327_can_channel_command(
    const JaglinkElm327CanChannelState *state,
    char *buffer,
    size_t buffer_size);

/** Accept one parsed response; every configuration stage requires `OK`. */
JaglinkElm327CanResult jaglink_elm327_can_channel_accept(
    JaglinkElm327CanChannelState *state,
    const JaglinkElm327Response *response);

/**
 * Render one complete diagnostic PDU as the hex command consumed by an
 * ELM327-managed ISO 15765 channel. No PCI/ISO-TP bytes are added here.
 */
JaglinkElm327CanResult jaglink_elm327_can_build_pdu_command(
    const uint8_t *pdu,
    size_t pdu_length,
    char *buffer,
    size_t buffer_size,
    size_t *written);

/**
 * Decode one complete PDU from an ELM response with CAN auto-formatting on.
 *
 * Both single-line hex payloads and ELM indexed multi-line output (`0:`,
 * `1:`...) are accepted. Optional three-hex-digit indexed total length is
 * validated. Caller output is copied only on complete success.
 */
JaglinkElm327CanResult jaglink_elm327_can_decode_pdu(
    const JaglinkElm327Response *response,
    uint8_t *pdu,
    size_t pdu_size,
    size_t *pdu_length);

#ifdef __cplusplus
}
#endif

#endif
