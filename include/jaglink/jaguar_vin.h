// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file jaguar_vin.h
 * @brief Offline Jaguar VIN decoding, beginning with X-TYPE/X400.
 *
 * Jaguar X400 VIN semantics differ from Mercedes FIN/Baumuster semantics.
 * JAGLINK owns this manufacturer-specific grammar; LINK only acquires the
 * standards-based VIN and transports normalized diagnostic events.
 */
#ifndef JAGLINK_JAGUAR_VIN_H
#define JAGLINK_JAGUAR_VIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_JAGUAR_VIN_LENGTH 17U

typedef enum {
    JAGLINK_JAGUAR_DRIVETRAIN_UNKNOWN = 0,
    JAGLINK_JAGUAR_DRIVETRAIN_FWD,
    JAGLINK_JAGUAR_DRIVETRAIN_AWD
} JaglinkJaguarDrivetrain;

typedef enum {
    JAGLINK_JAGUAR_TRANSMISSION_UNKNOWN = 0,
    JAGLINK_JAGUAR_TRANSMISSION_MANUAL,
    JAGLINK_JAGUAR_TRANSMISSION_AUTOMATIC
} JaglinkJaguarTransmission;

typedef enum {
    JAGLINK_JAGUAR_STEERING_UNKNOWN = 0,
    JAGLINK_JAGUAR_STEERING_LHD,
    JAGLINK_JAGUAR_STEERING_RHD
} JaglinkJaguarSteering;

typedef enum {
    JAGLINK_JAGUAR_BODY_UNKNOWN = 0,
    JAGLINK_JAGUAR_BODY_SALOON,
    JAGLINK_JAGUAR_BODY_ESTATE
} JaglinkJaguarBodyStyle;

typedef enum {
    JAGLINK_JAGUAR_FUEL_UNKNOWN = 0,
    JAGLINK_JAGUAR_FUEL_PETROL,
    JAGLINK_JAGUAR_FUEL_DIESEL
} JaglinkJaguarFuelType;

typedef struct {
    char code;
    const char *market;
    const char *airbag_specification;
} JaglinkJaguarMarketDefinition;

typedef struct {
    char code;
    JaglinkJaguarDrivetrain drivetrain;
    JaglinkJaguarTransmission transmission;
    JaglinkJaguarSteering steering;
} JaglinkJaguarTransmissionSteeringDefinition;

typedef struct {
    const char *code;
    JaglinkJaguarBodyStyle body_style;
    const char *series_class;
    const char *description;
} JaglinkJaguarBodyDefinition;

typedef struct {
    char code;
    unsigned int ecs_number;
} JaglinkJaguarEmissionDefinition;

typedef struct {
    char code;
    const char *assembly_plant;
    const char *assembly_country;
    const char *engine_description;
    const char *engine_family;
    JaglinkJaguarFuelType fuel;
    unsigned int displacement_cc;
    unsigned int rated_power_kw;
} JaglinkJaguarPlantEngineDefinition;

typedef struct {
    bool valid;
    bool jaguar_wmi;
    bool x400;

    char vin[JAGLINK_JAGUAR_VIN_LENGTH + 1U];
    char wmi[4];

    char market_code;
    const JaglinkJaguarMarketDefinition *market;

    char transmission_steering_code;
    const JaglinkJaguarTransmissionSteeringDefinition *transmission_steering;

    char body_code[3];
    const JaglinkJaguarBodyDefinition *body;

    char emission_code;
    const JaglinkJaguarEmissionDefinition *emission;

    char check_digit;
    char model_year_code;
    uint16_t model_year;

    char plant_engine_code;
    const JaglinkJaguarPlantEngineDefinition *plant_engine;

    char production_serial[7];
} JaglinkJaguarVinDecode;

const char *jaglink_jaguar_drivetrain_name(JaglinkJaguarDrivetrain value);
const char *jaglink_jaguar_transmission_name(JaglinkJaguarTransmission value);
const char *jaglink_jaguar_steering_name(JaglinkJaguarSteering value);
const char *jaglink_jaguar_body_style_name(JaglinkJaguarBodyStyle value);
const char *jaglink_jaguar_fuel_type_name(JaglinkJaguarFuelType value);

const JaglinkJaguarMarketDefinition *jaglink_jaguar_x400_market(char code);
const JaglinkJaguarTransmissionSteeringDefinition *
jaglink_jaguar_x400_transmission_steering(char code);
const JaglinkJaguarBodyDefinition *jaglink_jaguar_x400_body(const char code[3]);
const JaglinkJaguarEmissionDefinition *jaglink_jaguar_x400_emission(char code);
const JaglinkJaguarPlantEngineDefinition *
jaglink_jaguar_x400_plant_engine(char code);

bool jaglink_jaguar_vin_decode(
    const char *vin,
    JaglinkJaguarVinDecode *decoded);

/**
 * Format a concise product-facing identity from a decoded Jaguar VIN.
 * Returns non-zero only when the VIN is a recognised X400 configuration.
 */
int jaglink_jaguar_vin_format_summary(
    const char *vin,
    char *buffer,
    size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
