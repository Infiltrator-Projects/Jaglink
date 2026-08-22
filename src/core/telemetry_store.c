// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/telemetry.h"

bool jaglink_telemetry_store_record(
    JaglinkTelemetryStore *store,
    uint64_t timestamp_ms,
    const JaglinkObd2Sample *measurement)
{
    LinkTelemetryMeasurement converted;
    if (measurement == NULL) {
        return false;
    }
    converted.pid = measurement->pid;
    converted.value = measurement->value;
    converted.unit = (LinkObd2UnitCode)measurement->unit;
    return link_telemetry_store_record(store, timestamp_ms, &converted);
}

bool jaglink_telemetry_store_record_transcript(
    JaglinkTelemetryStore *store,
    uint64_t timestamp_ms,
    const char *command,
    const JaglinkElm327Response *response)
{
    if (response == NULL) {
        return false;
    }
    return link_telemetry_store_record_transcript(
        store, timestamp_ms, command, (uint32_t)response->result, response->text);
}
