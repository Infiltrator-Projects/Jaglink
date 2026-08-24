// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaguar.h"

#include <string.h>

static const char x400_network_provenance[] =
    "Jaguar Introduction to X-TYPE Service Training (2002) and Jaguar X-TYPE 2002 Electrical Guide";

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

static const JaglinkJaguarFuelSignalDefinition x400_fuel_signals[] = {
    {
        "x400-can-fuel-used",
        "CAN FUEL USED",
        "x400-powertrain-can",
        0x44dU,
        JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED,
        false,
        x400_fuel_used_provenance
    }
};

static const JaglinkJaguarVehicleProfile x400_profile = {
    .platform_code = "X400",
    .platform_family = "Jaguar X400 / Ford CD132",
    .display_name = "Jaguar X-Type (X400), 2001-2009",
    .first_model_year = 2001U,
    .last_model_year = 2009U,
    .networks = x400_networks,
    .network_count = sizeof(x400_networks) / sizeof(x400_networks[0])
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

bool jaglink_jaguar_network_definition_is_valid(const JaglinkJaguarNetworkDefinition *definition)
{
    if (definition == NULL || definition->key == NULL || definition->key[0] == '\0' ||
        definition->name == NULL || definition->name[0] == '\0' ||
        definition->nominal_baud == 0U || definition->provenance == NULL ||
        definition->provenance[0] == '\0') {
        return false;
    }
    if (definition->kind < JAGLINK_JAGUAR_NETWORK_CAN || definition->kind > JAGLINK_JAGUAR_NETWORK_D2B) return false;
    if (definition->role < JAGLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN || definition->role > JAGLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT) return false;
    if (definition->status < JAGLINK_JAGUAR_DEFINITION_CANDIDATE || definition->status > JAGLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED) return false;
    return true;
}

bool jaglink_jaguar_fuel_signal_definition_is_valid(
    const JaglinkJaguarFuelSignalDefinition *definition)
{
    if (definition == NULL || definition->key == NULL || definition->key[0] == '\0' ||
        definition->name == NULL || definition->name[0] == '\0' ||
        definition->network_key == NULL || definition->network_key[0] == '\0' ||
        definition->provenance == NULL || definition->provenance[0] == '\0' ||
        definition->message_id > 0x7ffU) {
        return false;
    }
    if (definition->status < JAGLINK_JAGUAR_DEFINITION_CANDIDATE ||
        definition->status > JAGLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED) {
        return false;
    }
    if (definition->decoder_verified &&
        definition->status != JAGLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED) {
        return false;
    }
    return true;
}

bool jaglink_jaguar_vehicle_profile_is_valid(const JaglinkJaguarVehicleProfile *profile)
{
    size_t index;
    if (profile == NULL || profile->platform_code == NULL || profile->platform_code[0] == '\0' ||
        profile->platform_family == NULL || profile->platform_family[0] == '\0' ||
        profile->display_name == NULL || profile->display_name[0] == '\0' ||
        profile->first_model_year == 0U || profile->last_model_year < profile->first_model_year ||
        profile->networks == NULL || profile->network_count == 0U) return false;
    for (index = 0U; index < profile->network_count; ++index) {
        size_t earlier;
        if (!jaglink_jaguar_network_definition_is_valid(&profile->networks[index])) return false;
        for (earlier = 0U; earlier < index; ++earlier) {
            if (strcmp(profile->networks[index].key, profile->networks[earlier].key) == 0) return false;
        }
    }
    return true;
}

const JaglinkJaguarNetworkDefinition *jaglink_jaguar_profile_find_network(const JaglinkJaguarVehicleProfile *profile, const char *key)
{
    size_t index;
    if (!jaglink_jaguar_vehicle_profile_is_valid(profile) || key == NULL || key[0] == '\0') return NULL;
    for (index = 0U; index < profile->network_count; ++index) {
        if (strcmp(profile->networks[index].key, key) == 0) return &profile->networks[index];
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

const JaglinkJaguarFuelSignalDefinition *jaglink_jaguar_x400_fuel_signal_at(
    size_t index)
{
    return index < jaglink_jaguar_x400_fuel_signal_count()
        ? &x400_fuel_signals[index] : NULL;
}

const JaglinkJaguarFuelSignalDefinition *jaglink_jaguar_x400_find_fuel_signal(
    const char *key)
{
    size_t index;
    if (key == NULL || key[0] == '\0') return NULL;
    for (index = 0U; index < jaglink_jaguar_x400_fuel_signal_count(); ++index) {
        if (strcmp(x400_fuel_signals[index].key, key) == 0)
            return &x400_fuel_signals[index];
    }
    return NULL;
}
