// SPDX-License-Identifier: GPL-3.0-or-later
#import "JagLinkBLETransport.h"
#import "jaglink/transport.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Return a C ABI transport backed by the supplied CoreBluetooth provider.
 *
 * The provider must outlive every JaglinkTransport copy created from it.
 */
JaglinkTransport JagLinkBLETransportMakeCTransport(JagLinkBLETransport *transport);

NS_ASSUME_NONNULL_END
