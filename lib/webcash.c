/* Copyright (c) 2022-2023 Mark Friedenbach
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "webcash.h"

#include <ctype.h>
#include <inttypes.h>
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
        while (j++ < 8) {
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

bstring wc_to_bstring(wc_amount_t amount) {
        int64_t quot = 0;
        int64_t rem = 0;
        int is_negative = 0;
        bstring str = NULL;
        if (amount < 0) {
                /* We handle this as a special case because otherwise the
                 * negation below would overflow. */
                if (amount == INT64_MIN) {
                        return cstr2bstr("-92233720368.54775808");
                }
                /* Otherwise we remember it is negative and calculate its
                 * absolute value. */
                is_negative = 1;
                amount = -amount;
        }
        quot = amount / WC_AMOUNT_SCALE;
        rem = amount % WC_AMOUNT_SCALE;
        if (rem == 0) {
                return bformat("%s%" PRId64, is_negative ? "-" : "", quot);
        }
        str = bformat("%s%" PRId64 ".%08" PRId64, is_negative ? "-" : "", quot, rem);
        while (str->slen > 0 && str->data[str->slen - 1] == '0') {
                btrunc(str, str->slen - 1);
        }
        return str;
}

/* The amount of memory to allocate for wc_secret_t.secret when default
 * initialized.  A webcash secret is traditionally an hex-encoded 32-bit
 * random or pseudorandom value, so we will allocate enough memory to store
 * that. */
#define WC_SECRET_DEFAULT_ALLOC_SIZE 64

wc_error_t wc_secret_new(wc_secret_t *secret) {
        bstring serial = NULL;
        if (!secret) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        serial = bfromcstralloc(WC_SECRET_DEFAULT_ALLOC_SIZE, "");
        if (serial == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        wc_secret_destroy(secret); /* avoid leaking memory */
        secret->amount = WC_ZERO;
        secret->serial = serial;
        return WC_SUCCESS;
}

wc_error_t wc_secret_from_cstring(wc_secret_t *secret, wc_amount_t amount, const char *serial) {
        bstring bstr = NULL;
        if (!secret) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (serial == NULL) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        bstr = bfromcstr(serial);
        if (bstr == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        wc_secret_destroy(secret); /* avoid leaking memory */
        secret->amount = amount;
        secret->serial = bstr;
        return WC_SUCCESS;
}

wc_error_t wc_secret_from_bstring(wc_secret_t *secret, wc_amount_t amount, bstring *serial) {
        if (!secret) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (serial == NULL || *serial == NULL || (*serial)->slen < 0 || (*serial)->data == NULL) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        wc_secret_destroy(secret); /* avoid leaking memory */
        secret->amount = amount;
        secret->serial = *serial;
        *serial = NULL;
        return WC_SUCCESS;
}

wc_error_t wc_secret_from_bstring_copy(wc_secret_t *secret, wc_amount_t amount, bstring serial) {
        bstring bstr = NULL;
        if (!secret) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (serial == NULL || serial->slen < 0 || serial->data == NULL) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        bstr = bstrcpy(serial);
        if (bstr == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        wc_secret_destroy(secret); /* avoid leaking memory */
        secret->amount = amount;
        secret->serial = bstr;
        return WC_SUCCESS;
}

wc_error_t wc_secret_is_valid(const wc_secret_t *secret) {
        int i = 0;
        if (!secret) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Zero valued or negative webcash are not allowed. */
        if (secret->amount < (wc_amount_t)1) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* The secret itself must be a valid string. */
        if (!secret->serial || secret->serial->slen < 0 || secret->serial->data == NULL) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* The secret must be a utf8-encoded unicode string with no NUL characters. */
        /* FIXME: Should check that it is actually utf8 unicode? */
        /* FIXME: Should there be a maximum length for the serial? */
        for (; i < secret->serial->slen; ++i) {
                if (secret->serial->data[i] == '\0') {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
        }
        return WC_SUCCESS;
}

wc_error_t wc_secret_destroy(wc_secret_t *wc) {
        if (!wc) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (bdestroy(wc->serial) != BSTR_OK) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        wc->amount = WC_ZERO;
        wc->serial = NULL;
        return WC_SUCCESS;
}

/* End of File
 */
