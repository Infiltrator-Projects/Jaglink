// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file scheduler.h
 * @brief Portable bounded request scheduler for live diagnostic polling.
 */
#ifndef JAGLINK_SCHEDULER_H
#define JAGLINK_SCHEDULER_H

#include "jaglink/parameter.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_SCHEDULER_MAX_ITEMS 64U

typedef enum {
    JAGLINK_SCHEDULER_PRIORITY_LOW = 0,
    JAGLINK_SCHEDULER_PRIORITY_NORMAL = 1,
    JAGLINK_SCHEDULER_PRIORITY_HIGH = 2,
    JAGLINK_SCHEDULER_PRIORITY_CRITICAL = 3
} JaglinkSchedulerPriority;

typedef enum {
    JAGLINK_SCHEDULER_RESULT_OK = 0,
    JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT,
    JAGLINK_SCHEDULER_RESULT_FULL,
    JAGLINK_SCHEDULER_RESULT_DUPLICATE,
    JAGLINK_SCHEDULER_RESULT_NOT_FOUND
} JaglinkSchedulerResult;

typedef enum {
    JAGLINK_SCHEDULER_NEXT_READY = 0,
    JAGLINK_SCHEDULER_NEXT_WAITING,
    JAGLINK_SCHEDULER_NEXT_PAUSED,
    JAGLINK_SCHEDULER_NEXT_EMPTY,
    JAGLINK_SCHEDULER_NEXT_INVALID_ARGUMENT
} JaglinkSchedulerNextResult;

typedef struct {
    JaglinkParameterKey key;
    uint8_t pid;
    bool pid_valid;
    uint32_t interval_ms;
    uint64_t next_due_ms;
    JaglinkSchedulerPriority priority;
    bool enabled;
} JaglinkSchedulerItem;

typedef struct {
    JaglinkSchedulerItem items[JAGLINK_SCHEDULER_MAX_ITEMS];
    size_t count;
    bool paused;
    uint64_t pause_started_ms;
} JaglinkScheduler;

typedef struct {
    size_t index;
    JaglinkParameterKey key;
    uint8_t pid;
    bool pid_valid;
    uint64_t due_ms;
    uint64_t wait_ms;
} JaglinkSchedulerDispatch;

const char *jaglink_scheduler_result_name(JaglinkSchedulerResult result);
const char *jaglink_scheduler_next_result_name(JaglinkSchedulerNextResult result);

void jaglink_scheduler_init(JaglinkScheduler *scheduler);

/** All scheduler timestamps use one caller-supplied monotonic millisecond clock. */
JaglinkSchedulerResult jaglink_scheduler_add_parameter(
    JaglinkScheduler *scheduler,
    const JaglinkParameterKey *key,
    uint32_t interval_ms,
    JaglinkSchedulerPriority priority,
    uint64_t first_due_ms);

JaglinkSchedulerResult jaglink_scheduler_set_parameter_enabled(
    JaglinkScheduler *scheduler,
    const JaglinkParameterKey *key,
    bool enabled);

/** Compatibility wrapper for standard OBD-II PID scheduling. */
JaglinkSchedulerResult jaglink_scheduler_add(
    JaglinkScheduler *scheduler,
    uint8_t pid,
    uint32_t interval_ms,
    JaglinkSchedulerPriority priority,
    uint64_t first_due_ms);

/** Compatibility wrapper for standard OBD-II PID scheduling. */
JaglinkSchedulerResult jaglink_scheduler_set_enabled(
    JaglinkScheduler *scheduler, uint8_t pid, bool enabled);

JaglinkSchedulerResult jaglink_scheduler_configure_standard_obd2(
    JaglinkScheduler *scheduler,
    const JaglinkObd2PidSet *supported,
    uint64_t first_due_ms);

void jaglink_scheduler_set_paused(
    JaglinkScheduler *scheduler, bool paused, uint64_t now_ms);

JaglinkSchedulerNextResult jaglink_scheduler_next(
    const JaglinkScheduler *scheduler,
    uint64_t now_ms,
    JaglinkSchedulerDispatch *dispatch);

JaglinkSchedulerResult jaglink_scheduler_mark_dispatched(
    JaglinkScheduler *scheduler, size_t index, uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
