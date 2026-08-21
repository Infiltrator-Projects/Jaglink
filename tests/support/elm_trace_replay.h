// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef JAGLINK_TEST_ELM_TRACE_REPLAY_H
#define JAGLINK_TEST_ELM_TRACE_REPLAY_H

#include "jaglink/elm327.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    const char *command;
    JaglinkElm327Result result;
    const char *response_text;
    bool ok_seen;
} JaglinkTestElmTraceEntry;

typedef struct {
    const JaglinkTestElmTraceEntry *entries;
    size_t count;
    size_t index;
    bool failed;
} JaglinkTestElmTraceReplay;

static inline void jaglink_test_elm_trace_replay_init(
    JaglinkTestElmTraceReplay *replay,
    const JaglinkTestElmTraceEntry *entries,
    size_t count)
{
    if (replay == NULL) {
        return;
    }
    replay->entries = entries;
    replay->count = count;
    replay->index = 0U;
    replay->failed = entries == NULL && count != 0U;
}

static inline bool jaglink_test_elm_trace_replay_next(
    JaglinkTestElmTraceReplay *replay,
    const char *command,
    JaglinkElm327Response *response)
{
    const JaglinkTestElmTraceEntry *entry;
    size_t length;

    if (replay == NULL || command == NULL || response == NULL ||
        replay->failed || replay->entries == NULL ||
        replay->index >= replay->count) {
        if (replay != NULL) {
            replay->failed = true;
        }
        return false;
    }

    entry = &replay->entries[replay->index];
    if (entry->command == NULL || strcmp(entry->command, command) != 0) {
        replay->failed = true;
        return false;
    }

    memset(response, 0, sizeof(*response));
    response->result = entry->result;
    response->ok_seen = entry->ok_seen;
    if (entry->response_text != NULL) {
        length = strlen(entry->response_text);
        if (length >= sizeof(response->text)) {
            replay->failed = true;
            return false;
        }
        memcpy(response->text, entry->response_text, length);
        response->text[length] = '\0';
        response->length = length;
    }

    replay->index++;
    return true;
}

static inline bool jaglink_test_elm_trace_replay_complete(
    const JaglinkTestElmTraceReplay *replay)
{
    return replay != NULL && !replay->failed && replay->index == replay->count;
}

#endif
