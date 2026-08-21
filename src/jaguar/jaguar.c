// SPDX-License-Identifier: GPL-3.0-or-later
#include "mblink/jaguar.h"

#include <string.h>

static const char x400_network_provenance[] =
    "Jaguar Introduction to X-TYPE Service Training (2002) and Jaguar X-TYPE 2002 Electrical Guide";

static const MblinkJaguarNetworkDefinition x400_networks[] = {
    { "x400-powertrain-can", "Powertrain CAN", MBLINK_JAGUAR_NETWORK_CAN,
      MBLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN, 500000U,
      MBLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_network_provenance },
    { "x400-body-scp", "Body SCP", MBLINK_JAGUAR_NETWORK_SCP,
      MBLINK_JAGUAR_NETWORK_ROLE_BODY, 41600U,
      MBLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_network_provenance },
    { "x400-serial-iso9141", "Serial Data Link (ISO 9141)", MBLINK_JAGUAR_NETWORK_ISO9141,
      MBLINK_JAGUAR_NETWORK_ROLE_SERIAL_DIAGNOSTIC, 10400U,
      MBLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_network_provenance },
    { "x400-audio-d2b", "D2B Optical", MBLINK_JAGUAR_NETWORK_D2B,
      MBLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT, 5600000U,
      MBLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED, x400_network_provenance }
};

static const MblinkJaguarVehicleProfile x400_profile = {
    .platform_code = "X400",
    .platform_family = "Jaguar X400 / Ford CD132",
    .display_name = "Jaguar X-Type (X400), 2001-2009",
    .first_model_year = 2001U,
    .last_model_year = 2009U,
    .networks = x400_networks,
    .network_count = sizeof(x400_networks) / sizeof(x400_networks[0])
};

const char *mblink_jaguar_definition_status_name(MblinkJaguarDefinitionStatus status)
{
    switch (status) {
    case MBLINK_JAGUAR_DEFINITION_CANDIDATE: return "candidate";
    case MBLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED: return "source-corroborated";
    case MBLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED: return "vehicle-verified";
    }
    return "unknown";
}

const char *mblink_jaguar_network_kind_name(MblinkJaguarNetworkKind kind)
{
    switch (kind) {
    case MBLINK_JAGUAR_NETWORK_CAN: return "can";
    case MBLINK_JAGUAR_NETWORK_SCP: return "scp";
    case MBLINK_JAGUAR_NETWORK_ISO9141: return "iso9141";
    case MBLINK_JAGUAR_NETWORK_D2B: return "d2b";
    }
    return "unknown";
}

const char *mblink_jaguar_network_role_name(MblinkJaguarNetworkRole role)
{
    switch (role) {
    case MBLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN: return "powertrain";
    case MBLINK_JAGUAR_NETWORK_ROLE_BODY: return "body";
    case MBLINK_JAGUAR_NETWORK_ROLE_SERIAL_DIAGNOSTIC: return "serial-diagnostic";
    case MBLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT: return "infotainment";
    }
    return "unknown";
}

bool mblink_jaguar_network_definition_is_valid(const MblinkJaguarNetworkDefinition *definition)
{
    if (definition == NULL || definition->key == NULL || definition->key[0] == '\0' ||
        definition->name == NULL || definition->name[0] == '\0' ||
        definition->nominal_baud == 0U || definition->provenance == NULL ||
        definition->provenance[0] == '\0') {
        return false;
    }
    if (definition->kind < MBLINK_JAGUAR_NETWORK_CAN || definition->kind > MBLINK_JAGUAR_NETWORK_D2B) return false;
    if (definition->role < MBLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN || definition->role > MBLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT) return false;
    if (definition->status < MBLINK_JAGUAR_DEFINITION_CANDIDATE || definition->status > MBLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED) return false;
    return true;
}

bool mblink_jaguar_vehicle_profile_is_valid(const MblinkJaguarVehicleProfile *profile)
{
    size_t index;
    if (profile == NULL || profile->platform_code == NULL || profile->platform_code[0] == '\0' ||
        profile->platform_family == NULL || profile->platform_family[0] == '\0' ||
        profile->display_name == NULL || profile->display_name[0] == '\0' ||
        profile->first_model_year == 0U || profile->last_model_year < profile->first_model_year ||
        profile->networks == NULL || profile->network_count == 0U) return false;
    for (index = 0U; index < profile->network_count; ++index) {
        size_t earlier;
        if (!mblink_jaguar_network_definition_is_valid(&profile->networks[index])) return false;
        for (earlier = 0U; earlier < index; ++earlier) {
            if (strcmp(profile->networks[index].key, profile->networks[earlier].key) == 0) return false;
        }
    }
    return true;
}

const MblinkJaguarNetworkDefinition *mblink_jaguar_profile_find_network(const MblinkJaguarVehicleProfile *profile, const char *key)
{
    size_t index;
    if (!mblink_jaguar_vehicle_profile_is_valid(profile) || key == NULL || key[0] == '\0') return NULL;
    for (index = 0U; index < profile->network_count; ++index) {
        if (strcmp(profile->networks[index].key, key) == 0) return &profile->networks[index];
    }
    return NULL;
}

const MblinkJaguarVehicleProfile *mblink_jaguar_x400_profile(void)
{
    return &x400_profile;
}
