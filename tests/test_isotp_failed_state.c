// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/isotp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(EXIT_FAILURE);
    }
}

static JaglinkIsoTpAddress address(void)
{
    const JaglinkIsoTpAddress value = {
        .tx_can_id = 0x7e0U,
        .rx_can_id = 0x7e8U,
        .tx_extended_id = false,
        .rx_extended_id = false,
        .addressing_mode = JAGLINK_ISOTP_ADDRESSING_NORMAL,
        .target_type = JAGLINK_ISOTP_TARGET_PHYSICAL,
        .tx_address_extension = 0U,
        .rx_address_extension = 0U
    };
    return value;
}

static void test_rx_failed_state(void)
{
    uint8_t buffer[32];
    JaglinkIsoTpRx receiver;
    JaglinkIsoTpCanFrame fc = {0};
    bool fc_ready = false;
    const JaglinkIsoTpRxConfig config = {
        .address = address(),
        .block_size = 0U,
        .stmin = 0U,
        .consecutive_timeout_us = 1000U
    };
    const JaglinkIsoTpCanFrame first = {
        .can_id = 0x7e8U,
        .extended_id = false,
        .length = 8U,
        .data = {0x10U, 0x0aU, 1U, 2U, 3U, 4U, 5U, 6U}
    };
    const JaglinkIsoTpCanFrame wrong = {
        .can_id = 0x7e8U,
        .extended_id = false,
        .length = 5U,
        .data = {0x22U, 7U, 8U, 9U, 10U}
    };

    require(jaglink_isotp_rx_init(&receiver, &config,
                                 buffer, sizeof(buffer)) ==
            JAGLINK_ISOTP_RESULT_OK, "RX init");
    require(jaglink_isotp_rx_feed(&receiver, &first, 0U,
                                 &fc, &fc_ready) ==
            JAGLINK_ISOTP_RESULT_OK, "RX first frame");
    require(jaglink_isotp_rx_feed(&receiver, &wrong, 10U,
                                 &fc, &fc_ready) ==
            JAGLINK_ISOTP_RESULT_WRONG_SEQUENCE,
            "RX wrong sequence");
    require(receiver.state == JAGLINK_ISOTP_RX_FAILED &&
            receiver.failure == JAGLINK_ISOTP_RESULT_WRONG_SEQUENCE,
            "RX latches original failure");
    require(jaglink_isotp_rx_tick(&receiver, 20U) ==
            JAGLINK_ISOTP_RESULT_WRONG_SEQUENCE,
            "RX tick preserves failure cause");
    require(jaglink_isotp_rx_feed(&receiver, &first, 20U,
                                 &fc, &fc_ready) ==
            JAGLINK_ISOTP_RESULT_WRONG_SEQUENCE,
            "RX feed preserves failure cause");
    jaglink_isotp_rx_reset(&receiver);
    require(receiver.state == JAGLINK_ISOTP_RX_IDLE &&
            receiver.failure == JAGLINK_ISOTP_RESULT_OK,
            "RX reset clears failed state and cause");
}

static void test_tx_failed_state(void)
{
    uint8_t payload[20] = {0};
    JaglinkIsoTpTx transmitter;
    JaglinkIsoTpCanFrame first;
    JaglinkIsoTpCanFrame next;
    const JaglinkIsoTpTxConfig config = {
        .address = address(),
        .flow_control_timeout_us = 1000U,
        .max_wait_frames = 0U
    };
    const JaglinkIsoTpCanFrame overflow = {
        .can_id = 0x7e8U,
        .extended_id = false,
        .length = 3U,
        .data = {0x32U, 0U, 0U}
    };

    require(jaglink_isotp_tx_init(&transmitter, &config,
                                 payload, sizeof(payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "TX init");
    require(jaglink_isotp_tx_start(&transmitter, 0U, &first) ==
            JAGLINK_ISOTP_RESULT_WAIT_FLOW_CONTROL,
            "TX first frame");
    require(jaglink_isotp_tx_accept_flow_control(&transmitter,
                                                &overflow, 10U) ==
            JAGLINK_ISOTP_RESULT_FLOW_CONTROL_OVERFLOW,
            "TX overflow failure");
    require(transmitter.state == JAGLINK_ISOTP_TX_FAILED &&
            transmitter.failure == JAGLINK_ISOTP_RESULT_FLOW_CONTROL_OVERFLOW,
            "TX latches original failure");
    require(jaglink_isotp_tx_tick(&transmitter, 20U) ==
            JAGLINK_ISOTP_RESULT_FLOW_CONTROL_OVERFLOW,
            "TX tick preserves failure cause");
    require(jaglink_isotp_tx_next(&transmitter, 20U, &next) ==
            JAGLINK_ISOTP_RESULT_FLOW_CONTROL_OVERFLOW,
            "TX next preserves failure cause");
    jaglink_isotp_tx_reset(&transmitter);
    require(transmitter.state == JAGLINK_ISOTP_TX_IDLE &&
            transmitter.failure == JAGLINK_ISOTP_RESULT_OK,
            "TX reset clears failed state and cause");
}

static void test_tx_reset_reuses_payload(void)
{
    static const uint8_t payload[] = {0x22U, 0xf1U, 0x90U};
    JaglinkIsoTpTx transmitter;
    JaglinkIsoTpCanFrame first;
    JaglinkIsoTpCanFrame second;
    const JaglinkIsoTpTxConfig config = {
        .address = address(),
        .flow_control_timeout_us = 1000U,
        .max_wait_frames = 0U
    };

    require(jaglink_isotp_tx_init(&transmitter, &config,
                                 payload, sizeof(payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "retransmit TX init");
    require(jaglink_isotp_tx_start(&transmitter, 0U, &first) ==
            JAGLINK_ISOTP_RESULT_COMPLETE,
            "first single-frame transmit completes");

    jaglink_isotp_tx_reset(&transmitter);
    require(transmitter.state == JAGLINK_ISOTP_TX_IDLE &&
            transmitter.payload == payload &&
            transmitter.payload_length == sizeof(payload),
            "TX reset preserves borrowed payload");
    require(jaglink_isotp_tx_start(&transmitter, 100U, &second) ==
            JAGLINK_ISOTP_RESULT_COMPLETE,
            "reset transmitter can retransmit payload");
    require(first.length == second.length &&
            first.can_id == second.can_id &&
            memcmp(first.data, second.data, first.length) == 0,
            "retransmitted frame matches original");
}

static void test_unrelated_flow_control_is_non_terminal(void)
{
    uint8_t payload[20] = {0};
    JaglinkIsoTpTx transmitter;
    JaglinkIsoTpCanFrame first;
    const JaglinkIsoTpTxConfig config = {
        .address = address(),
        .flow_control_timeout_us = 1000U,
        .max_wait_frames = 0U
    };
    const JaglinkIsoTpCanFrame unrelated = {
        .can_id = 0x7e9U,
        .extended_id = false,
        .length = 3U,
        .data = {0x30U, 0U, 0U}
    };
    const JaglinkIsoTpCanFrame expected = {
        .can_id = 0x7e8U,
        .extended_id = false,
        .length = 3U,
        .data = {0x30U, 0U, 0U}
    };

    require(jaglink_isotp_tx_init(&transmitter, &config,
                                 payload, sizeof(payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "unrelated TX init");
    require(jaglink_isotp_tx_start(&transmitter, 0U, &first) ==
            JAGLINK_ISOTP_RESULT_WAIT_FLOW_CONTROL,
            "unrelated TX waits for FC");
    require(jaglink_isotp_tx_accept_flow_control(
                &transmitter, &unrelated, 10U) ==
            JAGLINK_ISOTP_RESULT_UNEXPECTED_FRAME,
            "unrelated FC is rejected");
    require(transmitter.state == JAGLINK_ISOTP_TX_WAIT_FLOW_CONTROL &&
            transmitter.failure == JAGLINK_ISOTP_RESULT_OK,
            "unrelated FC does not poison transmitter");
    require(jaglink_isotp_tx_accept_flow_control(
                &transmitter, &expected, 20U) ==
            JAGLINK_ISOTP_RESULT_OK,
            "expected FC still accepted afterward");
    require(transmitter.state == JAGLINK_ISOTP_TX_SENDING,
            "TX continues after unrelated bus traffic");
}

int main(void)
{
    test_rx_failed_state();
    test_tx_failed_state();
    test_tx_reset_reuses_payload();
    test_unrelated_flow_control_is_non_terminal();
    puts("ISO-TP failed-state tests passed.");
    return EXIT_SUCCESS;
}
