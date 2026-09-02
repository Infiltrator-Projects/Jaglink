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
@property(nonatomic, copy, readonly, nullable) NSString *vehicleVINText;
@property(nonatomic, copy, readonly) NSString *vehiclePlatformText;
@property(nonatomic, copy, readonly) NSString *vehicleConfigurationText;
@property(nonatomic, copy, readonly) NSString *vehiclePowertrainText;
@property(nonatomic, copy, readonly) NSString *vehicleBuildText;
@property(nonatomic, copy, readonly) NSString *faultScanStatusText;
@property(nonatomic, copy, readonly) NSArray<NSString *> *storedDTCs;
@property(nonatomic, copy, readonly) NSArray<NSString *> *pendingDTCs;
@property(nonatomic, copy, readonly) NSArray<NSString *> *permanentDTCs;
@property(nonatomic, copy, readonly) NSString *readinessStatusText;
@property(nonatomic, copy, readonly) NSArray<NSString *> *readinessMonitorStatus;
@property(nonatomic, copy, readonly) NSArray<NSString *> *freezeFrameContext;
@property(nonatomic, copy, readonly) NSString *diagnosticCapabilityText;
@property(nonatomic, copy, readonly) NSString *diagnosticCapabilityDetailText;
@property(nonatomic, copy, readonly) NSString *standardResponderSummary;
@property(nonatomic, copy, readonly) NSString *supportedPIDSummary;
@property(nonatomic, copy, readonly) NSString *standardVINText;
@property(nonatomic, copy, readonly) NSArray<NSString *> *standardLiveValueRows;
@property(nonatomic, readonly, getter=isActive) BOOL active;
@property(nonatomic, readonly, getter=isReady) BOOL ready;
@property(nonatomic, readonly) NSUInteger recordedSampleCount;

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
- (void)startSimulated;
- (void)disconnect;
- (NSArray<NSNumber *> *)recentValuesForPID:(uint8_t)pid limit:(NSUInteger)limit;
- (BOOL)favouriteForPID:(uint8_t)pid;
- (void)setFavourite:(BOOL)favourite forPID:(uint8_t)pid;
- (nullable NSData *)csvDataSnapshot;
- (nullable NSString *)csvSnapshot;

@end

NS_ASSUME_NONNULL_END
