// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaglink.h"
#include "jaglink/transport.h"

#include "infiltratr/core.h"

#include <stddef.h>

#ifndef JAGLINK_VERSION
#error "JAGLINK_VERSION must be supplied by the build system"
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

static const JaglinkWorkspaceSectionDescriptor jaglink_workspace_sections[] = {
    { JAGLINK_WORKSPACE_VEHICLE, "vehicle", "Vehicle", "Vehicle identity, adapter and connection information" },
    { JAGLINK_WORKSPACE_MODULES, "modules", "Modules", "Discovered control modules and ECU identification" },
    { JAGLINK_WORKSPACE_FAULTS, "faults", "Faults", "Diagnostic trouble codes by control module" },
    { JAGLINK_WORKSPACE_LIVE_DATA, "live-data", "Live Data", "Search, select and favourite live diagnostic parameters" },
    { JAGLINK_WORKSPACE_TABLE, "table", "Table", "Dense live values for selected diagnostic parameters" },
    { JAGLINK_WORKSPACE_DASHBOARD, "dashboard", "Dashboard", "At-a-glance live diagnostic measurements" },
    { JAGLINK_WORKSPACE_GRAPHS, "graphs", "Graphs", "Time-series views for selected diagnostic parameters" },
    { JAGLINK_WORKSPACE_LOG, "log", "Log", "Diagnostic session history and exported telemetry" },
    { JAGLINK_WORKSPACE_SETTINGS, "settings", "Settings", "Adapter, units, logging and application preferences" }
};

const char *jaglink_version(void) { return JAGLINK_VERSION; }
bool jaglink_self_check(void) { return infiltratr_project_info_is_valid(&jaglink_project_info); }
size_t jaglink_workspace_section_count(void) { return sizeof(jaglink_workspace_sections) / sizeof(jaglink_workspace_sections[0]); }
const JaglinkWorkspaceSectionDescriptor *jaglink_workspace_section_at(size_t index) { return index < jaglink_workspace_section_count() ? &jaglink_workspace_sections[index] : NULL; }
const JaglinkWorkspaceSectionDescriptor *jaglink_workspace_section(JaglinkWorkspaceSection section)
{
    size_t index;
    for (index = 0U; index < jaglink_workspace_section_count(); ++index) if (jaglink_workspace_sections[index].section == section) return &jaglink_workspace_sections[index];
    return NULL;
}
bool jaglink_transport_is_valid(const JaglinkTransport *transport)
{
    if (transport == NULL || transport->struct_size < sizeof(*transport) || transport->abi_version != JAGLINK_TRANSPORT_ABI) return false;
    return transport->connect != NULL && transport->disconnect != NULL && transport->is_connected != NULL && transport->write != NULL && transport->set_receiver != NULL;
}
