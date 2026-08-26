// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaguar_vin.h"

#include <ctype.h>
#include <string.h>

#define ARRAY_LENGTH(values) (sizeof(values) / sizeof((values)[0]))

static const JaglinkJaguarMarketDefinition x400_markets[] = {
    {'A', "Rest of World", "Jaguar X400 global airbag specification A"},
    {'E', "United States", "Jaguar X400 US airbag specification 1"},
    {'G', "Canada", "Jaguar X400 Canada airbag specification 1"},
    {'K', "Japan", "Jaguar X400 Japan airbag specification"},
    {'T', "Mexico", "Jaguar X400 Mexico airbag specification 1"},
    {'W', "United States", "Jaguar X400 US airbag specification 2"},
    {'X', "Canada", "Jaguar X400 Canada airbag specification 2"},
    {'Y', "Mexico", "Jaguar X400 Mexico airbag specification 2"}
};

static const JaglinkJaguarTransmissionSteeringDefinition x400_transmission_steering[] = {
    {'A', JAGLINK_JAGUAR_DRIVETRAIN_AWD, JAGLINK_JAGUAR_TRANSMISSION_AUTOMATIC, JAGLINK_JAGUAR_STEERING_LHD},
    {'B', JAGLINK_JAGUAR_DRIVETRAIN_AWD, JAGLINK_JAGUAR_TRANSMISSION_MANUAL,    JAGLINK_JAGUAR_STEERING_LHD},
    {'C', JAGLINK_JAGUAR_DRIVETRAIN_AWD, JAGLINK_JAGUAR_TRANSMISSION_AUTOMATIC, JAGLINK_JAGUAR_STEERING_RHD},
    {'D', JAGLINK_JAGUAR_DRIVETRAIN_AWD, JAGLINK_JAGUAR_TRANSMISSION_MANUAL,    JAGLINK_JAGUAR_STEERING_RHD},
    {'E', JAGLINK_JAGUAR_DRIVETRAIN_FWD, JAGLINK_JAGUAR_TRANSMISSION_AUTOMATIC, JAGLINK_JAGUAR_STEERING_LHD},
    {'F', JAGLINK_JAGUAR_DRIVETRAIN_FWD, JAGLINK_JAGUAR_TRANSMISSION_MANUAL,    JAGLINK_JAGUAR_STEERING_LHD},
    {'G', JAGLINK_JAGUAR_DRIVETRAIN_FWD, JAGLINK_JAGUAR_TRANSMISSION_AUTOMATIC, JAGLINK_JAGUAR_STEERING_RHD},
    {'H', JAGLINK_JAGUAR_DRIVETRAIN_FWD, JAGLINK_JAGUAR_TRANSMISSION_MANUAL,    JAGLINK_JAGUAR_STEERING_RHD}
};

static const JaglinkJaguarBodyDefinition x400_bodies[] = {
    {"51", JAGLINK_JAGUAR_BODY_SALOON, "High Series",  "X-TYPE 4-door High Series saloon"},
    {"52", JAGLINK_JAGUAR_BODY_SALOON, "Entry Series", "X-TYPE 4-door Entry Series saloon"},
    {"53", JAGLINK_JAGUAR_BODY_SALOON, "Sport Series", "X-TYPE 4-door Sport Series saloon"},
    {"54", JAGLINK_JAGUAR_BODY_ESTATE, "High Series",  "X-TYPE 4-door High Series estate"},
    {"55", JAGLINK_JAGUAR_BODY_ESTATE, "Entry Series", "X-TYPE 4-door Entry Series estate"},
    {"56", JAGLINK_JAGUAR_BODY_ESTATE, "Sport Series", "X-TYPE 4-door Sport Series estate"}
};

/* Jaguar Cars Ltd 2005 X400 global VIN table. */
static const JaglinkJaguarEmissionDefinition x400_emissions[] = {
    {'B',2U},{'C',3U},{'D',4U},{'E',5U},{'F',6U},{'G',7U},
    {'K',10U},{'N',13U},{'P',14U},{'R',15U},{'S',16U},{'T',17U},
    {'U',18U},{'V',19U},{'W',20U},{'X',21U},{'Y',22U},
    {'1',23U},{'2',24U},{'3',25U},{'4',26U}
};

/*
 * Position 11 is explicitly both assembly-plant and engine-line information
 * for X400. Engine capacities/power use Jaguar's published X-TYPE engine data.
 */
static const JaglinkJaguarPlantEngineDefinition x400_plant_engines[] = {
    {'W', "Halewood", "United Kingdom", "3.0 V6 petrol", "AJ-V6 3.0",
     JAGLINK_JAGUAR_FUEL_PETROL, 2967U, 169U},
    {'X', "Halewood", "United Kingdom", "2.5 V6 petrol", "AJ-V6 2.5",
     JAGLINK_JAGUAR_FUEL_PETROL, 2497U, 144U},
    {'Y', "Halewood", "United Kingdom", "2.0 V6 petrol", "AJ-V6 2.0",
     JAGLINK_JAGUAR_FUEL_PETROL, 2099U, 115U},
    {'6', "Halewood", "United Kingdom", "2.0 inline-4 diesel", "2.0 diesel",
     JAGLINK_JAGUAR_FUEL_DIESEL, 1998U, 96U},
    {'B', "Halewood", "United Kingdom", "2.2 TDCi inline-4 diesel", "2.2 TDCi",
     JAGLINK_JAGUAR_FUEL_DIESEL, 2198U, 114U}
};

static bool vin_char_valid(char value)
{
    unsigned char c = (unsigned char)toupper((unsigned char)value);
    if (!isalnum(c)) return false;
    return c != (unsigned char)'I' &&
           c != (unsigned char)'O' &&
           c != (unsigned char)'Q';
}

static char upper_char(char value)
{
    return (char)toupper((unsigned char)value);
}

const char *jaglink_jaguar_drivetrain_name(JaglinkJaguarDrivetrain value)
{
    switch (value) {
    case JAGLINK_JAGUAR_DRIVETRAIN_UNKNOWN: return "unknown";
    case JAGLINK_JAGUAR_DRIVETRAIN_FWD: return "front-wheel drive";
    case JAGLINK_JAGUAR_DRIVETRAIN_AWD: return "all-wheel drive";
    }
    return "unknown";
}

const char *jaglink_jaguar_transmission_name(JaglinkJaguarTransmission value)
{
    switch (value) {
    case JAGLINK_JAGUAR_TRANSMISSION_UNKNOWN: return "unknown";
    case JAGLINK_JAGUAR_TRANSMISSION_MANUAL: return "manual";
    case JAGLINK_JAGUAR_TRANSMISSION_AUTOMATIC: return "automatic";
    }
    return "unknown";
}

const char *jaglink_jaguar_steering_name(JaglinkJaguarSteering value)
{
    switch (value) {
    case JAGLINK_JAGUAR_STEERING_UNKNOWN: return "unknown";
    case JAGLINK_JAGUAR_STEERING_LHD: return "left-hand drive";
    case JAGLINK_JAGUAR_STEERING_RHD: return "right-hand drive";
    }
    return "unknown";
}

const char *jaglink_jaguar_body_style_name(JaglinkJaguarBodyStyle value)
{
    switch (value) {
    case JAGLINK_JAGUAR_BODY_UNKNOWN: return "unknown";
    case JAGLINK_JAGUAR_BODY_SALOON: return "saloon";
    case JAGLINK_JAGUAR_BODY_ESTATE: return "estate";
    }
    return "unknown";
}

const char *jaglink_jaguar_fuel_type_name(JaglinkJaguarFuelType value)
{
    switch (value) {
    case JAGLINK_JAGUAR_FUEL_UNKNOWN: return "unknown";
    case JAGLINK_JAGUAR_FUEL_PETROL: return "petrol";
    case JAGLINK_JAGUAR_FUEL_DIESEL: return "diesel";
    }
    return "unknown";
}

const JaglinkJaguarMarketDefinition *jaglink_jaguar_x400_market(char code)
{
    size_t index;
    code = upper_char(code);
    for (index = 0U; index < ARRAY_LENGTH(x400_markets); ++index)
        if (x400_markets[index].code == code) return &x400_markets[index];
    return NULL;
}

const JaglinkJaguarTransmissionSteeringDefinition *
jaglink_jaguar_x400_transmission_steering(char code)
{
    size_t index;
    code = upper_char(code);
    for (index = 0U; index < ARRAY_LENGTH(x400_transmission_steering); ++index)
        if (x400_transmission_steering[index].code == code)
            return &x400_transmission_steering[index];
    return NULL;
}

const JaglinkJaguarBodyDefinition *jaglink_jaguar_x400_body(const char code[3])
{
    size_t index;
    if (code == NULL || code[0] == '\0' || code[1] == '\0' || code[2] != '\0')
        return NULL;
    for (index = 0U; index < ARRAY_LENGTH(x400_bodies); ++index)
        if (strncmp(code, x400_bodies[index].code, 2U) == 0)
            return &x400_bodies[index];
    return NULL;
}

const JaglinkJaguarEmissionDefinition *jaglink_jaguar_x400_emission(char code)
{
    size_t index;
    code = upper_char(code);
    for (index = 0U; index < ARRAY_LENGTH(x400_emissions); ++index)
        if (x400_emissions[index].code == code) return &x400_emissions[index];
    return NULL;
}

const JaglinkJaguarPlantEngineDefinition *
jaglink_jaguar_x400_plant_engine(char code)
{
    size_t index;
    code = upper_char(code);
    for (index = 0U; index < ARRAY_LENGTH(x400_plant_engines); ++index)
        if (x400_plant_engines[index].code == code) return &x400_plant_engines[index];
    return NULL;
}

static uint16_t x400_model_year(char code)
{
    if (code >= '1' && code <= '9')
        return (uint16_t)(2000U + (unsigned int)(code - '0'));
    return 0U;
}

bool jaglink_jaguar_vin_decode(
    const char *vin,
    JaglinkJaguarVinDecode *decoded)
{
    size_t index;

    if (decoded == NULL) return false;
    memset(decoded, 0, sizeof(*decoded));
    if (vin == NULL || strlen(vin) != JAGLINK_JAGUAR_VIN_LENGTH) return false;

    for (index = 0U; index < JAGLINK_JAGUAR_VIN_LENGTH; ++index)
        if (!vin_char_valid(vin[index])) return false;

    for (index = 0U; index < JAGLINK_JAGUAR_VIN_LENGTH; ++index)
        decoded->vin[index] = upper_char(vin[index]);
    decoded->vin[JAGLINK_JAGUAR_VIN_LENGTH] = '\0';

    memcpy(decoded->wmi, decoded->vin, 3U);
    decoded->wmi[3] = '\0';
    decoded->jaguar_wmi = strcmp(decoded->wmi, "SAJ") == 0;
    if (!decoded->jaguar_wmi) return false;

    decoded->market_code = decoded->vin[3];
    decoded->market = jaglink_jaguar_x400_market(decoded->market_code);

    decoded->transmission_steering_code = decoded->vin[4];
    decoded->transmission_steering =
        jaglink_jaguar_x400_transmission_steering(
            decoded->transmission_steering_code);

    decoded->body_code[0] = decoded->vin[5];
    decoded->body_code[1] = decoded->vin[6];
    decoded->body_code[2] = '\0';
    decoded->body = jaglink_jaguar_x400_body(decoded->body_code);

    decoded->emission_code = decoded->vin[7];
    decoded->emission = jaglink_jaguar_x400_emission(decoded->emission_code);

    decoded->check_digit = decoded->vin[8];
    decoded->model_year_code = decoded->vin[9];
    decoded->model_year = x400_model_year(decoded->model_year_code);

    decoded->plant_engine_code = decoded->vin[10];
    decoded->plant_engine =
        jaglink_jaguar_x400_plant_engine(decoded->plant_engine_code);

    memcpy(decoded->production_serial, decoded->vin + 11U, 6U);
    decoded->production_serial[6] = '\0';

    decoded->x400 =
        decoded->market != NULL &&
        decoded->transmission_steering != NULL &&
        decoded->body != NULL &&
        decoded->model_year >= 2001U &&
        decoded->model_year <= 2009U &&
        decoded->plant_engine != NULL;

    decoded->valid = true;
    return true;
}
