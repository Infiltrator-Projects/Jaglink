// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/scheduler.h"

JaglinkSchedulerResult jaglink_scheduler_configure_standard_obd2(
    JaglinkScheduler *scheduler,
    const JaglinkObd2PidSet *supported,
    uint64_t first_due_ms)
{
    if (supported == NULL) {
        return JAGLINK_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }
    return link_scheduler_configure_standard_obd2_bits(
        scheduler, supported->bits, first_due_ms);
}
