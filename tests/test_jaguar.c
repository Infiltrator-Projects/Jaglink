// SPDX-License-Identifier: GPL-3.0-or-later
#include "mblink/jaguar.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool check(bool condition, const char *message)
{
    if (!condition) fprintf(stderr, "jaglink-jaguar-test: %s\n", message);
    return condition;
}

int main(void)
{
    bool passed = true;
    const MblinkJaguarVehicleProfile *profile = mblink_jaguar_x400_profile();
    const MblinkJaguarNetworkDefinition *network;
    MblinkJaguarNetworkDefinition invalid = {
        "invalid", "Invalid", MBLINK_JAGUAR_NETWORK_CAN,
        MBLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN, 0U,
        MBLINK_JAGUAR_DEFINITION_CANDIDATE, "test"
    };

    passed &= check(profile != NULL, "X400 profile missing");
    passed &= check(mblink_jaguar_vehicle_profile_is_valid(profile), "X400 profile invalid");
    passed &= check(strcmp(profile->platform_code, "X400") == 0, "platform code mismatch");
    passed &= check(profile->first_model_year == 2001U && profile->last_model_year == 2009U, "model-year range mismatch");
    passed &= check(profile->network_count == 4U, "network count mismatch");

    network = mblink_jaguar_profile_find_network(profile, "x400-powertrain-can");
    passed &= check(network != NULL && network->kind == MBLINK_JAGUAR_NETWORK_CAN && network->nominal_baud == 500000U, "CAN definition mismatch");
    network = mblink_jaguar_profile_find_network(profile, "x400-body-scp");
    passed &= check(network != NULL && network->kind == MBLINK_JAGUAR_NETWORK_SCP && network->nominal_baud == 41600U, "SCP definition mismatch");
    network = mblink_jaguar_profile_find_network(profile, "x400-serial-iso9141");
    passed &= check(network != NULL && network->kind == MBLINK_JAGUAR_NETWORK_ISO9141 && network->nominal_baud == 10400U, "ISO9141 definition mismatch");
    network = mblink_jaguar_profile_find_network(profile, "x400-audio-d2b");
    passed &= check(network != NULL && network->kind == MBLINK_JAGUAR_NETWORK_D2B && network->nominal_baud == 5600000U, "D2B definition mismatch");

    passed &= check(mblink_jaguar_profile_find_network(profile, "missing") == NULL, "unknown network should not resolve");
    passed &= check(!mblink_jaguar_network_definition_is_valid(&invalid), "zero-baud definition should be invalid");
    passed &= check(strcmp(mblink_jaguar_definition_status_name(MBLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED), "source-corroborated") == 0, "status name mismatch");

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
