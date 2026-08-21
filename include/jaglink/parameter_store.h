// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file parameter_store.h
 * @brief Bounded protocol-neutral diagnostic parameter state and history.
 *
 * Definitions are borrowed from caller-owned/static catalogs and must outlive
 * the store registration. Samples are copied by value; their definition
 * pointer is canonicalised to the registered definition.
 */
#ifndef JAGLINK_PARAMETER_STORE_H
#define JAGLINK_PARAMETER_STORE_H

#include "jaglink/parameter.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_PARAMETER_STORE_DEFINITION_CAPACITY 256U
#define JAGLINK_PARAMETER_STORE_HISTORY_CAPACITY 1024U

typedef enum {
    JAGLINK_PARAMETER_STORE_OK = 0,
    JAGLINK_PARAMETER_STORE_INVALID_ARGUMENT,
    JAGLINK_PARAMETER_STORE_FULL,
    JAGLINK_PARAMETER_STORE_DUPLICATE_KEY,
    JAGLINK_PARAMETER_STORE_DUPLICATE_STABLE_KEY,
    JAGLINK_PARAMETER_STORE_NOT_FOUND,
    JAGLINK_PARAMETER_STORE_DEFINITION_MISMATCH
} JaglinkParameterStoreResult;

typedef struct {
    const JaglinkParameterDefinition *definition;
    JaglinkParameterSample latest;
    bool latest_valid;
    bool favourite;
} JaglinkParameterStoreSlot;

typedef struct {
    JaglinkParameterStoreSlot slots[JAGLINK_PARAMETER_STORE_DEFINITION_CAPACITY];
    JaglinkParameterSample history[JAGLINK_PARAMETER_STORE_HISTORY_CAPACITY];
    size_t slot_count;
    size_t history_head;
    size_t history_count;
    uint64_t total_sample_count;
} JaglinkParameterStore;

const char *jaglink_parameter_store_result_name(JaglinkParameterStoreResult result);

void jaglink_parameter_store_init(JaglinkParameterStore *store);

/** Clear latest/history state while preserving definitions and favourites. */
void jaglink_parameter_store_clear_samples(JaglinkParameterStore *store);

/** Register one borrowed definition; keys and stable keys must both be unique. */
JaglinkParameterStoreResult jaglink_parameter_store_register(
    JaglinkParameterStore *store,
    const JaglinkParameterDefinition *definition);

size_t jaglink_parameter_store_definition_count(const JaglinkParameterStore *store);

const JaglinkParameterDefinition *jaglink_parameter_store_definition_at(
    const JaglinkParameterStore *store,
    size_t index);

const JaglinkParameterDefinition *jaglink_parameter_store_definition(
    const JaglinkParameterStore *store,
    const JaglinkParameterKey *key);

const JaglinkParameterDefinition *jaglink_parameter_store_definition_for_stable_key(
    const JaglinkParameterStore *store,
    const char *stable_key);

JaglinkParameterStoreResult jaglink_parameter_store_set_favourite(
    JaglinkParameterStore *store,
    const JaglinkParameterKey *key,
    bool favourite);

bool jaglink_parameter_store_is_favourite(
    const JaglinkParameterStore *store,
    const JaglinkParameterKey *key);

/** Record one sample transactionally after validating its registered definition. */
JaglinkParameterStoreResult jaglink_parameter_store_record(
    JaglinkParameterStore *store,
    const JaglinkParameterSample *sample);

bool jaglink_parameter_store_latest(
    const JaglinkParameterStore *store,
    const JaglinkParameterKey *key,
    JaglinkParameterSample *sample);

size_t jaglink_parameter_store_history_count(const JaglinkParameterStore *store);
uint64_t jaglink_parameter_store_total_sample_count(const JaglinkParameterStore *store);

bool jaglink_parameter_store_history_at(
    const JaglinkParameterStore *store,
    size_t chronological_index,
    JaglinkParameterSample *sample);

#ifdef __cplusplus
}
#endif

#endif
