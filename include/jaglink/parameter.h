// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file parameter.h
 * @brief Protocol-neutral live diagnostic parameter contracts.
 *
 * This layer gives OBD-II and manufacturer UDS data one stable C-facing model
 * for live-data, table, dashboard and graph presentation. Protocol decoding
 * remains in its owning layer; this module owns only parameter identity,
 * metadata and presentation-safe scalar formatting.
 */
#ifndef JAGLINK_PARAMETER_H
#define JAGLINK_PARAMETER_H

#include "jaglink/obd2.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_PARAMETER_MODULE_STANDARD_OBD2 0U

typedef enum {
    JAGLINK_PARAMETER_PROTOCOL_OBD2 = 0,
    JAGLINK_PARAMETER_PROTOCOL_UDS
} JaglinkParameterProtocol;

/** Stable machine identity for one diagnostic parameter. */
typedef struct {
    JaglinkParameterProtocol protocol;
    uint32_t module;
    uint32_t identifier;
} JaglinkParameterKey;

/** Portable metadata for one scalar diagnostic parameter. */
typedef struct {
    JaglinkParameterKey key;
    const char *stable_key;
    const char *short_name;
    const char *name;
    const char *suffix;
    unsigned int decimal_places;
    bool clamp;
    double minimum;
    double maximum;
} JaglinkParameterDefinition;

typedef struct {
    const JaglinkParameterDefinition *definition;
    uint64_t timestamp_ms;
    bool available;
    double value;
} JaglinkParameterSample;

const char *jaglink_parameter_protocol_name(JaglinkParameterProtocol protocol);
bool jaglink_parameter_key_is_valid(const JaglinkParameterKey *key);
bool jaglink_parameter_key_equal(const JaglinkParameterKey *left,
                                const JaglinkParameterKey *right);
bool jaglink_parameter_definition_is_valid(
    const JaglinkParameterDefinition *definition);
bool jaglink_parameter_sample_is_valid(const JaglinkParameterSample *sample);

/** Format a scalar through Infiltratr Common's canonical scalar formatter. */
bool jaglink_parameter_format_value(
    const JaglinkParameterDefinition *definition,
    bool available,
    double value,
    char *buffer,
    size_t buffer_size);

bool jaglink_parameter_format_sample(
    const JaglinkParameterSample *sample,
    char *buffer,
    size_t buffer_size);

/** Standard OBD-II descriptors currently used by the live workspace. */
size_t jaglink_parameter_obd2_definition_count(void);
const JaglinkParameterDefinition *jaglink_parameter_obd2_definition_at(
    size_t index);
const JaglinkParameterDefinition *jaglink_parameter_obd2_definition(
    uint8_t pid);
const JaglinkParameterDefinition *jaglink_parameter_obd2_definition_for_stable_key(
    const char *stable_key);

/** Convert one decoded OBD-II scalar into the shared parameter model. */
bool jaglink_parameter_from_obd2(
    const JaglinkObd2Sample *sample,
    uint64_t timestamp_ms,
    JaglinkParameterSample *parameter);

#ifdef __cplusplus
}
#endif

#endif
