// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file jaguar.h
 * @brief Jaguar manufacturer profile and provenance contracts.
 */
#ifndef MBLINK_JAGUAR_H
#define MBLINK_JAGUAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MBLINK_JAGUAR_DEFINITION_CANDIDATE = 0,
    MBLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED,
    MBLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED
} MblinkJaguarDefinitionStatus;

typedef enum {
    MBLINK_JAGUAR_NETWORK_CAN = 0,
    MBLINK_JAGUAR_NETWORK_SCP,
    MBLINK_JAGUAR_NETWORK_ISO9141,
    MBLINK_JAGUAR_NETWORK_D2B
} MblinkJaguarNetworkKind;

typedef enum {
    MBLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN = 0,
    MBLINK_JAGUAR_NETWORK_ROLE_BODY,
    MBLINK_JAGUAR_NETWORK_ROLE_SERIAL_DIAGNOSTIC,
    MBLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT
} MblinkJaguarNetworkRole;

typedef struct {
    const char *key;
    const char *name;
    MblinkJaguarNetworkKind kind;
    MblinkJaguarNetworkRole role;
    uint32_t nominal_baud;
    MblinkJaguarDefinitionStatus status;
    const char *provenance;
} MblinkJaguarNetworkDefinition;

typedef struct {
    const char *platform_code;
    const char *platform_family;
    const char *display_name;
    uint16_t first_model_year;
    uint16_t last_model_year;
    const MblinkJaguarNetworkDefinition *networks;
    size_t network_count;
} MblinkJaguarVehicleProfile;

const char *mblink_jaguar_definition_status_name(MblinkJaguarDefinitionStatus status);
const char *mblink_jaguar_network_kind_name(MblinkJaguarNetworkKind kind);
const char *mblink_jaguar_network_role_name(MblinkJaguarNetworkRole role);
bool mblink_jaguar_network_definition_is_valid(const MblinkJaguarNetworkDefinition *definition);
bool mblink_jaguar_vehicle_profile_is_valid(const MblinkJaguarVehicleProfile *profile);
const MblinkJaguarNetworkDefinition *mblink_jaguar_profile_find_network(const MblinkJaguarVehicleProfile *profile, const char *key);
const MblinkJaguarVehicleProfile *mblink_jaguar_x400_profile(void);

#ifdef __cplusplus
}
#endif

#endif
