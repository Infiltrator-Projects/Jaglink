// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file uds.h
 * @brief Portable ISO 14229 UDS request/response and client-state foundation.
 *
 * UDS consumes complete diagnostic PDUs. ISO-TP/CAN segmentation and concrete
 * transports remain outside this layer. All response data pointers are borrowed
 * from the caller-supplied PDU and remain valid only while that PDU is valid.
 */
#ifndef JAGLINK_UDS_H
#define JAGLINK_UDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_UDS_SERVICE_DIAGNOSTIC_SESSION_CONTROL 0x10U
#define JAGLINK_UDS_SERVICE_READ_DATA_BY_IDENTIFIER 0x22U
#define JAGLINK_UDS_SERVICE_TESTER_PRESENT 0x3eU
#define JAGLINK_UDS_SERVICE_NEGATIVE_RESPONSE 0x7fU

#define JAGLINK_UDS_SESSION_DEFAULT 0x01U
#define JAGLINK_UDS_SESSION_PROGRAMMING 0x02U
#define JAGLINK_UDS_SESSION_EXTENDED 0x03U
#define JAGLINK_UDS_SESSION_SAFETY_SYSTEM 0x04U

#define JAGLINK_UDS_NRC_RESPONSE_PENDING 0x78U

typedef enum {
    JAGLINK_UDS_RESULT_OK = 0,
    JAGLINK_UDS_RESULT_COMPLETE,
    JAGLINK_UDS_RESULT_WAITING,
    JAGLINK_UDS_RESULT_RESPONSE_PENDING,
    JAGLINK_UDS_RESULT_NEGATIVE_RESPONSE,
    JAGLINK_UDS_RESULT_INVALID_ARGUMENT,
    JAGLINK_UDS_RESULT_BUFFER_TOO_SMALL,
    JAGLINK_UDS_RESULT_MALFORMED_PDU,
    JAGLINK_UDS_RESULT_UNEXPECTED_RESPONSE,
    JAGLINK_UDS_RESULT_BUSY,
    JAGLINK_UDS_RESULT_TIMEOUT,
    JAGLINK_UDS_RESULT_FAILED_STATE,
    JAGLINK_UDS_RESULT_UNSUPPORTED
} JaglinkUdsResult;

typedef enum {
    JAGLINK_UDS_RESPONSE_POSITIVE = 0,
    JAGLINK_UDS_RESPONSE_NEGATIVE
} JaglinkUdsResponseKind;

typedef struct {
    JaglinkUdsResponseKind kind;
    uint8_t request_service;
    uint8_t response_service;
    uint8_t negative_response_code;
    const uint8_t *data;
    size_t data_length;
} JaglinkUdsResponse;

typedef struct {
    uint8_t session_type;
    bool timing_present;
    uint16_t p2_server_max_ms;
    uint16_t p2_star_server_max_10ms;
} JaglinkUdsSessionResponse;

typedef struct {
    uint16_t identifier;
    const uint8_t *data;
    size_t data_length;
} JaglinkUdsDidRecord;

/** Caller/manufacturer-owned DID metadata consumed by the generic UDS layer. */
typedef struct {
    uint16_t identifier;
    const char *key;
    const char *name;
    size_t minimum_length;
    size_t maximum_length;
} JaglinkUdsDidDefinition;

typedef struct {
    const JaglinkUdsDidDefinition *definition;
    const uint8_t *data;
    size_t data_length;
} JaglinkUdsDidValue;

typedef struct {
    uint64_t p2_timeout_us;
    uint64_t p2_star_timeout_us;
} JaglinkUdsClientConfig;

typedef enum {
    JAGLINK_UDS_CLIENT_IDLE = 0,
    JAGLINK_UDS_CLIENT_WAITING_RESPONSE,
    JAGLINK_UDS_CLIENT_RESPONSE_PENDING,
    JAGLINK_UDS_CLIENT_COMPLETE,
    JAGLINK_UDS_CLIENT_FAILED
} JaglinkUdsClientState;

typedef struct {
    JaglinkUdsClientConfig initial_config;
    uint64_t p2_timeout_us;
    uint64_t p2_star_timeout_us;
    uint64_t deadline_us;
    uint8_t request_service;
    uint8_t request_subfunction;
    uint16_t request_did;
    uint8_t active_session;
    bool request_has_subfunction;
    bool request_has_did;
    JaglinkUdsClientState state;
    JaglinkUdsResult failure;
} JaglinkUdsClient;

const char *jaglink_uds_result_name(JaglinkUdsResult result);
const char *jaglink_uds_client_state_name(JaglinkUdsClientState state);
const char *jaglink_uds_negative_response_code_name(uint8_t code);

/** Decode one complete UDS response transactionally. */
JaglinkUdsResult jaglink_uds_decode_response(
    uint8_t request_service,
    const uint8_t *pdu,
    size_t pdu_length,
    JaglinkUdsResponse *response);

JaglinkUdsResult jaglink_uds_build_session_control_request(
    uint8_t session_type,
    bool suppress_positive_response,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *written);

JaglinkUdsResult jaglink_uds_decode_session_control_response(
    const uint8_t *pdu,
    size_t pdu_length,
    uint8_t expected_session_type,
    JaglinkUdsSessionResponse *response);

JaglinkUdsResult jaglink_uds_build_tester_present_request(
    bool suppress_positive_response,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *written);

JaglinkUdsResult jaglink_uds_decode_tester_present_response(
    const uint8_t *pdu,
    size_t pdu_length);

JaglinkUdsResult jaglink_uds_build_read_did_request(
    uint16_t identifier,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *written);

JaglinkUdsResult jaglink_uds_decode_read_did_response(
    const uint8_t *pdu,
    size_t pdu_length,
    uint16_t expected_identifier,
    JaglinkUdsDidRecord *record);

bool jaglink_uds_did_definition_is_valid(
    const JaglinkUdsDidDefinition *definition);

JaglinkUdsResult jaglink_uds_build_defined_did_request(
    const JaglinkUdsDidDefinition *definition,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *written);

JaglinkUdsResult jaglink_uds_decode_defined_did_response(
    const uint8_t *pdu,
    size_t pdu_length,
    const JaglinkUdsDidDefinition *definition,
    JaglinkUdsDidValue *value);

JaglinkUdsResult jaglink_uds_client_init(
    JaglinkUdsClient *client,
    const JaglinkUdsClientConfig *config);

/** Restore initial timing, default session and idle state. */
void jaglink_uds_client_reset(JaglinkUdsClient *client);

/**
 * Begin tracking a complete UDS request PDU at monotonic time `now_us`.
 *
 * Tracked DiagnosticSessionControl and TesterPresent requests must request a
 * positive response; suppress-positive operation is left to one-shot callers.
 */
JaglinkUdsResult jaglink_uds_client_begin(
    JaglinkUdsClient *client,
    const uint8_t *request_pdu,
    size_t request_length,
    uint64_t now_us);

/** Consume one complete response PDU for the active request. */
JaglinkUdsResult jaglink_uds_client_accept(
    JaglinkUdsClient *client,
    const uint8_t *response_pdu,
    size_t response_length,
    uint64_t now_us,
    JaglinkUdsResponse *response);

JaglinkUdsResult jaglink_uds_client_tick(
    JaglinkUdsClient *client,
    uint64_t now_us);

#ifdef __cplusplus
}
#endif

#endif
