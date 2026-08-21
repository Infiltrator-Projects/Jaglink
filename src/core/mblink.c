// SPDX-License-Identifier: GPL-3.0-or-later
#include "mblink/mblink.h"
#include "mblink/transport.h"

#include "infiltratr/core.h"

#include <stddef.h>

#ifndef MBLINK_VERSION
#error "MBLINK_VERSION must be supplied by the build system"
#endif

static const InfiltratrProjectInfo mblink_project_info = {
    .struct_size = sizeof(InfiltratrProjectInfo),
    .abi_version = INFILTRATR_PROJECT_INFO_ABI,
    .program_name = "JAGLINK",
    .executable_name = "jaglink",
    .application_id = "com.github.The-First-Infiltrator.Jaglink",
    .version = MBLINK_VERSION,
    .source_id = "The-First-Infiltrator/Jaglink",
    .build_profile = "portable-c11",
    .author = "Shannon Smith",
    .website = "https://github.com/The-First-Infiltrator/Jaglink",
    .license_id = "GPL-3.0-or-later",
    .comments = "Portable C Jaguar vehicle diagnostics core",
    .icon_name = "jaglink",
    .copyright_text = "Copyright (C) 2026 Shannon Smith"
};

static const MblinkWorkspaceSectionDescriptor mblink_workspace_sections[] = {
    { MBLINK_WORKSPACE_VEHICLE, "vehicle", "Vehicle", "Vehicle identity, adapter and connection information" },
    { MBLINK_WORKSPACE_MODULES, "modules", "Modules", "Discovered control modules and ECU identification" },
    { MBLINK_WORKSPACE_FAULTS, "faults", "Faults", "Diagnostic trouble codes by control module" },
    { MBLINK_WORKSPACE_LIVE_DATA, "live-data", "Live Data", "Search, select and favourite live diagnostic parameters" },
    { MBLINK_WORKSPACE_TABLE, "table", "Table", "Dense live values for selected diagnostic parameters" },
    { MBLINK_WORKSPACE_DASHBOARD, "dashboard", "Dashboard", "At-a-glance live diagnostic measurements" },
    { MBLINK_WORKSPACE_GRAPHS, "graphs", "Graphs", "Time-series views for selected diagnostic parameters" },
    { MBLINK_WORKSPACE_LOG, "log", "Log", "Diagnostic session history and exported telemetry" },
    { MBLINK_WORKSPACE_SETTINGS, "settings", "Settings", "Adapter, units, logging and application preferences" }
};

const char *mblink_version(void) { return MBLINK_VERSION; }
bool mblink_self_check(void) { return infiltratr_project_info_is_valid(&mblink_project_info); }
size_t mblink_workspace_section_count(void) { return sizeof(mblink_workspace_sections) / sizeof(mblink_workspace_sections[0]); }
const MblinkWorkspaceSectionDescriptor *mblink_workspace_section_at(size_t index) { return index < mblink_workspace_section_count() ? &mblink_workspace_sections[index] : NULL; }
const MblinkWorkspaceSectionDescriptor *mblink_workspace_section(MblinkWorkspaceSection section)
{
    size_t index;
    for (index = 0U; index < mblink_workspace_section_count(); ++index) if (mblink_workspace_sections[index].section == section) return &mblink_workspace_sections[index];
    return NULL;
}
bool mblink_transport_is_valid(const MblinkTransport *transport)
{
    if (transport == NULL || transport->struct_size < sizeof(*transport) || transport->abi_version != MBLINK_TRANSPORT_ABI) return false;
    return transport->connect != NULL && transport->disconnect != NULL && transport->is_connected != NULL && transport->write != NULL && transport->set_receiver != NULL;
}
