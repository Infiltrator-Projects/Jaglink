// SPDX-License-Identifier: GPL-3.0-or-later
#include "jaglink/isotp.h"
#include "jaglink/uds_services.h"
#include "jaglink/obd2.h"

#include <string.h>

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

    if (jaglink_obd2_pid_definition_count() != 234U) {
        fputs("JAGLINK did not inherit the completed LINK J1979 catalogue\n", stderr);
        return 1;
    }
    if (jaglink_obd2_parameter_identifier_namespace_count(0x01U) != 256U ||
        jaglink_obd2_parameter_identifier_namespace_count(0x05U) != 256U ||
        jaglink_obd2_parameter_identifier_namespace_count(0x06U) != 256U ||
        jaglink_obd2_parameter_identifier_namespace_count(0x09U) != 256U ||
        strcmp(jaglink_obd2_obdonuds_revision(), "J1979-2_202604") != 0) {
        fputs("JAGLINK did not inherit complete parameterized OBD namespaces\n", stderr);
        return 1;
    }
    {
        const JaglinkElm327ProtocolDefinition *legacy =
            jaglink_elm327_protocol_definition(JAGLINK_ELM327_PROTOCOL_SAE_J1850_PWM);
        char protocol_command[8];
        if (jaglink_elm327_protocol_definition_count() != 13U ||
            legacy == NULL || !legacy->classic_j1979_obd ||
            jaglink_elm327_build_set_protocol_command(
                JAGLINK_ELM327_PROTOCOL_ISO_9141_2,
                protocol_command, sizeof(protocol_command)) != JAGLINK_ELM327_RESULT_OK ||
            strcmp(protocol_command, "ATSP3") != 0) {
            fputs("JAGLINK did not inherit the legacy OBD transport model\n", stderr);
            return 1;
        }
    }
    if (jaglink_dtc_catalogue_definition_count() != 9533U ||
        strcmp(jaglink_dtc_range_model_revision(), "J2012_202509") != 0 ||
        strcmp(jaglink_dtc_catalogue_audit_revision(), "J2012DA_202607") != 0) {
        fputs("JAGLINK did not inherit the audited LINK J2012 catalogue\n", stderr);
        return 1;
    }
    {
        char command[16];
        uint16_t did = 0U;
        if (jaglink_obd2_obdonuds_pid_to_did(UINT16_C(0x0100), &did) !=
                JAGLINK_OBD2_RESULT_OK ||
            did != UINT16_C(0xf500) ||
            jaglink_obd2_build_obdonuds_pid_request(
                UINT16_C(0x0100), command, sizeof(command)) !=
                JAGLINK_OBD2_RESULT_OK ||
            strcmp(command, "22F500") != 0) {
            fputs("JAGLINK J1979-2 OBDonUDS facade is incomplete\n", stderr);
            return 1;
        }
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
