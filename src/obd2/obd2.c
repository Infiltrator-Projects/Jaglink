// SPDX-License-Identifier: GPL-3.0-or-later
/** @file obd2.c @brief JAGLINK facade over LINK's shared OBD-II core. */
#include "jaglink/obd2.h"
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IOS
#include "../link/platform/apple/LinkPortableObd2.c"
#endif

size_t jaglink_obd2_pid_definition_count(void)
{
    return link_obd2_pid_definition_count();
}

const JaglinkObd2PidDefinition *jaglink_obd2_pid_definition_at(size_t index)
{
    return link_obd2_pid_definition_at(index);
}

const JaglinkObd2PidDefinition *jaglink_obd2_pid_definition(uint8_t mode,
                                                            uint8_t pid)
{
    return link_obd2_pid_definition(mode, pid);
}
