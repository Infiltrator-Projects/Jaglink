// SPDX-License-Identifier: GPL-3.0-or-later
#import "JagLinkDiagnosticsController.h"

#import "../../src/link/platform/apple/LinkDiagnosticsController.h"
#import "jaglink/jaguar.h"
#import "jaglink/jaguar_vin.h"
#import "link/fuel_economy.h"

@interface JagLinkDiagnosticsController () <LinkDiagnosticsControllerDelegate>
@property(nonatomic, copy, readwrite, nullable) NSString *vehicleVINText;
@property(nonatomic, copy, readwrite) NSString *vehiclePlatformText;
@property(nonatomic, copy, readwrite) NSString *vehicleConfigurationText;
@property(nonatomic, copy, readwrite) NSString *vehiclePowertrainText;
@property(nonatomic, copy, readwrite) NSString *vehicleBuildText;
- (void)resetFuelEconomy;
- (LinkFuelEconomySnapshot)fuelEconomySnapshot;
@end

@implementation JagLinkDiagnosticsController {
    LinkDiagnosticsController *_shared;
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

- (instancetype)init
{
    self = [super init];
    if (self == nil) return nil;

    LinkDiagnosticFlowConfig flowConfig = LINK_DIAGNOSTIC_FLOW_CONFIG_INIT;
    /* Keep live CAN responder IDs (for example 7E8/7E9) in evidence. */
    flowConfig.preserve_live_response_headers = true;
    _shared = [[LinkDiagnosticsController alloc]
        initWithProductSlug:@"jaglink"
        flowConfig:flowConfig
        liveStatusText:
            @"Live standard OBD-II data; X400 manufacturer discovery not yet enabled"
        simulatedLiveStatusText:
            @"Simulated ELM327 · live standard OBD-II data"
        standardVINStatusText:
            @"Reading standard VIN for Jaguar X400 identification"];
    _shared.delegate = self;
    link_fuel_economy_init(&_fuelEconomy);

    _vehiclePlatformText = @"Jaguar X400 identity pending";
    _vehicleConfigurationText = @"Waiting for standard VIN";
    _vehiclePowertrainText = @"Waiting for standard VIN";
    _vehicleBuildText = @"Waiting for standard VIN";
    return self;
}

- (void)dealloc
{
    _shared.delegate = nil;
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

- (void)notifyDelegate
{
    id<JagLinkDiagnosticsControllerDelegate> delegate = self.delegate;
    if (delegate != nil)
        [delegate diagnosticsControllerDidUpdate:self];
}

- (NSString *)statusText { return _shared.statusText; }
- (nullable NSString *)peripheralName { return _shared.peripheralName; }
- (nullable NSString *)adapterIdentifier { return _shared.adapterIdentifier; }
- (NSString *)obdProtocolText { return _shared.obdProtocolText; }
- (NSString *)faultScanStatusText { return _shared.faultScanStatusText; }
- (NSArray<NSString *> *)storedDTCs { return _shared.storedDTCs; }
- (NSArray<NSString *> *)pendingDTCs { return _shared.pendingDTCs; }
- (NSArray<NSString *> *)permanentDTCs { return _shared.permanentDTCs; }
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
- (NSString *)readinessStatusText { return _shared.readinessStatusText; }
- (NSArray<NSString *> *)readinessMonitorStatus
{
    return _shared.readinessMonitorStatus;
}
- (NSArray<NSString *> *)freezeFrameContext
{
    return _shared.freezeFrameContext;
}
- (NSString *)diagnosticCapabilityText
{
    return _shared.diagnosticCapabilityText;
}
- (NSString *)diagnosticCapabilityDetailText
{
    return _shared.diagnosticCapabilityDetailText;
}
- (NSString *)standardResponderSummary
{
    return _shared.standardResponderSummary;
}
- (NSString *)supportedPIDSummary
{
    return _shared.supportedPIDSummary;
}
- (NSString *)standardVINText
{
    return _shared.standardVINText;
}
- (NSArray<NSString *> *)standardLiveValueRows
{
    return _shared.standardLiveValueRows;
}

- (NSArray<NSDictionary *> *)standardResponderProfiles
{
    const LinkDiagnosticFlow *flow = _shared.diagnosticFlow;
    if (flow == NULL || flow->supported_pid_responders.count == 0) {
        return @[];
    }

    NSMutableArray<NSDictionary *> *profiles = [NSMutableArray arrayWithCapacity:
        flow->supported_pid_responders.count];
    for (size_t index = 0;
         index < flow->supported_pid_responders.count;
         ++index) {
        const LinkObd2ResponderPidSet *entry =
            &flow->supported_pid_responders.entries[index];
        NSMutableArray<NSNumber *> *supportedPIDs = [NSMutableArray array];
        for (NSUInteger pid = 0; pid < 256; ++pid) {
            if (link_obd2_pid_set_contains(
                    &entry->supported_pids, (uint8_t)pid)) {
                [supportedPIDs addObject:@(pid)];
            }
        }
        [profiles addObject:@{
            @"responderID": @(entry->responder_id),
            @"extendedID": @(entry->extended_id),
            @"supportedPIDs": supportedPIDs.copy
        }];
    }
    return profiles;
}
- (BOOL)isActive { return _shared.isActive; }
- (BOOL)isReady { return _shared.isReady; }
- (NSUInteger)recordedSampleCount { return _shared.recordedSampleCount; }
- (NSArray<NSString *> *)availableLanguageTags { return _shared.availableLanguageTags; }
- (NSArray<NSString *> *)availableLanguageNames { return _shared.availableLanguageNames; }
- (NSString *)selectedLanguageTag { return _shared.selectedLanguageTag; }
- (NSArray<NSString *> *)availableMeasurementSystemKeys { return _shared.availableMeasurementSystemKeys; }
- (NSArray<NSString *> *)availableMeasurementSystemNames { return _shared.availableMeasurementSystemNames; }
- (NSString *)selectedMeasurementSystemKey { return _shared.selectedMeasurementSystemKey; }
- (NSString *)localizedTextForKey:(NSString *)key { return [_shared localizedTextForKey:key]; }
- (void)setSelectedLanguageTag:(NSString *)tag { [_shared setSelectedLanguageTag:tag]; }
- (void)setSelectedMeasurementSystemKey:(NSString *)key { [_shared setSelectedMeasurementSystemKey:key]; }

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

- (NSArray<NSNumber *> *)recentValuesForPID:(uint8_t)pid
                                      limit:(NSUInteger)limit
{
    return [_shared recentValuesForPID:pid limit:limit];
}

- (NSArray<NSNumber *> *)displayRecentValuesForPID:(uint8_t)pid
                                             limit:(NSUInteger)limit
{
    return [_shared displayRecentValuesForPID:pid limit:limit];
}

- (NSString *)displayUnitForPID:(uint8_t)pid
{
    return [_shared displayUnitForPID:pid];
}

- (NSArray<NSNumber *> *)displayRangeForPID:(uint8_t)pid
{
    return [_shared displayRangeForPID:pid];
}

- (BOOL)favouriteForPID:(uint8_t)pid
{
    return [_shared favouriteForPID:pid];
}

- (void)setFavourite:(BOOL)favourite forPID:(uint8_t)pid
{
    [_shared setFavourite:favourite forPID:pid];
}

- (nullable NSData *)csvDataSnapshot
{
    return [_shared csvDataSnapshot];
}

- (nullable NSString *)csvSnapshot
{
    return [_shared csvSnapshot];
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
