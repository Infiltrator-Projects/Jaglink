// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file jaglink.h
 * @brief Public portable C interface for the JAGLINK diagnostics core.
 */
#ifndef JAGLINK_JAGLINK_H
#define JAGLINK_JAGLINK_H

#include <stdbool.h>
#include <stddef.h>

#include "link/workspace.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Workspace ownership lives in LINK. These aliases preserve the JAGLINK-facing
 * source API while every front end consumes the same shared model.
 */
typedef LinkWorkspaceSection JaglinkWorkspaceSection;
typedef LinkWorkspaceSectionDescriptor JaglinkWorkspaceSectionDescriptor;

#define JAGLINK_WORKSPACE_VEHICLE LINK_WORKSPACE_VEHICLE
#define JAGLINK_WORKSPACE_OBD LINK_WORKSPACE_OBD
#define JAGLINK_WORKSPACE_MODULES LINK_WORKSPACE_MODULES
#define JAGLINK_WORKSPACE_FAULTS LINK_WORKSPACE_FAULTS
#define JAGLINK_WORKSPACE_LIVE_DATA LINK_WORKSPACE_LIVE_DATA
#define JAGLINK_WORKSPACE_TABLE LINK_WORKSPACE_TABLE
#define JAGLINK_WORKSPACE_DASHBOARD LINK_WORKSPACE_DASHBOARD
#define JAGLINK_WORKSPACE_GRAPHS LINK_WORKSPACE_GRAPHS
#define JAGLINK_WORKSPACE_LOG LINK_WORKSPACE_LOG
#define JAGLINK_WORKSPACE_SETTINGS LINK_WORKSPACE_SETTINGS
#define JAGLINK_WORKSPACE_SECTION_COUNT LINK_WORKSPACE_SECTION_COUNT

#define jaglink_workspace_section_count link_workspace_section_count
#define jaglink_workspace_section_at link_workspace_section_at
#define jaglink_workspace_section link_workspace_section

/** Return the semantic version of the linked JAGLINK core. */
const char *jaglink_version(void);

/** Return the build identity used by the current binary. */
const char *jaglink_build_profile(void);

/** Validate the core's shared project-metadata contract. */
bool jaglink_self_check(void);

#ifdef __cplusplus
}
#endif

#endif
