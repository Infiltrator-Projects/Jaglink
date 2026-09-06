// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file obd2.c
 * @brief iOS build bridge to LINK's shared SAE OBD-II portable entry point.
 */
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS
#include "../link/platform/apple/LinkPortableObd2.c"
#else
typedef int jaglink_obd2_compat_translation_unit;
#endif
