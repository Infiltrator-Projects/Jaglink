// SPDX-License-Identifier: GPL-3.0-or-later
#import "JagLinkDiagnosticsController.h"

#import "../../src/link/platform/apple/LinkDiagnosticsController.h"
#import "jaglink/jaguar.h"
#import "jaglink/jaguar_vin.h"
#import "link/fuel_economy.h"
#import "link/units.h"

@interface JagLinkDiagnosticsController () <LinkDiagnosticsControllerDelegate>
@property(nonatomic, copy, readwrite, nullable) NSString *vehicleVINText;
@property(nonatomic, copy, readwrite) NSString *vehiclePlatformText;
@property(nonatomic, copy, readwrite) NSString *vehicleConfigurationText;
@property(nonatomic, copy, readwrite) NSString *vehiclePowertrainText;
@property(nonatomic, copy, readwrite) NSString *vehicleBuildText;
- (void)resetFuelEconomy;
- (LinkFuelEconomySnapshot)fuelEconomySnapshot;
- (LinkUnitPreferences)displayUnitPreferences;
@end

@implementation JagLinkDiagnosticsController {
    LinkFuelEconomy _fuelEconomy;
}

static NSString *JagLinkStringFromCString(const char *value)
{
    if (value == NULL) return @"unknown";
    NSString *string = [NSString stringWithUTF8String:value];
    return string != nil ? string : @"unknown";
}

static uint64_t JagLinkMonotonicMilliseconds(void)
{
    NSTimeInterval uptime = NSProcessInfo.processInfo.systemUptime;
    if (uptime <= 0.0) return 0U;
    const double milliseconds = uptime * 1000.0;
    return milliseconds >= (double)UINT64_MAX
        ? UINT64_MAX : (uint64_t)milliseconds;
}

static NSString *JagLinkFuelEconomySourceText(LinkFuelEconomySource source)
{
    const char *name = link_fuel_economy_source_name(source);
    if (name == NULL || name[0] == '\0' ||
        source == LINK_FUEL_ECONOMY_SOURCE_NONE) {
        return @"Unavailable";
    }
    return JagLinkStringFromCString(name);
}

static NSString *JagLinkDTCDisplayText(
    LinkDiagnosticsController *shared, NSString *code)
{
    if (code.length == 0U) return @"";
    NSString *normalized = code.uppercaseString;
    const char *jaguar_title =
        jaglink_jaguar_x400_obd_dtc_title(normalized.UTF8String);
    if (jaguar_title != NULL) {
        return [NSString stringWithFormat:@"%@ — %@", normalized,
            JagLinkStringFromCString(jaguar_title)];
    }
    return [shared dtcDisplayTextForCode:normalized];
}

static NSArray<NSString *> *JagLinkDTCDisplayRows(
    LinkDiagnosticsController *shared, NSArray<NSString *> *codes)
{
    if (codes.count == 0U) return @[];
    NSMutableArray<NSString *> *rows =
        [[NSMutableArray alloc] initWithCapacity:codes.count];
    for (NSString *code in codes) {
        NSString *row = JagLinkDTCDisplayText(shared, code);
        if (row.length != 0U) [rows addObject:row];
    }
    return [rows copy];
}

static NSString *JagLinkFormatFuelEconomy(
    double litres_per_100km,
    const LinkUnitPreferences *preferences)
{
    double value = 0.0;
    const char *unit = NULL;
    if (preferences == NULL ||
        !link_units_convert_fuel_economy(
            litres_per_100km, preferences->fuel_economy, &value, &unit) ||
        unit == NULL) {
        return @"Unavailable";
    }
    return [NSString stringWithFormat:@"%.1f %@", value,
        JagLinkStringFromCString(unit)];
}

static NSString *JagLinkFormatFuelRate(
    double litres_per_hour,
    const LinkUnitPreferences *preferences)
{
    double value = 0.0;
    const char *unit = NULL;
    if (preferences == NULL ||
        !link_units_convert_fuel_rate(
            litres_per_hour, preferences->fuel_rate, &value, &unit) ||
        unit == NULL) {
        return @"Unavailable";
    }
    return [NSString stringWithFormat:@"%.2f %@", value,
        JagLinkStringFromCString(unit)];
}

static NSString *JagLinkFormatTrip(
    double litres,
    double kilometres,
    const LinkUnitPreferences *preferences)
{
    double fuel_value = 0.0;
    double distance_value = 0.0;
    const char *fuel_unit = NULL;
    const char *distance_unit = NULL;
    if (preferences == NULL ||
        !link_units_convert_fuel_volume(
            litres, preferences->fuel_volume, &fuel_value, &fuel_unit) ||
        !link_units_convert_distance(
            kilometres, preferences->distance, &distance_value, &distance_unit) ||
        fuel_unit == NULL || distance_unit == NULL) {
        return @"Unavailable";
    }
    return [NSString stringWithFormat:@"%.2f %@ over %.1f %@",
        fuel_value,
        JagLinkStringFromCString(fuel_unit),
        distance_value,
        JagLinkStringFromCString(distance_unit)];
}

- (instancetype)init
{
    LinkDiagnosticFlowConfig flowConfig = LINK_DIAGNOSTIC_FLOW_CONFIG_INIT;
    /* Keep responder identities for profile caching and per-controller evidence. */
    flowConfig.preserve_pid_discovery_response_headers = true;
    flowConfig.preserve_live_response_headers = true;
    self = [super
        initWithProductSlug:@"jaglink"
        flowConfig:flowConfig
        liveStatusText:
            @"Live standard OBD-II data; X400 manufacturer discovery not yet enabled"
        simulatedLiveStatusText:
            @"Simulated ELM327 · live standard OBD-II data"
        standardVINStatusText:
            @"Reading standard VIN for Jaguar X400 identification"
        simulatedAdapterIdentifier:@"ELM327 v2.3 JAGLINK SIM"
        simulatedVIN:@"SAJAC51M31XC12345"];
    if (self == nil) return nil;
    link_fuel_economy_init(&_fuelEconomy);

    _vehiclePlatformText = @"Jaguar X400 identity pending";
    _vehicleConfigurationText = @"Waiting for standard VIN";
    _vehiclePowertrainText = @"Waiting for standard VIN";
    _vehicleBuildText = @"Waiting for standard VIN";
    return self;
}

- (void)resetVehicleIdentity
{
    self.vehicleVINText = nil;
    self.vehiclePlatformText = @"Jaguar X400 identity pending";
    self.vehicleConfigurationText = @"Waiting for standard VIN";
    self.vehiclePowertrainText = @"Waiting for standard VIN";
    self.vehicleBuildText = @"Waiting for standard VIN";
}

- (void)resetFuelEconomy
{
    const uint64_t now_ms = JagLinkMonotonicMilliseconds();
    link_fuel_economy_init(&_fuelEconomy);
    link_fuel_economy_reset_trip(&_fuelEconomy, now_ms);
}

- (LinkFuelEconomySnapshot)fuelEconomySnapshot
{
    const uint64_t now_ms = JagLinkMonotonicMilliseconds();
    link_fuel_economy_tick(&_fuelEconomy, now_ms);
    return link_fuel_economy_snapshot(&_fuelEconomy, now_ms);
}

- (LinkUnitPreferences)displayUnitPreferences
{
    LinkMeasurementSystem system = LINK_MEASUREMENT_SYSTEM_METRIC;
    LinkUnitPreferences preferences;
    const char *key = _shared.selectedMeasurementSystemKey.UTF8String;
    if (key == NULL || !link_measurement_system_from_key(key, &system)) {
        system = LINK_MEASUREMENT_SYSTEM_METRIC;
    }
    if (!link_unit_preferences_from_measurement_system(system, &preferences)) {
        link_unit_preferences_metric(&preferences);
    }
    return preferences;
}

- (void)notifyDelegate
{
    id<JagLinkDiagnosticsControllerDelegate> delegate = self.delegate;
    if (delegate != nil)
        [delegate diagnosticsControllerDidUpdate:self];
}

- (NSArray<NSString *> *)storedDTCDisplayRows
{
    return JagLinkDTCDisplayRows(_shared, _shared.storedDTCs);
}
- (NSArray<NSString *> *)pendingDTCDisplayRows
{
    return JagLinkDTCDisplayRows(_shared, _shared.pendingDTCs);
}
- (NSArray<NSString *> *)permanentDTCDisplayRows
{
    return JagLinkDTCDisplayRows(_shared, _shared.permanentDTCs);
}
- (void)start
{
    [self resetVehicleIdentity];
    [self resetFuelEconomy];
    [_shared start];
}

- (void)startWithPeripheralIdentifier:(NSString *)peripheralIdentifier
{
    [self resetVehicleIdentity];
    [self resetFuelEconomy];
    [_shared startWithPeripheralIdentifier:peripheralIdentifier];
}

- (void)startSimulated
{
    [self resetVehicleIdentity];
    [self resetFuelEconomy];
    [_shared startSimulatedWithAdapterIdentifier:"ELM327 v2.3 JAGLINK SIM"
                                             vin:"SAJAC51M31XC12345"
                                 customResponder:NULL
                                         context:NULL];
}

- (void)disconnect
{
    [_shared disconnect];
    link_fuel_economy_init(&_fuelEconomy);
}

- (void)linkDiagnosticsControllerDidUpdate:
    (LinkDiagnosticsController *)controller
{
    (void)controller;
    [self notifyDelegate];
}

- (void)linkDiagnosticsController:(LinkDiagnosticsController *)controller
              didReceiveFlowEvent:(const LinkDiagnosticFlowEvent *)event
{
    (void)controller;
    if (event == NULL) return;

    if (event->kind == LINK_DIAGNOSTIC_FLOW_EVENT_LIVE_SAMPLE) {
        const uint64_t now_ms = JagLinkMonotonicMilliseconds();
        (void)link_fuel_economy_observe_obd2(
            &_fuelEconomy, &event->sample, now_ms);
        link_fuel_economy_tick(&_fuelEconomy, now_ms);
    }

    if (event->kind != LINK_DIAGNOSTIC_FLOW_EVENT_STANDARD_VIN) {
        return;
    }

    if (!event->vin_available || event->vin == NULL) {
        self.vehicleVINText = nil;
        self.vehiclePlatformText =
            @"Jaguar identity · standard VIN unavailable";
        self.vehicleConfigurationText =
            @"VIN not returned by SAE Mode 09 PID 02";
        self.vehiclePowertrainText =
            @"Powertrain remains unclassified from VIN";
        self.vehicleBuildText =
            @"Build identity unavailable from VIN";
        return;
    }

    self.vehicleVINText = JagLinkStringFromCString(event->vin);

    JaglinkJaguarVinDecode decoded;
    if (!jaglink_jaguar_vin_decode(event->vin, &decoded)) {
        self.vehiclePlatformText =
            @"VIN received · not a decodable Jaguar X400 VIN";
        self.vehicleConfigurationText =
            @"Jaguar-specific VIN fields unavailable";
        self.vehiclePowertrainText = @"Powertrain not inferred";
        self.vehicleBuildText = @"Build identity not inferred";
        return;
    }

    self.vehiclePlatformText = decoded.x400
        ? [NSString stringWithFormat:
            @"Jaguar X-TYPE · X400 · %u",
            (unsigned int)decoded.model_year]
        : @"Jaguar VIN decoded · X400 profile not confirmed";

    if (decoded.body != NULL &&
        decoded.transmission_steering != NULL) {
        self.vehicleConfigurationText = [NSString stringWithFormat:
            @"%@ · %@ · %@ · %@ · %@",
            JagLinkStringFromCString(
                jaglink_jaguar_body_style_name(
                    decoded.body->body_style)),
            JagLinkStringFromCString(decoded.body->series_class),
            JagLinkStringFromCString(
                jaglink_jaguar_drivetrain_name(
                    decoded.transmission_steering->drivetrain)),
            JagLinkStringFromCString(
                jaglink_jaguar_transmission_name(
                    decoded.transmission_steering->transmission)),
            JagLinkStringFromCString(
                jaglink_jaguar_steering_name(
                    decoded.transmission_steering->steering))];
    } else {
        self.vehicleConfigurationText =
            @"X400 configuration codes not fully recognised";
    }

    if (decoded.plant_engine != NULL) {
        self.vehiclePowertrainText = [NSString stringWithFormat:
            @"%@ · %@ · %u cc · %u kW",
            JagLinkStringFromCString(
                decoded.plant_engine->engine_description),
            JagLinkStringFromCString(
                jaglink_jaguar_fuel_type_name(
                    decoded.plant_engine->fuel)),
            decoded.plant_engine->displacement_cc,
            decoded.plant_engine->rated_power_kw];
        self.vehicleBuildText = [NSString stringWithFormat:
            @"%@, %@ · serial %@",
            JagLinkStringFromCString(
                decoded.plant_engine->assembly_plant),
            JagLinkStringFromCString(
                decoded.plant_engine->assembly_country),
            JagLinkStringFromCString(decoded.production_serial)];
    } else {
        self.vehiclePowertrainText =
            @"Engine-line code not yet catalogued";
        self.vehicleBuildText = [NSString stringWithFormat:
            @"Production serial %@",
            JagLinkStringFromCString(decoded.production_serial)];
    }
}

- (void)linkDiagnosticsControllerBeginManufacturerExtension:
    (LinkDiagnosticsController *)controller
{
    [controller failWithStatus:
        @"Unexpected manufacturer extension request in JAGLINK standard flow"];
}

- (BOOL)instantaneousFuelEconomyAvailable
{
    return [self fuelEconomySnapshot].instantaneous_available;
}

- (double)instantaneousFuelEconomyLPer100km
{
    return [self fuelEconomySnapshot].instantaneous_l_per_100km;
}

- (BOOL)averageFuelEconomyAvailable
{
    return [self fuelEconomySnapshot].average_available;
}

- (double)averageFuelEconomyLPer100km
{
    return [self fuelEconomySnapshot].average_l_per_100km;
}

- (BOOL)fuelRateAvailable
{
    return [self fuelEconomySnapshot].fuel_rate_available;
}

- (double)fuelRateLitresPerHour
{
    return [self fuelEconomySnapshot].fuel_rate_l_per_hour;
}

- (double)tripFuelLitres
{
    return [self fuelEconomySnapshot].trip_fuel_litres;
}

- (double)tripDistanceKilometres
{
    return [self fuelEconomySnapshot].trip_distance_km;
}

- (NSString *)instantaneousFuelEconomyText
{
    const LinkFuelEconomySnapshot snapshot = [self fuelEconomySnapshot];
    if (!snapshot.instantaneous_available) return @"Unavailable";
    const LinkUnitPreferences preferences = [self displayUnitPreferences];
    return JagLinkFormatFuelEconomy(
        snapshot.instantaneous_l_per_100km, &preferences);
}

- (NSString *)averageFuelEconomyText
{
    const LinkFuelEconomySnapshot snapshot = [self fuelEconomySnapshot];
    if (!snapshot.average_available) return @"Unavailable";
    const LinkUnitPreferences preferences = [self displayUnitPreferences];
    return JagLinkFormatFuelEconomy(snapshot.average_l_per_100km, &preferences);
}

- (NSString *)fuelRateText
{
    const LinkFuelEconomySnapshot snapshot = [self fuelEconomySnapshot];
    if (!snapshot.fuel_rate_available) return @"Unavailable";
    const LinkUnitPreferences preferences = [self displayUnitPreferences];
    return JagLinkFormatFuelRate(snapshot.fuel_rate_l_per_hour, &preferences);
}

- (NSString *)fuelTripText
{
    const LinkFuelEconomySnapshot snapshot = [self fuelEconomySnapshot];
    const LinkUnitPreferences preferences = [self displayUnitPreferences];
    return JagLinkFormatTrip(
        snapshot.trip_fuel_litres,
        snapshot.trip_distance_km,
        &preferences);
}

- (NSString *)fuelEconomySourceText
{
    LinkFuelEconomySnapshot snapshot = [self fuelEconomySnapshot];
    if (snapshot.instantaneous_available)
        return JagLinkFuelEconomySourceText(snapshot.instantaneous_source);
    if (snapshot.fuel_rate_available)
        return JagLinkFuelEconomySourceText(snapshot.fuel_rate_source);
    if (snapshot.average_available)
        return JagLinkFuelEconomySourceText(snapshot.average_source);
    return @"Unavailable";
}

- (NSString *)factoryFuelSignalStatusText
{
    return @"Jaguar factory fuel signal not yet enabled";
}

@end
