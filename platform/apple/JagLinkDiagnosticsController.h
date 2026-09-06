// SPDX-License-Identifier: GPL-3.0-or-later
#import <Foundation/Foundation.h>

#import "../../src/link/platform/apple/LinkDiagnosticsController.h"

NS_ASSUME_NONNULL_BEGIN

@class JagLinkDiagnosticsController;

@protocol JagLinkDiagnosticsControllerDelegate <NSObject>
- (void)diagnosticsControllerDidUpdate:(JagLinkDiagnosticsController *)controller;
@end

/**
 * Jaguar-specific extension over LINK's complete standard Apple controller.
 * Generic session, transport, SAE data, units, language and evidence APIs are
 * inherited from LinkProductDiagnosticsController.
 */
@interface JagLinkDiagnosticsController : LinkProductDiagnosticsController

- (instancetype)init;

@property(nonatomic, weak, nullable) id<JagLinkDiagnosticsControllerDelegate> delegate;
@property(nonatomic, copy, readonly, nullable) NSString *vehicleVINText;
@property(nonatomic, copy, readonly) NSString *vehiclePlatformText;
@property(nonatomic, copy, readonly) NSString *vehicleConfigurationText;
@property(nonatomic, copy, readonly) NSString *vehiclePowertrainText;
@property(nonatomic, copy, readonly) NSString *vehicleBuildText;

/** Jaguar-aware presentation rows; inherited raw DTC arrays remain evidence. */
@property(nonatomic, copy, readonly) NSArray<NSString *> *storedDTCDisplayRows;
@property(nonatomic, copy, readonly) NSArray<NSString *> *pendingDTCDisplayRows;
@property(nonatomic, copy, readonly) NSArray<NSString *> *permanentDTCDisplayRows;

@property(nonatomic, readonly) BOOL instantaneousFuelEconomyAvailable;
@property(nonatomic, readonly) double instantaneousFuelEconomyLPer100km;
@property(nonatomic, readonly) BOOL averageFuelEconomyAvailable;
@property(nonatomic, readonly) double averageFuelEconomyLPer100km;
@property(nonatomic, readonly) BOOL fuelRateAvailable;
@property(nonatomic, readonly) double fuelRateLitresPerHour;
@property(nonatomic, readonly) double tripFuelLitres;
@property(nonatomic, readonly) double tripDistanceKilometres;
@property(nonatomic, copy, readonly) NSString *instantaneousFuelEconomyText;
@property(nonatomic, copy, readonly) NSString *averageFuelEconomyText;
@property(nonatomic, copy, readonly) NSString *fuelRateText;
@property(nonatomic, copy, readonly) NSString *fuelTripText;
@property(nonatomic, copy, readonly) NSString *fuelEconomySourceText;
@property(nonatomic, copy, readonly) NSString *factoryFuelSignalStatusText;

@end

NS_ASSUME_NONNULL_END
