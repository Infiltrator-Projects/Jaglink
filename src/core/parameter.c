// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/parameter.h"

const char *jaglink_parameter_protocol_name(JaglinkParameterProtocol protocol)
{
    return link_parameter_protocol_name(protocol);
}

bool jaglink_parameter_key_is_valid(const JaglinkParameterKey *key)
{
    return link_parameter_key_is_valid(key);
}

bool jaglink_parameter_key_equal(const JaglinkParameterKey *left,
                                 const JaglinkParameterKey *right)
{
    return link_parameter_key_equal(left, right);
}

bool jaglink_parameter_definition_is_valid(
    const JaglinkParameterDefinition *definition)
{
    return link_parameter_definition_is_valid(definition);
}

bool jaglink_parameter_sample_is_valid(const JaglinkParameterSample *sample)
{
    return link_parameter_sample_is_valid(sample);
}

bool jaglink_parameter_format_value(
    const JaglinkParameterDefinition *definition,
    bool available,
    double value,
    char *buffer,
    size_t buffer_size)
{
    return link_parameter_format_value(
        definition, available, value, buffer, buffer_size);
}

bool jaglink_parameter_format_sample(
    const JaglinkParameterSample *sample,
    char *buffer,
    size_t buffer_size)
{
    return link_parameter_format_sample(sample, buffer, buffer_size);
}

size_t jaglink_parameter_obd2_definition_count(void)
{
    return link_parameter_obd2_definition_count();
}

const JaglinkParameterDefinition *jaglink_parameter_obd2_definition_at(size_t index)
{
    return link_parameter_obd2_definition_at(index);
}

const JaglinkParameterDefinition *jaglink_parameter_obd2_definition(uint8_t pid)
{
    return link_parameter_obd2_definition(pid);
}

const JaglinkParameterDefinition *jaglink_parameter_obd2_definition_for_stable_key(
    const char *stable_key)
{
    return link_parameter_obd2_definition_for_stable_key(stable_key);
}

bool jaglink_parameter_from_obd2(const JaglinkObd2Sample *sample,
                                 uint64_t timestamp_ms,
                                 JaglinkParameterSample *parameter)
{
    if (sample == NULL) {
        return false;
    }
    return link_parameter_from_obd2_scalar(
        sample->pid,
        (LinkObd2UnitCode)sample->unit,
        sample->value,
        timestamp_ms,
        parameter);
}
