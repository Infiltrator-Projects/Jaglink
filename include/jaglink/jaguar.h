// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file jaguar.h
 * @brief Jaguar manufacturer profile and provenance contracts.
 */
#ifndef JAGLINK_JAGUAR_H
#define JAGLINK_JAGUAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JAGLINK_JAGUAR_DEFINITION_CANDIDATE = 0,
    JAGLINK_JAGUAR_DEFINITION_SOURCE_CORROBORATED,
    JAGLINK_JAGUAR_DEFINITION_VEHICLE_VERIFIED
} JaglinkJaguarDefinitionStatus;

typedef enum {
    JAGLINK_JAGUAR_NETWORK_CAN = 0,
    JAGLINK_JAGUAR_NETWORK_SCP,
    JAGLINK_JAGUAR_NETWORK_ISO9141,
    JAGLINK_JAGUAR_NETWORK_D2B
} JaglinkJaguarNetworkKind;

typedef enum {
    JAGLINK_JAGUAR_NETWORK_ROLE_POWERTRAIN = 0,
    JAGLINK_JAGUAR_NETWORK_ROLE_BODY,
    JAGLINK_JAGUAR_NETWORK_ROLE_SERIAL_DIAGNOSTIC,
    JAGLINK_JAGUAR_NETWORK_ROLE_INFOTAINMENT
} JaglinkJaguarNetworkRole;

typedef struct {
    const char *key;
    const char *name;
    JaglinkJaguarNetworkKind kind;
    JaglinkJaguarNetworkRole role;
    uint32_t nominal_baud;
    JaglinkJaguarDefinitionStatus status;
    const char *provenance;
} JaglinkJaguarNetworkDefinition;

/**
 * One manufacturer-specific fuel/trip signal known to exist on an X400 network.
 *
 * `decoder_verified == false` is an explicit safety/evidence boundary: JAGLINK
 * may display the signal's documented existence and record raw evidence, but it
 * must not invent byte offsets, scaling or engineering units until those have
 * been corroborated and vehicle-verified.
 */
typedef struct {
    const char *key;
    const char *name;
    const char *network_key;
    uint32_t message_id;
    JaglinkJaguarDefinitionStatus status;
    bool decoder_verified;
    const char *provenance;
} JaglinkJaguarFuelSignalDefinition;

typedef struct {
    const char *platform_code;
    const char *platform_family;
    const char *display_name;
    uint16_t first_model_year;
    uint16_t last_model_year;
    const JaglinkJaguarNetworkDefinition *networks;
    size_t network_count;
} JaglinkJaguarVehicleProfile;

const char *jaglink_jaguar_definition_status_name(JaglinkJaguarDefinitionStatus status);
const char *jaglink_jaguar_network_kind_name(JaglinkJaguarNetworkKind kind);
const char *jaglink_jaguar_network_role_name(JaglinkJaguarNetworkRole role);
bool jaglink_jaguar_network_definition_is_valid(const JaglinkJaguarNetworkDefinition *definition);
bool jaglink_jaguar_fuel_signal_definition_is_valid(
    const JaglinkJaguarFuelSignalDefinition *definition);
bool jaglink_jaguar_vehicle_profile_is_valid(const JaglinkJaguarVehicleProfile *profile);
const JaglinkJaguarNetworkDefinition *jaglink_jaguar_profile_find_network(const JaglinkJaguarVehicleProfile *profile, const char *key);
const JaglinkJaguarVehicleProfile *jaglink_jaguar_x400_profile(void);

/** Number of documented X400 manufacturer fuel/trip signals. */
size_t jaglink_jaguar_x400_fuel_signal_count(void);

/** Returns a documented X400 manufacturer fuel/trip signal or NULL. */
const JaglinkJaguarFuelSignalDefinition *jaglink_jaguar_x400_fuel_signal_at(
    size_t index);

/** Finds one documented X400 manufacturer fuel/trip signal by stable key. */
const JaglinkJaguarFuelSignalDefinition *jaglink_jaguar_x400_find_fuel_signal(
    const char *key);

#ifdef __cplusplus
}
#endif

#endif
