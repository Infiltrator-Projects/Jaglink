/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef JAGLINK_DISCOVER_H
#define JAGLINK_DISCOVER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum jaglink_safety_decision {
    JAGLINK_SAFETY_BLOCK = 0,
    JAGLINK_SAFETY_ALLOW_READ_ONLY = 1
} jaglink_safety_decision;

typedef enum jaglink_safety_reason {
    JAGLINK_SAFETY_REASON_ALLOWED_OBD_READ = 0,
    JAGLINK_SAFETY_REASON_ALLOWED_UDS_READ,
    JAGLINK_SAFETY_REASON_EMPTY_REQUEST,
    JAGLINK_SAFETY_REASON_WRITE_OR_CONTROL,
    JAGLINK_SAFETY_REASON_ECU_RESET,
    JAGLINK_SAFETY_REASON_SECURITY_ACCESS,
    JAGLINK_SAFETY_REASON_ROUTINE_CONTROL,
    JAGLINK_SAFETY_REASON_DTC_CLEAR,
    JAGLINK_SAFETY_REASON_PROGRAMMING,
    JAGLINK_SAFETY_REASON_DENY_BY_DEFAULT
} jaglink_safety_reason;

typedef struct jaglink_safety_result {
    jaglink_safety_decision decision;
    jaglink_safety_reason reason;
    uint8_t service;
} jaglink_safety_result;

jaglink_safety_result jaglink_safety_classify(const uint8_t *payload, size_t length);
const char *jaglink_safety_reason_string(jaglink_safety_reason reason);

typedef struct jaglink_evidence_writer jaglink_evidence_writer;

jaglink_evidence_writer *jaglink_evidence_open(const char *path);
int jaglink_evidence_write_frame(jaglink_evidence_writer *writer,
                                 uint64_t timestamp_ns,
                                 const char *direction,
                                 const char *protocol,
                                 uint32_t can_id,
                                 const uint8_t *data,
                                 size_t length,
                                 const char *annotation);
int jaglink_evidence_write_annotation(jaglink_evidence_writer *writer,
                                      uint64_t timestamp_ns,
                                      const char *text);
int jaglink_evidence_flush(jaglink_evidence_writer *writer);
void jaglink_evidence_close(jaglink_evidence_writer *writer);

#ifdef __cplusplus
}
#endif

#endif
