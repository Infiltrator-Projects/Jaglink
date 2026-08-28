// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/telemetry.h"

static const char *jaglink_link_unit_name(uint32_t unit)
{
    return jaglink_obd2_unit_name((JaglinkObd2Unit)unit);
}

static const char *jaglink_link_result_name(uint32_t result)
{
    return jaglink_elm327_result_name((JaglinkElm327Result)result);
}

bool jaglink_telemetry_recorder_begin(
    JaglinkTelemetryRecorder *recorder,
    const JaglinkTelemetrySessionMetadata *metadata,
    JaglinkTelemetryTextSink sink,
    void *context)
{
    return link_telemetry_recorder_begin(
        recorder, metadata, "jaglink", sink, context);
}

bool jaglink_telemetry_recorder_continue(
    JaglinkTelemetryRecorder *recorder,
    const JaglinkTelemetrySessionMetadata *metadata,
    JaglinkTelemetryTextSink sink,
    void *context)
{
    return link_telemetry_recorder_continue(
        recorder, metadata, "jaglink", sink, context);
}

bool jaglink_telemetry_recorder_record_sample(
    JaglinkTelemetryRecorder *recorder,
    const JaglinkTelemetrySample *sample,
    bool favourite)
{
    if (sample == NULL) {
        return false;
    }
    return link_telemetry_recorder_record_sample_named(
        recorder,
        sample,
        favourite,
        jaglink_obd2_pid_name(sample->measurement.pid),
        jaglink_obd2_unit_name((JaglinkObd2Unit)sample->measurement.unit));
}

bool jaglink_telemetry_recorder_record_response(
    JaglinkTelemetryRecorder *recorder,
    uint64_t timestamp_ms,
    const char *command,
    const JaglinkElm327Response *response)
{
    if (response == NULL) {
        return false;
    }
    return link_telemetry_recorder_record_response_named(
        recorder,
        timestamp_ms,
        command,
        jaglink_elm327_result_name(response->result),
        response->text);
}

bool jaglink_telemetry_export_csv(
    const JaglinkTelemetryStore *store,
    const JaglinkTelemetrySessionMetadata *metadata,
    JaglinkTelemetryTextSink sink,
    void *context)
{
    return link_telemetry_export_csv_named(
        store,
        metadata,
        "jaglink",
        jaglink_obd2_pid_name,
        jaglink_link_unit_name,
        jaglink_link_result_name,
        sink,
        context);
}
