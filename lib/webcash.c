/* Copyright (c) 2022-2023 Mark Friedenbach
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "webcash.h"

#include "sqlite3.h"

#include <sha2/sha256.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>

static unsigned char hexdigits[] = "0123456789abcdef";
static unsigned char hexdigit_to_int(char c) {
        if (c >= '0' && c <= '9') {
                return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
        }
        return 0xff;
}

static struct sha256_ctx webcashwalletv1_midstate = SHA256_INIT;
wc_error_t wc_init(void) {
        const char *webcashwalletv1_tag_str = "webcashwalletv1";
        struct sha256 webcashwalletv1_tag;
        /* Setup libsha2 */
        sha256_auto_detect();
        /* Initialize the midstate for the webcashwalletv1 chain derivation tag. */
        sha256_init(&webcashwalletv1_midstate);
        sha256_update(&webcashwalletv1_midstate, (const unsigned char*)webcashwalletv1_tag_str, strlen(webcashwalletv1_tag_str));
        sha256_done(&webcashwalletv1_tag, &webcashwalletv1_midstate);
        sha256_init(&webcashwalletv1_midstate);
        sha256_update(&webcashwalletv1_midstate, webcashwalletv1_tag.u8, sizeof(webcashwalletv1_tag.u8));
        sha256_update(&webcashwalletv1_midstate, webcashwalletv1_tag.u8, sizeof(webcashwalletv1_tag.u8));
        return WC_SUCCESS;
}

wc_amount_t wc_zero(void) {
        return WC_ZERO;
}

wc_error_t wc_from_cstring(
        wc_amount_t *amt,
        int *noncanonical,
        const char *str
) {
        struct tagbstring tstr = {0};
        btfromcstr(tstr, str);
        return wc_from_bstring(amt, noncanonical, &tstr);
}

wc_error_t wc_from_bstring(
        wc_amount_t *amt,
        int *noncanonical,
        bstring str
) {
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
                *noncanonical = is_noncanonical;
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

wc_error_t wc_secret_from_cstring(
        wc_secret_t *secret,
        wc_amount_t amount,
        const char *serial
) {
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

wc_error_t wc_secret_from_bstring(
        wc_secret_t *secret,
        wc_amount_t amount,
        bstring *serial
) {
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

wc_error_t wc_secret_from_bstring_copy(
        wc_secret_t *secret,
        wc_amount_t amount,
        bstring serial
) {
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

wc_error_t wc_secret_destroy(wc_secret_t *secret) {
        if (!secret) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (bdestroy(secret->serial) != BSTR_OK) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        secret->amount = WC_ZERO;
        secret->serial = NULL;
        return WC_SUCCESS;
}

wc_error_t wc_secret_to_string(
        bstring *bstr,
        const wc_secret_t *secret
) {
        bstring amt = NULL;
        if (!bstr) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!secret) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (secret->serial == NULL || secret->serial->slen < 0 || secret->serial->data == NULL) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        amt = wc_to_bstring(secret->amount);
        if (amt == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        if (*bstr) {
                /* prevent memory leaks */
                bdestroy(*bstr);
        }
        *bstr = bformat("e%s:secret:%s", amt->data, secret->serial->data);
        bdestroy(amt); /* cleanup */
        if (*bstr == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        return WC_SUCCESS;
}

wc_error_t wc_secret_parse(
        wc_secret_t *secret,
        int *noncanonical,
        bstring bstr
) {
        struct tagbstring sep = bsStatic(":");
        int pos[3] = {0}; /* { amount, "secret", serial } */
        int is_noncanonical = 0;
        wc_error_t err = WC_SUCCESS;
        wc_secret_t ret = {0};
        /* Argument validation. */
        if (!secret) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (bstr == NULL || bstr->slen <= 0 || bstr->data == NULL) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Split the string into amount, "secret", and serial fields. */
        pos[0] = bstr->data[0] == 'e' ? 1 : 0; /* skip 'e' if present */
        pos[1] = binchr(bstr, pos[0], &sep);
        if (pos[1] == BSTR_ERR) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        pos[2] = binchr(bstr, pos[1] + 1, &sep);
        if (pos[2] == BSTR_ERR) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Check that the middle field is the ASCII text "secret". */
        btfromblk(sep, &bstr->data[pos[1] + 1], pos[2] - pos[1] - 1);
        if (biseqcstr(&sep, "secret") != 1) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Parse the amount and serial fields. */
        btfromblk(sep, &bstr->data[pos[0]], pos[1] - pos[0]);
        err = wc_from_bstring(&ret.amount, &is_noncanonical, &sep);
        if (err != WC_SUCCESS) {
                return err;
        }
        /* Canonical form includes the 'e'. */
        is_noncanonical = is_noncanonical || pos[0] == 0;
        ret.serial = bmidstr(bstr, pos[2] + 1, bstr->slen - pos[2] - 1);
        if (ret.serial == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        /* Write to output parameters. */
        wc_secret_destroy(secret); /* avoid leaking memory */
        *secret = ret;
        if (noncanonical) {
                *noncanonical = is_noncanonical;
        }
        return WC_SUCCESS;
}

wc_public_t wc_public_from_secret(const wc_secret_t *secret) {
        wc_public_t pub = {0};
        struct sha256_ctx ctx = SHA256_INIT;
        if (!secret) {
                return pub;
        }
        if (secret->serial == NULL || secret->serial->slen < 0 || secret->serial->data == NULL) {
                return pub;
        }
        sha256_update(&ctx, secret->serial->data, secret->serial->slen);
        sha256_done(&pub.hash, &ctx);
        pub.amount = secret->amount;
        return pub;
}

wc_error_t wc_public_is_valid(const wc_public_t *pub) {
        if (!pub) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (pub->amount < (wc_amount_t)1) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        return WC_SUCCESS;
}

wc_error_t wc_public_to_string(
        bstring *bstr,
        const wc_public_t *pub
) {
        bstring amt = NULL;
        if (!bstr) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!pub) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        amt = wc_to_bstring(pub->amount);
        if (amt == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        if (*bstr) {
                /* prevent memory leaks */
                bdestroy(*bstr);
        }
        *bstr = bformat("e%s:public:"
                        "%02x%02x%02x%02x%02x%02x%02x%02x"
                        "%02x%02x%02x%02x%02x%02x%02x%02x"
                        "%02x%02x%02x%02x%02x%02x%02x%02x"
                        "%02x%02x%02x%02x%02x%02x%02x%02x",
                        amt->data,
                        pub->hash.u8[0],  pub->hash.u8[1],  pub->hash.u8[2],  pub->hash.u8[3],
                        pub->hash.u8[4],  pub->hash.u8[5],  pub->hash.u8[6],  pub->hash.u8[7],
                        pub->hash.u8[8],  pub->hash.u8[9],  pub->hash.u8[10], pub->hash.u8[11],
                        pub->hash.u8[12], pub->hash.u8[13], pub->hash.u8[14], pub->hash.u8[15],
                        pub->hash.u8[16], pub->hash.u8[17], pub->hash.u8[18], pub->hash.u8[19],
                        pub->hash.u8[20], pub->hash.u8[21], pub->hash.u8[22], pub->hash.u8[23],
                        pub->hash.u8[24], pub->hash.u8[25], pub->hash.u8[26], pub->hash.u8[27],
                        pub->hash.u8[28], pub->hash.u8[29], pub->hash.u8[30], pub->hash.u8[31]);
        bdestroy(amt); /* cleanup */
        if (*bstr == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        return WC_SUCCESS;
}

wc_error_t wc_public_parse(
        wc_public_t *pub,
        int *noncanonical,
        bstring bstr
) {
        struct tagbstring sep = bsStatic(":");
        int pos[3] = {0}; /* { amount, "public", hash } */
        int is_noncanonical = 0;
        wc_error_t err = WC_SUCCESS;
        wc_public_t ret = {0};
        int j = 0, c = 0;
        /* Argument validation. */
        if (!pub) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (bstr == NULL || bstr->slen <= 0 || bstr->data == NULL) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Split the string into amount, "public", and hash fields. */
        pos[0] = bstr->data[0] == 'e' ? 1 : 0; /* skip 'e' if present */
        pos[1] = binchr(bstr, pos[0], &sep);
        if (pos[1] == BSTR_ERR) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        pos[2] = binchr(bstr, pos[1] + 1, &sep);
        if (pos[2] == BSTR_ERR) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (bstr->slen - pos[2] != 1/*:*/ + 64/*hash*/) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        for (j = 0; j < 64; ++j) {
                c = bstr->data[pos[2] + 1 + j];
                if (hexdigit_to_int(c) > 15) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
                if (c >= 'A' && c <= 'F') {
                        is_noncanonical = 1;
                }
        }
        /* Check that the middle field is the ASCII text "public". */
        btfromblk(sep, &bstr->data[pos[1] + 1], pos[2] - pos[1] - 1);
        if (biseqcstr(&sep, "public") != 1) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Parse the amount and hash fields. */
        btfromblk(sep, &bstr->data[pos[0]], pos[1] - pos[0]);
        c = 0;
        err = wc_from_bstring(&ret.amount, &c, &sep);
        is_noncanonical = is_noncanonical || c;
        if (err != WC_SUCCESS) {
                return err;
        }
        /* Canonical form includes the 'e'. */
        is_noncanonical = is_noncanonical || pos[0] == 0;
        for (j = 0; j < 32; ++j) {
                ret.hash.u8[j] = hexdigit_to_int(bstr->data[pos[2] + 1 + 2*j]) << 4
                               | hexdigit_to_int(bstr->data[pos[2] + 1 + 2*j + 1]);
        }
        /* Write to output parameters. */
        *pub = ret;
        if (noncanonical) {
                *noncanonical = is_noncanonical;
        }
        return WC_SUCCESS;
}

unsigned char wc_mining_nonces[4*1000] = {
        "MDAwMDAxMDAyMDAzMDA0MDA1MDA2MDA3MDA4MDA5MDEwMDExMDEyMDEzMDE0MDE1MDE2MDE3MDE4MDE5"
        "MDIwMDIxMDIyMDIzMDI0MDI1MDI2MDI3MDI4MDI5MDMwMDMxMDMyMDMzMDM0MDM1MDM2MDM3MDM4MDM5"
        "MDQwMDQxMDQyMDQzMDQ0MDQ1MDQ2MDQ3MDQ4MDQ5MDUwMDUxMDUyMDUzMDU0MDU1MDU2MDU3MDU4MDU5"
        "MDYwMDYxMDYyMDYzMDY0MDY1MDY2MDY3MDY4MDY5MDcwMDcxMDcyMDczMDc0MDc1MDc2MDc3MDc4MDc5"
        "MDgwMDgxMDgyMDgzMDg0MDg1MDg2MDg3MDg4MDg5MDkwMDkxMDkyMDkzMDk0MDk1MDk2MDk3MDk4MDk5"
        "MTAwMTAxMTAyMTAzMTA0MTA1MTA2MTA3MTA4MTA5MTEwMTExMTEyMTEzMTE0MTE1MTE2MTE3MTE4MTE5"
        "MTIwMTIxMTIyMTIzMTI0MTI1MTI2MTI3MTI4MTI5MTMwMTMxMTMyMTMzMTM0MTM1MTM2MTM3MTM4MTM5"
        "MTQwMTQxMTQyMTQzMTQ0MTQ1MTQ2MTQ3MTQ4MTQ5MTUwMTUxMTUyMTUzMTU0MTU1MTU2MTU3MTU4MTU5"
        "MTYwMTYxMTYyMTYzMTY0MTY1MTY2MTY3MTY4MTY5MTcwMTcxMTcyMTczMTc0MTc1MTc2MTc3MTc4MTc5"
        "MTgwMTgxMTgyMTgzMTg0MTg1MTg2MTg3MTg4MTg5MTkwMTkxMTkyMTkzMTk0MTk1MTk2MTk3MTk4MTk5"
        "MjAwMjAxMjAyMjAzMjA0MjA1MjA2MjA3MjA4MjA5MjEwMjExMjEyMjEzMjE0MjE1MjE2MjE3MjE4MjE5"
        "MjIwMjIxMjIyMjIzMjI0MjI1MjI2MjI3MjI4MjI5MjMwMjMxMjMyMjMzMjM0MjM1MjM2MjM3MjM4MjM5"
        "MjQwMjQxMjQyMjQzMjQ0MjQ1MjQ2MjQ3MjQ4MjQ5MjUwMjUxMjUyMjUzMjU0MjU1MjU2MjU3MjU4MjU5"
        "MjYwMjYxMjYyMjYzMjY0MjY1MjY2MjY3MjY4MjY5MjcwMjcxMjcyMjczMjc0Mjc1Mjc2Mjc3Mjc4Mjc5"
        "MjgwMjgxMjgyMjgzMjg0Mjg1Mjg2Mjg3Mjg4Mjg5MjkwMjkxMjkyMjkzMjk0Mjk1Mjk2Mjk3Mjk4Mjk5"
        "MzAwMzAxMzAyMzAzMzA0MzA1MzA2MzA3MzA4MzA5MzEwMzExMzEyMzEzMzE0MzE1MzE2MzE3MzE4MzE5"
        "MzIwMzIxMzIyMzIzMzI0MzI1MzI2MzI3MzI4MzI5MzMwMzMxMzMyMzMzMzM0MzM1MzM2MzM3MzM4MzM5"
        "MzQwMzQxMzQyMzQzMzQ0MzQ1MzQ2MzQ3MzQ4MzQ5MzUwMzUxMzUyMzUzMzU0MzU1MzU2MzU3MzU4MzU5"
        "MzYwMzYxMzYyMzYzMzY0MzY1MzY2MzY3MzY4MzY5MzcwMzcxMzcyMzczMzc0Mzc1Mzc2Mzc3Mzc4Mzc5"
        "MzgwMzgxMzgyMzgzMzg0Mzg1Mzg2Mzg3Mzg4Mzg5MzkwMzkxMzkyMzkzMzk0Mzk1Mzk2Mzk3Mzk4Mzk5"
        "NDAwNDAxNDAyNDAzNDA0NDA1NDA2NDA3NDA4NDA5NDEwNDExNDEyNDEzNDE0NDE1NDE2NDE3NDE4NDE5"
        "NDIwNDIxNDIyNDIzNDI0NDI1NDI2NDI3NDI4NDI5NDMwNDMxNDMyNDMzNDM0NDM1NDM2NDM3NDM4NDM5"
        "NDQwNDQxNDQyNDQzNDQ0NDQ1NDQ2NDQ3NDQ4NDQ5NDUwNDUxNDUyNDUzNDU0NDU1NDU2NDU3NDU4NDU5"
        "NDYwNDYxNDYyNDYzNDY0NDY1NDY2NDY3NDY4NDY5NDcwNDcxNDcyNDczNDc0NDc1NDc2NDc3NDc4NDc5"
        "NDgwNDgxNDgyNDgzNDg0NDg1NDg2NDg3NDg4NDg5NDkwNDkxNDkyNDkzNDk0NDk1NDk2NDk3NDk4NDk5"
        "NTAwNTAxNTAyNTAzNTA0NTA1NTA2NTA3NTA4NTA5NTEwNTExNTEyNTEzNTE0NTE1NTE2NTE3NTE4NTE5"
        "NTIwNTIxNTIyNTIzNTI0NTI1NTI2NTI3NTI4NTI5NTMwNTMxNTMyNTMzNTM0NTM1NTM2NTM3NTM4NTM5"
        "NTQwNTQxNTQyNTQzNTQ0NTQ1NTQ2NTQ3NTQ4NTQ5NTUwNTUxNTUyNTUzNTU0NTU1NTU2NTU3NTU4NTU5"
        "NTYwNTYxNTYyNTYzNTY0NTY1NTY2NTY3NTY4NTY5NTcwNTcxNTcyNTczNTc0NTc1NTc2NTc3NTc4NTc5"
        "NTgwNTgxNTgyNTgzNTg0NTg1NTg2NTg3NTg4NTg5NTkwNTkxNTkyNTkzNTk0NTk1NTk2NTk3NTk4NTk5"
        "NjAwNjAxNjAyNjAzNjA0NjA1NjA2NjA3NjA4NjA5NjEwNjExNjEyNjEzNjE0NjE1NjE2NjE3NjE4NjE5"
        "NjIwNjIxNjIyNjIzNjI0NjI1NjI2NjI3NjI4NjI5NjMwNjMxNjMyNjMzNjM0NjM1NjM2NjM3NjM4NjM5"
        "NjQwNjQxNjQyNjQzNjQ0NjQ1NjQ2NjQ3NjQ4NjQ5NjUwNjUxNjUyNjUzNjU0NjU1NjU2NjU3NjU4NjU5"
        "NjYwNjYxNjYyNjYzNjY0NjY1NjY2NjY3NjY4NjY5NjcwNjcxNjcyNjczNjc0Njc1Njc2Njc3Njc4Njc5"
        "NjgwNjgxNjgyNjgzNjg0Njg1Njg2Njg3Njg4Njg5NjkwNjkxNjkyNjkzNjk0Njk1Njk2Njk3Njk4Njk5"
        "NzAwNzAxNzAyNzAzNzA0NzA1NzA2NzA3NzA4NzA5NzEwNzExNzEyNzEzNzE0NzE1NzE2NzE3NzE4NzE5"
        "NzIwNzIxNzIyNzIzNzI0NzI1NzI2NzI3NzI4NzI5NzMwNzMxNzMyNzMzNzM0NzM1NzM2NzM3NzM4NzM5"
        "NzQwNzQxNzQyNzQzNzQ0NzQ1NzQ2NzQ3NzQ4NzQ5NzUwNzUxNzUyNzUzNzU0NzU1NzU2NzU3NzU4NzU5"
        "NzYwNzYxNzYyNzYzNzY0NzY1NzY2NzY3NzY4NzY5NzcwNzcxNzcyNzczNzc0Nzc1Nzc2Nzc3Nzc4Nzc5"
        "NzgwNzgxNzgyNzgzNzg0Nzg1Nzg2Nzg3Nzg4Nzg5NzkwNzkxNzkyNzkzNzk0Nzk1Nzk2Nzk3Nzk4Nzk5"
        "ODAwODAxODAyODAzODA0ODA1ODA2ODA3ODA4ODA5ODEwODExODEyODEzODE0ODE1ODE2ODE3ODE4ODE5"
        "ODIwODIxODIyODIzODI0ODI1ODI2ODI3ODI4ODI5ODMwODMxODMyODMzODM0ODM1ODM2ODM3ODM4ODM5"
        "ODQwODQxODQyODQzODQ0ODQ1ODQ2ODQ3ODQ4ODQ5ODUwODUxODUyODUzODU0ODU1ODU2ODU3ODU4ODU5"
        "ODYwODYxODYyODYzODY0ODY1ODY2ODY3ODY4ODY5ODcwODcxODcyODczODc0ODc1ODc2ODc3ODc4ODc5"
        "ODgwODgxODgyODgzODg0ODg1ODg2ODg3ODg4ODg5ODkwODkxODkyODkzODk0ODk1ODk2ODk3ODk4ODk5"
        "OTAwOTAxOTAyOTAzOTA0OTA1OTA2OTA3OTA4OTA5OTEwOTExOTEyOTEzOTE0OTE1OTE2OTE3OTE4OTE5"
        "OTIwOTIxOTIyOTIzOTI0OTI1OTI2OTI3OTI4OTI5OTMwOTMxOTMyOTMzOTM0OTM1OTM2OTM3OTM4OTM5"
        "OTQwOTQxOTQyOTQzOTQ0OTQ1OTQ2OTQ3OTQ4OTQ5OTUwOTUxOTUyOTUzOTU0OTU1OTU2OTU3OTU4OTU5"
        "OTYwOTYxOTYyOTYzOTY0OTY1OTY2OTY3OTY4OTY5OTcwOTcxOTcyOTczOTc0OTc1OTc2OTc3OTc4OTc5"
        "OTgwOTgxOTgyOTgzOTg0OTg1OTg2OTg3OTg4OTg5OTkwOTkxOTkyOTkzOTk0OTk1OTk2OTk3OTk4OTk5"
};
unsigned char wc_mining_final[4] = { 'f', 'Q', '=', '=' };

extern void WriteBE64(unsigned char* ptr, uint64_t x); /* provided by libsha2 */
void wc_mining_8way(
        unsigned char hashes[8*32],
        const struct sha256_ctx* ctx,
        const unsigned char nonce1[4],
        const unsigned char nonce2[4*8],
        const unsigned char final[4]
) {
	unsigned char blocks[8*64] = {0};
	int i = 0;
	for (; i < 8; ++i) {
		memcpy(blocks + 64*i + 0, nonce1, 4);
		memcpy(blocks + 64*i + 4, nonce2 + 4*i, 4);
		memcpy(blocks + 64*i + 8, final, 4);
		blocks[i*64 + 12] = 0x80; /* padding byte */
		WriteBE64(blocks + 64*i + 56, (ctx->bytes + 12) << 3);
	}
	sha256_midstate((struct sha256*)hashes, ctx->s, blocks, 8);
}

wc_error_t wc_derive_serial(
        bstring *bstr,
        const struct sha256 *root,
        uint64_t chaincode,
        uint64_t depth
) {
        char buf[64] = {0};
        if (!bstr) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!root) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        wc_derive_serials(buf, root, chaincode, depth, 1);
        if (*bstr) {
                /* prevent memory leaks */
                bdestroy(*bstr);
        }
        *bstr = blk2bstr(buf, 64);
        wc_memory_cleanse(buf, 64);
        if (*bstr == NULL) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        return WC_SUCCESS;
}

void wc_derive_serials(
        char out[],
        const struct sha256 *root,
        uint64_t chaincode,
        uint64_t depth,
        size_t count
) {
        unsigned char blocks[8*64] = {0};
        size_t n = 0, m = 0;
        int i = 0;
        if (count == 0) {
                return;
        }
        for (n = 0; n < (count < 8 ? count : 8); ++n) {
                memcpy(   blocks + 64*n +  0, root->u8, 32);
                WriteBE64(blocks + 64*n + 32, chaincode);
                        *(blocks + 64*n + 48) = 0x80; /* padding */
                WriteBE64(blocks + 64*n + 56, (webcashwalletv1_midstate.bytes + 48) << 3);
        }
        /* This black magic is “Duff's device.”  The outer switch operates as
         * goto switchboard, jumping execution into the loop at one of the
         * case labels.  After that initial jump you can ignore the switch
         * statement--it becomes a do-while loop.
         *
         * libsha2 SHA256 digests are most efficiently performed in parallel
         * with batch sizes of up to 8 hashes at a time, which is what this
         * code achieves.  The first time through the loop it performs the odd
         * number of hashing operations to get the number of remaining hashes
         * to be a multiple of eight, then it processes batches of eight on
         * each successive pass through the loop, until all serials have been
         * derived. */
        switch (count % 8) {
        case 0: do {    WriteBE64(blocks + 64*7 + 40, depth + 7); /* fallthrough */
        case 7:         WriteBE64(blocks + 64*6 + 40, depth + 6); /* fallthrough */
        case 6:         WriteBE64(blocks + 64*5 + 40, depth + 5); /* fallthrough */
        case 5:         WriteBE64(blocks + 64*4 + 40, depth + 4); /* fallthrough */
        case 4:         WriteBE64(blocks + 64*3 + 40, depth + 3); /* fallthrough */
        case 3:         WriteBE64(blocks + 64*2 + 40, depth + 2); /* fallthrough */
        case 2:         WriteBE64(blocks + 64*1 + 40, depth + 1); /* fallthrough */
        case 1:         WriteBE64(blocks + 64*0 + 40, depth + 0);
                        m = (count - 1) % 8 + 1; /* count % 8, but with 0 mod 8 becoming 8 */
                        sha256_midstate((struct sha256*)out, webcashwalletv1_midstate.s, blocks, m);
                        for (i = m*32 - 1; i >= 0; --i) {
                                out[2*i + 1] = hexdigits[(out[i] >> 0) & 0x0f];
                                out[2*i + 0] = hexdigits[(out[i] >> 4) & 0x0f];
                        }
                        out += m*64;
                        depth += m;
                        count -= m;
                } while (count > 0);
        }
        while (n) {
                wc_memory_cleanse(blocks + 64*(--n), 48);
        }
}

struct wc_storage {
        const struct wc_storage_callbacks* cb;
        wc_db_handle_t db; /* the main wallet database */
        wc_log_handle_t log; /* append-only recovery log */
};

wc_error_t wc_storage_open(
        wc_storage_handle_t *storage,
        const wc_storage_callbacks_t *callbacks,
        wc_log_url_t logurl,
        wc_db_url_t dburl
) {
        wc_storage_handle_t w = NULL;
        /* Must have a way of returning the allocated wallet to the caller. */
        if (!storage) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* A wallet requires both an active database connection and a recovery log.
         * The _close() callbacks may be NULL, if explicit closure is not required. */
        if (!callbacks || !callbacks->db_open || !callbacks->log_open) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Allocate the wallet storage structure. */
        w = malloc(sizeof(struct wc_storage));
        if (!w) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        /* Initialize the wallet storage structure. */
        w->cb = callbacks;
        w->log = callbacks->log_open(logurl);
        if (!w->log) {
                free(w);
                return WC_ERROR_LOG_OPEN_FAILED;
        }
        w->db = callbacks->db_open(dburl);
        if (!w->db) {
                if (callbacks->log_close) {
                        callbacks->log_close(w->log);
                }
                free(w);
                return WC_ERROR_DB_OPEN_FAILED;
        }
        /* Return the wallet structure. */
        *storage = w;
        return WC_SUCCESS;
}

wc_error_t wc_storage_close(wc_storage_handle_t w) {
        if (!w) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (w->db) {
                if  (w->cb && w->cb->db_close) {
                        w->cb->db_close(w->db);
                }
                w->db = NULL;
        }
        if (w->log) {
                if (w->cb && w->cb->log_close) {
                        w->cb->log_close(w->log);
                }
                w->log = NULL;
        }
        w->cb = NULL;
        free(w);
        return WC_SUCCESS;
}

wc_error_t wc_storage_enumerate_terms(
        wc_storage_handle_t w,
        wc_terms_t *terms,
        size_t *count
) {
        wc_error_t e = WC_SUCCESS;
        wc_db_terms_t *dbterms = (wc_db_terms_t*)terms;
        size_t i = 0, j = 0;
        if (!w || !count) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!w->db) {
                return WC_ERROR_DB_CLOSED;
        }
        if (!w->cb || !w->cb->all_terms) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        e = w->cb->all_terms(w->db, dbterms, count);
        if (e == WC_SUCCESS && !terms) {
                e = WC_ERROR_INSUFFICIENT_CAPACITY;
        }
        if (e != WC_SUCCESS) {
                return e;
        }
        for (i = *count; i > 0;) {
                --i; /* zero-based indexing */
                terms[i].text = dbterms[i].text;
                if (!gmtime_r(&dbterms[i].when, &terms[i].when)) {
                        for (j = 0; j < i; ++j) {
                                bdestroy(dbterms[j].text);
                        }
                        for (; j < *count; ++j) {
                                bdestroy(terms[j].text);
                        }
                        return WC_ERROR_DB_CORRUPT;
                }
        }
        return WC_SUCCESS;
}

wc_error_t wc_storage_have_accepted_terms(
        wc_storage_handle_t w,
        int *accepted
) {
        if (!w || !accepted) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!w->db) {
                return WC_ERROR_DB_CLOSED;
        }
        if (!w->cb || !w->cb->any_terms) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        return w->cb->any_terms(w->db, accepted);
}

wc_error_t wc_storage_are_terms_accepted(
        wc_storage_handle_t w,
        int *accepted,
        struct tm *when,
        bstring terms
) {
        wc_error_t e = WC_SUCCESS;
        time_t t = 0;
        int found = 0;
        if (!w || !terms || !(accepted || when)) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!w->db) {
                return WC_ERROR_DB_CLOSED;
        }
        if (!w->cb || !w->cb->terms_accepted) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        e = w->cb->terms_accepted(w->db, terms, &t);
        if (e != WC_SUCCESS) {
                return e;
        }
        found = (t != 0);
        if (when && found) {
                if (t + WC_TIME_EPOCH < t) {
                        return WC_ERROR_OVERFLOW;
                }
                t += WC_TIME_EPOCH;
                if (!gmtime_r(&t, when)) {
                        return WC_ERROR_DB_CORRUPT;
                }
        }
        if (accepted) {
                *accepted = found;
        }
        return WC_SUCCESS;
}

wc_error_t wc_storage_accept_terms(
        wc_storage_handle_t w,
        bstring terms,
        struct tm *now
) {
        time_t t = 0;
        if (!w || !terms) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!w->db) {
                return WC_ERROR_DB_CLOSED;
        }
        if (!w->cb || !w->cb->accept_terms) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (now) {
                t = mktime(now);
                if (t == (time_t)-1 || t < WC_TIME_EPOCH) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
        } else {
                t = time(NULL);
                if (t == (time_t)-1 || t < WC_TIME_EPOCH) {
                        /* The only reason time() might fail is integer overflow. */
                        return WC_ERROR_OVERFLOW;
                }
        }
        t -= WC_TIME_EPOCH;
        return w->cb->accept_terms(w->db, terms, t);
}

struct wc_server {
        const struct wc_server_callbacks *cb;
        wc_conn_handle_t conn;
};

wc_error_t wc_server_connect(
        wc_server_handle_t *server,
        const wc_server_callbacks_t *callbacks,
        wc_server_url_t url
) {
        wc_server_handle_t c = NULL;
        /* Must have a way of returning the allocated server connection to the caller. */
        if (!server) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* A server object without a server connection is pretty useless. */
        if (!callbacks || !callbacks->connect) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Allocate the server object structure. */
        c = malloc(sizeof(struct wc_server));
        if (!c) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        /* Initialize the server object structure. */
        c->cb = callbacks;
        c->conn = callbacks->connect(url);
        if (!c->conn) {
                free(c);
                return WC_ERROR_CONNECT_FAILED;
        }
        /* Return the server object structure. */
        *server = c;
        return WC_SUCCESS;
}

wc_error_t wc_server_disconnect(wc_server_handle_t c) {
        if (!c) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (c->conn) {
                if  (c->cb && c->cb->disconnect) {
                        c->cb->disconnect(c->conn);
                }
                c->conn = NULL;
        }
        c->cb = NULL;
        free(c);
        return WC_SUCCESS;
}

wc_error_t wc_server_get_terms(
        wc_server_handle_t c,
        bstring *terms
) {
        if (!c || !terms) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!c->conn) {
                return WC_ERROR_NOT_CONNECTED;
        }
        if (!c->cb || !c->cb->get_terms) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        return c->cb->get_terms(c->conn, terms);
}

struct wc_ui {
        const struct wc_ui_callbacks *cb;
        wc_window_handle_t hwnd;
};

wc_error_t wc_ui_startup(
        wc_ui_handle_t *ui_out,
        const wc_ui_callbacks_t *callbacks,
        wc_window_params_t params
) {
        wc_ui_handle_t ui = NULL;
        /* Must have a way of returning the initialize user interface to the caller. */
        if (!ui_out) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* The startup callback is required. */
        if (!callbacks || !callbacks->startup) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Allocate the user interface object. */
        ui = malloc(sizeof(struct wc_ui));
        if (!ui) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        /* Initialize the user interface object. */
        ui->cb = callbacks;
        ui->hwnd = callbacks->startup(params);
        if (!ui->hwnd) {
                free(ui);
                return WC_ERROR_STARTUP_FAILED;
        }
        /* Return the user interface object. */
        *ui_out = ui;
        return WC_SUCCESS;
}

wc_error_t wc_ui_shutdown(wc_ui_handle_t ui) {
        if (!ui) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (ui->hwnd) {
                if  (ui->cb && ui->cb->shutdown) {
                        ui->cb->shutdown(ui->hwnd);
                }
                ui->hwnd = NULL;
        }
        ui->cb = NULL;
        free(ui);
        return WC_SUCCESS;
}

wc_error_t wc_ui_show_terms(
        wc_ui_handle_t ui,
        int *accepted,
        bstring terms
) {
        if (!ui || !accepted || !terms) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (!ui->hwnd) {
                return WC_ERROR_HEADLESS;
        }
        if (!ui->cb || !ui->cb->show_terms) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        return ui->cb->show_terms(ui->hwnd, accepted, terms);
}

struct wc_wallet {
        wc_storage_handle_t storage;
        wc_server_handle_t server;
        wc_ui_handle_t ui;
        bstring terms;
        int accepted;
        struct tm terms_tm;
};

wc_error_t wc_wallet_configure(
        wc_wallet_handle_t *wallet,
        wc_storage_handle_t storage,
        wc_server_handle_t server,
        wc_ui_handle_t ui
) {
        wc_wallet_handle_t ctx = NULL;
        /* Must have a way of returning the allocated wallet to the caller. */
        if (!wallet) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* A wallet object without storage is pretty useless. */
        if (!storage) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* A wallet object without a server connection is pretty useless. */
        if (!server) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* A wallet object without a user interface is pointless. */
        if (!ui) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Allocate the wallet object structure. */
        ctx = malloc(sizeof(struct wc_wallet));
        if (!ctx) {
                return WC_ERROR_OUT_OF_MEMORY;
        }
        /* Initialize the wallet object structure. */
        ctx->storage = storage;
        ctx->server = server;
        ctx->ui = ui;
        ctx->terms = NULL;
        ctx->accepted = 0;
        memset(&ctx->terms_tm, 0, sizeof(struct tm));
        /* Return the wallet object structure. */
        *wallet = ctx;
        return WC_SUCCESS;
}

wc_error_t wc_wallet_release(wc_wallet_handle_t wallet) {
        wc_error_t e = WC_SUCCESS;
        if (!wallet) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        if (wallet->terms) {
                bdestroy(wallet->terms);
                wallet->terms = NULL;
        }
        if (wallet->ui) {
                e = e || wc_ui_shutdown(wallet->ui);
                wallet->ui = NULL;
        }
        if (wallet->server) {
                e = e || wc_server_disconnect(wallet->server);
                wallet->server = NULL;
        }
        if (wallet->storage) {
                e = e || wc_storage_close(wallet->storage);
                wallet->storage = NULL;
        }
        free(wallet);
        return e;
}

wc_error_t wc_wallet_terms_of_service(
        wc_wallet_handle_t wallet,
        bstring *terms,
        int *accepted,
        struct tm *when
) {
        wc_error_t e = WC_SUCCESS;
        time_t now = 0;
        /* Need a wallet context. */
        if (!wallet) {
                return WC_ERROR_INVALID_ARGUMENT;
        }
        /* Fetch the current terms of service from the server. */
        if (!wallet->terms) {
                e = wc_server_get_terms(wallet->server, &wallet->terms);
                if (e == WC_SUCCESS && !wallet->terms) {
                        /* wc_server_get_terms should have returned an error code */
                        e = WC_ERROR_UNKNOWN;
                }
                if (e != WC_SUCCESS) {
                        return e;
                }
                /* Clear acceptance cache. */
                wallet->accepted = 0;
                memset(&wallet->terms_tm, 0, sizeof(struct tm));
        }
        /* Check if the user previously accepted these terms. */
        if (!wallet->accepted) {
                e = wc_storage_are_terms_accepted(wallet->storage, &wallet->accepted, &wallet->terms_tm, wallet->terms);
                if (e != WC_SUCCESS) {
                        return e;
                }
        }
        /* If the terms have not been accepted, show them to the user. */
        if (!wallet->accepted) {
                /* Show the terms of service to the user. */
                e = wc_ui_show_terms(wallet->ui, &wallet->accepted, wallet->terms);
                if (e != WC_SUCCESS) {
                        return e;
                }
                /* If the user has accepted the terms, cache the acceptance. */
                if (wallet->accepted) {
                        now = time(NULL);
                        if (!gmtime_r(&now, &wallet->terms_tm)) {
                                return WC_ERROR_OVERFLOW;
                        }
                        /* If the following call fails, then the terms of
                         * service acceptance was not saved to the wallet
                         * storage.  This is bad, but not a show stopper.
                         * The terms will just be displayed again the next
                         * time the wallet starts. */
                        wc_storage_accept_terms(wallet->storage, wallet->terms, &wallet->terms_tm);
                }
        }
        /* If the terms have already been accepted, return cached values. */
        if (terms) {
                if (*terms) {
                        bdestroy(*terms);
                        *terms = NULL;
                }
                *terms = bstrcpy(wallet->terms);
                if (!*terms) {
                        return WC_ERROR_OUT_OF_MEMORY;
                }
        }
        if (accepted) {
                *accepted = wallet->accepted;
        }
        if (when && wallet->accepted) {
                memcpy(when, &wallet->terms_tm, sizeof(struct tm));
        }
        return WC_SUCCESS;
}

/* End of File
 */
