// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file JagLinkBLETransport.h
 * @brief Compatibility names for LINK's shared CoreBluetooth provider.
 */
#import "../../src/link/platform/apple/LinkBLETransport.h"

#define JagLinkBLETransport LinkBLETransport
#define JagLinkBLETransportDelegate LinkBLETransportDelegate

typedef LinkBLETransportState JagLinkBLETransportState;

#define JagLinkBLETransportStateIdle LinkBLETransportStateIdle
#define JagLinkBLETransportStateWaitingForBluetooth LinkBLETransportStateWaitingForBluetooth
#define JagLinkBLETransportStateScanning LinkBLETransportStateScanning
#define JagLinkBLETransportStateConnecting LinkBLETransportStateConnecting
#define JagLinkBLETransportStateDiscovering LinkBLETransportStateDiscovering
#define JagLinkBLETransportStateProbing LinkBLETransportStateProbing
#define JagLinkBLETransportStateReady LinkBLETransportStateReady
#define JagLinkBLETransportStateDisconnected LinkBLETransportStateDisconnected
#define JagLinkBLETransportStateFailed LinkBLETransportStateFailed
