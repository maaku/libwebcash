/* Copyright (c) 2022-2023 Mark Friedenbach
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "webcash.h"

#include <ctype.h>
#include <stdint.h>

wc_amount_t wc_zero(void) {
        return WC_ZERO;
}

wc_error_t wc_from_cstring(wc_amount_t *amt, int *noncanonical, const char *str) {
        struct tagbstring tstr = {0};
        btfromcstr(tstr, str);
        return wc_from_bstring(amt, noncanonical, &tstr);
}

wc_error_t wc_from_bstring(wc_amount_t *amt, int *noncanonical, bstring str) {
        const unsigned char* pos = NULL;
        const unsigned char* end = NULL;
        uint64_t u64 = UINT64_C(0);
        int is_noncanonical = 0;
        int is_negative = 0;
        int is_fractional = 0;
        int j = 0;

        /* Sanity: must provide output parameter. */
        if (!amt) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Sanity: no invalid bstrings allowed. */
        if (!str || str->slen < 0 || (str->mlen >= 0 && str->mlen < str->slen) || !str->data) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Sanity: no empty strings allowed. */
        if (str->slen == 0) {
                return WC_ERROR_INVALID_ARGUMENT;
        }

        pos = &str->data[0];
        end = &str->data[str->slen];

        /* Remove any surrounding quotation marks, if any.  This is purely for
         * compatibility reasons with other webcash APIs. */
        while (*pos == '"' && *(end - 1) == '"') {
                ++pos; --end;
                /* An empty set of quotes is not a valid encoding. */
                if (pos >= end) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
        }

        is_negative = (*pos == '-');
        if (is_negative) {
                ++pos;
                /* A single minus sign is not a valid encoding. */
                if (pos == end) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
        }

        /* Purely fractional amounts require a leading zero before the decimal
         * point, and the canonical representation of zero is a single zero
         * digit.  But only one zero, and in all other cases no leading zeros
         * are allowed. */
        if (!isdigit(*pos)) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (pos[0] == '0' && (pos + 1) != end && pos[1] != '.') {
                is_noncanonical = 1;
        }

        /* Parse the integral part. */
        for (; pos != end && isdigit(*pos); ++pos) {
                /* Overflow checked arithmetic. */
                if (u64 > (UINT64_MAX / 10)) {
                        return WC_ERROR_OVERFLOW;
                }
                u64 *= UINT64_C(10);
                if (u64 > (UINT64_MAX - (*pos - '0'))) {
                        return WC_ERROR_OVERFLOW;
                }
                u64 += (*pos - '0');
        }

        /* The fractional portion is optional. */
        if (pos != end) {
                /* We know it's not a digit. */
                if (*pos != '.') {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
                /* Skip past the decimal point. */
                ++pos;
                /* If there is a decimal point, there must be at least one
                 * digit that follows. */
                if (pos == end) {
                        is_noncanonical = 1;
                }
                /* Tailing fractional zero digits are non-canonical. */
                if (end[-1] == '0') {
                        is_noncanonical = 1;
                }
                /* Read up to eight digits. */
                for (; j < 8 && pos != end; ++j, ++pos) {
                        /* Must be a decimal digit. */
                        if (!isdigit(*pos)) {
                                return WC_ERROR_INVALID_ARGUMENT;
                        }
                        if (*pos != '0') {
                                is_fractional = 1;
                        }
                        /* Overflow checked arithmetic. */
                        if (u64 > (UINT64_MAX / 10)) {
                                return WC_ERROR_OVERFLOW;
                        }
                        u64 *= UINT64_C(10);
                        if (u64 > (UINT64_MAX - (*pos - '0'))) {
                                return WC_ERROR_OVERFLOW;
                        }
                        u64 += (*pos - '0');
                }
                /* We ought to now be at the end of the input.  There could be
                 * some trailing zeros in a non-canonical input, however. */
                for (; pos != end; ++pos) {
                        if (*pos != '0') {
                                return WC_ERROR_INVALID_ARGUMENT;
                        }
                        is_noncanonical = 1;
                }
                /* If there was only '0' digits in the fractional part, this
                 * is a non-canonical representation. */
                if (!is_fractional) {
                        is_noncanonical = 1;
                }
        }

        /* Scale by any elided fractional digits. */
        while ( j++ < 8) {
                /* Overflow checked arithmetic. */
                if (u64 > (UINT64_MAX / 10)) {
                        return WC_ERROR_OVERFLOW;
                }
                u64 *= UINT64_C(10);
        }

        /* Check for signed overflow */
        if (is_negative && u64 > ((uint64_t)INT64_MAX + 1)) {
                return WC_ERROR_OVERFLOW;
        }
        if (!is_negative && u64 > (uint64_t)INT64_MAX) {
                return WC_ERROR_OVERFLOW;
        }

        /* Check for negative zero. */
        if (!u64 && is_negative) {
                is_noncanonical = 1;
        }

        /* Return the result. */
        if (noncanonical) {
                *noncanonical = !!is_noncanonical;
        }
        *amt = is_negative ? -u64 : u64;
        return WC_SUCCESS;
}

/* End of File
 */
