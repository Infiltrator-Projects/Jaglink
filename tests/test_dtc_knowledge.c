// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/obd2.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define EXPECTED_OBDEX_DTC_COUNT 9533U
#define EXPECTED_OBDEX_SNAPSHOT "bc58b0eb7273226a1aabae98e956b70b8362bda1"

static int failures;

static void check(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

static void check_known(const char *code,
                        JaglinkDtcSystem system,
                        const char *message)
{
    JaglinkDtcKnowledge knowledge;
    check(jaglink_dtc_resolve(code, &knowledge), message);
    check(knowledge.definition_known, message);
    check(knowledge.system == system, message);
    check(knowledge.origin == JAGLINK_DTC_ORIGIN_STANDARD_GENERIC, message);
    check(knowledge.source == JAGLINK_DTC_SOURCE_STANDARD_GENERIC, message);
    check(knowledge.title[0] != '\0', message);
    check(knowledge.category[0] != '\0', message);
}

int main(void)
{
    JaglinkDtcKnowledge knowledge;
    char status[LINK_DTC_STATUS_TEXT_LENGTH];

    check(jaglink_dtc_catalogue_definition_count() == EXPECTED_OBDEX_DTC_COUNT,
          "complete shared OBDex generic DTC count");
    check(strcmp(jaglink_dtc_catalogue_snapshot(),
                 EXPECTED_OBDEX_SNAPSHOT) == 0,
          "shared OBDex snapshot");

    check_known("P0420", JAGLINK_DTC_SYSTEM_POWERTRAIN,
                "P0 generic definition");
    check_known("P2002", JAGLINK_DTC_SYSTEM_POWERTRAIN,
                "P2 generic definition");
    check_known("B0001", JAGLINK_DTC_SYSTEM_BODY,
                "B0 generic definition");
    check_known("C0031", JAGLINK_DTC_SYSTEM_CHASSIS,
                "C0 generic definition");
    check_known("U0100", JAGLINK_DTC_SYSTEM_NETWORK,
                "U0 generic definition");

    check(jaglink_dtc_resolve("P1450", &knowledge),
          "valid manufacturer-specific code");
    check(!knowledge.definition_known,
          "manufacturer code stays unmapped by shared generic catalogue");
    check(knowledge.origin == JAGLINK_DTC_ORIGIN_MANUFACTURER_SPECIFIC,
          "manufacturer classification preserved");

    check(jaglink_dtc_format_uds_status(
              UINT8_C(0x0d), status, sizeof(status)),
          "format UDS status");
    check(strstr(status, "Test failed") != NULL &&
          strstr(status, "Pending") != NULL &&
          strstr(status, "Confirmed") != NULL,
          "UDS status meanings");

    if (failures != 0) {
        fprintf(stderr, "%d JAGLINK DTC knowledge test(s) failed\n", failures);
        return 1;
    }
    puts("JAGLINK shared DTC knowledge tests passed");
    return 0;
}
