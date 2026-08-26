// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file jaguar.h
 * @brief Jaguar manufacturer profile, factory diagnostic and provenance contracts.
 */
#ifndef JAGLINK_JAGUAR_H
#define JAGLINK_JAGUAR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "jaglink/jaguar_vin.h"

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

typedef enum {
    JAGLINK_JAGUAR_MODULE_ECM = 0,
    JAGLINK_JAGUAR_MODULE_TCM,
    JAGLINK_JAGUAR_MODULE_INSTRUMENT_CLUSTER,
    JAGLINK_JAGUAR_MODULE_ABS_DSC,
    JAGLINK_JAGUAR_MODULE_CLIMATE,
    JAGLINK_JAGUAR_MODULE_GECM,
    JAGLINK_JAGUAR_MODULE_RESTRAINTS,
    JAGLINK_JAGUAR_MODULE_AUDIO,
    JAGLINK_JAGUAR_MODULE_OTHER
} JaglinkJaguarModuleKind;

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
 * Source-corroborated factory diagnostic CAN path.
 *
 * The request/response identifiers describe Jaguar's documented diagnostic
 * message routing only. They do not authorize any particular payload or imply
 * that a modern UDS/ISO-TP request format is valid for the X400 module.
 */
typedef struct {
    const char *key;
    const char *name;
    JaglinkJaguarModuleKind module;
    const char *network_key;
    uint32_t request_message_id;
    uint32_t response_message_id;
    JaglinkJaguarDefinitionStatus status;
    const char *provenance;
} JaglinkJaguarDiagnosticEndpointDefinition;

/**
 * Identity for one documented Jaguar factory DTC.
 *
 * These records intentionally keep only compact identity/category metadata.
 * The raw code remains authoritative and full workshop-manual prose is not
 * copied into the program. `generic_obd2_accessible` is false for definitions
 * that belong to Jaguar's factory/module diagnostic view rather than the
 * legislated generic OBD-II retrieval path.
 */
typedef struct {
    const char *code;
    const char *stable_key;
    JaglinkJaguarModuleKind module;
    const char *category;
    bool generic_obd2_accessible;
    JaglinkJaguarDefinitionStatus status;
    const char *provenance;
} JaglinkJaguarFactoryDtcDefinition;

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
    const JaglinkJaguarDiagnosticEndpointDefinition *diagnostic_endpoints;
    size_t diagnostic_endpoint_count;
    const JaglinkJaguarFactoryDtcDefinition *factory_dtcs;
    size_t factory_dtc_count;
} JaglinkJaguarVehicleProfile;

const char *jaglink_jaguar_definition_status_name(JaglinkJaguarDefinitionStatus status);
const char *jaglink_jaguar_network_kind_name(JaglinkJaguarNetworkKind kind);
const char *jaglink_jaguar_network_role_name(JaglinkJaguarNetworkRole role);
const char *jaglink_jaguar_module_kind_name(JaglinkJaguarModuleKind module);
bool jaglink_jaguar_network_definition_is_valid(const JaglinkJaguarNetworkDefinition *definition);
bool jaglink_jaguar_diagnostic_endpoint_definition_is_valid(
    const JaglinkJaguarDiagnosticEndpointDefinition *definition);
bool jaglink_jaguar_factory_dtc_definition_is_valid(
    const JaglinkJaguarFactoryDtcDefinition *definition);
bool jaglink_jaguar_fuel_signal_definition_is_valid(
    const JaglinkJaguarFuelSignalDefinition *definition);
bool jaglink_jaguar_vehicle_profile_is_valid(const JaglinkJaguarVehicleProfile *profile);
const JaglinkJaguarNetworkDefinition *jaglink_jaguar_profile_find_network(
    const JaglinkJaguarVehicleProfile *profile, const char *key);
const JaglinkJaguarDiagnosticEndpointDefinition *
jaglink_jaguar_profile_find_diagnostic_endpoint(
    const JaglinkJaguarVehicleProfile *profile, JaglinkJaguarModuleKind module);
const JaglinkJaguarFactoryDtcDefinition *jaglink_jaguar_profile_find_factory_dtc(
    const JaglinkJaguarVehicleProfile *profile,
    JaglinkJaguarModuleKind module,
    const char *code);
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
