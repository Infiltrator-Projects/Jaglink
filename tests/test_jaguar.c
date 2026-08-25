// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaguar.h"

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
    const JaglinkJaguarVehicleProfile *profile = jaglink_jaguar_x400_profile();
    const JaglinkJaguarNetworkDefinition *network;
    const JaglinkJaguarFuelSignalDefinition *fuel_signal;
    const JaglinkJaguarDiagnosticEndpointDefinition *endpoint;
    const JaglinkJaguarFactoryDtcDefinition *factory_dtc;
    JaglinkJaguarNetworkDefinition invalid = {
        "invalid", "Invalid", JAGLINK_JAGUAR_NETWORK_CAN,
        JAGLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN, 0U,
        JAGLINK_JAGUAR_DEFINITION_CANDIDATE, "test"
    };
    JaglinkJaguarFuelSignalDefinition invalid_fuel = {
        "bad", "Bad", "x400-powertrain-can", 0x800U,
        JAGLINK_JAGUAR_DEFINITION_CANDIDATE, false, "test"
    };
    JaglinkJaguarDiagnosticEndpointDefinition invalid_endpoint = {
        "bad", "Bad", JAGLINK_JAGUAR_MODULE_ECM, "x400-powertrain-can",
        0x800U, 0x7ecU, JAGLINK_JAGUAR_DEFINITION_CANDIDATE, "test"
    };
    JaglinkJaguarFactoryDtcDefinition invalid_dtc = {
        "Q1202", "bad", JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER,
        "test", false, JAGLINK_JAGUAR_DEFINITION_CANDIDATE, "test"
    };

    passed &= check(profile != NULL, "X400 profile missing");
    passed &= check(jaglink_jaguar_vehicle_profile_is_valid(profile), "X400 profile invalid");
    passed &= check(strcmp(profile->platform_code, "X400") == 0, "platform code mismatch");
    passed &= check(profile->first_model_year == 2001U && profile->last_model_year == 2009U, "model-year range mismatch");
    passed &= check(profile->network_count == 4U, "network count mismatch");
    passed &= check(profile->diagnostic_endpoint_count == 5U, "factory diagnostic endpoint count mismatch");
    passed &= check(profile->factory_dtc_count >= 16U, "factory DTC catalogue unexpectedly small");

    network = jaglink_jaguar_profile_find_network(profile, "x400-powertrain-can");
    passed &= check(network != NULL && network->kind == JAGLINK_JAGUAR_NETWORK_CAN && network->nominal_baud == 500000U, "CAN definition mismatch");
    network = jaglink_jaguar_profile_find_network(profile, "x400-body-scp");
    passed &= check(network != NULL && network->kind == JAGLINK_JAGUAR_NETWORK_SCP && network->nominal_baud == 41600U, "SCP definition mismatch");
    network = jaglink_jaguar_profile_find_network(profile, "x400-serial-iso9141");
    passed &= check(network != NULL && network->kind == JAGLINK_JAGUAR_NETWORK_ISO9141 && network->nominal_baud == 10400U, "ISO9141 definition mismatch");
    network = jaglink_jaguar_profile_find_network(profile, "x400-audio-d2b");
    passed &= check(network != NULL && network->kind == JAGLINK_JAGUAR_NETWORK_D2B && network->nominal_baud == 5600000U, "D2B definition mismatch");

    endpoint = jaglink_jaguar_profile_find_diagnostic_endpoint(profile, JAGLINK_JAGUAR_MODULE_ECM);
    passed &= check(endpoint != NULL && endpoint->request_message_id == 0x7e8U && endpoint->response_message_id == 0x7ecU,
                    "ECM factory diagnostic path mismatch");
    endpoint = jaglink_jaguar_profile_find_diagnostic_endpoint(profile, JAGLINK_JAGUAR_MODULE_TCM);
    passed &= check(endpoint != NULL && endpoint->request_message_id == 0x7e9U && endpoint->response_message_id == 0x7edU,
                    "TCM factory diagnostic path mismatch");
    endpoint = jaglink_jaguar_profile_find_diagnostic_endpoint(profile, JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER);
    passed &= check(endpoint != NULL && endpoint->request_message_id == 0x7eaU && endpoint->response_message_id == 0x7eeU,
                    "instrument-cluster factory diagnostic path mismatch");
    endpoint = jaglink_jaguar_profile_find_diagnostic_endpoint(profile, JAGLINK_JAGUAR_MODULE_ABS_DSC);
    passed &= check(endpoint != NULL && endpoint->request_message_id == 0x7ebU && endpoint->response_message_id == 0x7efU,
                    "ABS factory diagnostic path mismatch");
    endpoint = jaglink_jaguar_profile_find_diagnostic_endpoint(profile, JAGLINK_JAGUAR_MODULE_CLIMATE);
    passed &= check(endpoint != NULL && endpoint->request_message_id == 0x7c4U && endpoint->response_message_id == 0x7c5U,
                    "climate factory diagnostic path mismatch");

    factory_dtc = jaglink_jaguar_profile_find_factory_dtc(
        profile, JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER, "B1205");
    passed &= check(factory_dtc != NULL && !factory_dtc->generic_obd2_accessible,
                    "factory trip-computer DTC missing or misclassified");
    factory_dtc = jaglink_jaguar_profile_find_factory_dtc(
        profile, JAGLINK_JAGUAR_MODULE_GECM, "U1041");
    passed &= check(factory_dtc != NULL && strcmp(factory_dtc->category, "network") == 0,
                    "factory GECM network DTC missing");
    factory_dtc = jaglink_jaguar_profile_find_factory_dtc(
        profile, JAGLINK_JAGUAR_MODULE_ABS_DSC, "U1900");
    passed &= check(factory_dtc != NULL, "factory ABS/DSC U1900 DTC missing");
    passed &= check(jaglink_jaguar_profile_find_factory_dtc(
                        profile, JAGLINK_JAGUAR_MODULE_ECM, "U1900") == NULL,
                    "module-specific factory DTC lookup leaked across modules");

    passed &= check(jaglink_jaguar_x400_fuel_signal_count() == 1U,
                    "factory fuel signal count mismatch");
    fuel_signal = jaglink_jaguar_x400_find_fuel_signal("x400-can-fuel-used");
    passed &= check(fuel_signal != NULL, "CAN FUEL USED signal missing");
    passed &= check(fuel_signal != NULL && fuel_signal->message_id == 0x44dU,
                    "CAN FUEL USED identifier mismatch");
    passed &= check(fuel_signal != NULL &&
                    fuel_signal->status == JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED,
                    "CAN FUEL USED provenance status mismatch");
    passed &= check(fuel_signal != NULL && !fuel_signal->decoder_verified,
                    "unverified factory payload must not be treated as decoded");
    passed &= check(fuel_signal != NULL &&
                    jaglink_jaguar_fuel_signal_definition_is_valid(fuel_signal),
                    "factory fuel signal definition invalid");
    passed &= check(jaglink_jaguar_x400_fuel_signal_at(1U) == NULL,
                    "out-of-range factory fuel signal should not resolve");

    passed &= check(jaglink_jaguar_profile_find_network(profile, "missing") == NULL, "unknown network should not resolve");
    passed &= check(!jaglink_jaguar_network_definition_is_valid(&invalid), "zero-baud definition should be invalid");
    passed &= check(!jaglink_jaguar_fuel_signal_definition_is_valid(&invalid_fuel),
                    "out-of-range CAN identifier should be invalid");
    passed &= check(!jaglink_jaguar_diagnostic_endpoint_definition_is_valid(&invalid_endpoint),
                    "out-of-range diagnostic CAN identifier should be invalid");
    passed &= check(!jaglink_jaguar_factory_dtc_definition_is_valid(&invalid_dtc),
                    "invalid factory DTC code should be rejected");
    passed &= check(strcmp(jaglink_jaguar_definition_status_name(JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED), "source-corroborated") == 0, "status name mismatch");
    passed &= check(strcmp(jaglink_jaguar_module_kind_name(JAGLINK_JAGUAR_MODULE_ECM), "ECM") == 0,
                    "module name mismatch");

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
