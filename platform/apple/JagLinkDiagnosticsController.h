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
@property(nonatomic, copy, readonly) NSString *faultScanStatusText;
@property(nonatomic, copy, readonly) NSArray<NSString *> *storedDTCs;
@property(nonatomic, copy, readonly) NSArray<NSString *> *pendingDTCs;
@property(nonatomic, copy, readonly) NSArray<NSString *> *permanentDTCs;
@property(nonatomic, readonly, getter=isActive) BOOL active;
@property(nonatomic, readonly, getter=isReady) BOOL ready;
@property(nonatomic, readonly) NSUInteger recordedSampleCount;

- (void)start;
- (void)disconnect;
- (NSArray<NSNumber *> *)recentValuesForPID:(uint8_t)pid limit:(NSUInteger)limit;
- (BOOL)favouriteForPID:(uint8_t)pid;
- (void)setFavourite:(BOOL)favourite forPID:(uint8_t)pid;
- (nullable NSString *)csvSnapshot;

@end

NS_ASSUME_NONNULL_END
