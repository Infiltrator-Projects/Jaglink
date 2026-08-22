// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file transport.h
 * @brief JAGLINK compatibility facade for LINK's byte-stream transport ABI.
 *
 * LINK owns the transport contract.  JAGLINK retains its historical public
 * type and function names so Jaguar-specific callers remain source and ABI
 * compatible while the shared implementation lives below the product layer.
 */
#ifndef JAGLINK_TRANSPORT_H
#define JAGLINK_TRANSPORT_H

#include "link/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_TRANSPORT_ABI LINK_TRANSPORT_ABI
#define JAGLINK_TRANSPORT_OK LINK_TRANSPORT_OK
#define JAGLINK_TRANSPORT_NOT_CONNECTED LINK_TRANSPORT_NOT_CONNECTED
#define JAGLINK_TRANSPORT_BUSY LINK_TRANSPORT_BUSY
#define JAGLINK_TRANSPORT_TIMEOUT LINK_TRANSPORT_TIMEOUT
#define JAGLINK_TRANSPORT_IO_ERROR LINK_TRANSPORT_IO_ERROR
#define JAGLINK_TRANSPORT_UNSUPPORTED LINK_TRANSPORT_UNSUPPORTED
#define JAGLINK_TRANSPORT_INVALID_ARGUMENT LINK_TRANSPORT_INVALID_ARGUMENT

typedef LinkTransportStatus JaglinkTransportStatus;
typedef LinkTransportReceiveFn JaglinkTransportReceiveFn;
typedef LinkTransport JaglinkTransport;

#define JAGLINK_TRANSPORT_INIT LINK_TRANSPORT_INIT

/** Forward ABI validation to the single implementation owned by LINK. */
bool jaglink_transport_is_valid(const JaglinkTransport *transport);

#ifdef __cplusplus
}
#endif

#endif
