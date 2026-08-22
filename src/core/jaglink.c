// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaglink.h"
#include "jaglink/transport.h"

#include "infiltratr/core.h"

#include <stddef.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#ifndef JAGLINK_VERSION
#error "JAGLINK_VERSION must be supplied by the build system"
#endif

/* Normal CMake builds use LINK::Core; native iOS compiles the exact pinned
 * LINK sources directly into the app core. */
#if defined(__APPLE__) && TARGET_OS_IOS
#include "../link/src/core/workspace.c"
#include "../link/src/core/parameter.c"
#include "../link/src/core/scheduler.c"
#include "../link/src/core/telemetry.c"
#endif

static const InfiltratrProjectInfo jaglink_project_info = {
    .struct_size = sizeof(InfiltratrProjectInfo),
    .abi_version = INFILTRATR_PROJECT_INFO_ABI,
    .program_name = "JAGLINK",
    .executable_name = "jaglink",
    .application_id = "com.github.The-First-Infiltrator.Jaglink",
    .version = JAGLINK_VERSION,
    .source_id = "The-First-Infiltrator/Jaglink",
    .build_profile = "portable-c11",
    .author = "Shannon Smith",
    .website = "https://github.com/The-First-Infiltrator/Jaglink",
    .license_id = "GPL-3.0-or-later",
    .comments = "Portable C Jaguar vehicle diagnostics core",
    .icon_name = "jaglink",
    .copyright_text = "Copyright (C) 2026 Shannon Smith"
};

const char *jaglink_version(void)
{
    return JAGLINK_VERSION;
}

bool jaglink_self_check(void)
{
    return infiltratr_project_info_is_valid(&jaglink_project_info);
}

bool jaglink_transport_is_valid(const JaglinkTransport *transport)
{
    if (transport == NULL || transport->struct_size < sizeof(*transport) ||
        transport->abi_version != JAGLINK_TRANSPORT_ABI) {
        return false;
    }

    return transport->connect != NULL &&
           transport->disconnect != NULL &&
           transport->is_connected != NULL &&
           transport->write != NULL &&
           transport->set_receiver != NULL;
}
