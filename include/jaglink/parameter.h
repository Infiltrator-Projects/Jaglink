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

#define jaglink_parameter_protocol_name link_parameter_protocol_name
#define jaglink_parameter_key_is_valid link_parameter_key_is_valid
#define jaglink_parameter_key_equal link_parameter_key_equal
#define jaglink_parameter_definition_is_valid link_parameter_definition_is_valid
#define jaglink_parameter_sample_is_valid link_parameter_sample_is_valid
#define jaglink_parameter_format_value link_parameter_format_value
#define jaglink_parameter_format_sample link_parameter_format_sample
#define jaglink_parameter_obd2_definition_count link_parameter_obd2_definition_count
#define jaglink_parameter_obd2_definition_at link_parameter_obd2_definition_at
#define jaglink_parameter_obd2_definition link_parameter_obd2_definition
#define jaglink_parameter_obd2_definition_for_stable_key link_parameter_obd2_definition_for_stable_key

bool jaglink_parameter_from_obd2(const JaglinkObd2Sample *sample,
                                 uint64_t timestamp_ms,
                                 JaglinkParameterSample *parameter);

#ifdef __cplusplus
}
#endif
#endif
