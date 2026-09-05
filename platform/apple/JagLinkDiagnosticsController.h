// SPDX-License-Identifier: GPL-3.0-or-later
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class JagLinkDiagnosticsController;

@protocol JagLinkDiagnosticsControllerDelegate <NSObject>
- (void)diagnosticsControllerDidUpdate:(JagLinkDiagnosticsController *)controller;
@end

@interface JagLinkDiagnosticsController : NSObject

@property(nonatomic, weak, nullable) id<JagLinkDiagnosticsControllerDelegate> delegate;
@property(nonatomic, copy, readonly) NSString *statusText;
@property(nonatomic, copy, readonly, nullable) NSString *peripheralName;
@property(nonatomic, copy, readonly, nullable) NSString *adapterIdentifier;
/** Active OBD transport identified by LINK (protocol, speed/init and auto-selection). */
@property(nonatomic, copy, readonly) NSString *obdProtocolText;
@property(nonatomic, copy, readonly, nullable) NSString *vehicleVINText;
@property(nonatomic, copy, readonly) NSString *vehiclePlatformText;
@property(nonatomic, copy, readonly) NSString *vehicleConfigurationText;
@property(nonatomic, copy, readonly) NSString *vehiclePowertrainText;
@property(nonatomic, copy, readonly) NSString *vehicleBuildText;
@property(nonatomic, copy, readonly) NSString *faultScanStatusText;
@property(nonatomic, copy, readonly) NSArray<NSString *> *storedDTCs;
@property(nonatomic, copy, readonly) NSArray<NSString *> *pendingDTCs;
@property(nonatomic, copy, readonly) NSArray<NSString *> *permanentDTCs;
/** Presentation rows; raw DTC arrays above remain unchanged for evidence/profile data. */
@property(nonatomic, copy, readonly) NSArray<NSString *> *storedDTCDisplayRows;
@property(nonatomic, copy, readonly) NSArray<NSString *> *pendingDTCDisplayRows;
@property(nonatomic, copy, readonly) NSArray<NSString *> *permanentDTCDisplayRows;
@property(nonatomic, copy, readonly) NSString *readinessStatusText;
@property(nonatomic, copy, readonly) NSArray<NSString *> *readinessMonitorStatus;
@property(nonatomic, copy, readonly) NSArray<NSString *> *freezeFrameContext;
@property(nonatomic, copy, readonly) NSString *diagnosticCapabilityText;
@property(nonatomic, copy, readonly) NSString *diagnosticCapabilityDetailText;
@property(nonatomic, copy, readonly) NSString *standardResponderSummary;
@property(nonatomic, copy, readonly) NSString *supportedPIDSummary;
@property(nonatomic, copy, readonly) NSString *standardVINText;
@property(nonatomic, copy, readonly) NSArray<NSString *> *standardLiveValueRows;
/** Generic standard OBD-II responder capability snapshots for vehicle profiles. */
@property(nonatomic, copy, readonly) NSArray<NSDictionary *> *standardResponderProfiles;
@property(nonatomic, readonly, getter=isActive) BOOL active;
@property(nonatomic, readonly, getter=isReady) BOOL ready;
@property(nonatomic, readonly) NSUInteger recordedSampleCount;
@property(nonatomic, copy, readonly) NSArray<NSString *> *availableLanguageTags;
@property(nonatomic, copy, readonly) NSArray<NSString *> *availableLanguageNames;
@property(nonatomic, copy, readonly) NSString *selectedLanguageTag;
@property(nonatomic, copy, readonly) NSArray<NSString *> *availableMeasurementSystemKeys;
@property(nonatomic, copy, readonly) NSArray<NSString *> *availableMeasurementSystemNames;
@property(nonatomic, copy, readonly) NSString *selectedMeasurementSystemKey;

@property(nonatomic, readonly) BOOL instantaneousFuelEconomyAvailable;
@property(nonatomic, readonly) double instantaneousFuelEconomyLPer100km;
@property(nonatomic, readonly) BOOL averageFuelEconomyAvailable;
@property(nonatomic, readonly) double averageFuelEconomyLPer100km;
@property(nonatomic, readonly) BOOL fuelRateAvailable;
@property(nonatomic, readonly) double fuelRateLitresPerHour;
@property(nonatomic, readonly) double tripFuelLitres;
@property(nonatomic, readonly) double tripDistanceKilometres;
@property(nonatomic, copy, readonly) NSString *fuelEconomySourceText;
@property(nonatomic, copy, readonly) NSString *factoryFuelSignalStatusText;

- (void)start;
/** Start a real session against one exact CoreBluetooth peripheral UUID. */
- (void)startWithPeripheralIdentifier:(NSString *)peripheralIdentifier;
- (void)startSimulated;
- (void)disconnect;
- (NSString *)localizedTextForKey:(NSString *)key;
- (void)setSelectedLanguageTag:(NSString *)tag;
- (void)setSelectedMeasurementSystemKey:(NSString *)key;
/** Canonical history retained for evidence/backwards compatibility. */
- (NSArray<NSNumber *> *)recentValuesForPID:(uint8_t)pid limit:(NSUInteger)limit;
/** LINK-owned presentation conversion for the selected unit system. */
- (NSArray<NSNumber *> *)displayRecentValuesForPID:(uint8_t)pid limit:(NSUInteger)limit;
- (NSString *)displayUnitForPID:(uint8_t)pid;
- (NSArray<NSNumber *> *)displayRangeForPID:(uint8_t)pid;
- (BOOL)favouriteForPID:(uint8_t)pid;
- (void)setFavourite:(BOOL)favourite forPID:(uint8_t)pid;
- (nullable NSData *)csvDataSnapshot;
- (nullable NSString *)csvSnapshot;

@end

NS_ASSUME_NONNULL_END
