// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/parameter.h"
#include "jaglink/parameter_store.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool check(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "jaglink-parameter-store-test: %s\n", message);
    }
    return condition;
}

int main(void)
{
    bool passed = true;
    JaglinkParameterStore store;
    const JaglinkParameterDefinition *rpm =
        jaglink_parameter_obd2_definition(0x0cU);
    const JaglinkParameterDefinition *coolant =
        jaglink_parameter_obd2_definition(0x05U);
    JaglinkObd2Sample obd = { 0x0cU, 1500.0, JAGLINK_OBD2_UNIT_RPM };
    JaglinkParameterSample parameter;
    JaglinkParameterSample latest;

    jaglink_parameter_store_init(&store);
    passed &= check(jaglink_parameter_store_definition_count(&store) == 0U,
                    "new store should have no definitions");
    passed &= check(jaglink_parameter_store_history_count(&store) == 0U,
                    "new store should have no history");

    passed &= check(rpm != NULL && coolant != NULL,
                    "standard definitions missing");
    if (rpm == NULL || coolant == NULL) {
        return EXIT_FAILURE;
    }

    passed &= check(jaglink_parameter_store_register(&store, rpm) ==
                        JAGLINK_PARAMETER_STORE_OK,
                    "RPM registration failed");
    passed &= check(jaglink_parameter_store_register(&store, coolant) ==
                        JAGLINK_PARAMETER_STORE_OK,
                    "coolant registration failed");
    passed &= check(jaglink_parameter_store_definition_count(&store) == 2U,
                    "definition count mismatch");
    passed &= check(jaglink_parameter_store_definition_at(&store, 0U) == rpm,
                    "definition order mismatch");
    passed &= check(jaglink_parameter_store_definition(&store, &rpm->key) == rpm,
                    "key lookup mismatch");
    passed &= check(jaglink_parameter_store_definition_for_stable_key(
                        &store, "obd2.engine.coolant") == coolant,
                    "stable-key lookup mismatch");
    passed &= check(jaglink_parameter_store_register(&store, rpm) ==
                        JAGLINK_PARAMETER_STORE_DUPLICATE_KEY,
                    "duplicate key should be rejected");

    {
        JaglinkParameterDefinition duplicate_stable = *rpm;
        duplicate_stable.key.identifier = 0x77U;
        passed &= check(jaglink_parameter_store_register(
                            &store, &duplicate_stable) ==
                            JAGLINK_PARAMETER_STORE_DUPLICATE_STABLE_KEY,
                        "duplicate stable key should be rejected");
    }

    passed &= check(jaglink_parameter_store_set_favourite(
                        &store, &rpm->key, true) == JAGLINK_PARAMETER_STORE_OK,
                    "set favourite failed");
    passed &= check(jaglink_parameter_store_is_favourite(&store, &rpm->key),
                    "favourite state missing");

    passed &= check(jaglink_parameter_from_obd2(&obd, 10U, &parameter),
                    "OBD conversion failed");
    passed &= check(jaglink_parameter_store_record(&store, &parameter) ==
                        JAGLINK_PARAMETER_STORE_OK,
                    "record failed");
    passed &= check(jaglink_parameter_store_latest(
                        &store, &rpm->key, &latest),
                    "latest lookup failed");
    passed &= check(latest.definition == rpm && latest.timestamp_ms == 10U &&
                    latest.available && latest.value == 1500.0,
                    "latest sample mismatch");
    passed &= check(jaglink_parameter_store_history_count(&store) == 1U &&
                    jaglink_parameter_store_total_sample_count(&store) == 1U,
                    "history counters mismatch");

    {
        JaglinkParameterDefinition copied_definition = *rpm;
        JaglinkParameterSample wrong_definition = parameter;
        wrong_definition.definition = &copied_definition;
        passed &= check(jaglink_parameter_store_record(
                            &store, &wrong_definition) ==
                            JAGLINK_PARAMETER_STORE_DEFINITION_MISMATCH,
                        "non-canonical definition pointer should be rejected");
        passed &= check(jaglink_parameter_store_total_sample_count(&store) == 1U,
                        "failed record changed sample count");
    }

    jaglink_parameter_store_clear_samples(&store);
    passed &= check(jaglink_parameter_store_history_count(&store) == 0U &&
                    jaglink_parameter_store_total_sample_count(&store) == 0U,
                    "clear did not reset sample state");
    passed &= check(!jaglink_parameter_store_latest(
                        &store, &rpm->key, &latest),
                    "clear left latest sample valid");
    passed &= check(jaglink_parameter_store_is_favourite(&store, &rpm->key),
                    "clear should preserve favourites");
    passed &= check(jaglink_parameter_store_definition_count(&store) == 2U,
                    "clear should preserve definitions");

    for (size_t index = 0U;
         index < JAGLINK_PARAMETER_STORE_HISTORY_CAPACITY + 5U;
         ++index) {
        obd.value = 1000.0 + (double)index;
        passed &= check(jaglink_parameter_from_obd2(
                            &obd, (uint64_t)index, &parameter),
                        "loop OBD conversion failed");
        passed &= check(jaglink_parameter_store_record(&store, &parameter) ==
                            JAGLINK_PARAMETER_STORE_OK,
                        "loop record failed");
    }

    passed &= check(jaglink_parameter_store_history_count(&store) ==
                        JAGLINK_PARAMETER_STORE_HISTORY_CAPACITY,
                    "history ring did not cap at capacity");
    passed &= check(jaglink_parameter_store_total_sample_count(&store) ==
                        JAGLINK_PARAMETER_STORE_HISTORY_CAPACITY + 5U,
                    "total sample count should exceed retained history");

    {
        JaglinkParameterSample first;
        JaglinkParameterSample last;
        passed &= check(jaglink_parameter_store_history_at(&store, 0U, &first),
                        "oldest retained sample unavailable");
        passed &= check(jaglink_parameter_store_history_at(
                            &store,
                            JAGLINK_PARAMETER_STORE_HISTORY_CAPACITY - 1U,
                            &last),
                        "newest retained sample unavailable");
        passed &= check(first.timestamp_ms == 5U,
                        "history ring retained the wrong oldest sample");
        passed &= check(last.timestamp_ms ==
                            JAGLINK_PARAMETER_STORE_HISTORY_CAPACITY + 4U,
                        "history ring retained the wrong newest sample");
        passed &= check(!jaglink_parameter_store_history_at(
                            &store,
                            JAGLINK_PARAMETER_STORE_HISTORY_CAPACITY,
                            &last),
                        "out-of-range history index should fail");
    }

    passed &= check(strcmp(jaglink_parameter_store_result_name(
                               JAGLINK_PARAMETER_STORE_DEFINITION_MISMATCH),
                           "definition-mismatch") == 0,
                    "result name mismatch");

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
