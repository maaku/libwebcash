/* Copyright (c) 2022-2023 Mark Friedenbach
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef WEBCASH_H
#define WEBCASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bstrlib.h>

/**
 * @brief A webcash library error code.
 */
typedef enum wc_error {
        WC_SUCCESS = 0,                 /**< Success */
        WC_ERROR_OUT_OF_MEMORY = -1,    /**< Out of memory */
        WC_ERROR_INVALID_ARGUMENT = -2, /**< Invalid argument */
        WC_ERROR_OVERFLOW = -3          /**< Overflow */
} wc_error_t;

/**
 * @brief A webcash value / amount.
 */
typedef int64_t wc_amount_t;
#define WC_AMOUNT_SCALE ((wc_amount_t)100000000LL)

/**
 * @brief Zero-initialization for wc_amount_t variables.
 *
 * A helpful macro declaration that can be used to initialize
 * wc_amount_t values to zero, at declaration or anytime thereafter.
 *
 * Example:
 *    wc_amount_t my_amount = WC_ZERO;
 *    // ... later ...
 *    my_amount = WC_ZERO;
 */
#define WC_ZERO ((wc_amount_t)0LL)

/**
 * @brief A function which returns a wc_amount_t of value zero.
 *
 * This function is provided for circumstances where a function or closure is
 * required to provide initial values.  Otherwise it is simpler and more
 * performant to use the WC_ZERO macro.
 *
 * @return wc_amount_t Always returns zero.
 */
wc_amount_t wc_zero(void);

/**
 * @brief Convert a C string to a wc_amount_t.
 *
 * This function converts a C string containing a decimal number
 * representation of a webcash value to a wc_amount_t.  The string may
 * optionally contain a minus sign prefix and/or a decimal point, but no other
 * characters.  Up to 8 digits after the decimal point are supported.  The
 * resulting wc_amount_t value is scaled by 10^8 so that that the value can be
 * stored as an integer.
 *
 * @param amt A pointer to a wc_amount_t to receive the parsed value.
 * @param noncanonical An optional pointer to an int to receive a flag
 * indicating whether the parsed representation was found to be in canonical
 * format (0) or not (1).  May be NULL if not needed.
 * @param str A C string containing a decimal number representation of a
 * webcash value.
 * @return wc_error_t WC_SUCCESS if the string was successfully parsed, or an
 * error code otherwise.
 */
wc_error_t wc_from_cstring(wc_amount_t *amt, int *mutated, const char *str);

/**
 * @brief Convert a bstring to a wc_amount_t.
 *
 * This function converts a bstring containing a decimal number representation
 * of a webcash value to a wc_amount_t.  The string may optionally contain a
 * minus sign prefix and/or a decimal point, but no other characters.  Up to 8
 * digits after the decimal point are supported.  The resulting wc_amount_t
 * value is scaled by 10^8 so that that the value can be stored as an integer.
 *
 * @param amt A pointer to a wc_amount_t to receive the parsed value.
 * @param noncanonical An optional pointer to an int to receive a flag
 * indicating whether the parsed representation was found to be in canonical
 * format (0) or not (1).  May be NULL if not needed.
 * @param str A bstring containing a decimal number representation of a
 * webcash value.
 * @return wc_error_t WC_SUCCESS if the string was successfully parsed, or an
 * error code otherwise.
 */
wc_error_t wc_from_bstring(wc_amount_t *amt, int *mutated, bstring str);

/**
 * @brief Convert a wc_amount_t to a bstring.
 *
 * This function converts a wc_amount_t to a bstring containing a decimal
 * number representation of the webcash value.  The resulting string will
 * contain a decimal point if the amount represent a fractional webcash value,
 * but no other characters.  Up to 8 digits after the decimal point may be
 * generated, so long as the trailing digit is nonzero.
 *
 * @param amount The wc_amount_t to convert.
 * @return bstring A bstring containing a decimal number representation of the webcash value.
 */
bstring wc_to_bstring(wc_amount_t amount);

/**
 * @brief A webcash secret and the amount it protects.
 *
 * @amount: The amount of webcash protected by the secret.
 * @serial: The secret itself, as a bstring.
 *
 * A webcash secret is a unicode string which is used to protect a webcash
 * amount.  The server stores only the hash of this value in its database, and
 * requires anyone using the webcash to present the hash preimage (the secret)
 * as authorization.  The secret is a unicode string, and may contain any
 * unicode characters except for the null character.
 */
typedef struct wc_secret {
        wc_amount_t amount;
        bstring serial;
} wc_secret_t;

wc_error_t wc_secret_new(wc_secret_t *secret);
wc_error_t wc_secret_from_cstring(wc_secret_t *secret, wc_amount_t amount, const char *serial);
wc_error_t wc_secret_from_bstring(wc_secret_t *secret, wc_amount_t amount, bstring *serial);
wc_error_t wc_secret_from_bstring_copy(wc_secret_t *secret, wc_amount_t amount, bstring serial);
wc_error_t wc_secret_is_valid(const wc_secret_t *secret);
wc_error_t wc_secret_destroy(wc_secret_t *wc);

#ifdef __cplusplus
}
#endif

#endif /* WEBCASH_H */

/* End of File
 */
