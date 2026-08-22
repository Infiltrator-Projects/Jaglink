/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "jaglink/discover.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char *path = "jaglink-evidence-test.jsonl";
    const unsigned char bytes[] = {0x01U, 0x09U, 0x0AU, 0xFFU};
    char buffer[2048];
    size_t used;
    FILE *file;
    jaglink_evidence_writer *writer = jaglink_evidence_open(path);

    assert(writer != NULL);
    assert(jaglink_evidence_write_frame(writer, 123456789ULL, "rx", "CAN", 0x7E8U,
                                        bytes, sizeof(bytes), "operator \"note\"") == 0);
    assert(jaglink_evidence_write_annotation(writer, 123456790ULL, "line1\nline2") == 0);
    assert(jaglink_evidence_flush(writer) == 0);
    jaglink_evidence_close(writer);

    file = fopen(path, "rb");
    assert(file != NULL);
    used = fread(buffer, 1U, sizeof(buffer) - 1U, file);
    buffer[used] = '\0';
    (void)fclose(file);
    (void)remove(path);

    assert(strstr(buffer, "\"timestamp_ns\":123456789") != NULL);
    assert(strstr(buffer, "\"can_id\":\"0x000007E8\"") != NULL);
    assert(strstr(buffer, "\"data\":\"01090AFF\"") != NULL);
    assert(strstr(buffer, "operator \\\"note\\\"") != NULL);
    assert(strstr(buffer, "line1\\nline2") != NULL);
    return 0;
}
