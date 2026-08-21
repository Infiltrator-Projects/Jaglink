// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file obd2.h
 * @brief Portable SAE OBD-II request, parser and decoder API.
 *
 * This layer consumes normalised ELM327 responses and owns standard OBD-II
 * request construction, supported-PID discovery, common live/freeze-frame PID
 * formulas, readiness decoding, VIN extraction and diagnostic trouble codes.
 * It is independent of BLE, CoreBluetooth and manufacturer-specific data.
 */
#ifndef JAGLINK_OBD2_H
#define JAGLINK_OBD2_H

#include "jaglink/elm327.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_OBD2_VIN_LENGTH 17U
#define JAGLINK_OBD2_DTC_TEXT_LENGTH 6U
#define JAGLINK_OBD2_MAX_DTCS 64U
#define JAGLINK_OBD2_PID_SET_BYTES 32U

typedef enum {
    JAGLINK_OBD2_RESULT_OK = 0,
    JAGLINK_OBD2_RESULT_INVALID_ARGUMENT,
    JAGLINK_OBD2_RESULT_ELM_ERROR,
    JAGLINK_OBD2_RESULT_MALFORMED_RESPONSE,
    JAGLINK_OBD2_RESULT_UNEXPECTED_RESPONSE,
    JAGLINK_OBD2_RESULT_UNSUPPORTED_PID,
    JAGLINK_OBD2_RESULT_BUFFER_TOO_SMALL,
    JAGLINK_OBD2_RESULT_TOO_MANY_DTCS,
    JAGLINK_OBD2_RESULT_NOT_AUTHORIZED
} JaglinkObd2Result;

typedef enum {
    JAGLINK_OBD2_UNIT_NONE = 0,
    JAGLINK_OBD2_UNIT_PERCENT,
    JAGLINK_OBD2_UNIT_CELSIUS,
    JAGLINK_OBD2_UNIT_KPA,
    JAGLINK_OBD2_UNIT_RPM,
    JAGLINK_OBD2_UNIT_KMH,
    JAGLINK_OBD2_UNIT_GRAMS_PER_SECOND,
    JAGLINK_OBD2_UNIT_VOLTS,
    JAGLINK_OBD2_UNIT_LITRES_PER_HOUR
} JaglinkObd2Unit;

typedef struct {
    uint8_t pid;
    double value;
    JaglinkObd2Unit unit;
} JaglinkObd2Sample;

typedef struct {
    uint8_t bits[JAGLINK_OBD2_PID_SET_BYTES];
} JaglinkObd2PidSet;

typedef enum {
    JAGLINK_OBD2_DTC_STORED = 0,
    JAGLINK_OBD2_DTC_PENDING,
    JAGLINK_OBD2_DTC_PERMANENT
} JaglinkObd2DtcKind;

typedef struct {
    JaglinkObd2DtcKind kind;
    char code[JAGLINK_OBD2_DTC_TEXT_LENGTH];
} JaglinkObd2Dtc;

typedef struct {
    JaglinkObd2Dtc entries[JAGLINK_OBD2_MAX_DTCS];
    size_t count;
} JaglinkObd2DtcList;

typedef struct {
    bool mil_on;
    uint8_t confirmed_dtc_count;
    bool compression_ignition;
    uint8_t continuous_supported;
    uint8_t continuous_incomplete;
    uint8_t noncontinuous_supported;
    uint8_t noncontinuous_incomplete;
    uint8_t raw[4];
} JaglinkObd2Readiness;

typedef struct {
    bool confirmed;
    bool acknowledge_readiness_reset;
} JaglinkObd2ClearAuthorization;

#define JAGLINK_OBD2_CLEAR_AUTHORIZATION_INIT \
    { .confirmed = false, .acknowledge_readiness_reset = false }

const char *jaglink_obd2_result_name(JaglinkObd2Result result);
const char *jaglink_obd2_unit_name(JaglinkObd2Unit unit);
const char *jaglink_obd2_pid_name(uint8_t pid);

JaglinkObd2Result jaglink_obd2_build_live_pid_request(
    uint8_t pid, char *buffer, size_t buffer_size);

JaglinkObd2Result jaglink_obd2_build_freeze_pid_request(
    uint8_t pid, uint8_t frame_number, char *buffer, size_t buffer_size);

JaglinkObd2Result jaglink_obd2_build_supported_pid_request(
    uint8_t base_pid, char *buffer, size_t buffer_size);

JaglinkObd2Result jaglink_obd2_build_vin_request(
    char *buffer, size_t buffer_size);

JaglinkObd2Result jaglink_obd2_build_dtc_request(
    JaglinkObd2DtcKind kind, char *buffer, size_t buffer_size);

JaglinkObd2Result jaglink_obd2_build_clear_dtc_request(
    const JaglinkObd2ClearAuthorization *authorization,
    char *buffer, size_t buffer_size);

void jaglink_obd2_pid_set_clear(JaglinkObd2PidSet *set);
bool jaglink_obd2_pid_set_contains(const JaglinkObd2PidSet *set, uint8_t pid);

/** Union one supported-PID block transactionally; `set` changes only on OK. */
JaglinkObd2Result jaglink_obd2_accept_supported_pids(
    const JaglinkElm327Response *response,
    uint8_t base_pid,
    JaglinkObd2PidSet *set,
    bool *has_more);

JaglinkObd2Result jaglink_obd2_decode_live_pid(
    const JaglinkElm327Response *response,
    uint8_t pid,
    JaglinkObd2Sample *sample);

JaglinkObd2Result jaglink_obd2_decode_freeze_pid(
    const JaglinkElm327Response *response,
    uint8_t pid,
    uint8_t frame_number,
    JaglinkObd2Sample *sample);

JaglinkObd2Result jaglink_obd2_decode_readiness(
    const JaglinkElm327Response *response,
    JaglinkObd2Readiness *readiness);

JaglinkObd2Result jaglink_obd2_decode_vin(
    const JaglinkElm327Response *response,
    char vin[JAGLINK_OBD2_VIN_LENGTH + 1U]);

/** Decode a complete DTC response transactionally; `list` changes only on OK. */
JaglinkObd2Result jaglink_obd2_decode_dtcs(
    const JaglinkElm327Response *response,
    JaglinkObd2DtcKind kind,
    JaglinkObd2DtcList *list);

JaglinkObd2Result jaglink_obd2_decode_dtc_pair(
    uint8_t high,
    uint8_t low,
    char code[JAGLINK_OBD2_DTC_TEXT_LENGTH]);

#ifdef __cplusplus
}
#endif

#endif
