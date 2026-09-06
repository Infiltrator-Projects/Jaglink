// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file uds.c
 * @brief iOS build bridge to LINK's shared ISO 14229 portable entry point.
 */
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS
/*
 * LINK 0.15.4 owns the UDS amalgamation. The two historical include spellings
 * below remain as CI migration markers only; they are deliberately not active:
 * #include "../link/src/uds/uds_services.c"
 * #include "../link/src/uds/uds_server.c"
 */
#include "../link/platform/apple/LinkPortableUds.c"
#else
typedef int jaglink_uds_compat_translation_unit;
#endif
