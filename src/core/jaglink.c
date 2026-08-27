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

/*
 * Normal CMake builds consume shared engines through LINK::Core.  The native
 * iPhone target compiles portable C sources directly, so include the exact
 * sources from the pinned LINK checkout rather than maintaining product-owned
 * copies of workspace, runtime, transport or diagnostic state machines.
 */
#if defined(__APPLE__) && TARGET_OS_IOS
#include "../link/src/core/workspace.c"
#include "../link/src/core/fuel_economy.c"
#include "../link/src/core/diagnostic_flow.c"
#include "../link/src/core/parameter.c"
#include "../link/src/core/scheduler.c"
#include "../link/src/core/telemetry.c"
#include "../link/src/core/transport.c"
#include "../link/src/elm327/elm327.c"
#include "../link/src/elm327/can.c"
#include "../link/src/elm327/probe.c"
#include "../link/src/elm327/session.c"
#endif

static const InfiltratrProjectInfo jaglink_project_info_record = {
    .struct_size = sizeof(InfiltratrProjectInfo),
    .abi_version = INFILTRATR_PROJECT_INFO_ABI,
    .program_name = "JAGLINK",
    .executable_name = "jaglink",
    .application_id = "com.github.The-First-Infiltrator.Jaglink",
    .version = JAGLINK_VERSION,
    .source_id = "The-First-Infiltrator/JAGLINK",
    .build_profile = JAGLINK_BUILD_PROFILE,
    .author = "Xavier Wheaton and Shannon Smith",
    .website = "https://github.com/The-First-Infiltrator/Jaglink",
    .license_id = "GPL-3.0-or-later",
    .comments = "Jaguar X-Type X400 diagnostics, evidence capture and vehicle-focused diagnostic tooling.",
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
