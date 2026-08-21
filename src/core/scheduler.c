// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file scheduler.c
 * @brief Portable live diagnostic request scheduler.
 */
#include "jaglink/scheduler.h"

#include "infiltratr/core.h"
#include "infiltratr/timing.h"

#include <string.h>

static bool jaglink_scheduler_priority_valid(JaglinkSchedulerPriority priority)
{
    return priority >= JAGLINK_SCHEDULER_PRIORITY_LOW &&
           priority <= JAGLINK_SCHEDULER_PRIORITY_CRITICAL;
}

static JaglinkParameterKey jaglink_scheduler_obd2_key(uint8_t pid)
{
    JaglinkParameterKey key = {
        JAGLINK_PARAMETER_PROTOCOL_OBD2,
        JAGLINK_PARAMETER_MODULE_STANDARD_OBD2,
        (uint32_t)pid
    };
    return key;
}

static bool jaglink_scheduler_key_to_pid(
    const JaglinkParameterKey *key,
    uint8_t *pid)
{
    if (!jaglink_parameter_key_is_valid(key) || pid == NULL ||
        key->protocol != JAGLINK_PARAMETER_PROTOCOL_OBD2 ||
        key->module != JAGLINK_PARAMETER_MODULE_STANDARD_OBD2 ||
        key->identifier > UINT8_MAX) {
        return false;
    }
    *pid = (uint8_t)key->identifier;
    return true;
}

const char *jaglink_scheduler_result_name(JaglinkSchedulerResult result)
{
    switch (result) {
    case JAGLINK_SCHEDULER_RESULT_OK: return "ok";
    case JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT: return "invalid-argument";
    case JAGLINK_SCHEDULER_RESULT_FULL: return "full";
    case JAGLINK_SCHEDULER_RESULT_DUPLICATE: return "duplicate";
    case JAGLINK_SCHEDULER_RESULT_NOT_FOUND: return "not-found";
    }
    return "unknown";
}

const char *jaglink_scheduler_next_result_name(JaglinkSchedulerNextResult result)
{
    switch (result) {
    case JAGLINK_SCHEDULER_NEXT_READY: return "ready";
    case JAGLINK_SCHEDULER_NEXT_WAITING: return "waiting";
    case JAGLINK_SCHEDULER_NEXT_PAUSED: return "paused";
    case JAGLINK_SCHEDULER_NEXT_EMPTY: return "empty";
    case JAGLINK_SCHEDULER_NEXT_INVALID_ARGUMENT: return "invalid-argument";
    }
    return "unknown";
}

void jaglink_scheduler_init(JaglinkScheduler *scheduler)
{
    if (scheduler != NULL) {
        memset(scheduler, 0, sizeof(*scheduler));
    }
}

JaglinkSchedulerResult jaglink_scheduler_add_parameter(
    JaglinkScheduler *scheduler,
    const JaglinkParameterKey *key,
    uint32_t interval_ms,
    JaglinkSchedulerPriority priority,
    uint64_t first_due_ms)
{
    size_t index;
    JaglinkSchedulerItem item;
    uint8_t pid = 0U;

    if (scheduler == NULL || !jaglink_parameter_key_is_valid(key) ||
        interval_ms == 0U || !jaglink_scheduler_priority_valid(priority)) {
        return JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    for (index = 0U; index < scheduler->count; ++index) {
        if (jaglink_parameter_key_equal(&scheduler->items[index].key, key)) {
            return JAGLINK_SCHEDULER_RESULT_DUPLICATE;
        }
    }

    if (scheduler->count >= JAGLINK_SCHEDULER_MAX_ITEMS) {
        return JAGLINK_SCHEDULER_RESULT_FULL;
    }

    memset(&item, 0, sizeof(item));
    item.key = *key;
    item.pid_valid = jaglink_scheduler_key_to_pid(key, &pid);
    item.pid = pid;
    item.interval_ms = interval_ms;
    item.next_due_ms = first_due_ms;
    item.priority = priority;
    item.enabled = true;
    scheduler->items[scheduler->count] = item;
    scheduler->count++;
    return JAGLINK_SCHEDULER_RESULT_OK;
}

JaglinkSchedulerResult jaglink_scheduler_set_parameter_enabled(
    JaglinkScheduler *scheduler,
    const JaglinkParameterKey *key,
    bool enabled)
{
    size_t index;

    if (scheduler == NULL || !jaglink_parameter_key_is_valid(key)) {
        return JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    for (index = 0U; index < scheduler->count; ++index) {
        if (jaglink_parameter_key_equal(&scheduler->items[index].key, key)) {
            scheduler->items[index].enabled = enabled;
            return JAGLINK_SCHEDULER_RESULT_OK;
        }
    }
    return JAGLINK_SCHEDULER_RESULT_NOT_FOUND;
}

JaglinkSchedulerResult jaglink_scheduler_add(
    JaglinkScheduler *scheduler,
    uint8_t pid,
    uint32_t interval_ms,
    JaglinkSchedulerPriority priority,
    uint64_t first_due_ms)
{
    const JaglinkParameterKey key = jaglink_scheduler_obd2_key(pid);
    return jaglink_scheduler_add_parameter(
        scheduler, &key, interval_ms, priority, first_due_ms);
}

JaglinkSchedulerResult jaglink_scheduler_set_enabled(
    JaglinkScheduler *scheduler, uint8_t pid, bool enabled)
{
    const JaglinkParameterKey key = jaglink_scheduler_obd2_key(pid);
    return jaglink_scheduler_set_parameter_enabled(scheduler, &key, enabled);
}

typedef struct {
    uint8_t pid;
    uint32_t interval_ms;
    JaglinkSchedulerPriority priority;
} JaglinkStandardSchedule;

JaglinkSchedulerResult jaglink_scheduler_configure_standard_obd2(
    JaglinkScheduler *scheduler,
    const JaglinkObd2PidSet *supported,
    uint64_t first_due_ms)
{
    static const JaglinkStandardSchedule plan[] = {
        { 0x0cU, 250U,  JAGLINK_SCHEDULER_PRIORITY_CRITICAL },
        { 0x0dU, 500U,  JAGLINK_SCHEDULER_PRIORITY_HIGH },
        { 0x0bU, 500U,  JAGLINK_SCHEDULER_PRIORITY_HIGH },
        { 0x23U, 500U,  JAGLINK_SCHEDULER_PRIORITY_HIGH },
        { 0x7aU, 750U,  JAGLINK_SCHEDULER_PRIORITY_HIGH },
        { 0x7cU, 1000U, JAGLINK_SCHEDULER_PRIORITY_HIGH },
        { 0x11U, 500U,  JAGLINK_SCHEDULER_PRIORITY_HIGH },
        { 0x04U, 750U,  JAGLINK_SCHEDULER_PRIORITY_NORMAL },
        { 0x10U, 750U,  JAGLINK_SCHEDULER_PRIORITY_NORMAL },
        { 0x2cU, 750U,  JAGLINK_SCHEDULER_PRIORITY_NORMAL },
        { 0x2dU, 1000U, JAGLINK_SCHEDULER_PRIORITY_NORMAL },
        { 0x5eU, 1000U, JAGLINK_SCHEDULER_PRIORITY_NORMAL },
        { 0x78U, 1000U, JAGLINK_SCHEDULER_PRIORITY_NORMAL },
        { 0x3cU, 1500U, JAGLINK_SCHEDULER_PRIORITY_NORMAL },
        { 0x05U, 1500U, JAGLINK_SCHEDULER_PRIORITY_LOW },
        { 0x0fU, 1500U, JAGLINK_SCHEDULER_PRIORITY_LOW },
        { 0x33U, 2000U, JAGLINK_SCHEDULER_PRIORITY_LOW },
        { 0x42U, 2000U, JAGLINK_SCHEDULER_PRIORITY_LOW },
        { 0x46U, 3000U, JAGLINK_SCHEDULER_PRIORITY_LOW },
        { 0x5cU, 1500U, JAGLINK_SCHEDULER_PRIORITY_LOW }
    };
    size_t index;

    if (scheduler == NULL || supported == NULL) {
        return JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    jaglink_scheduler_init(scheduler);
    for (index = 0U; index < INFILTRATR_ARRAY_LENGTH(plan); ++index) {
        JaglinkSchedulerResult result;

        if (!jaglink_obd2_pid_set_contains(supported, plan[index].pid)) {
            continue;
        }
        result = jaglink_scheduler_add(scheduler,
                                      plan[index].pid,
                                      plan[index].interval_ms,
                                      plan[index].priority,
                                      first_due_ms);
        if (result != JAGLINK_SCHEDULER_RESULT_OK) {
            jaglink_scheduler_init(scheduler);
            return result;
        }
    }

    return JAGLINK_SCHEDULER_RESULT_OK;
}

void jaglink_scheduler_set_paused(
    JaglinkScheduler *scheduler, bool paused, uint64_t now_ms)
{
    size_t index;
    uint64_t pause_duration = 0U;

    if (scheduler == NULL || scheduler->paused == paused) {
        return;
    }

    if (paused) {
        scheduler->paused = true;
        scheduler->pause_started_ms = now_ms;
        return;
    }

    if (now_ms >= scheduler->pause_started_ms) {
        pause_duration = now_ms - scheduler->pause_started_ms;
    }
    for (index = 0U; index < scheduler->count; ++index) {
        scheduler->items[index].next_due_ms =
            infiltratr_u64_add_saturating(
                scheduler->items[index].next_due_ms, pause_duration);
    }
    scheduler->paused = false;
    scheduler->pause_started_ms = 0U;
}

JaglinkSchedulerNextResult jaglink_scheduler_next(
    const JaglinkScheduler *scheduler,
    uint64_t now_ms,
    JaglinkSchedulerDispatch *dispatch)
{
    bool have_enabled = false;
    bool have_due = false;
    size_t selected = 0U;
    size_t index;
    uint64_t earliest_due = UINT64_MAX;

    if (dispatch != NULL) {
        memset(dispatch, 0, sizeof(*dispatch));
    }
    if (scheduler == NULL || dispatch == NULL) {
        return JAGLINK_SCHEDULER_NEXT_INVALID_ARGUMENT;
    }
    if (scheduler->paused) {
        return JAGLINK_SCHEDULER_NEXT_PAUSED;
    }

    for (index = 0U; index < scheduler->count; ++index) {
        const JaglinkSchedulerItem *item = &scheduler->items[index];
        if (!item->enabled) {
            continue;
        }

        if (!have_enabled || item->next_due_ms < earliest_due) {
            earliest_due = item->next_due_ms;
        }
        have_enabled = true;

        if (item->next_due_ms > now_ms) {
            continue;
        }

        if (!have_due ||
            item->next_due_ms < scheduler->items[selected].next_due_ms ||
            (item->next_due_ms == scheduler->items[selected].next_due_ms &&
             item->priority > scheduler->items[selected].priority) ||
            (item->next_due_ms == scheduler->items[selected].next_due_ms &&
             item->priority == scheduler->items[selected].priority &&
             index < selected)) {
            selected = index;
            have_due = true;
        }
    }

    if (!have_enabled) {
        return JAGLINK_SCHEDULER_NEXT_EMPTY;
    }

    if (!have_due) {
        dispatch->due_ms = earliest_due;
        dispatch->wait_ms = earliest_due > now_ms ? earliest_due - now_ms : 0U;
        return JAGLINK_SCHEDULER_NEXT_WAITING;
    }

    dispatch->index = selected;
    dispatch->key = scheduler->items[selected].key;
    dispatch->pid = scheduler->items[selected].pid;
    dispatch->pid_valid = scheduler->items[selected].pid_valid;
    dispatch->due_ms = scheduler->items[selected].next_due_ms;
    dispatch->wait_ms = 0U;
    return JAGLINK_SCHEDULER_NEXT_READY;
}

JaglinkSchedulerResult jaglink_scheduler_mark_dispatched(
    JaglinkScheduler *scheduler, size_t index, uint64_t now_ms)
{
    JaglinkSchedulerItem *item;
    uint64_t interval;

    if (scheduler == NULL || index >= scheduler->count) {
        return JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    item = &scheduler->items[index];
    interval = (uint64_t)item->interval_ms;
    if (!infiltratr_periodic_deadline_advance(
            item->next_due_ms, now_ms, interval, &item->next_due_ms)) {
        return JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }
    return JAGLINK_SCHEDULER_RESULT_OK;
}
