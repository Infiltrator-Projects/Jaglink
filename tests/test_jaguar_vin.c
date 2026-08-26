// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaguar_vin.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed: %s at %s:%d\n", #expr, __FILE__, __LINE__); \
    return 1; } } while (0)

static int test_jaguar_heritage_x400_estate(void)
{
    JaglinkJaguarVinDecode decoded;

    CHECK(jaglink_jaguar_vin_decode("SAJAD56L64WD78435", &decoded));
    CHECK(decoded.valid);
    CHECK(decoded.jaguar_wmi);
    CHECK(decoded.x400);
    CHECK(strcmp(decoded.wmi, "SAJ") == 0);
    CHECK(decoded.market != NULL);
    CHECK(strcmp(decoded.market->market, "Rest of World") == 0);
    CHECK(decoded.transmission_steering != NULL);
    CHECK(decoded.transmission_steering->drivetrain == JAGLINK_JAGUAR_DRIVETRAIN_AWD);
    CHECK(decoded.transmission_steering->transmission == JAGLINK_JAGUAR_TRANSMISSION_MANUAL);
    CHECK(decoded.transmission_steering->steering == JAGLINK_JAGUAR_STEERING_RHD);
    CHECK(decoded.body != NULL);
    CHECK(decoded.body->body_style == JAGLINK_JAGUAR_BODY_ESTATE);
    CHECK(strcmp(decoded.body->series_class, "Sport Series") == 0);
    CHECK(decoded.model_year == 2004U);
    CHECK(decoded.plant_engine != NULL);
    CHECK(strcmp(decoded.plant_engine->assembly_plant, "Halewood") == 0);
    CHECK(strcmp(decoded.plant_engine->assembly_country, "United Kingdom") == 0);
    CHECK(strcmp(decoded.plant_engine->engine_description, "3.0 V6 petrol") == 0);
    CHECK(decoded.plant_engine->fuel == JAGLINK_JAGUAR_FUEL_PETROL);
    CHECK(decoded.plant_engine->displacement_cc == 2967U);
    CHECK(strcmp(decoded.production_serial, "D78435") == 0);
    {
        char summary[384];
        CHECK(jaglink_jaguar_vin_format_summary(
                  "SAJAD56L64WD78435", summary, sizeof(summary)));
        CHECK(strstr(summary, "Jaguar X-TYPE X400") != NULL);
        CHECK(strstr(summary, "2004") != NULL);
        CHECK(strstr(summary, "estate Sport Series") != NULL);
        CHECK(strstr(summary, "all-wheel drive") != NULL);
        CHECK(strstr(summary, "manual") != NULL);
        CHECK(strstr(summary, "right-hand drive") != NULL);
        CHECK(strstr(summary, "3.0 V6 petrol") != NULL);
        CHECK(strstr(summary, "Halewood, United Kingdom") != NULL);
        CHECK(strstr(summary, "serial D78435") != NULL);
    }
    return 0;
}

static int test_x400_25_saloon(void)
{
    JaglinkJaguarVinDecode decoded;

    CHECK(jaglink_jaguar_vin_decode("SAJAB51M31XC14670", &decoded));
    CHECK(decoded.x400);
    CHECK(decoded.transmission_steering != NULL);
    CHECK(decoded.transmission_steering->drivetrain == JAGLINK_JAGUAR_DRIVETRAIN_AWD);
    CHECK(decoded.transmission_steering->transmission == JAGLINK_JAGUAR_TRANSMISSION_MANUAL);
    CHECK(decoded.transmission_steering->steering == JAGLINK_JAGUAR_STEERING_LHD);
    CHECK(decoded.body != NULL);
    CHECK(decoded.body->body_style == JAGLINK_JAGUAR_BODY_SALOON);
    CHECK(strcmp(decoded.body->series_class, "High Series") == 0);
    CHECK(decoded.model_year == 2001U);
    CHECK(decoded.plant_engine != NULL);
    CHECK(strcmp(decoded.plant_engine->engine_description, "2.5 V6 petrol") == 0);
    CHECK(decoded.plant_engine->displacement_cc == 2497U);
    CHECK(strcmp(decoded.production_serial, "C14670") == 0);
    return 0;
}

static int test_x400_20_fwd_auto(void)
{
    JaglinkJaguarVinDecode decoded;

    CHECK(jaglink_jaguar_vin_decode("SAJAE52N08YJ32279", &decoded));
    CHECK(decoded.x400);
    CHECK(decoded.transmission_steering != NULL);
    CHECK(decoded.transmission_steering->drivetrain == JAGLINK_JAGUAR_DRIVETRAIN_FWD);
    CHECK(decoded.transmission_steering->transmission == JAGLINK_JAGUAR_TRANSMISSION_AUTOMATIC);
    CHECK(decoded.transmission_steering->steering == JAGLINK_JAGUAR_STEERING_LHD);
    CHECK(decoded.body != NULL);
    CHECK(strcmp(decoded.body->series_class, "Entry Series") == 0);
    CHECK(decoded.model_year == 2008U);
    CHECK(decoded.plant_engine != NULL);
    CHECK(strcmp(decoded.plant_engine->engine_description, "2.0 V6 petrol") == 0);
    CHECK(decoded.plant_engine->displacement_cc == 2099U);
    return 0;
}

static int test_x400_diesel_shape(void)
{
    JaglinkJaguarVinDecode decoded;

    CHECK(jaglink_jaguar_vin_decode("SAJAH54R07BJ12345", &decoded));
    CHECK(decoded.x400);
    CHECK(decoded.transmission_steering != NULL);
    CHECK(decoded.transmission_steering->drivetrain == JAGLINK_JAGUAR_DRIVETRAIN_FWD);
    CHECK(decoded.transmission_steering->transmission == JAGLINK_JAGUAR_TRANSMISSION_MANUAL);
    CHECK(decoded.transmission_steering->steering == JAGLINK_JAGUAR_STEERING_RHD);
    CHECK(decoded.body != NULL);
    CHECK(decoded.body->body_style == JAGLINK_JAGUAR_BODY_ESTATE);
    CHECK(decoded.plant_engine != NULL);
    CHECK(decoded.plant_engine->fuel == JAGLINK_JAGUAR_FUEL_DIESEL);
    CHECK(decoded.plant_engine->displacement_cc == 2198U);
    CHECK(decoded.plant_engine->rated_power_kw == 114U);
    return 0;
}

static int test_unknown_codes_are_preserved_not_guessed(void)
{
    JaglinkJaguarVinDecode decoded;

    CHECK(jaglink_jaguar_vin_decode("SAJAJ50Z00CJ12345", &decoded));
    CHECK(decoded.valid);
    CHECK(decoded.jaguar_wmi);
    CHECK(!decoded.x400);
    CHECK(decoded.transmission_steering == NULL);
    CHECK(decoded.body == NULL);
    CHECK(decoded.model_year == 0U);
    CHECK(decoded.plant_engine == NULL);
    CHECK(strcmp(decoded.production_serial, "J12345") == 0);
    return 0;
}

static int test_invalid_vin_rejected(void)
{
    JaglinkJaguarVinDecode decoded;

    CHECK(!jaglink_jaguar_vin_decode(NULL, &decoded));
    CHECK(!jaglink_jaguar_vin_decode("SAJAD56L64WD7843", &decoded));
    CHECK(!jaglink_jaguar_vin_decode("SAJAD56L64WD7843I", &decoded));
    CHECK(!jaglink_jaguar_vin_decode("WDD2073472F126610", &decoded));
    CHECK(!jaglink_jaguar_vin_decode("SAJAD56L64WD78435", NULL));
    return 0;
}

int main(void)
{
    if (test_jaguar_heritage_x400_estate() != 0) return 1;
    if (test_x400_25_saloon() != 0) return 1;
    if (test_x400_20_fwd_auto() != 0) return 1;
    if (test_x400_diesel_shape() != 0) return 1;
    if (test_unknown_codes_are_preserved_not_guessed() != 0) return 1;
    if (test_invalid_vin_rejected() != 0) return 1;
    puts("Jaguar X400 VIN decoder tests passed");
    return 0;
}
