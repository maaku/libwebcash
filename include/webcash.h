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
#include <sha2/sha256.h>

/**
 * @brief A webcash library error code.
 */
typedef enum wc_error {
        WC_SUCCESS = 0,                 /**< Success */
        WC_ERROR_INVALID_ARGUMENT = -1, /**< Invalid argument */
        WC_ERROR_OUT_OF_MEMORY = -2,    /**< Out of memory */
        WC_ERROR_OVERFLOW = -3          /**< Overflow */
} wc_error_t;

/**
 * @brief Initialize libwebcash.
 *
 * This function initializes the libwebcash library.  It must be called before
 * any other libwebcash functions are called.  It is safe to call this function
 * multiple times.
 *
 * @return wc_error_t WC_SUCCESS.
 */
wc_error_t wc_init(void);

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

/**
 * @brief Initialize a wc_secret_t.
 *
 * This function initializes a wc_secret_t to a zero amount and an empty
 * serial string with sufficient allocation to hold a typical webcash secret.
 *
 * @param secret The wc_secret_t structure to initialize.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_OUT_OF_MEMORY.
 */
wc_error_t wc_secret_new(wc_secret_t *secret);

/**
 * @brief Initialize a wc_secret_t with a given amount and serial.
 *
 * This function initializes a wc_secret_t to the given amount and serial,
 * specified as a C string.  Can fail if the bstring allocation fails, due to
 * lack of memory.
 *
 * @param secret The wc_secret_t structure to initialize.
 * @param amount The amount of webcash protected by the secret.
 * @param serial The webcash secret / serial itself, as a C string.
 * @return wc_error_t WC_SUCCESS, WC_ERROR_INVALID_ARGUMENT, or
 * WC_ERROR_OUT_OF_MEMORY.
 */
wc_error_t wc_secret_from_cstring(wc_secret_t *secret, wc_amount_t amount, const char *serial);

/**
 * @brief Initialize a wc_secret_t with a given amount and serial.
 *
 * This function initializes a wc_secret_t to the given amount and serial,
 * specified as a bstring.  The wc_secret_t takes ownership of the bstring,
 * and the caller's copy of the bstring point is set to NULL.
 *
 * Since the wc_secret_t takes ownership of the bstring, no allocation is
 * performed and this function will always return WC_SUCCESS if its arguments
 * are valid.
 *
 * @param secret The wc_secret_t structure to initialize.
 * @param amount The amount of webcash protected by the secret.
 * @param serial The webcash secret / serial itself, as a bstring.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_secret_from_bstring(wc_secret_t *secret, wc_amount_t amount, bstring *serial);

/**
 * @brief Initialize a wc_secret_t with a given amount and serial.
 *
 * This function initializes a wc_secret_t to the given amount and serial,
 * specified as a bstring.  The wc_secret_t makes a copy of the bstring, and
 * the caller's copy of the bstring is not modified.  Can fail if the bstring
 * clone operation fails, due to lack of memory.
 *
 * @param secret The wc_secret_t structure to initialize.
 * @param amount The amount of webcash protected by the secret.
 * @param serial The webcash secret / serial itself, as a bstring.
 * @return wc_error_t WC_SUCCESS, WC_ERROR_INVALID_ARGUMENT, or
 * WC_ERROR_OUT_OF_MEMORY.
 */
wc_error_t wc_secret_from_bstring_copy(wc_secret_t *secret, wc_amount_t amount, bstring serial);

/**
 * @brief Check whether a wc_secret_t is valid.
 *
 * This function checks whether a wc_secret_t is valid.  A wc_secret_t is
 * valid if its amount is a positive value and its serial is nonempty.
 *
 * @param secret The wc_secret_t to check.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_secret_is_valid(const wc_secret_t *secret);

/**
 * @brief Destroy a wc_secret_t.
 *
 * This function destroys a wc_secret_t, freeing any memory it allocated.
 * After this function returns, the wc_secret_t is no longer valid and must be
 * reinitialized before being used again.
 *
 * @param secret The wc_secret_t to destroy.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_secret_destroy(wc_secret_t *secret);

/**
 * @brief A webcash public hash and the amount allocated to it.
 *
 * @amount: The amount of webcash allocated to the hash value.
 * @hash: The hash of the webcash secret / serial.
 *
 * A webcash public hash is a 256-bit hash of a webcash secret.  The server
 * stores only this hash in its database, and requires anyone using the
 * webcash to present the hash preimage (the secret) as authorization.  The
 * server can also look up how much webcash is allocated to the hash value and
 * report that to the user.
 */
typedef struct wc_public {
        wc_amount_t amount;
        struct sha256 hash;
} wc_public_t;

/**
 * @brief Initialize a wc_public_t.
 *
 * This macro expans to an initializer list statement to assign a zero amount
 * and a zero hash to a wc_public_t structure.  It is equivalent to zeroing
 * out the structure.
 */
#define WC_PUBLIC_INIT { WC_ZERO, {0} }

/**
 * @brief Initialize a wc_public_t from a wc_secret_t, copying over the same
 * amount and hashing the serial to get the public hash value.
 *
 * @param secret The wc_secret_t to initialize the wc_public_t from.
 * @return wc_public_t The initialized wc_public_t.
 */
wc_public_t wc_public_from_secret(const wc_secret_t* secret);

/**
 * @brief Check whether a wc_public_t is valid.
 *
 * This function checks whether a wc_public_t is valid.  A wc_public_t is
 * valid if its amount is a positive value.
 *
 * @param pub The wc_public_t to check.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_public_is_valid(const wc_public_t *pub);

#ifdef __cplusplus
}
#endif

#endif /* WEBCASH_H */

/* End of File
 */
