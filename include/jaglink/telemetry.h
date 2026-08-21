// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file telemetry.h
 * @brief Portable typed sample history, favourites and CSV session export.
 */
#ifndef JAGLINK_TELEMETRY_H
#define JAGLINK_TELEMETRY_H

#include "jaglink/elm327.h"
#include "jaglink/obd2.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_TELEMETRY_HISTORY_CAPACITY 512U
#define JAGLINK_TELEMETRY_ADAPTER_TEXT_LENGTH 96U
#define JAGLINK_TELEMETRY_VEHICLE_TEXT_LENGTH 64U
#define JAGLINK_TELEMETRY_TRANSCRIPT_CAPACITY 64U
#define JAGLINK_TELEMETRY_TRANSCRIPT_COMMAND_LENGTH 64U
#define JAGLINK_TELEMETRY_TRANSCRIPT_RESPONSE_LENGTH 192U

typedef struct {
    uint64_t sequence;
    uint64_t timestamp_ms;
    JaglinkObd2Sample measurement;
} JaglinkTelemetrySample;

typedef struct {
    uint64_t timestamp_ms;
    JaglinkElm327Result result;
    char command[JAGLINK_TELEMETRY_TRANSCRIPT_COMMAND_LENGTH];
    char response[JAGLINK_TELEMETRY_TRANSCRIPT_RESPONSE_LENGTH];
} JaglinkTelemetryTranscriptEntry;

typedef struct {
    JaglinkTelemetrySample history[JAGLINK_TELEMETRY_HISTORY_CAPACITY];
    JaglinkTelemetrySample latest[256U];
    bool latest_valid[256U];
    bool favourite[256U];
    JaglinkTelemetryTranscriptEntry transcript[JAGLINK_TELEMETRY_TRANSCRIPT_CAPACITY];
    size_t transcript_head;
    size_t transcript_count;
    size_t history_head;
    size_t history_count;
    uint64_t next_sequence;
    uint64_t total_sample_count;
} JaglinkTelemetryStore;

typedef struct {
    uint64_t started_epoch_ms;
    uint64_t ended_epoch_ms;
    char adapter_identifier[JAGLINK_TELEMETRY_ADAPTER_TEXT_LENGTH];
    char vehicle_identifier[JAGLINK_TELEMETRY_VEHICLE_TEXT_LENGTH];
} JaglinkTelemetrySessionMetadata;

typedef bool (*JaglinkTelemetryTextSink)(
    void *context, const char *bytes, size_t length);

typedef struct {
    JaglinkTelemetryTextSink sink;
    void *context;
    bool started;
    bool finished;
    bool failed;
} JaglinkTelemetryRecorder;

void jaglink_telemetry_store_init(JaglinkTelemetryStore *store);

/** Clear samples, latest values and transcript while preserving favourites. */
void jaglink_telemetry_store_clear_samples(JaglinkTelemetryStore *store);

bool jaglink_telemetry_store_record(
    JaglinkTelemetryStore *store,
    uint64_t timestamp_ms,
    const JaglinkObd2Sample *measurement);

bool jaglink_telemetry_store_latest(
    const JaglinkTelemetryStore *store,
    uint8_t pid,
    JaglinkTelemetrySample *sample);

size_t jaglink_telemetry_store_history_count(
    const JaglinkTelemetryStore *store);

uint64_t jaglink_telemetry_store_total_sample_count(
    const JaglinkTelemetryStore *store);

bool jaglink_telemetry_store_history_at(
    const JaglinkTelemetryStore *store,
    size_t chronological_index,
    JaglinkTelemetrySample *sample);

void jaglink_telemetry_store_set_favourite(
    JaglinkTelemetryStore *store, uint8_t pid, bool favourite);

bool jaglink_telemetry_store_is_favourite(
    const JaglinkTelemetryStore *store, uint8_t pid);

bool jaglink_telemetry_store_record_transcript(
    JaglinkTelemetryStore *store,
    uint64_t timestamp_ms,
    const char *command,
    const JaglinkElm327Response *response);

size_t jaglink_telemetry_store_transcript_count(
    const JaglinkTelemetryStore *store);

bool jaglink_telemetry_store_transcript_at(
    const JaglinkTelemetryStore *store,
    size_t chronological_index,
    JaglinkTelemetryTranscriptEntry *entry);

void jaglink_telemetry_session_metadata_init(
    JaglinkTelemetrySessionMetadata *metadata,
    uint64_t started_epoch_ms,
    const char *adapter_identifier,
    const char *vehicle_identifier);

void jaglink_telemetry_session_metadata_set_adapter(
    JaglinkTelemetrySessionMetadata *metadata,
    const char *adapter_identifier);

void jaglink_telemetry_session_metadata_set_vehicle(
    JaglinkTelemetrySessionMetadata *metadata,
    const char *vehicle_identifier);

void jaglink_telemetry_session_metadata_finish(
    JaglinkTelemetrySessionMetadata *metadata,
    uint64_t ended_epoch_ms);

void jaglink_telemetry_recorder_init(JaglinkTelemetryRecorder *recorder);

bool jaglink_telemetry_recorder_begin(
    JaglinkTelemetryRecorder *recorder,
    const JaglinkTelemetrySessionMetadata *metadata,
    JaglinkTelemetryTextSink sink,
    void *context);

/** Recorder write failure is terminal until `jaglink_telemetry_recorder_init()`. */
bool jaglink_telemetry_recorder_record_sample(
    JaglinkTelemetryRecorder *recorder,
    const JaglinkTelemetrySample *sample,
    bool favourite);

bool jaglink_telemetry_recorder_record_response(
    JaglinkTelemetryRecorder *recorder,
    uint64_t timestamp_ms,
    const char *command,
    const JaglinkElm327Response *response);

bool jaglink_telemetry_recorder_finish(
    JaglinkTelemetryRecorder *recorder, uint64_t ended_epoch_ms);

bool jaglink_telemetry_export_csv(
    const JaglinkTelemetryStore *store,
    const JaglinkTelemetrySessionMetadata *metadata,
    JaglinkTelemetryTextSink sink,
    void *context);

#ifdef __cplusplus
}
#endif

#endif
