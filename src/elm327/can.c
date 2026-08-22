// SPDX-License-Identifier: GPL-3.0-or-later
/** @file can.c @brief JAGLINK ABI wrappers for LINK's ELM-managed CAN channel. */
#include "jaglink/elm327_can.h"

const char *jaglink_elm327_can_result_name(JaglinkElm327CanResult result)
{
    return link_elm327_can_result_name(result);
}

const char *jaglink_elm327_can_stage_name(JaglinkElm327CanStage stage)
{
    return link_elm327_can_stage_name(stage);
}

bool jaglink_elm327_can_channel_config_is_valid(
    const JaglinkElm327CanChannelConfig *config)
{
    return link_elm327_can_channel_config_is_valid(config);
}

JaglinkElm327CanResult jaglink_elm327_can_channel_begin(
    JaglinkElm327CanChannelState *state,
    const JaglinkElm327CanChannelConfig *config)
{
    return link_elm327_can_channel_begin(state, config);
}

JaglinkElm327CanResult jaglink_elm327_can_channel_command(
    const JaglinkElm327CanChannelState *state,
    char *buffer,
    size_t buffer_size)
{
    return link_elm327_can_channel_command(state, buffer, buffer_size);
}

JaglinkElm327CanResult jaglink_elm327_can_channel_accept(
    JaglinkElm327CanChannelState *state,
    const JaglinkElm327Response *response)
{
    return link_elm327_can_channel_accept(state, response);
}

JaglinkElm327CanResult jaglink_elm327_can_build_pdu_command(
    const uint8_t *pdu,
    size_t pdu_length,
    char *buffer,
    size_t buffer_size,
    size_t *written)
{
    return link_elm327_can_build_pdu_command(
        pdu, pdu_length, buffer, buffer_size, written);
}

JaglinkElm327CanResult jaglink_elm327_can_decode_pdu(
    const JaglinkElm327Response *response,
    uint8_t *pdu,
    size_t pdu_size,
    size_t *pdu_length)
{
    return link_elm327_can_decode_pdu(response, pdu, pdu_size, pdu_length);
}
