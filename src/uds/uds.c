// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file uds.c
 * @brief iOS build bridge to LINK's shared ISO 14229 portable entry point.
 */
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS
/* LINK owns UDS composition; JAGLINK includes only its public Apple bridge. */
#include "../link/platform/apple/LinkPortableUds.c"
#else
typedef int jaglink_uds_compat_translation_unit;
#endif
