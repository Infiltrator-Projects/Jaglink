// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/isotp.h"
#include "jaglink/uds_services.h"

#include <stdio.h>

int main(void)
{
    const JaglinkUdsServiceDefinition *routine;

    if (JAGLINK_ISOTP_CAN_FD_MAX_DATA_LENGTH != 64U ||
        !jaglink_isotp_can_data_length_is_valid(
            true, JAGLINK_ISOTP_CAN_FD_MAX_DATA_LENGTH)) {
        fputs("JAGLINK CAN-FD ISO-TP facade is incomplete\n", stderr);
        return 1;
    }

    if (jaglink_uds_standard_service_count() !=
        JAGLINK_UDS_STANDARD_SERVICE_COUNT) {
        fputs("JAGLINK UDS service facade has the wrong catalogue size\n", stderr);
        return 1;
    }

    routine = jaglink_uds_standard_service_find(
        JAGLINK_UDS_SERVICE_ROUTINE_CONTROL);
    if (routine == NULL ||
        routine->service != JAGLINK_UDS_SERVICE_ROUTINE_CONTROL ||
        routine->effect != JAGLINK_UDS_SERVICE_EFFECT_STATE_CHANGING) {
        fputs("JAGLINK UDS service facade did not preserve LINK metadata\n",
              stderr);
        return 1;
    }

    puts("JAGLINK LINK facade passed");
    return 0;
}
