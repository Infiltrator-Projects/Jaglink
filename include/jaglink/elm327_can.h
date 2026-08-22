// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file elm327_can.h
 * @brief JAGLINK compatibility facade for LINK's ELM-managed CAN channel.
 */
#ifndef JAGLINK_ELM327_CAN_H
#define JAGLINK_ELM327_CAN_H

#include "link/elm327_can.h"
#include "jaglink/elm327.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_ELM327_CAN_MAX_REQUEST_PDU LINK_ELM327_CAN_MAX_REQUEST_PDU
#define JAGLINK_ELM327_CAN_STAGE_SET_HEADER LINK_ELM327_CAN_STAGE_SET_HEADER
#define JAGLINK_ELM327_CAN_STAGE_SET_RECEIVE_ADDRESS LINK_ELM327_CAN_STAGE_SET_RECEIVE_ADDRESS
#define JAGLINK_ELM327_CAN_STAGE_ENABLE_AUTO_FORMATTING LINK_ELM327_CAN_STAGE_ENABLE_AUTO_FORMATTING
#define JAGLINK_ELM327_CAN_STAGE_ENABLE_FLOW_CONTROL LINK_ELM327_CAN_STAGE_ENABLE_FLOW_CONTROL
#define JAGLINK_ELM327_CAN_STAGE_COMPLETE LINK_ELM327_CAN_STAGE_COMPLETE
#define JAGLINK_ELM327_CAN_STAGE_FAILED LINK_ELM327_CAN_STAGE_FAILED
#define JAGLINK_ELM327_CAN_RESULT_OK LINK_ELM327_CAN_RESULT_OK
#define JAGLINK_ELM327_CAN_RESULT_INVALID_ARGUMENT LINK_ELM327_CAN_RESULT_INVALID_ARGUMENT
#define JAGLINK_ELM327_CAN_RESULT_BUFFER_TOO_SMALL LINK_ELM327_CAN_RESULT_BUFFER_TOO_SMALL
#define JAGLINK_ELM327_CAN_RESULT_PDU_TOO_LARGE LINK_ELM327_CAN_RESULT_PDU_TOO_LARGE
#define JAGLINK_ELM327_CAN_RESULT_ELM_ERROR LINK_ELM327_CAN_RESULT_ELM_ERROR
#define JAGLINK_ELM327_CAN_RESULT_MALFORMED_RESPONSE LINK_ELM327_CAN_RESULT_MALFORMED_RESPONSE
#define JAGLINK_ELM327_CAN_RESULT_UNEXPECTED_RESPONSE LINK_ELM327_CAN_RESULT_UNEXPECTED_RESPONSE
#define JAGLINK_ELM327_CAN_RESULT_FAILED_STATE LINK_ELM327_CAN_RESULT_FAILED_STATE

typedef LinkElm327CanChannelConfig JaglinkElm327CanChannelConfig;
typedef LinkElm327CanStage JaglinkElm327CanStage;
typedef LinkElm327CanResult JaglinkElm327CanResult;
typedef LinkElm327CanChannelState JaglinkElm327CanChannelState;

const char *jaglink_elm327_can_result_name(JaglinkElm327CanResult result);
const char *jaglink_elm327_can_stage_name(JaglinkElm327CanStage stage);
bool jaglink_elm327_can_channel_config_is_valid(
    const JaglinkElm327CanChannelConfig *config);
JaglinkElm327CanResult jaglink_elm327_can_channel_begin(
    JaglinkElm327CanChannelState *state,
    const JaglinkElm327CanChannelConfig *config);
JaglinkElm327CanResult jaglink_elm327_can_channel_command(
    const JaglinkElm327CanChannelState *state,
    char *buffer,
    size_t buffer_size);
JaglinkElm327CanResult jaglink_elm327_can_channel_accept(
    JaglinkElm327CanChannelState *state,
    const JaglinkElm327Response *response);
JaglinkElm327CanResult jaglink_elm327_can_build_pdu_command(
    const uint8_t *pdu,
    size_t pdu_length,
    char *buffer,
    size_t buffer_size,
    size_t *written);
JaglinkElm327CanResult jaglink_elm327_can_decode_pdu(
    const JaglinkElm327Response *response,
    uint8_t *pdu,
    size_t pdu_size,
    size_t *pdu_length);

#ifdef __cplusplus
}
#endif

#endif
