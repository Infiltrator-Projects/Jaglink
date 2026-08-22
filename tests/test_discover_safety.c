/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "jaglink/discover.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

static void expect(uint8_t service, jaglink_safety_decision decision, jaglink_safety_reason reason)
{
    const uint8_t request[] = { service, 0x00U, 0x00U };
    const jaglink_safety_result got = jaglink_safety_classify(request, sizeof(request));
    assert(got.service == service);
    assert(got.decision == decision);
    assert(got.reason == reason);
}

int main(void)
{
    static const uint8_t allowed_obd[] = {0x01U, 0x03U, 0x07U, 0x09U, 0x0AU};
    static const uint8_t programming[] = {0x34U, 0x35U, 0x36U, 0x37U};
    size_t i;
    for (i = 0U; i < sizeof(allowed_obd); ++i)
        expect(allowed_obd[i], JAGLINK_SAFETY_ALLOW_READ_ONLY, JAGLINK_SAFETY_REASON_ALLOWED_OBD_READ);
    expect(0x19U, JAGLINK_SAFETY_ALLOW_READ_ONLY, JAGLINK_SAFETY_REASON_ALLOWED_UDS_READ);
    expect(0x22U, JAGLINK_SAFETY_ALLOW_READ_ONLY, JAGLINK_SAFETY_REASON_ALLOWED_UDS_READ);
    expect(0x04U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_DTC_CLEAR);
    expect(0x14U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_DTC_CLEAR);
    expect(0x11U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_ECU_RESET);
    expect(0x27U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_SECURITY_ACCESS);
    expect(0x29U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_SECURITY_ACCESS);
    expect(0x31U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_ROUTINE_CONTROL);
    expect(0x2EU, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_WRITE_OR_CONTROL);
    expect(0x3DU, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_WRITE_OR_CONTROL);
    for (i = 0U; i < sizeof(programming); ++i)
        expect(programming[i], JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_PROGRAMMING);
    expect(0x99U, JAGLINK_SAFETY_BLOCK, JAGLINK_SAFETY_REASON_DENY_BY_DEFAULT);
    assert(jaglink_safety_classify(NULL, 0U).decision == JAGLINK_SAFETY_BLOCK);
    return 0;
}
