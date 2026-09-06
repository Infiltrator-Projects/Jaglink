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
 * JAGLINK consumes LINK's all-in-one Apple portable-core entry point.
 * Generic LINK implementation ownership stays in LINK; this file supplies
 * only JAGLINK metadata and compatibility wiring.
 */
#if defined(__APPLE__) && TARGET_OS_IOS
#ifndef LINK_SOURCE_REVISION
#define LINK_SOURCE_REVISION JAGLINK_EMBEDDED_LINK_REVISION
#define JAGLINK_DEFINED_LINK_SOURCE_REVISION 1
#endif
#include "../link/platform/apple/LinkPortableCore.c"
#ifdef JAGLINK_DEFINED_LINK_SOURCE_REVISION
#undef JAGLINK_DEFINED_LINK_SOURCE_REVISION
#undef LINK_SOURCE_REVISION
#endif

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
