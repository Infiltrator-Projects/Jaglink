// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaglink.h"
#include "jaglink/transport.h"

#include "infiltratr/core.h"

#include <stddef.h>

#ifndef JAGLINK_VERSION
#error "JAGLINK_VERSION must be supplied by the build system"
#endif

/*
 * CMake links LINK::Core and defines LINK_WORKSPACE_EXTERNAL. The Apple
 * project compiles portable C sources directly, so it pulls the exact same
 * LINK workspace implementation from the pinned submodule here instead of
 * carrying a JAGLINK copy.
 */
#ifndef LINK_WORKSPACE_EXTERNAL
#include "../link/src/core/workspace.c"
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
