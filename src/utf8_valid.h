#ifndef UTF8_VALID_H
#define UTF8_VALID_H

#include "simde_avx2/avx2.h"

/*
 * http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf - page 94
 *
 * Table 3-7. Well-Formed UTF-8 Byte Sequences
 *
 * +--------------------+------------+-------------+------------+-------------+
 * | Code Points        | First Byte | Second Byte | Third Byte | Fourth Byte |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0000..U+007F     | 00..7F     |             |            |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0080..U+07FF     | C2..DF     | 80..BF      |            |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+0800..U+0FFF     | E0         | A0..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+1000..U+CFFF     | E1..EC     | 80..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+D000..U+D7FF     | ED         | 80..9F      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+E000..U+FFFF     | EE..EF     | 80..BF      | 80..BF     |             |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+10000..U+3FFFF   | F0         | 90..BF      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+40000..U+FFFFF   | F1..F3     | 80..BF      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 * | U+100000..U+10FFFF | F4         | 80..8F      | 80..BF     | 80..BF      |
 * +--------------------+------------+-------------+------------+-------------+
 */

bool utf8_valid_naive(const unsigned char *data, size_t len, size_t *error_index)
{
    size_t err_idx = 0;

    while (len) {
        int bytes;
        const unsigned char byte1 = data[0];

        /* 00..7F */
        if (byte1 <= 0x7F) {
            bytes = 1;
        /* C2..DF, 80..BF */
        } else if (len >= 2 && byte1 >= 0xC2 && byte1 <= 0xDF &&
                (signed char)data[1] <= (signed char)0xBF) {
            bytes = 2;
        } else if (len >= 3) {
            const unsigned char byte2 = data[1];

            /* Is byte2, byte3 between 0x80 ~ 0xBF */
            const int byte2_ok = (signed char)byte2 <= (signed char)0xBF;
            const int byte3_ok = (signed char)data[2] <= (signed char)0xBF;

            if (byte2_ok && byte3_ok &&
                     /* E0, A0..BF, 80..BF */
                    ((byte1 == 0xE0 && byte2 >= 0xA0) ||
                     /* E1..EC, 80..BF, 80..BF */
                     (byte1 >= 0xE1 && byte1 <= 0xEC) ||
                     /* ED, 80..9F, 80..BF */
                     (byte1 == 0xED && byte2 <= 0x9F) ||
                     /* EE..EF, 80..BF, 80..BF */
                     (byte1 >= 0xEE && byte1 <= 0xEF))) {
                bytes = 3;
            } else if (len >= 4) {
                /* Is byte4 between 0x80 ~ 0xBF */
                const int byte4_ok = (signed char)data[3] <= (signed char)0xBF;

                if (byte2_ok && byte3_ok && byte4_ok &&
                         /* F0, 90..BF, 80..BF, 80..BF */
                        ((byte1 == 0xF0 && byte2 >= 0x90) ||
                         /* F1..F3, 80..BF, 80..BF, 80..BF */
                         (byte1 >= 0xF1 && byte1 <= 0xF3) ||
                         /* F4, 80..8F, 80..BF, 80..BF */
                         (byte1 == 0xF4 && byte2 <= 0x8F))) {
                    bytes = 4;
                } else {
                    *error_index = err_idx;
                    return false;
                }
            } else {
                *error_index = err_idx;
                return false;
            }
        } else {
            *error_index = err_idx;
            return false;
        }

        len -= bytes;
        err_idx += bytes;
        data += bytes;
    }

    return true;
}

/*
 * Map high nibble of "First Byte" to legal character length minus 1
 * 0x00 ~ 0xBF --> 0
 * 0xC0 ~ 0xDF --> 1
 * 0xE0 ~ 0xEF --> 2
 * 0xF0 ~ 0xFF --> 3
 */
static const int8_t _first_len_tbl[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
};

/* Map "First Byte" to 8-th item of range table (0xC2 ~ 0xF4) */
static const int8_t _first_range_tbl[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8,
};


/*
 * Range table, map range index to min and max values
 * Index 0    : 00 ~ 7F (First Byte, ascii)
 * Index 1,2,3: 80 ~ BF (Second, Third, Fourth Byte)
 * Index 4    : A0 ~ BF (Second Byte after E0)
 * Index 5    : 80 ~ 9F (Second Byte after ED)
 * Index 6    : 90 ~ BF (Second Byte after F0)
 * Index 7    : 80 ~ 8F (Second Byte after F4)
 * Index 8    : C2 ~ F4 (First Byte, non ascii)
 * Index 9~15 : illegal: i >= 127 && i <= -128
 */
static const int8_t _range_min_tbl[] = {
    0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80,
    0xC2, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
    0x00, 0x80, 0x80, 0x80, 0xA0, 0x80, 0x90, 0x80,
    0xC2, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
};
static const int8_t _range_max_tbl[] = {
    0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F,
    0xF4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x7F, 0xBF, 0xBF, 0xBF, 0xBF, 0x9F, 0xBF, 0x8F,
    0xF4, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
};

/*
 * Tables for fast handling of four special First Bytes(E0,ED,F0,F4), after
 * which the Second Byte are not 80~BF. It contains "range index adjustment".
 * +------------+---------------+------------------+----------------+
 * | First Byte | original range| range adjustment | adjusted range |
 * +------------+---------------+------------------+----------------+
 * | E0         | 2             | 2                | 4              |
 * +------------+---------------+------------------+----------------+
 * | ED         | 2             | 3                | 5              |
 * +------------+---------------+------------------+----------------+
 * | F0         | 3             | 3                | 6              |
 * +------------+---------------+------------------+----------------+
 * | F4         | 4             | 4                | 8              |
 * +------------+---------------+------------------+----------------+
 */
/* index1 -> E0, index14 -> ED */
static const int8_t _df_ee_tbl[] = {
    0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
    0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
};
/* index1 -> F0, index5 -> F4 */
static const int8_t _ef_fe_tbl[] = {
    0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


static inline simde__m256i push_last_byte_of_a_to_b(simde__m256i a, simde__m256i b) {
  return simde_mm256_alignr_epi8(b, simde_mm256_permute2x128_si256(a, b, 0x21), 15);
}

static inline simde__m256i push_last_2bytes_of_a_to_b(simde__m256i a, simde__m256i b) {
  return simde_mm256_alignr_epi8(b, simde_mm256_permute2x128_si256(a, b, 0x21), 14);
}

static inline simde__m256i push_last_3bytes_of_a_to_b(simde__m256i a, simde__m256i b) {
  return simde_mm256_alignr_epi8(b, simde_mm256_permute2x128_si256(a, b, 0x21), 13);
}

bool utf8_valid(const unsigned char *data, size_t len, size_t *error_index) {
    int err_idx = 1;

    if (len >= 32) {
        simde__m256i prev_input = simde_mm256_set1_epi8(0);
        simde__m256i prev_first_len = simde_mm256_set1_epi8(0);

        /* Cached tables */
        const simde__m256i first_len_tbl =
            simde_mm256_loadu_si256((const simde__m256i *)_first_len_tbl);
        const simde__m256i first_range_tbl =
            simde_mm256_loadu_si256((const simde__m256i *)_first_range_tbl);
        const simde__m256i range_min_tbl =
            simde_mm256_loadu_si256((const simde__m256i *)_range_min_tbl);
        const simde__m256i range_max_tbl =
            simde_mm256_loadu_si256((const simde__m256i *)_range_max_tbl);
        const simde__m256i df_ee_tbl =
            simde_mm256_loadu_si256((const simde__m256i *)_df_ee_tbl);
        const simde__m256i ef_fe_tbl =
            simde_mm256_loadu_si256((const simde__m256i *)_ef_fe_tbl);

        while (len >= 32) {
            const simde__m256i input = simde_mm256_loadu_si256((const simde__m256i *)data);

            /* high_nibbles = input >> 4 */
            const simde__m256i high_nibbles =
                simde_mm256_and_si256(simde_mm256_srli_epi16(input, 4), simde_mm256_set1_epi8(0x0F));

            /* first_len = legal character length minus 1 */
            /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
            /* first_len = first_len_tbl[high_nibbles] */
            simde__m256i first_len = simde_mm256_shuffle_epi8(first_len_tbl, high_nibbles);

            /* First Byte: set range index to 8 for bytes within 0xC0 ~ 0xFF */
            /* range = first_range_tbl[high_nibbles] */
            simde__m256i range = simde_mm256_shuffle_epi8(first_range_tbl, high_nibbles);

            /* Second Byte: set range index to first_len */
            /* 0 for 00~7F, 1 for C0~DF, 2 for E0~EF, 3 for F0~FF */
            /* range |= (first_len, prev_first_len) << 1 byte */
            range = simde_mm256_or_si256(
                    range, push_last_byte_of_a_to_b(prev_first_len, first_len));

            /* Third Byte: set range index to saturate_sub(first_len, 1) */
            /* 0 for 00~7F, 0 for C0~DF, 1 for E0~EF, 2 for F0~FF */
            simde__m256i tmp1, tmp2;

            /* tmp1 = (first_len, prev_first_len) << 2 bytes */
            tmp1 = push_last_2bytes_of_a_to_b(prev_first_len, first_len);
            /* tmp2 = saturate_sub(tmp1, 1) */
            tmp2 = simde_mm256_subs_epu8(tmp1, simde_mm256_set1_epi8(1));

            /* range |= tmp2 */
            range = simde_mm256_or_si256(range, tmp2);

            /* Fourth Byte: set range index to saturate_sub(first_len, 2) */
            /* 0 for 00~7F, 0 for C0~DF, 0 for E0~EF, 1 for F0~FF */
            /* tmp1 = (first_len, prev_first_len) << 3 bytes */
            tmp1 = push_last_3bytes_of_a_to_b(prev_first_len, first_len);
            /* tmp2 = saturate_sub(tmp1, 2) */
            tmp2 = simde_mm256_subs_epu8(tmp1, simde_mm256_set1_epi8(2));
            /* range |= tmp2 */
            range = simde_mm256_or_si256(range, tmp2);

            /*
             * Now we have below range indices caluclated
             * Correct cases:
             * - 8 for C0~FF
             * - 3 for 1st byte after F0~FF
             * - 2 for 1st byte after E0~EF or 2nd byte after F0~FF
             * - 1 for 1st byte after C0~DF or 2nd byte after E0~EF or
             *         3rd byte after F0~FF
             * - 0 for others
             * Error cases:
             *   9,10,11 if non ascii First Byte overlaps
             *   E.g., F1 80 C2 90 --> 8 3 10 2, where 10 indicates error
             */

            /* Adjust Second Byte range for special First Bytes(E0,ED,F0,F4) */
            /* Overlaps lead to index 9~15, which are illegal in range table */
            simde__m256i shift1, pos, range2;
            /* shift1 = (input, prev_input) << 1 byte */
            shift1 = push_last_byte_of_a_to_b(prev_input, input);
            pos = simde_mm256_sub_epi8(shift1, simde_mm256_set1_epi8(0xEF));
            /*
             * shift1:  | EF  F0 ... FE | FF  00  ... ...  DE | DF  E0 ... EE |
             * pos:     | 0   1      15 | 16  17           239| 240 241    255|
             * pos-240: | 0   0      0  | 0   0            0  | 0   1      15 |
             * pos+112: | 112 113    127|       >= 128        |     >= 128    |
             */
            tmp1 = simde_mm256_subs_epu8(pos, simde_mm256_set1_epi8((uint8_t)240));
            range2 = simde_mm256_shuffle_epi8(df_ee_tbl, tmp1);
            tmp2 = simde_mm256_adds_epu8(pos, simde_mm256_set1_epi8(112));
            range2 = simde_mm256_add_epi8(range2, simde_mm256_shuffle_epi8(ef_fe_tbl, tmp2));

            range = simde_mm256_add_epi8(range, range2);

            /* Load min and max values per calculated range index */
            simde__m256i minv = simde_mm256_shuffle_epi8(range_min_tbl, range);
            simde__m256i maxv = simde_mm256_shuffle_epi8(range_max_tbl, range);

            /* Check value range */
            simde__m256i error = simde_mm256_cmpgt_epi8(minv, input);
            error = simde_mm256_or_si256(error, simde_mm256_cmpgt_epi8(input, maxv));
            /* 5% performance drop from this conditional branch */
            if (!simde_mm256_testz_si256(error, error))
                break;

            prev_input = input;
            prev_first_len = first_len;

            data += 32;
            len -= 32;
            err_idx += 32;
        }

        /* Error in first 16 bytes */
        if (err_idx == 1)
            goto do_naive;

        /* Find previous token (not 80~BF) */
        int32_t token4 = simde_mm256_extract_epi32(prev_input, 7);
        const int8_t *token = (const int8_t *)&token4;
        int lookahead = 0;
        if (token[3] > (int8_t)0xBF)
            lookahead = 1;
        else if (token[2] > (int8_t)0xBF)
            lookahead = 2;
        else if (token[1] > (int8_t)0xBF)
            lookahead = 3;

        data -= lookahead;
        len += lookahead;
        err_idx -= lookahead;
    }

    /* Check remaining bytes with naive method */
    size_t err_idx2;
do_naive:
    if (!utf8_valid_naive(data, len, &err_idx2)) {
        *error_index = err_idx + err_idx2 - 1;
        return false;
    }
    return true;
}


#endif