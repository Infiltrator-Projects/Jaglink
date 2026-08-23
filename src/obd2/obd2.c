// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file obd2.c
 * @brief iOS build bridge to LINK's shared OBD-II and generic DTC knowledge implementations.
 */
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS
#include "../link/src/obd2/obd2.c"
#include "../link/src/obd2/dtc_knowledge.c"
#else
typedef int jaglink_obd2_compat_translation_unit;
#endif
