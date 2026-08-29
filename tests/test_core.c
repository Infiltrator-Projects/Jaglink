// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/jaglink.h"
#include "jaglink/project_info.h"
#include "jaglink/transport.h"

#include "infiltratr/core.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef JAGLINK_TEST_EXPECTED_VERSION
#error "JAGLINK_TEST_EXPECTED_VERSION must be supplied by the build system"
#endif

static JaglinkTransportStatus mock_connect(void *context)
{
    (void)context;
    return JAGLINK_TRANSPORT_OK;
}

static void mock_disconnect(void *context)
{
    (void)context;
}

static bool mock_is_connected(void *context)
{
    (void)context;
    return true;
}

static JaglinkTransportStatus mock_write(void *context,
                                        const uint8_t *data,
                                        size_t size)
{
    (void)context;
    return (data != NULL && size > 0U) ? JAGLINK_TRANSPORT_OK
                                       : JAGLINK_TRANSPORT_INVALID_ARGUMENT;
}

static void mock_set_receiver(void *context,
                              JaglinkTransportReceiveFn receiver,
                              void *receiver_context)
{
    (void)context;
    (void)receiver;
    (void)receiver_context;
}

static bool check(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "jaglink-core-test: %s\n", message);
    }
    return condition;
}

static bool check_workspace(void)
{
    size_t index;
    bool passed = true;

    if (!check(jaglink_workspace_section_count() ==
                   (size_t)JAGLINK_WORKSPACE_SECTION_COUNT,
               "workspace section count mismatch")) {
        passed = false;
    }

    for (index = 0U; index < jaglink_workspace_section_count(); ++index) {
        const JaglinkWorkspaceSectionDescriptor *descriptor =
            jaglink_workspace_section_at(index);
        if (!check(descriptor != NULL,
                   "workspace descriptor missing")) {
            passed = false;
            continue;
        }
        if (!check(descriptor->section == (JaglinkWorkspaceSection)index,
                   "workspace section order is not stable") ||
            !check(descriptor->key != NULL && descriptor->key[0] != '\0',
                   "workspace key missing") ||
            !check(descriptor->title != NULL && descriptor->title[0] != '\0',
                   "workspace title missing") ||
            !check(descriptor->summary != NULL && descriptor->summary[0] != '\0',
                   "workspace summary missing") ||
            !check(jaglink_workspace_section(descriptor->section) == descriptor,
                   "workspace identifier lookup mismatch")) {
            passed = false;
        }
    }

    if (!check(jaglink_workspace_section_at(jaglink_workspace_section_count()) == NULL,
               "out-of-range workspace index should fail") ||
        !check(jaglink_workspace_section(JAGLINK_WORKSPACE_SECTION_COUNT) == NULL,
               "out-of-range workspace identifier should fail")) {
        passed = false;
    }

    return passed;
}

int main(void)
{
    bool passed = true;
    JaglinkTransport transport = JAGLINK_TRANSPORT_INIT;
    const InfiltratrProjectInfo *project_info = jaglink_project_info();
    static const uint8_t probe[] = { 'A', 'T', 'I', '\r' };

    if (!check(strcmp(jaglink_version(), JAGLINK_TEST_EXPECTED_VERSION) == 0,
               "linked JAGLINK version does not match build version")) {
        passed = false;
    }
    if (!check(jaglink_self_check(), "project identity validation failed")) {
        passed = false;
    }
    if (!check(project_info != NULL &&
               infiltratr_project_info_is_valid(project_info),
               "project info record is invalid") ||
        !check(strcmp(project_info->program_name, "JAGLINK") == 0,
               "project info program name mismatch") ||
        !check(strcmp(project_info->version, JAGLINK_TEST_EXPECTED_VERSION) == 0,
               "project info version mismatch") ||
        !check(strcmp(project_info->license_id, "GPL-3.0-or-later") == 0,
               "project info licence mismatch") ||
        !check(strcmp(project_info->icon_name, "jaglink") == 0,
               "project info icon mismatch") ||
        !check(strcmp(project_info->source_id, "Infiltrator-Projects/Jaglink") == 0,
               "project info source identity mismatch") ||
        !check(strcmp(project_info->build_profile, jaglink_build_profile()) == 0,
               "project info build profile mismatch")) {
        passed = false;
    }
    if (!check_workspace()) {
        passed = false;
    }
    if (!check(!jaglink_transport_is_valid(&transport),
               "empty transport should be invalid")) {
        passed = false;
    }

    transport.connect = mock_connect;
    transport.disconnect = mock_disconnect;
    transport.is_connected = mock_is_connected;
    transport.write = mock_write;
    transport.set_receiver = mock_set_receiver;

    if (!check(jaglink_transport_is_valid(&transport),
               "complete transport should be valid")) {
        passed = false;
    }

    transport.abi_version = JAGLINK_TRANSPORT_ABI + 1U;
    if (!check(!jaglink_transport_is_valid(&transport),
               "unknown transport ABI should be rejected")) {
        passed = false;
    }
    transport.abi_version = JAGLINK_TRANSPORT_ABI;

    if (!check(transport.connect(transport.context) == JAGLINK_TRANSPORT_OK,
               "mock connect failed")) {
        passed = false;
    }
    if (!check(transport.is_connected(transport.context),
               "mock transport did not report connected")) {
        passed = false;
    }
    if (!check(transport.write(transport.context, probe, sizeof(probe)) ==
                   JAGLINK_TRANSPORT_OK,
               "mock write failed")) {
        passed = false;
    }

    transport.disconnect(transport.context);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
