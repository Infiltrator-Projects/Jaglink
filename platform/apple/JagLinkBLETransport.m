// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * JAGLINK compatibility compilation shim. The CoreBluetooth implementation is
 * owned by LINK and compiled into this product target from the pinned gitlink.
 */
#import "JagLinkBLETransport+JAGLINK.h"
#include "../../src/link/platform/apple/LinkBLETransport.m"
