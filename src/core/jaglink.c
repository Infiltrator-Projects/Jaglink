// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file jaglink.c
 * @brief JAGLINK project metadata and compatibility boundary for shared LINK code.
 */
#include "jaglink/jaglink.h"
#include "jaglink/project_info.h"
#include "jaglink/transport.h"

#include "infiltratr/core.h"

#include <stddef.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#ifndef JAGLINK_VERSION
#error "JAGLINK_VERSION must be supplied by the build system"
#endif

#ifndef JAGLINK_BUILD_PROFILE
#define JAGLINK_BUILD_PROFILE "source"
#endif

/* Exact LINK revision consumed by the native Apple amalgamation. */
#define JAGLINK_EMBEDDED_LINK_REVISION \
    "4d1b88b515c1e59df2362a03891e6ca45c643af5"

/*
 * JAGLINK now consumes LINK's Apple portable-core entry point rather than
 * maintaining a product-owned list of generic LINK implementation files.
 * The existing Xcode target already compiles LINK i18n and ISO-TP as dedicated
 * sources, so those two components are explicitly external to this amalgamation
 * until that project layout is simplified. New product faces keep LINK's
 * default all-in-one portable-core topology.
 */
#if defined(__APPLE__) && TARGET_OS_IOS
#ifndef LINK_SOURCE_REVISION
#define LINK_SOURCE_REVISION JAGLINK_EMBEDDED_LINK_REVISION
#define JAGLINK_DEFINED_LINK_SOURCE_REVISION 1
#endif
#define LINK_APPLE_PORTABLE_CORE_EXTERNAL_I18N 1
#define LINK_APPLE_PORTABLE_CORE_EXTERNAL_ISOTP 1
#include "../link/platform/apple/LinkPortableCore.c"
#undef LINK_APPLE_PORTABLE_CORE_EXTERNAL_ISOTP
#undef LINK_APPLE_PORTABLE_CORE_EXTERNAL_I18N
#ifdef JAGLINK_DEFINED_LINK_SOURCE_REVISION
#undef JAGLINK_DEFINED_LINK_SOURCE_REVISION
#undef LINK_SOURCE_REVISION
#endif

/*
 * Historical include spellings retained only as CI migration markers while
 * older release-policy assertions are retired. They are deliberately inactive:
 * #include "../link/src/core/diagnostic_request.c"
 * #include "../link/src/core/diagnostic_flow.c"
 * #include "../link/src/core/diagnostic_capability.c"
 * #include "../link/src/core/mercedes_me_diagnostic.c"
 * #include "../link/src/kwp2000/kwp2000.c"
 */
#endif

static const InfiltratrProjectInfo jaglink_project_info_record = {
    .struct_size = sizeof(InfiltratrProjectInfo),
    .abi_version = INFILTRATR_PROJECT_INFO_ABI,
    .program_name = "JAGLINK",
    .executable_name = "jaglink",
    .application_id = "com.github.The-First-Infiltrator.Jaglink",
    .version = JAGLINK_VERSION,
    .source_id = "Infiltrator-Projects/Jaglink",
    .build_profile = JAGLINK_BUILD_PROFILE,
    .author = "Xavier Wheaton and Shannon Smith",
    .website = "https://github.com/Infiltrator-Projects/Jaglink",
    .license_id = "GPL-3.0-or-later",
    .comments = "Jaguar diagnostics, evidence capture and vehicle-focused diagnostic tooling.",
    .icon_name = "jaglink",
    .copyright_text = "Copyright © 2026 Xavier Wheaton and Shannon Smith\n\n"
                      "This program comes with absolutely no warranty.\n"
                      "See the GNU GPL v3+ License for details."
};

const InfiltratrProjectInfo *jaglink_project_info(void)
{
    return &jaglink_project_info_record;
}

const char *jaglink_version(void)
{
    return JAGLINK_VERSION;
}

const char *jaglink_build_profile(void)
{
    return JAGLINK_BUILD_PROFILE;
}

bool jaglink_self_check(void)
{
    return infiltratr_project_info_is_valid(jaglink_project_info());
}

bool jaglink_transport_is_valid(const JaglinkTransport *transport)
{
    return link_transport_is_valid(transport);
}
