// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file telemetry_csv.c
 * @brief Streaming and snapshot CSV telemetry output.
 */
#include "jaglink/telemetry.h"

#include "infiltratr/format.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static bool jaglink_csv_emit(
    JaglinkTelemetryTextSink sink,
    void *context,
    const char *text)
{
    return sink != NULL && text != NULL &&
           sink(context, text, strlen(text));
}

static bool jaglink_csv_emit_quoted(
    JaglinkTelemetryTextSink sink,
    void *context,
    const char *text)
{
    if (!jaglink_csv_emit(sink, context, "\"")) {
        return false;
    }

    const char *cursor = text != NULL ? text : "";
    const char *segment = cursor;
    while (*cursor != '\0') {
        if (*cursor == '"') {
            if (cursor > segment &&
                !sink(context, segment, (size_t)(cursor - segment))) {
                return false;
            }
            if (!jaglink_csv_emit(sink, context, "\"\"")) {
                return false;
            }
            segment = cursor + 1;
        }
        ++cursor;
    }

    if (cursor > segment &&
        !sink(context, segment, (size_t)(cursor - segment))) {
        return false;
    }
    return jaglink_csv_emit(sink, context, "\"");
}

static bool jaglink_csv_emit_metadata_text(
    JaglinkTelemetryTextSink sink,
    void *context,
    const char *key,
    const char *value)
{
    if (!jaglink_csv_emit(sink, context, "# ") ||
        !jaglink_csv_emit(sink, context, key) ||
        !jaglink_csv_emit(sink, context, ",")) {
        return false;
    }
    if (!jaglink_csv_emit_quoted(sink, context, value)) {
        return false;
    }
    return jaglink_csv_emit(sink, context, "\n");
}

static bool jaglink_csv_format_value(double value, char *buffer, size_t size)
{
    InfiltratrScalarFormatOptions options =
        INFILTRATR_SCALAR_FORMAT_OPTIONS_INIT;
    options.decimal_places = 6U;
    options.unavailable_text = "";
    if (!infiltratr_format_scalar(true, (long double)value,
                                  &options, buffer, size)) {
        return false;
    }

    for (char *cursor = buffer; *cursor != '\0'; ++cursor) {
        if (*cursor == ',') {
            *cursor = '.';
        }
    }

    char *end = buffer + strlen(buffer);
    while (end > buffer && end[-1] == '0') {
        --end;
    }
    if (end > buffer && end[-1] == '.') {
        --end;
    }
    *end = '\0';
    return true;
}

static bool jaglink_telemetry_recorder_latch_failure(
    JaglinkTelemetryRecorder *recorder)
{
    if (recorder != NULL) {
        recorder->failed = true;
    }
    return false;
}

void jaglink_telemetry_recorder_init(JaglinkTelemetryRecorder *recorder)
{
    if (recorder != NULL) {
        memset(recorder, 0, sizeof(*recorder));
    }
}

bool jaglink_telemetry_recorder_begin(
    JaglinkTelemetryRecorder *recorder,
    const JaglinkTelemetrySessionMetadata *metadata,
    JaglinkTelemetryTextSink sink,
    void *context)
{
    if (recorder == NULL || metadata == NULL || sink == NULL ||
        recorder->started || recorder->failed) {
        return false;
    }

    recorder->sink = sink;
    recorder->context = context;
    recorder->started = true;
    recorder->finished = false;
    recorder->failed = false;

    char line[192];
    int written = snprintf(line, sizeof(line),
                           "# jaglink_session_stream_version,1\n"
                           "# session_started_epoch_ms,%llu\n",
                           (unsigned long long)metadata->started_epoch_ms);
    if (written < 0 || (size_t)written >= sizeof(line) ||
        !jaglink_csv_emit(sink, context, line) ||
        !jaglink_csv_emit_metadata_text(
            sink, context, "adapter_identifier", metadata->adapter_identifier) ||
        !jaglink_csv_emit_metadata_text(
            sink, context, "vehicle_identifier", metadata->vehicle_identifier) ||
        !jaglink_csv_emit(
            sink, context,
            "record_type,sequence,timestamp_ms,pid,name,value,unit,favourite,command,result,response\n")) {
        return jaglink_telemetry_recorder_latch_failure(recorder);
    }
    return true;
}

bool jaglink_telemetry_recorder_record_sample(
    JaglinkTelemetryRecorder *recorder,
    const JaglinkTelemetrySample *sample,
    bool favourite)
{
    if (recorder == NULL || !recorder->started || recorder->finished ||
        recorder->failed || sample == NULL ||
        !isfinite(sample->measurement.value)) {
        return false;
    }

    char prefix[64];
    char row[256];
    char value[64];
    int written;

    if (!jaglink_csv_format_value(sample->measurement.value,
                                 value, sizeof(value))) {
        return false;
    }

    written = snprintf(prefix, sizeof(prefix), "sample,%llu,",
                       (unsigned long long)sample->sequence);
    if (written < 0 || (size_t)written >= sizeof(prefix)) {
        return false;
    }
    written = snprintf(row, sizeof(row), "%llu,0x%02X,",
                       (unsigned long long)sample->timestamp_ms,
                       (unsigned int)sample->measurement.pid);
    if (written < 0 || (size_t)written >= sizeof(row)) {
        return false;
    }

    if (!jaglink_csv_emit(recorder->sink, recorder->context, prefix) ||
        !jaglink_csv_emit(recorder->sink, recorder->context, row) ||
        !jaglink_csv_emit_quoted(
            recorder->sink, recorder->context,
            jaglink_obd2_pid_name(sample->measurement.pid)) ||
        !jaglink_csv_emit(recorder->sink, recorder->context, ",") ||
        !jaglink_csv_emit(recorder->sink, recorder->context, value) ||
        !jaglink_csv_emit(recorder->sink, recorder->context, ",") ||
        !jaglink_csv_emit_quoted(
            recorder->sink, recorder->context,
            jaglink_obd2_unit_name(sample->measurement.unit))) {
        return jaglink_telemetry_recorder_latch_failure(recorder);
    }

    written = snprintf(row, sizeof(row), ",%u,\"\",\"\",\"\"\n",
                       favourite ? 1U : 0U);
    if (written < 0 || (size_t)written >= sizeof(row)) {
        return false;
    }
    if (!jaglink_csv_emit(recorder->sink, recorder->context, row)) {
        return jaglink_telemetry_recorder_latch_failure(recorder);
    }
    return true;
}

bool jaglink_telemetry_recorder_record_response(
    JaglinkTelemetryRecorder *recorder,
    uint64_t timestamp_ms,
    const char *command,
    const JaglinkElm327Response *response)
{
    if (recorder == NULL || !recorder->started || recorder->finished ||
        recorder->failed || recorder->sink == NULL || command == NULL ||
        response == NULL) {
        return false;
    }

    char line[96];
    const int written = snprintf(
        line, sizeof(line), "transcript,,%llu,,,,,,",
        (unsigned long long)timestamp_ms);
    if (written < 0 || (size_t)written >= sizeof(line)) {
        return false;
    }
    if (!jaglink_csv_emit(recorder->sink, recorder->context, line) ||
        !jaglink_csv_emit_quoted(recorder->sink, recorder->context, command) ||
        !jaglink_csv_emit(recorder->sink, recorder->context, ",") ||
        !jaglink_csv_emit_quoted(
            recorder->sink, recorder->context,
            jaglink_elm327_result_name(response->result)) ||
        !jaglink_csv_emit(recorder->sink, recorder->context, ",") ||
        !jaglink_csv_emit_quoted(
            recorder->sink, recorder->context, response->text) ||
        !jaglink_csv_emit(recorder->sink, recorder->context, "\n")) {
        return jaglink_telemetry_recorder_latch_failure(recorder);
    }
    return true;
}

bool jaglink_telemetry_recorder_finish(
    JaglinkTelemetryRecorder *recorder, uint64_t ended_epoch_ms)
{
    if (recorder == NULL || !recorder->started || recorder->finished ||
        recorder->failed) {
        return false;
    }

    char line[96];
    int written = snprintf(line, sizeof(line),
                           "# session_ended_epoch_ms,%llu\n",
                           (unsigned long long)ended_epoch_ms);
    if (written < 0 || (size_t)written >= sizeof(line)) {
        return false;
    }
    if (!jaglink_csv_emit(recorder->sink, recorder->context, line)) {
        return jaglink_telemetry_recorder_latch_failure(recorder);
    }
    recorder->finished = true;
    return true;
}

bool jaglink_telemetry_export_csv(
    const JaglinkTelemetryStore *store,
    const JaglinkTelemetrySessionMetadata *metadata,
    JaglinkTelemetryTextSink sink,
    void *context)
{
    if (store == NULL || metadata == NULL || sink == NULL) {
        return false;
    }

    char line[256];
    int written = snprintf(line, sizeof(line),
                           "# jaglink_csv_version,1\n"
                           "# session_started_epoch_ms,%llu\n"
                           "# session_ended_epoch_ms,%llu\n",
                           (unsigned long long)metadata->started_epoch_ms,
                           (unsigned long long)metadata->ended_epoch_ms);
    if (written < 0 || (size_t)written >= sizeof(line) ||
        !jaglink_csv_emit(sink, context, line) ||
        !jaglink_csv_emit_metadata_text(
            sink, context, "adapter_identifier",
            metadata->adapter_identifier) ||
        !jaglink_csv_emit_metadata_text(
            sink, context, "vehicle_identifier",
            metadata->vehicle_identifier) ||
        !jaglink_csv_emit(
            sink, context,
            "sequence,timestamp_ms,pid,name,value,unit,favourite\n")) {
        return false;
    }

    for (size_t index = 0U; index < store->history_count; ++index) {
        JaglinkTelemetrySample sample;
        if (!jaglink_telemetry_store_history_at(store, index, &sample)) {
            return false;
        }

        char value[64];
        if (!jaglink_csv_format_value(
                sample.measurement.value, value, sizeof(value))) {
            return false;
        }

        written = snprintf(line, sizeof(line),
                           "%llu,%llu,0x%02X,",
                           (unsigned long long)sample.sequence,
                           (unsigned long long)sample.timestamp_ms,
                           (unsigned int)sample.measurement.pid);
        if (written < 0 || (size_t)written >= sizeof(line) ||
            !jaglink_csv_emit(sink, context, line) ||
            !jaglink_csv_emit_quoted(
                sink, context,
                jaglink_obd2_pid_name(sample.measurement.pid)) ||
            !jaglink_csv_emit(sink, context, ",") ||
            !jaglink_csv_emit(sink, context, value) ||
            !jaglink_csv_emit(sink, context, ",") ||
            !jaglink_csv_emit_quoted(
                sink, context,
                jaglink_obd2_unit_name(sample.measurement.unit))) {
            return false;
        }

        written = snprintf(
            line, sizeof(line), ",%u\n",
            store->favourite[sample.measurement.pid] ? 1U : 0U);
        if (written < 0 || (size_t)written >= sizeof(line) ||
            !jaglink_csv_emit(sink, context, line)) {
            return false;
        }
    }

    if (!jaglink_csv_emit(sink, context, "# diagnostic_transcript\n") ||
        !jaglink_csv_emit(sink, context,
                         "timestamp_ms,command,result,response\n")) {
        return false;
    }

    for (size_t index = 0U; index < store->transcript_count; ++index) {
        JaglinkTelemetryTranscriptEntry entry;
        if (!jaglink_telemetry_store_transcript_at(store, index, &entry)) {
            return false;
        }

        written = snprintf(line, sizeof(line), "%llu,",
                           (unsigned long long)entry.timestamp_ms);
        if (written < 0 || (size_t)written >= sizeof(line) ||
            !jaglink_csv_emit(sink, context, line) ||
            !jaglink_csv_emit_quoted(sink, context, entry.command) ||
            !jaglink_csv_emit(sink, context, ",") ||
            !jaglink_csv_emit_quoted(
                sink, context, jaglink_elm327_result_name(entry.result)) ||
            !jaglink_csv_emit(sink, context, ",") ||
            !jaglink_csv_emit_quoted(sink, context, entry.response) ||
            !jaglink_csv_emit(sink, context, "\n")) {
            return false;
        }
    }

    return true;
}
