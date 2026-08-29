// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/scheduler.h"
#include "jaglink/telemetry.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed: %s at %s:%d\n", #expr, __FILE__, __LINE__); \
    return 1; } } while (0)

typedef struct {
    char data[65536];
    size_t length;
} TextBuffer;

typedef struct {
    TextBuffer output;
    size_t writes;
    size_t fail_on_write;
} FailingTextBuffer;

static bool text_sink(void *context, const char *bytes, size_t length)
{
    TextBuffer *buffer = context;
    if (buffer == NULL || bytes == NULL ||
        length > sizeof(buffer->data) - buffer->length - 1U) {
        return false;
    }
    memcpy(buffer->data + buffer->length, bytes, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
    return true;
}

static bool failing_text_sink(void *context, const char *bytes, size_t length)
{
    FailingTextBuffer *buffer = context;
    if (buffer == NULL) {
        return false;
    }
    if (buffer->writes++ == buffer->fail_on_write) {
        return false;
    }
    return text_sink(&buffer->output, bytes, length);
}

static void set_supported(JaglinkObd2PidSet *set, uint8_t pid)
{
    set->bits[pid / 8U] |= (uint8_t)(1U << (pid % 8U));
}

static int test_scheduler(void)
{
    JaglinkScheduler scheduler;
    JaglinkSchedulerDispatch dispatch;

    jaglink_scheduler_init(&scheduler);
    CHECK(jaglink_scheduler_next(NULL, 0U, &dispatch) ==
          JAGLINK_SCHEDULER_NEXT_INVALID_ARGUMENT);
    CHECK(jaglink_scheduler_next(&scheduler, 0U, NULL) ==
          JAGLINK_SCHEDULER_NEXT_INVALID_ARGUMENT);

    CHECK(jaglink_scheduler_add(&scheduler, 0x05U, 1000U,
                               JAGLINK_SCHEDULER_PRIORITY_LOW, 100U) ==
          JAGLINK_SCHEDULER_RESULT_OK);
    CHECK(jaglink_scheduler_add(&scheduler, 0x0cU, 250U,
                               JAGLINK_SCHEDULER_PRIORITY_CRITICAL, 100U) ==
          JAGLINK_SCHEDULER_RESULT_OK);
    CHECK(jaglink_scheduler_add(&scheduler, 0x0cU, 500U,
                               JAGLINK_SCHEDULER_PRIORITY_HIGH, 100U) ==
          JAGLINK_SCHEDULER_RESULT_DUPLICATE);

    CHECK(jaglink_scheduler_next(&scheduler, 90U, &dispatch) ==
          JAGLINK_SCHEDULER_NEXT_WAITING);
    CHECK(dispatch.wait_ms == 10U);
    CHECK(jaglink_scheduler_next(&scheduler, 100U, &dispatch) ==
          JAGLINK_SCHEDULER_NEXT_READY);
    CHECK(dispatch.pid == 0x0cU);
    CHECK(jaglink_scheduler_mark_dispatched(
              &scheduler, dispatch.index, 100U) ==
          JAGLINK_SCHEDULER_RESULT_OK);
    CHECK(scheduler.items[dispatch.index].next_due_ms == 350U);

    jaglink_scheduler_set_paused(&scheduler, true, 200U);
    CHECK(jaglink_scheduler_next(&scheduler, 500U, &dispatch) ==
          JAGLINK_SCHEDULER_NEXT_PAUSED);
    jaglink_scheduler_set_paused(&scheduler, false, 700U);
    CHECK(scheduler.items[0].next_due_ms == 600U);
    CHECK(scheduler.items[1].next_due_ms == 850U);

    CHECK(jaglink_scheduler_next(&scheduler, 700U, &dispatch) ==
          JAGLINK_SCHEDULER_NEXT_READY);
    CHECK(dispatch.pid == 0x05U);
    CHECK(jaglink_scheduler_mark_dispatched(
              &scheduler, dispatch.index, 2700U) ==
          JAGLINK_SCHEDULER_RESULT_OK);
    CHECK(scheduler.items[dispatch.index].next_due_ms == 3600U);

    jaglink_scheduler_init(&scheduler);
    CHECK(jaglink_scheduler_add(&scheduler, 0x0cU, 1U,
                               JAGLINK_SCHEDULER_PRIORITY_CRITICAL, 0U) ==
          JAGLINK_SCHEDULER_RESULT_OK);
    CHECK(jaglink_scheduler_mark_dispatched(&scheduler, 0U, UINT64_MAX) ==
          JAGLINK_SCHEDULER_RESULT_OK);
    CHECK(scheduler.items[0].next_due_ms == UINT64_MAX);

    JaglinkObd2PidSet supported = { { 0 } };
    set_supported(&supported, 0x0cU);
    set_supported(&supported, 0x05U);
    set_supported(&supported, 0x0dU);
    CHECK(jaglink_scheduler_configure_standard_obd2(
              &scheduler, &supported, 1000U) ==
          JAGLINK_SCHEDULER_RESULT_OK);
    CHECK(scheduler.count == 3U);
    CHECK(scheduler.items[0].pid == 0x0cU);
    CHECK(scheduler.items[1].pid == 0x0dU);
    CHECK(scheduler.items[2].pid == 0x05U);
    return 0;
}

static int test_scheduler_fairness(void)
{
    JaglinkScheduler scheduler;
    JaglinkSchedulerDispatch dispatch;
    JaglinkObd2PidSet supported = { { 0 } };
    bool seen[256] = { false };
    static const uint8_t expected[] = {
        0x0cU, 0x0dU, 0x0bU, 0x11U, 0x04U, 0x10U, 0x05U, 0x0fU
    };
    uint64_t now_ms = 0U;

    for (size_t index = 0U; index < sizeof(expected); ++index) {
        set_supported(&supported, expected[index]);
    }
    CHECK(jaglink_scheduler_configure_standard_obd2(
              &scheduler, &supported, 0U) == JAGLINK_SCHEDULER_RESULT_OK);
    CHECK(scheduler.count == sizeof(expected));

    for (size_t dispatch_count = 0U; dispatch_count < 16U; ++dispatch_count) {
        CHECK(jaglink_scheduler_next(&scheduler, now_ms, &dispatch) ==
              JAGLINK_SCHEDULER_NEXT_READY);
        seen[dispatch.pid] = true;
        CHECK(jaglink_scheduler_mark_dispatched(
                  &scheduler, dispatch.index, now_ms) ==
              JAGLINK_SCHEDULER_RESULT_OK);
        now_ms += 300U;
    }

    for (size_t index = 0U; index < sizeof(expected); ++index) {
        CHECK(seen[expected[index]]);
    }
    return 0;
}

static int test_telemetry(void)
{
    JaglinkTelemetryStore store;
    jaglink_telemetry_store_init(&store);
    jaglink_telemetry_store_set_favourite(&store, 0x0cU, true);

    for (size_t index = 0U;
         index < JAGLINK_TELEMETRY_HISTORY_CAPACITY + 3U;
         ++index) {
        JaglinkObd2Sample measurement = {
            .pid = (index % 2U == 0U) ? 0x0cU : 0x05U,
            .value = (double)index + 0.25,
            .unit = (index % 2U == 0U)
                ? JAGLINK_OBD2_UNIT_RPM
                : JAGLINK_OBD2_UNIT_CELSIUS
        };
        CHECK(jaglink_telemetry_store_record(
                  &store, 1000U + (uint64_t)index, &measurement));
    }

    CHECK(jaglink_telemetry_store_history_count(&store) ==
          JAGLINK_TELEMETRY_HISTORY_CAPACITY);
    CHECK(jaglink_telemetry_store_total_sample_count(&store) ==
          JAGLINK_TELEMETRY_HISTORY_CAPACITY + 3U);
    JaglinkTelemetrySample oldest;
    CHECK(jaglink_telemetry_store_history_at(&store, 0U, &oldest));
    CHECK(oldest.sequence == 3U);

    JaglinkTelemetrySample latest;
    CHECK(jaglink_telemetry_store_latest(&store, 0x0cU, &latest));
    CHECK(latest.measurement.pid == 0x0cU);
    CHECK(jaglink_telemetry_store_is_favourite(&store, 0x0cU));
    CHECK(!jaglink_telemetry_store_is_favourite(&store, 0x05U));

    JaglinkElm327Response response = {
        .result = JAGLINK_ELM327_RESULT_OK,
        .length = 8U
    };
    (void)snprintf(response.text, sizeof(response.text), "41 0C 1A F8");
    CHECK(jaglink_telemetry_store_record_transcript(
              &store, 2000U, "010C", &response));
    CHECK(jaglink_telemetry_store_transcript_count(&store) == 1U);
    JaglinkTelemetryTranscriptEntry transcript;
    CHECK(jaglink_telemetry_store_transcript_at(&store, 0U, &transcript));
    CHECK(strcmp(transcript.command, "010C") == 0);
    CHECK(strcmp(transcript.response, "41 0C 1A F8") == 0);

    JaglinkTelemetrySessionMetadata metadata;
    jaglink_telemetry_session_metadata_init(
        &metadata, 1234U, "ELM327, test \"adapter\"", "C207");
    jaglink_telemetry_session_metadata_finish(&metadata, 5678U);

    TextBuffer output = { { 0 }, 0U };
    CHECK(jaglink_telemetry_export_csv(
              &store, &metadata, text_sink, &output));
    CHECK(strstr(output.data, "# jaglink_csv_version,1\n") != NULL);
    CHECK(strstr(output.data,
                 "# adapter_identifier,\"ELM327, test \"\"adapter\"\"\"\n") != NULL);
    CHECK(strstr(output.data,
                 "sequence,timestamp_ms,pid,name,value,unit,favourite\n") != NULL);
    CHECK(strstr(output.data, ",0x0C,\"Engine speed\",") != NULL);
    CHECK(strstr(output.data, "# diagnostic_transcript\n") != NULL);
    CHECK(strstr(output.data,
                 "timestamp_ms,command,result,response\n") != NULL);
    CHECK(strstr(output.data,
                 "2000,\"010C\",\"ok\",\"41 0C 1A F8\"\n") != NULL);

    TextBuffer stream = { { 0 }, 0U };
    JaglinkTelemetryRecorder recorder;
    jaglink_telemetry_recorder_init(&recorder);
    CHECK(jaglink_telemetry_recorder_begin(
              &recorder, &metadata, text_sink, &stream));
    CHECK(jaglink_telemetry_recorder_record_sample(
              &recorder, &latest, true));
    CHECK(jaglink_telemetry_recorder_record_response(
              &recorder, 2000U, "010C", &response));
    CHECK(jaglink_telemetry_recorder_finish(&recorder, 9000U));
    CHECK(strstr(stream.data, "# jaglink_session_stream_version,1\n") != NULL);
    CHECK(strstr(stream.data, "# link_version,\"") != NULL);
    CHECK(strstr(stream.data, "# build_id,\"") != NULL);
    {
        char expected_version[128];
        char expected_profile[128];
        (void)snprintf(expected_version, sizeof(expected_version),
                       "# jaglink_version,\"%s\"\n",
                       JAGLINK_TEST_EXPECTED_VERSION);
        (void)snprintf(expected_profile, sizeof(expected_profile),
                       "# jaglink_build_profile,\"%s\"\n",
                       JAGLINK_TEST_EXPECTED_BUILD_PROFILE);
        CHECK(strstr(stream.data, expected_version) != NULL);
        CHECK(strstr(stream.data, expected_profile) != NULL);
    }
    CHECK(strstr(stream.data,
                 "record_type,sequence,timestamp_ms,pid,name,value,unit,favourite,command,result,response\n") != NULL);
    CHECK(strstr(stream.data, "sample,") != NULL);
    CHECK(strstr(stream.data, "transcript,,2000,") != NULL);
    CHECK(strstr(stream.data, "# session_ended_epoch_ms,9000\n") != NULL);
    jaglink_telemetry_recorder_init(&recorder);
    CHECK(jaglink_telemetry_recorder_continue(
              &recorder, &metadata, text_sink, &stream));
    CHECK(jaglink_telemetry_recorder_record_response(
              &recorder, 3000U, "ATI", &response));
    CHECK(jaglink_telemetry_recorder_finish(&recorder, 10000U));
    const char *stream_header = strstr(
        stream.data, "# jaglink_session_stream_version,1\n");
    CHECK(stream_header != NULL);
    CHECK(strstr(stream_header + 1,
                 "# jaglink_session_stream_version,1\n") == NULL);

    FailingTextBuffer failing = { .fail_on_write = SIZE_MAX };
    jaglink_telemetry_recorder_init(&recorder);
    CHECK(jaglink_telemetry_recorder_begin(
              &recorder, &metadata, failing_text_sink, &failing));
    failing.fail_on_write = failing.writes + 1U;
    CHECK(!jaglink_telemetry_recorder_record_sample(
              &recorder, &latest, true));
    CHECK(recorder.failed);
    CHECK(!jaglink_telemetry_recorder_record_response(
              &recorder, 2000U, "010C", &response));
    CHECK(!jaglink_telemetry_recorder_finish(&recorder, 9000U));
    jaglink_telemetry_recorder_init(&recorder);
    CHECK(!recorder.failed && !recorder.started && !recorder.finished);
    return 0;
}

int main(void)
{
    if (test_scheduler() != 0) {
        return 1;
    }
    if (test_scheduler_fairness() != 0) {
        return 1;
    }
    if (test_telemetry() != 0) {
        return 1;
    }
    return 0;
}
