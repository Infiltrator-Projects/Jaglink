// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file jaglink.h
 * @brief Public portable C interface for the JAGLINK diagnostics core.
 */
#ifndef JAGLINK_JAGLINK_H
#define JAGLINK_JAGLINK_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Stable top-level diagnostic workspace shared by every front end. */
typedef enum JaglinkWorkspaceSection {
    JAGLINK_WORKSPACE_VEHICLE = 0,
    JAGLINK_WORKSPACE_MODULES,
    JAGLINK_WORKSPACE_FAULTS,
    JAGLINK_WORKSPACE_LIVE_DATA,
    JAGLINK_WORKSPACE_TABLE,
    JAGLINK_WORKSPACE_DASHBOARD,
    JAGLINK_WORKSPACE_GRAPHS,
    JAGLINK_WORKSPACE_LOG,
    JAGLINK_WORKSPACE_SETTINGS,
    JAGLINK_WORKSPACE_SECTION_COUNT
} JaglinkWorkspaceSection;

/** Shared section metadata. Platform shells own only presentation details. */
typedef struct JaglinkWorkspaceSectionDescriptor {
    JaglinkWorkspaceSection section;
    const char *key;
    const char *title;
    const char *summary;
} JaglinkWorkspaceSectionDescriptor;

/** Return the semantic version of the linked JAGLINK core. */
const char *jaglink_version(void);

/** Validate the core's shared project-metadata contract. */
bool jaglink_self_check(void);

/** Return the number of stable top-level diagnostic workspace sections. */
size_t jaglink_workspace_section_count(void);

/** Return shared metadata for a workspace section index, or NULL if invalid. */
const JaglinkWorkspaceSectionDescriptor *jaglink_workspace_section_at(size_t index);

/** Return shared metadata for a workspace section identifier, or NULL if invalid. */
const JaglinkWorkspaceSectionDescriptor *jaglink_workspace_section(
    JaglinkWorkspaceSection section);

#ifdef __cplusplus
}
#endif

#endif
