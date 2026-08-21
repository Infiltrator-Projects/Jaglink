// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/isotp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *message)
{
    fprintf(stderr, "FAIL: %s\n", message);
    exit(EXIT_FAILURE);
}

static void require(bool condition, const char *message)
{
    if (!condition) {
        fail(message);
    }
}

static JaglinkIsoTpAddress tester_address(void)
{
    JaglinkIsoTpAddress address = {
        .tx_can_id = 0x7e0U,
        .rx_can_id = 0x7e8U,
        .tx_extended_id = false,
        .rx_extended_id = false,
        .addressing_mode = JAGLINK_ISOTP_ADDRESSING_NORMAL,
        .target_type = JAGLINK_ISOTP_TARGET_PHYSICAL,
        .tx_address_extension = 0U,
        .rx_address_extension = 0U
    };
    return address;
}

static JaglinkIsoTpAddress ecu_address(void)
{
    JaglinkIsoTpAddress address = tester_address();
    address.tx_can_id = 0x7e8U;
    address.rx_can_id = 0x7e0U;
    return address;
}

static JaglinkIsoTpCanFrame make_frame(
    uint32_t can_id, const uint8_t *bytes, size_t length)
{
    JaglinkIsoTpCanFrame frame;
    memset(&frame, 0, sizeof(frame));
    require(length <= sizeof(frame.data), "frame length");
    frame.can_id = can_id;
    frame.length = (uint8_t)length;
    memcpy(frame.data, bytes, length);
    return frame;
}

static JaglinkIsoTpCanFrame make_fc(uint8_t status,
                                    uint8_t block_size,
                                    uint8_t stmin)
{
    const uint8_t bytes[] = {
        (uint8_t)(0x30U | status), block_size, stmin
    };
    return make_frame(0x7e8U, bytes, sizeof(bytes));
}

static void test_stmin_and_address_validation(void)
{
    uint32_t us = 0U;
    JaglinkIsoTpAddress address = tester_address();

    require(jaglink_isotp_stmin_to_us(0x00U, &us) && us == 0U,
            "STmin zero");
    require(jaglink_isotp_stmin_to_us(0x7fU, &us) && us == 127000U,
            "STmin milliseconds");
    require(jaglink_isotp_stmin_to_us(0xf1U, &us) && us == 100U,
            "STmin 100 us");
    require(jaglink_isotp_stmin_to_us(0xf9U, &us) && us == 900U,
            "STmin 900 us");
    require(!jaglink_isotp_stmin_to_us(0x80U, &us),
            "reserved STmin rejected");

    require(jaglink_isotp_address_is_valid(&address), "11-bit address");
    address.tx_can_id = 0x18da10f1U;
    address.rx_can_id = 0x18daf110U;
    address.tx_extended_id = true;
    address.rx_extended_id = true;
    require(jaglink_isotp_address_is_valid(&address), "29-bit address");
    address.rx_can_id = 0x20000000U;
    require(!jaglink_isotp_address_is_valid(&address),
            "invalid 29-bit address rejected");
}

static void test_single_frames(void)
{
    uint8_t rx_buffer[16];
    JaglinkIsoTpRx receiver;
    JaglinkIsoTpCanFrame fc;
    bool fc_ready = false;
    const JaglinkIsoTpRxConfig rx_config = {
        .address = tester_address(),
        .block_size = 0U,
        .stmin = 0U,
        .consecutive_timeout_us = 100000U
    };
    const uint8_t raw[] = { 0x03U, 0x62U, 0xf1U, 0x90U };
    JaglinkIsoTpCanFrame frame = make_frame(0x7e8U, raw, sizeof(raw));
    const uint8_t tx_payload[] = { 0x22U, 0xf1U, 0x90U };
    const JaglinkIsoTpTxConfig tx_config = {
        .address = tester_address(),
        .flow_control_timeout_us = 100000U,
        .max_wait_frames = 2U
    };
    JaglinkIsoTpTx transmitter;
    size_t length = 0U;
    const uint8_t *payload;

    require(jaglink_isotp_rx_init(&receiver, &rx_config,
                                 rx_buffer, sizeof(rx_buffer)) ==
            JAGLINK_ISOTP_RESULT_OK, "single RX init");
    require(jaglink_isotp_rx_feed(&receiver, &frame, 0U,
                                 &fc, &fc_ready) ==
            JAGLINK_ISOTP_RESULT_COMPLETE, "single RX complete");
    require(!fc_ready, "single RX needs no FC");
    payload = jaglink_isotp_rx_payload(&receiver, &length);
    require(payload != NULL && length == 3U &&
            memcmp(payload, &raw[1], 3U) == 0,
            "single RX payload");

    require(jaglink_isotp_tx_init(&transmitter, &tx_config,
                                 tx_payload, sizeof(tx_payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "single TX init");
    require(jaglink_isotp_tx_start(&transmitter, 0U, &frame) ==
            JAGLINK_ISOTP_RESULT_COMPLETE, "single TX complete");
    require(frame.can_id == 0x7e0U && frame.length == 4U &&
            frame.data[0] == 0x03U &&
            memcmp(&frame.data[1], tx_payload, sizeof(tx_payload)) == 0,
            "single TX bytes");
}

static void test_extended_addressing(void)
{
    uint8_t buffer[16];
    JaglinkIsoTpRx receiver;
    JaglinkIsoTpCanFrame fc;
    bool fc_ready = false;
    JaglinkIsoTpRxConfig config = {
        .address = tester_address(),
        .block_size = 0U,
        .stmin = 0U,
        .consecutive_timeout_us = 100000U
    };
    const uint8_t raw[] = { 0xf1U, 0x03U, 0x62U, 0x01U, 0x02U };
    JaglinkIsoTpCanFrame frame = make_frame(0x7e8U, raw, sizeof(raw));

    config.address.addressing_mode = JAGLINK_ISOTP_ADDRESSING_EXTENDED;
    config.address.tx_address_extension = 0xf1U;
    config.address.rx_address_extension = 0xf1U;
    require(jaglink_isotp_rx_init(&receiver, &config,
                                 buffer, sizeof(buffer)) ==
            JAGLINK_ISOTP_RESULT_OK, "extended init");
    require(jaglink_isotp_rx_feed(&receiver, &frame, 0U,
                                 &fc, &fc_ready) ==
            JAGLINK_ISOTP_RESULT_COMPLETE, "extended single frame");
    require(receiver.received_length == 3U && buffer[0] == 0x62U,
            "extended payload");
}

static void test_end_to_end_multiframe(void)
{
    uint8_t payload[130];
    uint8_t received[sizeof(payload)];
    JaglinkIsoTpTx transmitter;
    JaglinkIsoTpRx receiver;
    const JaglinkIsoTpTxConfig tx_config = {
        .address = tester_address(),
        .flow_control_timeout_us = 10000U,
        .max_wait_frames = 2U
    };
    const JaglinkIsoTpRxConfig rx_config = {
        .address = ecu_address(),
        .block_size = 4U,
        .stmin = 0xf1U,
        .consecutive_timeout_us = 10000U
    };
    JaglinkIsoTpCanFrame data_frame;
    JaglinkIsoTpCanFrame fc_frame;
    JaglinkIsoTpResult tx_result;
    JaglinkIsoTpResult rx_result;
    bool fc_ready = false;
    uint64_t now_us = 0U;
    size_t iterations = 0U;
    size_t length = 0U;
    const uint8_t *complete;

    for (size_t i = 0U; i < sizeof(payload); ++i) {
        payload[i] = (uint8_t)(i ^ 0x5aU);
    }

    require(jaglink_isotp_tx_init(&transmitter, &tx_config,
                                 payload, sizeof(payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "multi TX init");
    require(jaglink_isotp_rx_init(&receiver, &rx_config,
                                 received, sizeof(received)) ==
            JAGLINK_ISOTP_RESULT_OK, "multi RX init");

    tx_result = jaglink_isotp_tx_start(&transmitter, now_us, &data_frame);
    require(tx_result == JAGLINK_ISOTP_RESULT_WAIT_FLOW_CONTROL,
            "first frame emitted");
    rx_result = jaglink_isotp_rx_feed(&receiver, &data_frame, now_us,
                                     &fc_frame, &fc_ready);
    require(rx_result == JAGLINK_ISOTP_RESULT_OK && fc_ready,
            "first frame produces FC");
    require(jaglink_isotp_tx_accept_flow_control(&transmitter,
                                                &fc_frame, now_us) ==
            JAGLINK_ISOTP_RESULT_OK, "first FC accepted");
    require(transmitter.separation_time_us == 100U,
            "sub-millisecond STmin preserved");

    while (transmitter.state != JAGLINK_ISOTP_TX_COMPLETE) {
        require(++iterations < 100U, "bounded segmentation loop");
        tx_result = jaglink_isotp_tx_next(&transmitter,
                                         now_us, &data_frame);
        if (tx_result == JAGLINK_ISOTP_RESULT_WAIT_SEPARATION) {
            now_us = transmitter.next_send_us;
            continue;
        }
        require(tx_result == JAGLINK_ISOTP_RESULT_OK ||
                tx_result == JAGLINK_ISOTP_RESULT_COMPLETE,
                "consecutive frame emitted");

        fc_ready = false;
        rx_result = jaglink_isotp_rx_feed(&receiver, &data_frame, now_us,
                                         &fc_frame, &fc_ready);
        require(rx_result == JAGLINK_ISOTP_RESULT_OK ||
                rx_result == JAGLINK_ISOTP_RESULT_COMPLETE,
                "consecutive frame accepted");
        if (fc_ready) {
            require(jaglink_isotp_tx_accept_flow_control(&transmitter,
                                                        &fc_frame, now_us) ==
                    JAGLINK_ISOTP_RESULT_OK, "block FC accepted");
        }
        now_us += 100U;
    }

    require(receiver.state == JAGLINK_ISOTP_RX_COMPLETE,
            "receiver completed");
    complete = jaglink_isotp_rx_payload(&receiver, &length);
    require(complete != NULL && length == sizeof(payload) &&
            memcmp(complete, payload, sizeof(payload)) == 0,
            "reassembled payload exact");
    require(iterations > 16U, "sequence number wrapped");
}

static void test_receive_errors(void)
{
    uint8_t buffer[16];
    JaglinkIsoTpRx receiver;
    JaglinkIsoTpCanFrame fc;
    bool fc_ready = false;
    const JaglinkIsoTpRxConfig config = {
        .address = tester_address(),
        .block_size = 0U,
        .stmin = 0U,
        .consecutive_timeout_us = 1000U
    };
    const uint8_t ff[] = { 0x10U, 0x0aU, 1U, 2U, 3U, 4U, 5U, 6U };
    const uint8_t wrong[] = { 0x22U, 7U, 8U, 9U, 10U };
    JaglinkIsoTpCanFrame frame = make_frame(0x7e8U, ff, sizeof(ff));

    require(jaglink_isotp_rx_init(&receiver, &config,
                                 buffer, sizeof(buffer)) ==
            JAGLINK_ISOTP_RESULT_OK, "error RX init");
    require(jaglink_isotp_rx_feed(&receiver, &frame, 100U,
                                 &fc, &fc_ready) ==
            JAGLINK_ISOTP_RESULT_OK, "error first frame");
    frame = make_frame(0x7e8U, wrong, sizeof(wrong));
    require(jaglink_isotp_rx_feed(&receiver, &frame, 200U,
                                 &fc, &fc_ready) ==
            JAGLINK_ISOTP_RESULT_WRONG_SEQUENCE,
            "wrong sequence rejected");

    jaglink_isotp_rx_reset(&receiver);
    frame = make_frame(0x7e8U, ff, sizeof(ff));
    require(jaglink_isotp_rx_feed(&receiver, &frame, 100U,
                                 &fc, &fc_ready) ==
            JAGLINK_ISOTP_RESULT_OK, "timeout first frame");
    require(jaglink_isotp_rx_tick(&receiver, 1099U) ==
            JAGLINK_ISOTP_RESULT_OK, "before RX timeout");
    require(jaglink_isotp_rx_tick(&receiver, 1100U) ==
            JAGLINK_ISOTP_RESULT_TIMEOUT, "RX timeout");

    {
        uint8_t small[8];
        JaglinkIsoTpRx small_receiver;
        const uint8_t large_ff[] = {
            0x10U, 0x20U, 1U, 2U, 3U, 4U, 5U, 6U
        };
        frame = make_frame(0x7e8U, large_ff, sizeof(large_ff));
        require(jaglink_isotp_rx_init(&small_receiver, &config,
                                     small, sizeof(small)) ==
                JAGLINK_ISOTP_RESULT_OK, "small RX init");
        fc_ready = false;
        require(jaglink_isotp_rx_feed(&small_receiver, &frame, 0U,
                                     &fc, &fc_ready) ==
                JAGLINK_ISOTP_RESULT_BUFFER_TOO_SMALL,
                "buffer overflow rejected");
        require(fc_ready && (fc.data[0] & 0x0fU) ==
                JAGLINK_ISOTP_FLOW_OVERFLOW,
                "overflow FC emitted");
    }
}

static void test_transmit_flow_control_errors(void)
{
    uint8_t payload[20] = {0};
    JaglinkIsoTpTx transmitter;
    const JaglinkIsoTpTxConfig config = {
        .address = tester_address(),
        .flow_control_timeout_us = 1000U,
        .max_wait_frames = 2U
    };
    JaglinkIsoTpCanFrame frame;
    JaglinkIsoTpCanFrame fc;

    require(jaglink_isotp_tx_init(&transmitter, &config,
                                 payload, sizeof(payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "WAIT init");
    require(jaglink_isotp_tx_start(&transmitter, 0U, &frame) ==
            JAGLINK_ISOTP_RESULT_WAIT_FLOW_CONTROL, "WAIT first frame");
    fc = make_fc(JAGLINK_ISOTP_FLOW_WAIT, 0U, 0U);
    require(jaglink_isotp_tx_accept_flow_control(&transmitter, &fc, 100U) ==
            JAGLINK_ISOTP_RESULT_FLOW_CONTROL_WAIT, "WAIT 1");
    require(jaglink_isotp_tx_accept_flow_control(&transmitter, &fc, 200U) ==
            JAGLINK_ISOTP_RESULT_FLOW_CONTROL_WAIT, "WAIT 2");
    require(jaglink_isotp_tx_accept_flow_control(&transmitter, &fc, 300U) ==
            JAGLINK_ISOTP_RESULT_WAIT_FRAME_LIMIT, "WAIT limit");

    require(jaglink_isotp_tx_init(&transmitter, &config,
                                 payload, sizeof(payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "overflow TX init");
    require(jaglink_isotp_tx_start(&transmitter, 0U, &frame) ==
            JAGLINK_ISOTP_RESULT_WAIT_FLOW_CONTROL, "overflow first frame");
    fc = make_fc(JAGLINK_ISOTP_FLOW_OVERFLOW, 0U, 0U);
    require(jaglink_isotp_tx_accept_flow_control(&transmitter, &fc, 100U) ==
            JAGLINK_ISOTP_RESULT_FLOW_CONTROL_OVERFLOW,
            "FC overflow rejected");

    require(jaglink_isotp_tx_init(&transmitter, &config,
                                 payload, sizeof(payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "timeout TX init");
    require(jaglink_isotp_tx_start(&transmitter, 500U, &frame) ==
            JAGLINK_ISOTP_RESULT_WAIT_FLOW_CONTROL, "timeout first frame");
    require(jaglink_isotp_tx_tick(&transmitter, 1499U) ==
            JAGLINK_ISOTP_RESULT_WAIT_FLOW_CONTROL, "before TX timeout");
    require(jaglink_isotp_tx_tick(&transmitter, 1500U) ==
            JAGLINK_ISOTP_RESULT_TIMEOUT, "TX timeout");
}

static void test_functional_multiframe_rejected(void)
{
    uint8_t payload[8] = {0};
    JaglinkIsoTpTx transmitter;
    JaglinkIsoTpTxConfig config = {
        .address = tester_address(),
        .flow_control_timeout_us = 1000U,
        .max_wait_frames = 0U
    };
    JaglinkIsoTpCanFrame frame;

    config.address.target_type = JAGLINK_ISOTP_TARGET_FUNCTIONAL;
    require(jaglink_isotp_tx_init(&transmitter, &config,
                                 payload, sizeof(payload)) ==
            JAGLINK_ISOTP_RESULT_OK, "functional init");
    require(jaglink_isotp_tx_start(&transmitter, 0U, &frame) ==
            JAGLINK_ISOTP_RESULT_UNSUPPORTED,
            "functional multi-frame rejected");
}

int main(void)
{
    test_stmin_and_address_validation();
    test_single_frames();
    test_extended_addressing();
    test_end_to_end_multiframe();
    test_receive_errors();
    test_transmit_flow_control_errors();
    test_functional_multiframe_rejected();
    puts("ISO-TP tests passed.");
    return EXIT_SUCCESS;
}
