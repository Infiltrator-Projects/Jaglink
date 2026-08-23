// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/isotp.h"
#include "jaglink/obd2.h"
#include "jaglink/uds_services.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    const JaglinkUdsServiceDefinition *routine;
    JaglinkDtcKnowledge fault;
    char status[LINK_DTC_STATUS_TEXT_LENGTH];

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

    if (jaglink_dtc_catalogue_definition_count() != 9533U ||
        !jaglink_dtc_resolve("P0401", &fault) ||
        !fault.definition_known ||
        strcmp(fault.title,
               "Exhaust Gas Recirculation Flow Insufficient Detected") != 0 ||
        strcmp(fault.category, "powertrain") != 0 ||
        fault.origin != JAGLINK_DTC_ORIGIN_STANDARD_GENERIC) {
        fputs("JAGLINK generic DTC knowledge facade is incomplete\n", stderr);
        return 1;
    }

    if (!jaglink_dtc_resolve("P1450", &fault) ||
        fault.definition_known ||
        fault.origin != JAGLINK_DTC_ORIGIN_MANUFACTURER_SPECIFIC) {
        fputs("JAGLINK must preserve manufacturer-specific DTC ownership\n",
              stderr);
        return 1;
    }

    if (!jaglink_dtc_format_uds_status(0x0dU, status, sizeof(status)) ||
        strstr(status, "Test failed") == NULL ||
        strstr(status, "Pending") == NULL ||
        strstr(status, "Confirmed") == NULL) {
        fputs("JAGLINK shared UDS DTC status semantics are incomplete\n",
              stderr);
        return 1;
    }

    puts("JAGLINK LINK facade passed");
    return 0;
}
