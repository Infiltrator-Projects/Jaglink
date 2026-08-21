// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file isotp.h
 * @brief Portable ISO-TP (ISO 15765-2) transport-layer foundation.
 *
 * Classical-CAN buffers are caller-owned and all protocol state is bounded.
 * Timing values use one caller-supplied monotonic microsecond clock.
 */
#ifndef JAGLINK_ISOTP_H
#define JAGLINK_ISOTP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_ISOTP_CLASSIC_CAN_DATA_LENGTH 8U
#define JAGLINK_ISOTP_MAX_PDU_LENGTH 4095U

typedef enum {
    JAGLINK_ISOTP_ADDRESSING_NORMAL = 0,
    JAGLINK_ISOTP_ADDRESSING_EXTENDED,
    JAGLINK_ISOTP_ADDRESSING_MIXED
} JaglinkIsoTpAddressingMode;

typedef enum {
    JAGLINK_ISOTP_TARGET_PHYSICAL = 0,
    JAGLINK_ISOTP_TARGET_FUNCTIONAL
} JaglinkIsoTpTargetType;

typedef enum {
    JAGLINK_ISOTP_FLOW_CONTINUE_TO_SEND = 0,
    JAGLINK_ISOTP_FLOW_WAIT = 1,
    JAGLINK_ISOTP_FLOW_OVERFLOW = 2
} JaglinkIsoTpFlowStatus;

typedef enum {
    JAGLINK_ISOTP_RESULT_OK = 0,
    JAGLINK_ISOTP_RESULT_COMPLETE,
    JAGLINK_ISOTP_RESULT_WAIT_FLOW_CONTROL,
    JAGLINK_ISOTP_RESULT_WAIT_SEPARATION,
    JAGLINK_ISOTP_RESULT_FLOW_CONTROL_WAIT,
    JAGLINK_ISOTP_RESULT_INVALID_ARGUMENT,
    JAGLINK_ISOTP_RESULT_INVALID_FRAME,
    JAGLINK_ISOTP_RESULT_UNEXPECTED_FRAME,
    JAGLINK_ISOTP_RESULT_WRONG_SEQUENCE,
    JAGLINK_ISOTP_RESULT_BUFFER_TOO_SMALL,
    JAGLINK_ISOTP_RESULT_PAYLOAD_TOO_LARGE,
    JAGLINK_ISOTP_RESULT_TIMEOUT,
    JAGLINK_ISOTP_RESULT_FLOW_CONTROL_OVERFLOW,
    JAGLINK_ISOTP_RESULT_WAIT_FRAME_LIMIT,
    JAGLINK_ISOTP_RESULT_UNSUPPORTED
} JaglinkIsoTpResult;

typedef enum {
    JAGLINK_ISOTP_RX_IDLE = 0,
    JAGLINK_ISOTP_RX_RECEIVING,
    JAGLINK_ISOTP_RX_COMPLETE,
    JAGLINK_ISOTP_RX_FAILED
} JaglinkIsoTpRxState;

typedef enum {
    JAGLINK_ISOTP_TX_IDLE = 0,
    JAGLINK_ISOTP_TX_WAIT_FLOW_CONTROL,
    JAGLINK_ISOTP_TX_SENDING,
    JAGLINK_ISOTP_TX_COMPLETE,
    JAGLINK_ISOTP_TX_FAILED
} JaglinkIsoTpTxState;

typedef struct {
    uint32_t tx_can_id;
    uint32_t rx_can_id;
    bool tx_extended_id;
    bool rx_extended_id;
    JaglinkIsoTpAddressingMode addressing_mode;
    JaglinkIsoTpTargetType target_type;
    uint8_t tx_address_extension;
    uint8_t rx_address_extension;
} JaglinkIsoTpAddress;

typedef struct {
    uint32_t can_id;
    bool extended_id;
    uint8_t length;
    uint8_t data[JAGLINK_ISOTP_CLASSIC_CAN_DATA_LENGTH];
} JaglinkIsoTpCanFrame;

typedef struct {
    JaglinkIsoTpAddress address;
    uint8_t block_size;
    uint8_t stmin;
    uint64_t consecutive_timeout_us;
} JaglinkIsoTpRxConfig;

typedef struct {
    JaglinkIsoTpRxConfig config;
    uint8_t *buffer;
    size_t capacity;
    size_t expected_length;
    size_t received_length;
    uint8_t next_sequence;
    uint8_t block_counter;
    uint64_t deadline_us;
    JaglinkIsoTpRxState state;
    JaglinkIsoTpResult failure;
} JaglinkIsoTpRx;

typedef struct {
    JaglinkIsoTpAddress address;
    uint64_t flow_control_timeout_us;
    uint8_t max_wait_frames;
} JaglinkIsoTpTxConfig;

typedef struct {
    JaglinkIsoTpTxConfig config;
    const uint8_t *payload;
    size_t payload_length;
    size_t offset;
    uint8_t next_sequence;
    uint8_t block_size;
    uint8_t block_counter;
    uint8_t wait_frame_count;
    uint32_t separation_time_us;
    uint64_t next_send_us;
    uint64_t deadline_us;
    JaglinkIsoTpTxState state;
    JaglinkIsoTpResult failure;
} JaglinkIsoTpTx;

const char *jaglink_isotp_result_name(JaglinkIsoTpResult result);
const char *jaglink_isotp_rx_state_name(JaglinkIsoTpRxState state);
const char *jaglink_isotp_tx_state_name(JaglinkIsoTpTxState state);

bool jaglink_isotp_address_is_valid(const JaglinkIsoTpAddress *address);
bool jaglink_isotp_stmin_to_us(uint8_t stmin, uint32_t *microseconds);

/** Initialise RX; `buffer` is borrowed until the receiver is no longer used. */
JaglinkIsoTpResult jaglink_isotp_rx_init(
    JaglinkIsoTpRx *receiver,
    const JaglinkIsoTpRxConfig *config,
    uint8_t *buffer,
    size_t capacity);

/** Reset RX state and clear any latched terminal failure. */
void jaglink_isotp_rx_reset(JaglinkIsoTpRx *receiver);

/** Feed one addressed CAN frame at monotonic time `now_us`. */
JaglinkIsoTpResult jaglink_isotp_rx_feed(
    JaglinkIsoTpRx *receiver,
    const JaglinkIsoTpCanFrame *frame,
    uint64_t now_us,
    JaglinkIsoTpCanFrame *flow_control_frame,
    bool *flow_control_ready);

JaglinkIsoTpResult jaglink_isotp_rx_tick(
    JaglinkIsoTpRx *receiver,
    uint64_t now_us);

const uint8_t *jaglink_isotp_rx_payload(
    const JaglinkIsoTpRx *receiver,
    size_t *length);

/**
 * Initialise TX with a borrowed payload.
 *
 * The payload must remain valid until the transmitter is reinitialised or no
 * longer used. `jaglink_isotp_tx_reset()` preserves it for retransmission.
 */
JaglinkIsoTpResult jaglink_isotp_tx_init(
    JaglinkIsoTpTx *transmitter,
    const JaglinkIsoTpTxConfig *config,
    const uint8_t *payload,
    size_t payload_length);

/** Reset TX progress/failure state while preserving configuration and payload. */
void jaglink_isotp_tx_reset(JaglinkIsoTpTx *transmitter);

JaglinkIsoTpResult jaglink_isotp_tx_start(
    JaglinkIsoTpTx *transmitter,
    uint64_t now_us,
    JaglinkIsoTpCanFrame *frame);

/**
 * Accept Flow Control addressed to this transmitter.
 *
 * Well-formed frames for another CAN/address-extension endpoint return
 * UNEXPECTED_FRAME without destroying the active transfer.
 */
JaglinkIsoTpResult jaglink_isotp_tx_accept_flow_control(
    JaglinkIsoTpTx *transmitter,
    const JaglinkIsoTpCanFrame *frame,
    uint64_t now_us);

JaglinkIsoTpResult jaglink_isotp_tx_next(
    JaglinkIsoTpTx *transmitter,
    uint64_t now_us,
    JaglinkIsoTpCanFrame *frame);

JaglinkIsoTpResult jaglink_isotp_tx_tick(
    JaglinkIsoTpTx *transmitter,
    uint64_t now_us);

#ifdef __cplusplus
}
#endif

#endif
