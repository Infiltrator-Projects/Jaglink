// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file transport.h
 * @brief Platform-neutral transport boundary for libjaglink.
 *
 * Platform providers implement this interface without exposing native
 * framework types to the portable diagnostics core.
 */
#ifndef JAGLINK_TRANSPORT_H
#define JAGLINK_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JAGLINK_TRANSPORT_ABI 1U

typedef enum {
    JAGLINK_TRANSPORT_OK = 0,
    JAGLINK_TRANSPORT_NOT_CONNECTED,
    JAGLINK_TRANSPORT_BUSY,
    JAGLINK_TRANSPORT_TIMEOUT,
    JAGLINK_TRANSPORT_IO_ERROR,
    JAGLINK_TRANSPORT_UNSUPPORTED,
    JAGLINK_TRANSPORT_INVALID_ARGUMENT
} JaglinkTransportStatus;

/**
 * Receive callback installed by the protocol layer.
 *
 * Providers must serialize delivery for one transport instance. `data` is
 * borrowed and only valid for the duration of the callback.
 */
typedef void (*JaglinkTransportReceiveFn)(void *context,
                                         const uint8_t *data,
                                         size_t size);

/**
 * Platform-neutral byte-stream provider contract.
 *
 * `context` and all provider-owned resources must outlive every transport copy
 * using them. `write()` must consume or copy the supplied bytes before it
 * returns. `set_receiver(context, NULL, NULL)` detaches any previously
 * installed receiver and must be supported by every provider.
 */
typedef struct {
    size_t struct_size;
    uint32_t abi_version;
    void *context;
    JaglinkTransportStatus (*connect)(void *context);
    void (*disconnect)(void *context);
    bool (*is_connected)(void *context);
    JaglinkTransportStatus (*write)(void *context,
                                   const uint8_t *data,
                                   size_t size);
    void (*set_receiver)(void *context,
                         JaglinkTransportReceiveFn receiver,
                         void *receiver_context);
} JaglinkTransport;

#define JAGLINK_TRANSPORT_INIT \
    { .struct_size = sizeof(JaglinkTransport), \
      .abi_version = JAGLINK_TRANSPORT_ABI, \
      .context = NULL, .connect = NULL, .disconnect = NULL, \
      .is_connected = NULL, .write = NULL, .set_receiver = NULL }

/** Validate ABI metadata and all mandatory operations. */
bool jaglink_transport_is_valid(const JaglinkTransport *transport);

#ifdef __cplusplus
}
#endif

#endif
