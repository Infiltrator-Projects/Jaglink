// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/parameter.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool check(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "jaglink-parameter-test: %s\n", message);
    }
    return condition;
}

int main(void)
{
    bool passed = true;
    char buffer[64];
    JaglinkObd2Sample obd = { 0x0cU, 1234.4, JAGLINK_OBD2_UNIT_RPM };
    JaglinkParameterSample parameter = { 0 };
    const JaglinkParameterDefinition *rpm =
        jaglink_parameter_obd2_definition(0x0cU);
    const JaglinkParameterDefinition *maf =
        jaglink_parameter_obd2_definition(0x10U);
    const JaglinkParameterDefinition *rail =
        jaglink_parameter_obd2_definition(0x23U);
    const JaglinkParameterDefinition *dpf_pressure =
        jaglink_parameter_obd2_definition(0x7aU);

    const size_t descriptor_count = jaglink_parameter_obd2_definition_count();
    passed &= check(descriptor_count == 28U,
                    "standard descriptor count mismatch");
    passed &= check(rpm != NULL && maf != NULL && rail != NULL &&
                    dpf_pressure != NULL,
                    "expected OBD descriptors missing");
    passed &= check(jaglink_parameter_obd2_definition(0xffU) == NULL,
                    "unknown PID unexpectedly has a descriptor");
    passed &= check(jaglink_parameter_obd2_definition_at(descriptor_count) == NULL,
                    "out-of-range descriptor index should fail");
    passed &= check(
        jaglink_parameter_obd2_definition_for_stable_key("obd2.engine.rpm") == rpm,
        "stable-key lookup mismatch");
    passed &= check(
        jaglink_parameter_obd2_definition_for_stable_key(
            "obd2.dpf.bank1_delta_pressure") == dpf_pressure,
        "DPF stable-key lookup mismatch");
    passed &= check(
        jaglink_parameter_obd2_definition_for_stable_key("obd2.missing") == NULL,
        "unknown stable key unexpectedly resolved");

    if (rpm != NULL) {
        JaglinkParameterKey same = rpm->key;
        JaglinkParameterKey different = rpm->key;
        different.identifier++;
        passed &= check(jaglink_parameter_definition_is_valid(rpm),
                        "RPM definition should validate");
        passed &= check(strcmp(rpm->short_name, "RPM") == 0,
                        "RPM short name mismatch");
        passed &= check(jaglink_parameter_key_equal(&rpm->key, &same),
                        "equal keys did not compare equal");
        passed &= check(!jaglink_parameter_key_equal(&rpm->key, &different),
                        "different keys compared equal");
    }

    passed &= check(jaglink_parameter_from_obd2(&obd, 42U, &parameter),
                    "OBD conversion failed");
    passed &= check(parameter.definition == rpm && parameter.available &&
                    parameter.timestamp_ms == 42U &&
                    fabs(parameter.value - 1234.4) < 0.0001,
                    "OBD conversion produced wrong parameter");
    passed &= check(jaglink_parameter_format_sample(
                        &parameter, buffer, sizeof(buffer)) &&
                    strcmp(buffer, "1234 rpm") == 0,
                    "RPM formatting mismatch");

    obd.pid = 0x10U;
    obd.value = 12.34;
    obd.unit = JAGLINK_OBD2_UNIT_GRAMS_PER_SECOND;
    passed &= check(jaglink_parameter_from_obd2(&obd, 100U, &parameter),
                    "MAF conversion failed");
    passed &= check(parameter.definition == maf &&
                    jaglink_parameter_format_sample(
                        &parameter, buffer, sizeof(buffer)) &&
                    strcmp(buffer, "12.3 g/s") == 0,
                    "MAF formatting mismatch");

    obd.pid = 0x23U;
    obd.value = 123400.0;
    obd.unit = JAGLINK_OBD2_UNIT_KPA;
    passed &= check(jaglink_parameter_from_obd2(&obd, 110U, &parameter),
                    "rail-pressure conversion failed");
    passed &= check(parameter.definition == rail &&
                    jaglink_parameter_format_sample(
                        &parameter, buffer, sizeof(buffer)) &&
                    strcmp(buffer, "123.4 MPa") == 0,
                    "rail-pressure formatting mismatch");

    obd.pid = 0x7aU;
    obd.value = 2.34;
    obd.unit = JAGLINK_OBD2_UNIT_KPA;
    passed &= check(jaglink_parameter_from_obd2(&obd, 120U, &parameter),
                    "DPF pressure conversion failed");
    passed &= check(parameter.definition == dpf_pressure &&
                    jaglink_parameter_format_sample(
                        &parameter, buffer, sizeof(buffer)) &&
                    strcmp(buffer, "2.34 kPa") == 0,
                    "DPF pressure formatting mismatch");

    if (rpm != NULL) {
        JaglinkParameterSample unavailable = { rpm, 0U, false, NAN };
        passed &= check(jaglink_parameter_sample_is_valid(&unavailable),
                        "unavailable sample should validate");
        passed &= check(jaglink_parameter_format_sample(
                            &unavailable, buffer, sizeof(buffer)) &&
                        strcmp(buffer, "N/A") == 0,
                        "unavailable formatting mismatch");
    }

    {
        JaglinkParameterSample sentinel = parameter;
        obd.pid = 0x0cU;
        obd.value = 2000.0;
        obd.unit = JAGLINK_OBD2_UNIT_KPA;
        passed &= check(!jaglink_parameter_from_obd2(&obd, 200U, &parameter),
                        "wrong OBD unit should be rejected");
        passed &= check(memcmp(&parameter, &sentinel, sizeof(parameter)) == 0,
                        "failed conversion must be transactional");
    }

    {
        JaglinkParameterDefinition uds = {
            { JAGLINK_PARAMETER_PROTOCOL_UDS, 1U, 0xf190U },
            "jaguar.generic.example", "EXAMPLE", "Example UDS value", "", 1U,
            false, 0.0, 0.0
        };
        JaglinkParameterDefinition invalid = uds;
        passed &= check(jaglink_parameter_definition_is_valid(&uds),
                        "valid UDS definition rejected");
        invalid.key.identifier = 0x10000U;
        passed &= check(!jaglink_parameter_definition_is_valid(&invalid),
                        "out-of-range UDS identifier accepted");
        invalid = uds;
        invalid.decimal_places = 10U;
        passed &= check(!jaglink_parameter_definition_is_valid(&invalid),
                        "invalid decimal precision accepted");
        invalid = uds;
        invalid.short_name = "";
        passed &= check(!jaglink_parameter_definition_is_valid(&invalid),
                        "empty short name accepted");
    }

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
