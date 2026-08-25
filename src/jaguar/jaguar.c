// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaguar.h"

#include <ctype.h>
#include <string.h>

static const char x400_network_provenance[] =
    "Jaguar Introduction to X-TYPE Service Training (2002) and Jaguar X-TYPE 2002 Electrical Guide";
static const char x400_diagnostic_provenance[] =
    "Jaguar X-TYPE 2002 Electrical Guide CAN message matrix, factory diagnostic data identifiers";
static const char x400_dtc_provenance[] =
    "Jaguar X-TYPE 2002 Model Year DTC Summaries: factory module B/C/U diagnostic catalogue";
static const char x400_fuel_used_provenance[] =
    "Jaguar X-TYPE Electrical Guide CAN message matrix: ID 0x44D, CAN FUEL USED, ECM to instrument cluster, data for trip computer calculations. Numerical byte layout/scaling is not yet vehicle-verified.";

static const JaglinkJaguarNetworkDefinition x400_networks[] = {
    { "x400-powertrain-can", "Powertrain CAN", JAGLINK_JAGUAR_NETWORK_CAN,
      JAGLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN, 500000U,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_network_provenance },
    { "x400-body-scp", "Body SCP", JAGLINK_JAGUAR_NETWORK_SCP,
      JAGLINK_JAGUAR_NETWORK_ROLE_BODY, 41600U,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_network_provenance },
    { "x400-serial-iso9141", "Serial Data Link (ISO 9141)", JAGLINK_JAGUAR_NETWORK_ISO9141,
      JAGLINK_JAGUAR_NETWORK_ROLE_SERIAL_DIAGNOSTIC, 10400U,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_network_provenance },
    { "x400-audio-d2b", "D2B Optical", JAGLINK_JAGUAR_NETWORK_D2B,
      JAGLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT, 5600000U,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_network_provenance }
};

static const JaglinkJaguarDiagnosticEndpointDefinition x400_diagnostic_endpoints[] = {
    { "x400-diag-climate", "Air conditioning control module", JAGLINK_JAGUAR_MODULE_CLIMATE,
      "x400-powertrain-can", 0x7c4U, 0x7c5U,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_diagnostic_provenance },
    { "x400-diag-ecm", "Engine control module", JAGLINK_JAGUAR_MODULE_ECM,
      "x400-powertrain-can", 0x7e8U, 0x7ecU,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_diagnostic_provenance },
    { "x400-diag-tcm", "Transmission control module", JAGLINK_JAGUAR_MODULE_TCM,
      "x400-powertrain-can", 0x7e9U, 0x7edU,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_diagnostic_provenance },
    { "x400-diag-ic", "Instrument cluster", JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER,
      "x400-powertrain-can", 0x7eaU, 0x7eeU,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_diagnostic_provenance },
    { "x400-diag-abs", "ABS / DSC control module", JAGLINK_JAGUAR_MODULE_ABS_DSC,
      "x400-powertrain-can", 0x7ebU, 0x7efU,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_diagnostic_provenance }
};

static const JaglinkJaguarFactoryDtcDefinition x400_factory_dtcs[] = {
    { "B1202", "x400.ic.fuel-level-sensor-1", JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER,
      "fuel-level", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "B1204", "x400.ic.fuel-level-sensor-1-short", JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER,
      "fuel-level", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "B1205", "x400.ic.trip-computer-switch", JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER,
      "trip-computer", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "B1213", "x400.ic.security-key-count", JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER,
      "security", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "B1231", "x400.rcm.crash-data", JAGLINK_JAGUAR_MODULE_RESTRAINTS,
      "restraints", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "C1095", "x400.abs.hydraulic-pump", JAGLINK_JAGUAR_MODULE_ABS_DSC,
      "braking", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "C1137", "x400.abs.internal", JAGLINK_JAGUAR_MODULE_ABS_DSC,
      "braking", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "C1145", "x400.abs.rh-front-wheel-speed", JAGLINK_JAGUAR_MODULE_ABS_DSC,
      "wheel-speed", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "C1155", "x400.abs.lh-front-wheel-speed", JAGLINK_JAGUAR_MODULE_ABS_DSC,
      "wheel-speed", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "C1956", "x400.dsc.steering-angle", JAGLINK_JAGUAR_MODULE_ABS_DSC,
      "stability-control", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "U1041", "x400.gecm.vehicle-speed-network", JAGLINK_JAGUAR_MODULE_GECM,
      "network", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "U1135", "x400.gecm.ignition-switch-network", JAGLINK_JAGUAR_MODULE_GECM,
      "network", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "U1147", "x400.gecm.key-in-network", JAGLINK_JAGUAR_MODULE_GECM,
      "network", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "U1262", "x400.gecm.scp-network", JAGLINK_JAGUAR_MODULE_GECM,
      "network", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "U1900", "x400.abs.can-network", JAGLINK_JAGUAR_MODULE_ABS_DSC,
      "network", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance },
    { "U1900", "x400.ic.can-network", JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER,
      "network", false, JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_dtc_provenance }
};

static const JaglinkJaguarFuelSignalDefinition x400_fuel_signals[] = {
    { "x400-can-fuel-used", "CAN FUEL USED", "x400-powertrain-can", 0x44dU,
      JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, false, x400_fuel_used_provenance }
};

static const JaglinkJaguarVehicleProfile x400_profile = {
    .platform_code = "X400",
    .platform_family = "Jaguar X400 / Ford CD132",
    .display_name = "Jaguar X-Type (X400), 2001-2009",
    .first_model_year = 2001U,
    .last_model_year = 2009U,
    .networks = x400_networks,
    .network_count = sizeof(x400_networks) / sizeof(x400_networks[0]),
    .diagnostic_endpoints = x400_diagnostic_endpoints,
    .diagnostic_endpoint_count = sizeof(x400_diagnostic_endpoints) / sizeof(x400_diagnostic_endpoints[0]),
    .factory_dtcs = x400_factory_dtcs,
    .factory_dtc_count = sizeof(x400_factory_dtcs) / sizeof(x400_factory_dtcs[0])
};

const char *jaglink_jaguar_definition_status_name(JaglinkJaguarDefinitionStatus status)
{
    switch (status) {
    case JAGLINK_JAGUAR_DEFINITION_CANDIDATE: return "candidate";
    case JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED: return "source-corroborated";
    case JAGLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED: return "vehicle-verified";
    }
    return "unknown";
}

const char *jaglink_jaguar_network_kind_name(JaglinkJaguarNetworkKind kind)
{
    switch (kind) {
    case JAGLINK_JAGUAR_NETWORK_CAN: return "can";
    case JAGLINK_JAGUAR_NETWORK_SCP: return "scp";
    case JAGLINK_JAGUAR_NETWORK_ISO9141: return "iso9141";
    case JAGLINK_JAGUAR_NETWORK_D2B: return "d2b";
    }
    return "unknown";
}

const char *jaglink_jaguar_network_role_name(JaglinkJaguarNetworkRole role)
{
    switch (role) {
    case JAGLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN: return "powertrain";
    case JAGLINK_JAGUAR_NETWORK_ROLE_BODY: return "body";
    case JAGLINK_JAGUAR_NETWORK_ROLE_SERIAL_DIAGNOSTIC: return "serial-diagnostic";
    case JAGLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT: return "infotainment";
    }
    return "unknown";
}

const char *jaglink_jaguar_module_kind_name(JaglinkJaguarModuleKind module)
{
    switch (module) {
    case JAGLINK_JAGUAR_MODULE_ECM: return "ECM";
    case JAGLINK_JAGUAR_MODULE_TCM: return "TCM";
    case JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER: return "IC";
    case JAGLINK_JAGUAR_MODULE_ABS_DSC: return "ABS/DSC";
    case JAGLINK_JAGUAR_MODULE_CLIMATE: return "A/CCM";
    case JAGLINK_JAGUAR_MODULE_GECM: return "GECM";
    case JAGLINK_JAGUAR_MODULE_RESTRAINTS: return "RCM";
    case JAGLINK_JAGUAR_MODULE_AUDIO: return "AUDIO";
    case JAGLINK_JAGUAR_MODULE_OTHER: return "other";
    }
    return "unknown";
}

static bool valid_status(JaglinkJaguarDefinitionStatus status)
{
    return status >= JAGLINK_JAGUAR_DEFINITION_CANDIDATE &&
           status <= JAGLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED;
}

static bool valid_module(JaglinkJaguarModuleKind module)
{
    return module >= JAGLINK_JAGUAR_MODULE_ECM && module <= JAGLINK_JAGUAR_MODULE_OTHER;
}

static bool valid_dtc_code(const char *code)
{
    size_t index;
    if (code == NULL || strlen(code) != 5U ||
        (code[0] != 'P' && code[0] != 'B' && code[0] != 'C' && code[0] != 'U')) return false;
    for (index = 1U; index < 5U; ++index) {
        if (!isxdigit((unsigned char)code[index]) ||
            (code[index] >= 'a' && code[index] <= 'f')) return false;
    }
    return true;
}

bool jaglink_jaguar_network_definition_is_valid(const JaglinkJaguarNetworkDefinition *definition)
{
    if (definition == NULL || definition->key == NULL || definition->key[0] == '\0' ||
        definition->name == NULL || definition->name[0] == '\0' ||
        definition->nominal_baud == 0U || definition->provenance == NULL ||
        definition->provenance[0] == '\0') return false;
    if (definition->kind < JAGLINK_JAGUAR_NETWORK_CAN || definition->kind > JAGLINK_JAGUAR_NETWORK_D2B) return false;
    if (definition->role < JAGLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN || definition->role > JAGLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT) return false;
    return valid_status(definition->status);
}

bool jaglink_jaguar_diagnostic_endpoint_definition_is_valid(
    const JaglinkJaguarDiagnosticEndpointDefinition *definition)
{
    return definition != NULL && definition->key != NULL && definition->key[0] != '\0' &&
           definition->name != NULL && definition->name[0] != '\0' && valid_module(definition->module) &&
           definition->network_key != NULL && definition->network_key[0] != '\0' &&
           definition->request_message_id <= 0x7ffU && definition->response_message_id <= 0x7ffU &&
           definition->request_message_id != definition->response_message_id && valid_status(definition->status) &&
           definition->provenance != NULL && definition->provenance[0] != '\0';
}

bool jaglink_jaguar_factory_dtc_definition_is_valid(
    const JaglinkJaguarFactoryDtcDefinition *definition)
{
    return definition != NULL && valid_dtc_code(definition->code) &&
           definition->stable_key != NULL && definition->stable_key[0] != '\0' &&
           valid_module(definition->module) && definition->category != NULL &&
           definition->category[0] != '\0' && valid_status(definition->status) &&
           definition->provenance != NULL && definition->provenance[0] != '\0';
}

bool jaglink_jaguar_fuel_signal_definition_is_valid(
    const JaglinkJaguarFuelSignalDefinition *definition)
{
    if (definition == NULL || definition->key == NULL || definition->key[0] == '\0' ||
        definition->name == NULL || definition->name[0] == '\0' ||
        definition->network_key == NULL || definition->network_key[0] == '\0' ||
        definition->provenance == NULL || definition->provenance[0] == '\0' ||
        definition->message_id > 0x7ffU || !valid_status(definition->status)) return false;
    return !definition->decoder_verified ||
           definition->status == JAGLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED;
}

bool jaglink_jaguar_vehicle_profile_is_valid(const JaglinkJaguarVehicleProfile *profile)
{
    size_t index;
    if (profile == NULL || profile->platform_code == NULL || profile->platform_code[0] == '\0' ||
        profile->platform_family == NULL || profile->platform_family[0] == '\0' ||
        profile->display_name == NULL || profile->display_name[0] == '\0' ||
        profile->first_model_year == 0U || profile->last_model_year < profile->first_model_year ||
        profile->networks == NULL || profile->network_count == 0U ||
        (profile->diagnostic_endpoint_count != 0U && profile->diagnostic_endpoints == NULL) ||
        (profile->factory_dtc_count != 0U && profile->factory_dtcs == NULL)) return false;
    for (index = 0U; index < profile->network_count; ++index) {
        size_t earlier;
        if (!jaglink_jaguar_network_definition_is_valid(&profile->networks[index])) return false;
        for (earlier = 0U; earlier < index; ++earlier)
            if (strcmp(profile->networks[index].key, profile->networks[earlier].key) == 0) return false;
    }
    for (index = 0U; index < profile->diagnostic_endpoint_count; ++index) {
        const JaglinkJaguarDiagnosticEndpointDefinition *endpoint = &profile->diagnostic_endpoints[index];
        if (!jaglink_jaguar_diagnostic_endpoint_definition_is_valid(endpoint) ||
            jaglink_jaguar_profile_find_network(profile, endpoint->network_key) == NULL) return false;
    }
    for (index = 0U; index < profile->factory_dtc_count; ++index)
        if (!jaglink_jaguar_factory_dtc_definition_is_valid(&profile->factory_dtcs[index])) return false;
    return true;
}

const JaglinkJaguarNetworkDefinition *jaglink_jaguar_profile_find_network(
    const JaglinkJaguarVehicleProfile *profile, const char *key)
{
    size_t index;
    if (profile == NULL || key == NULL || key[0] == '\0') return NULL;
    for (index = 0U; index < profile->network_count; ++index)
        if (strcmp(profile->networks[index].key, key) == 0) return &profile->networks[index];
    return NULL;
}

const JaglinkJaguarDiagnosticEndpointDefinition *
jaglink_jaguar_profile_find_diagnostic_endpoint(
    const JaglinkJaguarVehicleProfile *profile, JaglinkJaguarModuleKind module)
{
    size_t index;
    if (profile == NULL || !valid_module(module)) return NULL;
    for (index = 0U; index < profile->diagnostic_endpoint_count; ++index)
        if (profile->diagnostic_endpoints[index].module == module) return &profile->diagnostic_endpoints[index];
    return NULL;
}

const JaglinkJaguarFactoryDtcDefinition *jaglink_jaguar_profile_find_factory_dtc(
    const JaglinkJaguarVehicleProfile *profile,
    JaglinkJaguarModuleKind module,
    const char *code)
{
    size_t index;
    if (profile == NULL || !valid_module(module) || !valid_dtc_code(code)) return NULL;
    for (index = 0U; index < profile->factory_dtc_count; ++index) {
        const JaglinkJaguarFactoryDtcDefinition *definition = &profile->factory_dtcs[index];
        if (definition->module == module && strcmp(definition->code, code) == 0) return definition;
    }
    return NULL;
}

const JaglinkJaguarVehicleProfile *jaglink_jaguar_x400_profile(void)
{
    return &x400_profile;
}

size_t jaglink_jaguar_x400_fuel_signal_count(void)
{
    return sizeof(x400_fuel_signals) / sizeof(x400_fuel_signals[0]);
}

const JaglinkJaguarFuelSignalDefinition *jaglink_jaguar_x400_fuel_signal_at(size_t index)
{
    return index < jaglink_jaguar_x400_fuel_signal_count() ? &x400_fuel_signals[index] : NULL;
}

const JaglinkJaguarFuelSignalDefinition *jaglink_jaguar_x400_find_fuel_signal(const char *key)
{
    size_t index;
    if (key == NULL || key[0] == '\0') return NULL;
    for (index = 0U; index < jaglink_jaguar_x400_fuel_signal_count(); ++index)
        if (strcmp(x400_fuel_signals[index].key, key) == 0) return &x400_fuel_signals[index];
    return NULL;
}
