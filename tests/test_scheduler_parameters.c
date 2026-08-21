// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/scheduler.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool check(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "jaglink-scheduler-parameter-test: %s\n", message);
    }
    return condition;
}

int main(void)
{
    bool passed = true;
    JaglinkScheduler scheduler;
    JaglinkSchedulerDispatch dispatch;
    JaglinkParameterKey obd = {
        JAGLINK_PARAMETER_PROTOCOL_OBD2,
        JAGLINK_PARAMETER_MODULE_STANDARD_OBD2,
        0x0cU
    };
    JaglinkParameterKey uds_engine = {
        JAGLINK_PARAMETER_PROTOCOL_UDS,
        1U,
        0xf190U
    };
    JaglinkParameterKey uds_transmission = {
        JAGLINK_PARAMETER_PROTOCOL_UDS,
        2U,
        0xf190U
    };
    JaglinkParameterKey invalid_uds = {
        JAGLINK_PARAMETER_PROTOCOL_UDS,
        1U,
        0x10000U
    };

    jaglink_scheduler_init(&scheduler);
    passed &= check(jaglink_scheduler_add_parameter(
                        &scheduler, &obd, 250U,
                        JAGLINK_SCHEDULER_PRIORITY_CRITICAL, 0U) ==
                        JAGLINK_SCHEDULER_RESULT_OK,
                    "OBD parameter registration failed");
    passed &= check(scheduler.items[0].pid_valid &&
                    scheduler.items[0].pid == 0x0cU &&
                    jaglink_parameter_key_equal(&scheduler.items[0].key, &obd),
                    "OBD compatibility identity mismatch");

    passed &= check(jaglink_scheduler_add_parameter(
                        &scheduler, &uds_engine, 500U,
                        JAGLINK_SCHEDULER_PRIORITY_HIGH, 0U) ==
                        JAGLINK_SCHEDULER_RESULT_OK,
                    "UDS engine parameter registration failed");
    passed &= check(!scheduler.items[1].pid_valid &&
                    jaglink_parameter_key_equal(
                        &scheduler.items[1].key, &uds_engine),
                    "UDS item should not expose a legacy PID");

    passed &= check(jaglink_scheduler_add_parameter(
                        &scheduler, &uds_transmission, 500U,
                        JAGLINK_SCHEDULER_PRIORITY_NORMAL, 0U) ==
                        JAGLINK_SCHEDULER_RESULT_OK,
                    "same DID in another module should be allowed");
    passed &= check(jaglink_scheduler_add_parameter(
                        &scheduler, &uds_engine, 1000U,
                        JAGLINK_SCHEDULER_PRIORITY_LOW, 0U) ==
                        JAGLINK_SCHEDULER_RESULT_DUPLICATE,
                    "duplicate full key should be rejected");
    passed &= check(jaglink_scheduler_add_parameter(
                        &scheduler, &invalid_uds, 1000U,
                        JAGLINK_SCHEDULER_PRIORITY_LOW, 0U) ==
                        JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT,
                    "out-of-range UDS identifier accepted");

    passed &= check(jaglink_scheduler_next(&scheduler, 0U, &dispatch) ==
                        JAGLINK_SCHEDULER_NEXT_READY,
                    "scheduler did not dispatch due item");
    passed &= check(dispatch.pid_valid && dispatch.pid == 0x0cU &&
                    jaglink_parameter_key_equal(&dispatch.key, &obd),
                    "OBD dispatch identity mismatch");
    passed &= check(jaglink_scheduler_mark_dispatched(
                        &scheduler, dispatch.index, 0U) ==
                        JAGLINK_SCHEDULER_RESULT_OK,
                    "OBD dispatch mark failed");

    passed &= check(jaglink_scheduler_next(&scheduler, 0U, &dispatch) ==
                        JAGLINK_SCHEDULER_NEXT_READY,
                    "scheduler did not dispatch UDS item");
    passed &= check(!dispatch.pid_valid &&
                    jaglink_parameter_key_equal(&dispatch.key, &uds_engine),
                    "UDS dispatch identity mismatch");

    passed &= check(jaglink_scheduler_set_parameter_enabled(
                        &scheduler, &uds_engine, false) ==
                        JAGLINK_SCHEDULER_RESULT_OK,
                    "disable UDS key failed");
    passed &= check(!scheduler.items[1].enabled,
                    "UDS key remained enabled");
    passed &= check(jaglink_scheduler_set_enabled(
                        &scheduler, 0x0cU, false) ==
                        JAGLINK_SCHEDULER_RESULT_OK,
                    "legacy PID disable wrapper failed");
    passed &= check(!scheduler.items[0].enabled,
                    "legacy PID disable did not reach keyed item");

    passed &= check(JAGLINK_SCHEDULER_MAX_ITEMS >= 64U,
                    "scheduler capacity is too small for expanded live data");

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
