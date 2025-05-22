#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "greatest/greatest.h"
#include "aligned/aligned.h"
#include "utf8_valid.h"

TEST test_utf8_valid(void) {
    const alignas(32) unsigned char *data = (unsigned char *)"we on a world tour نحن في جولة حول العالم nous sommes en tournée mondiale мы в мировом турне a wa lori irin-ajo agbaye 私たちは世界ツアー中です είμαστε σε παγκόσμια περιοδεία በአለም ጉብኝት ላይ ነን jesteśmy w trasie dookoła świata 우리는 세계 여행을 하고 있어요 យើងកំពុងធ្វើដំណើរជុំវិញពិភពលោក ನಾವು ವಿಶ್ವ ಪ್ರವಾಸದಲ್ಲಿದ್ದೇವೆ. մենք համաշխարհային շրջագայության մեջ ենք míele xexeame katã ƒe tsaɖiɖi aɖe dzi เรากำลังทัวร์รอบโลก हम विश्व भ्रमण पर हैं pachantinpi puriypin kashanchis אנחנו בסיבוב הופעות עולמי kaulâh bâdâ è tur dhunnya qegħdin fuq tour tad-dinja ང་ཚོ་འཛམ་གླིང་སྐོར་བསྐྱོད་བྱེད་བཞིན་ཡོད།";
    size_t len = strlen((const char *)data);
    size_t error_index;
    bool valid = utf8_valid(data, len, &error_index);
    ASSERT(valid);

    const alignas(32) unsigned char *invalid = (unsigned char *)"abcdefghijklmnopqrstuvwxyzabcd\x80\x01";
    len = strlen((const char *)invalid);
    valid = utf8_valid(invalid, len, &error_index);
    ASSERT(!valid);
    ASSERT(error_index == 30);

    PASS();
}


/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line options, initialization. */

    RUN_TEST(test_utf8_valid);

    GREATEST_MAIN_END();        /* display results */
}