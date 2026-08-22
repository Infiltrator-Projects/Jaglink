// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef JAGLINK_PARAMETER_H
#define JAGLINK_PARAMETER_H

#include "jaglink/obd2.h"
#include "link/parameter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef LinkParameterProtocol JaglinkParameterProtocol;
typedef LinkParameterKey JaglinkParameterKey;
typedef LinkParameterDefinition JaglinkParameterDefinition;
typedef LinkParameterSample JaglinkParameterSample;

#define JAGLINK_PARAMETER_MODULE_STANDARD_OBD2 LINK_PARAMETER_MODULE_STANDARD_OBD2
#define JAGLINK_PARAMETER_PROTOCOL_OBD2 LINK_PARAMETER_PROTOCOL_OBD2
#define JAGLINK_PARAMETER_PROTOCOL_UDS LINK_PARAMETER_PROTOCOL_UDS

const char *jaglink_parameter_protocol_name(JaglinkParameterProtocol protocol);
bool jaglink_parameter_key_is_valid(const JaglinkParameterKey *key);
bool jaglink_parameter_key_equal(const JaglinkParameterKey *left,
                                 const JaglinkParameterKey *right);
bool jaglink_parameter_definition_is_valid(
    const JaglinkParameterDefinition *definition);
bool jaglink_parameter_sample_is_valid(const JaglinkParameterSample *sample);
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
size_t jaglink_parameter_obd2_definition_count(void);
const JaglinkParameterDefinition *jaglink_parameter_obd2_definition_at(size_t index);
const JaglinkParameterDefinition *jaglink_parameter_obd2_definition(uint8_t pid);
const JaglinkParameterDefinition *jaglink_parameter_obd2_definition_for_stable_key(
    const char *stable_key);
bool jaglink_parameter_from_obd2(const JaglinkObd2Sample *sample,
                                 uint64_t timestamp_ms,
                                 JaglinkParameterSample *parameter);

#ifdef __cplusplus
}
#endif
#endif
