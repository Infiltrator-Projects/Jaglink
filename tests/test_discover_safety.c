/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "jaglink/discover.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static int expect(uint8_t service, jaglink_safety_decision decision, jaglink_safety_reason reason)
{
    const uint8_t request[] = { service, 0x00U, 0x00U };
    const jaglink_safety_result got = jaglink_safety_classify(request, sizeof(request));

    if (got.service != service || got.decision != decision || got.reason != reason) {
        (void)fprintf(stderr,
                      "service 0x%02X: got decision=%d reason=%d service=0x%02X; expected decision=%d reason=%d\n",
                      (unsigned int)service,
                      (int)got.decision,
                      (int)got.reason,
                      (unsigned int)got.service,
                      (int)decision,
                      (int)reason);
        return 1;
    }
    return 0;
}

int main(void)
{
    static const uint8_t allowed_obd[] = {0x01U, 0x03U, 0x07U, 0x09U, 0x0AU};
    static const uint8_t programming[] = {0x34U, 0x35U, 0x36U, 0x37U};
    size_t i;
    int failures = 0;
    jaglink_safety_result empty;

    for (i = 0U; i < sizeof(allowed_obd) / sizeof(allowed_obd[0]); ++i) {
        failures += expect(allowed_obd[i], JAGLINK_SAFETY_ALLOW_READ_ONLY,
                           JAGLINK_SAFETY_REASON_ALLOWED_OBD_READ);
    }
    failures += expect(0x19U, JAGLINK_SAFETY_ALLOW_READ_ONLY, JAGLINK_SAFETY_REASON_ALLOWED_UDS_READ);
    failures += expect(0x22U, JAGLINK_SAFETY_ALLOW_READ_ONLY, JAGLINK_SAFETY_REASON_ALLOWED_UDS_READ);
    failures += expect(0x04U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_DTC_CLEAR);
    failures += expect(0x14U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_DTC_CLEAR);
    failures += expect(0x11U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_ECU_RESET);
    failures += expect(0x27U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_SECURITY_ACCESS);
    failures += expect(0x29U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_SECURITY_ACCESS);
    failures += expect(0x31U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_ROUTINE_CONTROL);
    failures += expect(0x2EU, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_WRITE_OR_CONTROL);
    failures += expect(0x3DU, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_WRITE_OR_CONTROL);
    for (i = 0U; i < sizeof(programming) / sizeof(programming[0]); ++i) {
        failures += expect(programming[i], JAGLINK_SAFETY_BLOCK,
                           JAGLINK_SAFETY_REASON_PROGRAMMING);
    }
    failures += expect(0x99U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_DENY_BY_DEFAULT);

    empty = jaglink_safety_classify(NULL, 0U);
    if (empty.decision != JAGLINK_SAFETY_BLOCK || empty.reason != JAGLINK_SAFETY_REASON_EMPTY_REQUEST) {
        (void)fprintf(stderr, "empty request was not blocked with EMPTY_REQUEST reason\n");
        ++failures;
    }

    return failures == 0 ? 0 : 1;
}
