// SPDX-License-Identifier: GPL-3.0-or-later
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class JagLinkBLETransport;

typedef NS_ENUM(NSInteger, JagLinkBLETransportState) {
    JagLinkBLETransportStateIdle = 0,
    JagLinkBLETransportStateWaitingForBluetooth,
    JagLinkBLETransportStateScanning,
    JagLinkBLETransportStateConnecting,
    JagLinkBLETransportStateDiscovering,
    JagLinkBLETransportStateProbing,
    JagLinkBLETransportStateReady,
    JagLinkBLETransportStateDisconnected,
    JagLinkBLETransportStateFailed
};

@protocol JagLinkBLETransportDelegate <NSObject>
- (void)bleTransportDidUpdate:(JagLinkBLETransport *)transport;
@end

/**
 * CoreBluetooth implementation of the JAGLINK byte-stream transport boundary.
 *
 * The public Objective-C surface intentionally contains no OBD-II parsing.
 * BLE/GATT discovery remains a platform concern; diagnostic interpretation
 * remains in libjaglink.
 */
@interface JagLinkBLETransport : NSObject

@property(nonatomic, weak, nullable) id<JagLinkBLETransportDelegate> delegate;
@property(nonatomic, readonly) JagLinkBLETransportState state;
@property(nonatomic, copy, readonly) NSString *statusText;
@property(nonatomic, copy, readonly, nullable) NSString *peripheralName;
@property(nonatomic, copy, readonly, nullable) NSString *adapterIdentifier;
@property(nonatomic, copy, readonly, nullable) NSString *serviceUUID;
@property(nonatomic, copy, readonly, nullable) NSString *writeCharacteristicUUID;
@property(nonatomic, copy, readonly, nullable) NSString *notifyCharacteristicUUID;
@property(nonatomic, readonly, getter=isReady) BOOL ready;

- (void)start;
- (void)disconnect;

@end

NS_ASSUME_NONNULL_END
