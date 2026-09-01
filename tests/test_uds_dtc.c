// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/uds_dtc.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed: %s at %s:%d\n", #expr, __FILE__, __LINE__); \
    return 1; } } while (0)

static int test_complete_report_facade(void)
{
    JaglinkUdsDtcInformationRequest request =
        JAGLINK_UDS_DTC_INFORMATION_REQUEST_INIT;
    JaglinkUdsDtcInformationResponse response;
    uint8_t buffer[8];
    size_t written = 0U;
    const uint8_t wwh_response[] = {
        0x59U, 0x55U, 0x33U, 0xffU, 0x04U,
        0x12U, 0x34U, 0x56U, 0x09U
    };

    CHECK(jaglink_uds_dtc_report_definition_count() ==
          JAGLINK_UDS_DTC_REPORT_SUBFUNCTION_COUNT);
    CHECK(JAGLINK_UDS_DTC_REPORT_SUBFUNCTION_COUNT == 27U);
    CHECK(jaglink_uds_dtc_report_definition(
              JAGLINK_UDS_DTC_REPORT_USER_MEMORY_EXT_DATA_BY_DTC_NUMBER) !=
          NULL);

    request.subfunction = JAGLINK_UDS_DTC_REPORT_WWH_OBD_BY_MASK_RECORD;
    request.functional_group_identifier = 0x33U;
    request.status_mask = 0xa5U;
    request.severity_mask = 0xc0U;
    CHECK(jaglink_uds_build_read_dtc_information_request(
              &request, buffer, sizeof(buffer), &written) ==
          JAGLINK_UDS_RESULT_OK);
    CHECK(written == 5U);
    CHECK(buffer[0] == 0x19U && buffer[1] == 0x42U);
    CHECK(buffer[2] == 0x33U && buffer[3] == 0xa5U &&
          buffer[4] == 0xc0U);

    CHECK(jaglink_uds_decode_read_dtc_information_response(
              JAGLINK_UDS_DTC_REPORT_WWH_OBD_WITH_PERMANENT_STATUS,
              wwh_response, sizeof(wwh_response), &response) ==
          JAGLINK_UDS_RESULT_OK);
    CHECK(response.functional_group_identifier_available);
    CHECK(response.functional_group_identifier == 0x33U);
    CHECK(response.records_length == 4U);
    return 0;
}

static int test_build_request(void)
{
    uint8_t request[4] = { 0xa5U, 0xa5U, 0xa5U, 0xa5U };
    size_t written = 99U;

    CHECK(jaglink_uds_build_report_dtcs_by_status_mask_request(
              JAGLINK_UDS_DTC_STATUS_MASK_ALL,
              request, sizeof(request), &written) == JAGLINK_UDS_RESULT_OK);
    CHECK(written == 3U);
    CHECK(request[0] == 0x19U);
    CHECK(request[1] == 0x02U);
    CHECK(request[2] == 0xffU);

    request[0] = 0xa5U;
    written = 99U;
    CHECK(jaglink_uds_build_report_dtcs_by_status_mask_request(
              JAGLINK_UDS_DTC_STATUS_MASK_ALL,
              request, 2U, &written) == JAGLINK_UDS_RESULT_BUFFER_TOO_SMALL);
    CHECK(request[0] == 0U);
    CHECK(written == 0U);

    CHECK(jaglink_uds_build_report_dtcs_by_status_mask_request(
              0U, request, sizeof(request), &written) ==
          JAGLINK_UDS_RESULT_INVALID_ARGUMENT);
    return 0;
}

static int test_decode_records(void)
{
    const uint8_t pdu[] = {
        0x59U, 0x02U, 0xffU,
        0x12U, 0x34U, 0x56U, 0x09U,
        0xabU, 0xcdU, 0xefU, 0x28U
    };
    JaglinkUdsDtcList list;
    char text[7];

    memset(&list, 0xa5, sizeof(list));
    CHECK(jaglink_uds_decode_report_dtcs_by_status_mask_response(
              pdu, sizeof(pdu), &list) == JAGLINK_UDS_RESULT_OK);
    CHECK(list.availability_mask == 0xffU);
    CHECK(list.count == 2U);
    CHECK(!list.truncated);
    CHECK(list.records[0].code == UINT32_C(0x123456));
    CHECK(list.records[0].status == 0x09U);
    CHECK(list.records[1].code == UINT32_C(0xabcdef));
    CHECK(list.records[1].status == 0x28U);
    CHECK(jaglink_uds_dtc_status_matches(
        &list.records[0], JAGLINK_UDS_DTC_STATUS_CONFIRMED_DTC));
    CHECK(!jaglink_uds_dtc_status_matches(
        &list.records[0], JAGLINK_UDS_DTC_STATUS_PENDING_DTC));
    CHECK(jaglink_uds_dtc_format_hex(list.records[1].code, text, sizeof(text)));
    CHECK(strcmp(text, "ABCDEF") == 0);
    return 0;
}

static int test_empty_and_invalid_responses(void)
{
    const uint8_t empty[] = { 0x59U, 0x02U, 0xffU };
    const uint8_t wrong_subfunction[] = { 0x59U, 0x0aU, 0xffU };
    const uint8_t truncated[] = { 0x59U, 0x02U };
    const uint8_t partial_record[] = {
        0x59U, 0x02U, 0xffU, 0x12U, 0x34U
    };
    const uint8_t negative[] = { 0x7fU, 0x19U, 0x31U };
    JaglinkUdsDtcList list;

    CHECK(jaglink_uds_decode_report_dtcs_by_status_mask_response(
              empty, sizeof(empty), &list) == JAGLINK_UDS_RESULT_OK);
    CHECK(list.count == 0U);

    CHECK(jaglink_uds_decode_report_dtcs_by_status_mask_response(
              wrong_subfunction, sizeof(wrong_subfunction), &list) ==
          JAGLINK_UDS_RESULT_UNEXPECTED_RESPONSE);
    CHECK(jaglink_uds_decode_report_dtcs_by_status_mask_response(
              truncated, sizeof(truncated), &list) ==
          JAGLINK_UDS_RESULT_MALFORMED_PDU);
    CHECK(jaglink_uds_decode_report_dtcs_by_status_mask_response(
              partial_record, sizeof(partial_record), &list) ==
          JAGLINK_UDS_RESULT_MALFORMED_PDU);
    CHECK(jaglink_uds_decode_report_dtcs_by_status_mask_response(
              negative, sizeof(negative), &list) ==
          JAGLINK_UDS_RESULT_NEGATIVE_RESPONSE);
    return 0;
}

static int test_formatter_guards(void)
{
    char text[7] = "bad";

    CHECK(!jaglink_uds_dtc_format_hex(UINT32_C(0x01000000), text, sizeof(text)));
    CHECK(text[0] == '\0');
    CHECK(!jaglink_uds_dtc_format_hex(UINT32_C(0x123456), text, 6U));
    CHECK(text[0] == '\0');
    return 0;
}

int main(void)
{
    if (test_complete_report_facade() != 0) return 1;
    if (test_build_request() != 0) return 1;
    if (test_decode_records() != 0) return 1;
    if (test_empty_and_invalid_responses() != 0) return 1;
    if (test_formatter_guards() != 0) return 1;
    puts("UDS DTC tests passed");
    return 0;
}
