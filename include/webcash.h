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
#include <time.h> /* for time_t, struct tm */

/*****************************************************************************
 * libwebcash API
 *
 * The libwebcash library is has five different primary components:
 *
 * 1. A context-independent interface for creating and manipulating webcash
 *    datastructures, such as serializing/deserializing webcash claim codes,
 *    public hashes, and amount strings, and generating new webcash claim
 *    codes from a wallet root secret.
 *
 * 2. A storage interface for interacting with the wallet backing store and
 *    recovery log.  The caller initializes this component with a set of
 *    callbacks for interacting with platform-specific data storage APIs.
 *
 *    A default implementation is provided which uses SQLite as the backing
 *    store and a simple file as the recovery log, using POSIX file locking.
 *
 * 3. A server connection interface for doing individual calls to the webcash
 *    server for things like getting the current terms of service, checking
 *    the validity of a webcash claim code or public hash, doing a secret
 *    replacement, getting the current mining parameters, or submitting a
 *    mining payload.
 *
 *    No complete default implementation is provided as TLS connection to the
 *    webcash server is necessary, but too complex for this simple C library
 *    to safely provide.  Use your platform or language's native HTTPS client
 *    library to implement this interface and provide the necessary callbacks.
 *
 * 4. A set of interface callbacks for interacting with the user, such as
 *    prompting for acceptance of the Webcash terms of service, confirmation
 *    of intent before doing a secret replacement, and displaying webcash
 *    payment codes.
 *
 *    No default implementation is provided as user interfaces are highly
 *    platform specific.  Use your platform or language's native UI library to
 *    implement this interface and provide the necessary callbacks.
 *
 * 5. A Webcash wallet context interface for doing things like creating a new
 *    wallet, opening an existing wallet, recovering a wallet from its master
 *    secret, making payments, mining, and getting the current wallet balance.
 *
 *    The Webcash wallet context interface is constructed with handles to the
 *    storage, server connection, and user interface components, and uses them
 *    to implement the platform-independent logic of a Webcash wallet.
 *
 * Most users of the library will only need to use the Webcash wallet context
 * interface, and will have to provide just the necessary callbacks for the
 * other components to work in the platform environment on which it is run.
 *****************************************************************************/

/*****************************************************************************
 * Context-independent Webcash datastructures and APIs
 *****************************************************************************/

/**
 * @brief A webcash library error code.
 */
typedef enum wc_error {
        /** Success */
        WC_SUCCESS = 0,
        /** Invalid argument */
        WC_ERROR_INVALID_ARGUMENT,
        /** Insufficient capacity */
        WC_ERROR_INSUFFICIENT_CAPACITY,
        /** Out of memory */
        WC_ERROR_OUT_OF_MEMORY,
        /** Overflow */
        WC_ERROR_OVERFLOW,
        /** Database closed */
        WC_ERROR_DB_CLOSED,
        /** Database open failed */
        WC_ERROR_DB_OPEN_FAILED,
        /** Database corrupt */
        WC_ERROR_DB_CORRUPT,
        /** Recovery log open failed */
        WC_ERROR_LOG_OPEN_FAILED,
        /** Not connected to server */
        WC_ERROR_NOT_CONNECTED,
        /** Connection to server failed */
        WC_ERROR_CONNECT_FAILED,
        /** No user interface online */
        WC_ERROR_HEADLESS,
        /** User interface startup failed */
        WC_ERROR_STARTUP_FAILED,
        /** Unknown error.  Should never happen! */
        WC_ERROR_UNKNOWN
} wc_error_t;

/**
 * @brief A timestamp value containing seconds since the Webcash epoch.
 */
typedef time_t wc_time_t;

/**
 * @brief The offset in seconds from the Unix epoch to the Webcash epoch.
 */
#define WC_TIME_EPOCH 1641067200 /* 2022-01-01T00:00:00Z */

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
wc_error_t wc_from_cstring(
        wc_amount_t *amt,
        int *noncanonical,
        const char *str);

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
wc_error_t wc_from_bstring(
        wc_amount_t *amt,
        int *noncanonical,
        bstring str);

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
#ifdef __cplusplus
        wc_secret() : amount(WC_ZERO), serial(nullptr) {}
        wc_secret(const wc_secret &other) : amount(other.amount), serial(bstrcpy(other.serial)) {
                if (serial == nullptr) {
                        throw std::bad_alloc();
                }
        }
        wc_secret(wc_secret &&other) : amount(other.amount), serial(other.serial) {
                if (this != &other) {
                        other.serial = nullptr;
                }
        }
        wc_secret& operator=(const wc_secret &other) {
                if (this != &other) {
                        amount = other.amount;
                        bassign(serial, other.serial);
                }
                return *this;
        }
        wc_secret& operator=(wc_secret &&other) {
                if (this != &other) {
                        amount = other.amount;
                        serial = other.serial;
                        other.serial = nullptr;
                }
                return *this;
        }
        ~wc_secret() {
                bdestroy(serial);
                serial = nullptr;
        }
#endif
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
wc_error_t wc_secret_from_cstring(
        wc_secret_t *secret,
        wc_amount_t amount,
        const char *serial);

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
wc_error_t wc_secret_from_bstring(
        wc_secret_t *secret,
        wc_amount_t amount,
        bstring *serial);

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
wc_error_t wc_secret_from_bstring_copy(
        wc_secret_t *secret,
        wc_amount_t amount,
        bstring serial);

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
 * @brief Convert a wc_secret_t to a string.
 *
 * This function converts a wc_secret_t to a bstring containing the standard
 * webcash claim code representation of the secret, of the format
 * "e{amount}:secret:{serial}".  The bstring is allocated by this function,
 * and the caller is responsible for freeing it.
 *
 * @param bstr An output argument to be filled with a freshly allocated
 * bstring containing the claim code.
 * @param secret The wc_secret_t to serialize.
 * @return wc_error_t WC_SUCCESS, WC_ERROR_INVALID_ARGUMENT, or
 * WC_ERROR_OUT_OF_MEMORY.
 */
wc_error_t wc_secret_to_string(
        bstring *bstr,
        const wc_secret_t *secret);

/**
 * @brief Parse a wc_secret_t from a string.
 *
 * This function parses a wc_secret_t from a bstring containing a webcash
 * claim code of the standard format "e{amount}:secret:{serial}".  The serial
 * of the webcash secret is newly allocated, and the caller is responsible for
 * freeing both secret and the passed-in bstring.
 *
 * @param secret An output argument to be filled with the parsed wc_secret_t.
 * @param noncanonical An output argument to be filled with a boolean
 * indicating whether the claim code deviated from canonical format.
 * @param bstr The bstring containing the claim code to parse.
 * @return wc_error_t WC_SUCCESS, WC_ERROR_INVALID_ARGUMENT, or
 * WC_ERROR_OUT_OF_MEMORY.
 */
wc_error_t wc_secret_parse(
        wc_secret_t *secret,
        int *noncanonical,
        bstring bstr);

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
wc_public_t wc_public_from_secret(const wc_secret_t *secret);

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

/**
 * @brief Encode a wc_public_t as a bstring.
 *
 * This function serializes the passed in wc_public_t to a heap-allocated
 * string, stored in the passed-in bstring pointer.  The caller is assumes
 * responsibility for freeing the bstring.
 *
 * @param bstr An output argument to be filled with a freshly allocated
 * bstring containing the encoded wc_public_t.
 * @param pub The wc_public_t to encode.
 * @return wc_error_t WC_SUCCESS, WC_ERROR_INVALID_ARGUMENT, or
 * WC_ERROR_OUT_OF_MEMORY.
 */
wc_error_t wc_public_to_string(
        bstring *bstr,
        const wc_public_t *pub);

/**
 * @brief Parse a wc_public_t from a bstring.
 *
 * This function parses a wc_public_t from a bstring containing a webcash
 * public hash of the standard format "e{amount}:public:{hash}", with the hash
 * being a 32-byte hexadecimal number with lowercase letters (uppercase
 * letters are understood but noncanonical).
 *
 * @param pub An output argument to be filled with the parsed wc_public_t.
 * @param noncanonical An output argument to be filled with a boolean
 * indicating whether the public hash deviated from canonical format.
 * @param bstr The bstring containing the public hash to parse.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_public_parse(
        wc_public_t *pub,
        int *noncanonical,
        bstring bstr);

/**
 * @brief The strings "000" through "999", base64-encoded.
 *
 * Efficient webcash mining is achieved by using sha256 midstates to compute
 * the hashes of multiple webcash mining payloads in parallel.  Since webcash
 * mining payloads are base64-encoded JSON, doing this efficiently requires
 * having the mining payloads differ only by a short, pre-encoded base64
 * value.
 *
 * This array contains the base64-encoded strings "000" through "999" in
 * order, so that the mining payloads can be efficiently generated by
 * replacing the last three or six bytes of the base64-encoded JSON with the
 * appropriate values from this array, to generate a JSON nonce value within
 * the range of 0..999 or 0..999999.
 */
extern unsigned char wc_mining_nonces[4*1000];

/**
 * @brief The final four bytes of a base64-encoded webcash mining payload.
 *
 * When using wc_mining_nonces to generate nonce digits for a webcash mining
 * payload, the JSON payload still needs to be terminated with a '}'
 * character.  This 4-character string is the base64 encoding of the JSON
 * object closing brace.
 */
extern unsigned char wc_mining_final[4];

/**
 * @brief Complete eight different SHA-256 hashes in parallel, using a common
 * midstate but different nonce values, as needed for efficient webcash
 * mining.
 *
 * This function completes eight different but equal-length SHA-256 hashes in
 * parallel, starting with the same midstate but appending different nonce
 * values for some of the terminating bytes.  The API is designed for use by
 * webcash mining software.
 *
 * @param ctx The SHA-256 hash midstate.
 * @param nonce1 The first 4-byte nonce value to append to the hash, which is used for all hashes.
 * @param nonce2 An array of 4-byte nonce values to be appended next, one for each hash.
 * @param hashes An array of 32-byte buffers to be filled with the resulting hash values.
 */
void wc_mining_8way(
        unsigned char hashes[8*32],
        const struct sha256_ctx* ctx,
        const unsigned char nonce1[4],
        const unsigned char nonce2[4*8],
        const unsigned char final[4]);

/**
 * @brief Derive a webcash secret from a master / root secret and chaincode +
 * depth.
 *
 * Deterministic webcash wallets derive successive webcash secrets from a
 * single master secret using a deterministic application of the SHA256
 * hashing function.  The chaincode specifies which sequence of secrets to
 * use, and the depth is the ordinal number of the secret from this chain.
 *
 * This API performs bstring allocation and therefore can fail.  See also
 * wc_derive_serials which performs multiple secret derivations in parallel
 * (where supported by the underlying hardware), storing generated secrets to
 * a caller-provided buffer and never fails.
 *
 * @param serial A pointer to a bstring to be filled with the generated serial.
 * @param hdroot The master secret to derive from.
 * @param chaincode The chaincode to use.
 * @param depth The depth of the secret to derive.
 * @return wc_error_t WC_SUCCESS, WC_ERROR_INVALID_ARGUMENT, or
 * WC_ERROR_OUT_OF_MEMORY.
 */
wc_error_t wc_derive_serial(
        bstring *bstr,
        const struct sha256 *hdroot,
        uint64_t chaincode,
        uint64_t depth);

/**
 * @brief Derive multiple webcash secrets from a master / root secret and
 * chaincode + depth.
 *
 * This function derives multiple webcash secrets from a single master secret
 * using a deterministic application of the SHA256 hashing function, making
 * use of parallel / vector code on platforms that support it.  The chaincode
 * specifies which sequence of secrets to use, and start is the ordinal number
 * of the first secret from that chain to generate.  A total of count
 * consecutive secrets from the chain are generated.  The secrets are written
 * to a caller-provided buffer, which must be at least count * 64 bytes in
 * size.  [64 = 2 * sizeof(struct sha256), the size of a hex-encoded hash
 * value.]
 *
 * @param out A buffer to be filled with the generated secrets, of length at least count * 64 bytes.
 * @param hdroot The master secret to derive from.
 * @param chaincode The chaincode to use.
 * @param start The depth of the first secret to derive.
 * @param count The number of secrets to derive.
 */
void wc_derive_serials(
        char out[],
        const struct sha256 *hdroot,
        uint64_t chaincode,
        uint64_t start,
        size_t count);

/**
 * @brief The terms of service.
 *
 * The terms of service are UTF-8 encoded text documents that the user must
 * accept before using the server or wallet APIs.  The terms are stored in the
 * database upon acceptance, and compared against the server's terms when the
 * wallet context is opened.  If the server's terms have changed since the
 * last time the wallet was opened, the user must accept the new terms before
 * continuing.
 *
 * The time at which the terms were accepted is also stored in the database,
 * in the form of a wc_time_t value (seconds since WC_TIME_EPOCH).  For
 * convenience, this value is converted into a standard struct tm value when
 * returned by these storage interface APIs.
 */
typedef struct wc_terms {
        struct tm when; /* UTC time zone */
        bstring text; /* UTF-8 (text/plain) */
#ifdef __cplusplus
        wc_terms() : when{0}, text(nullptr) {}
        wc_terms(const wc_terms &other) : when(other.when), text(bstrcpy(other.text)) {
                if (text == nullptr) {
                        throw std::bad_alloc();
                }
        }
        wc_terms(wc_terms &&other) : when(other.when), text(other.text) {
                if (this != &other) {
                        other.text = nullptr;
                }
        }
        wc_terms& operator=(const wc_terms &other) {
                if (this != &other) {
                        when = other.when;
                        bassign(text, other.text);
                }
                return *this;
        }
        wc_terms& operator=(wc_terms &&other) {
                if (this != &other) {
                        when = other.when;
                        text = other.text;
                        other.text = nullptr;
                }
                return *this;
        }
        ~wc_terms() {
                bdestroy(text);
                text = nullptr;
        }
#endif
} wc_terms_t;

/*****************************************************************************
 * Storage interface (database and recovery log)
 *****************************************************************************/

/* Implementation details of these structures are specific to the client. */
typedef struct wc_log *wc_log_handle_t;
typedef struct wc_log_url *wc_log_url_t;
typedef struct wc_db *wc_db_handle_t;
typedef struct wc_db_url *wc_db_url_t;

typedef struct wc_db_terms {
        wc_time_t when; /* sec since WC_TIME_EPOCH */
        bstring text; /* UTF-8 (text/plain) */
#ifdef __cplusplus
        wc_db_terms() : when{0}, text(nullptr) {}
        wc_db_terms(const wc_db_terms &other) : when(other.when), text(bstrcpy(other.text)) {
                if (text == nullptr) {
                        throw std::bad_alloc();
                }
        }
        wc_db_terms(wc_db_terms &&other) : when(other.when), text(other.text) {
                if (this != &other) {
                        other.text = nullptr;
                }
        }
        wc_db_terms& operator=(const wc_db_terms &other) {
                if (this != &other) {
                        when = other.when;
                        bassign(text, other.text);
                }
                return *this;
        }
        wc_db_terms& operator=(wc_db_terms &&other) {
                if (this != &other) {
                        when = other.when;
                        text = other.text;
                        other.text = nullptr;
                }
                return *this;
        }
        ~wc_db_terms() {
                bdestroy(text);
                text = nullptr;
        }
#endif
} wc_db_terms_t;

/**
 * @brief Callbacks for interacting with data storage for the wallet,
 * including both a read-write database and an append-only recovery log.
 */
typedef struct wc_storage_callbacks {
        /* Initialization */
        wc_log_handle_t (*log_open)(wc_log_url_t logurl);
        void (*log_close)(wc_log_handle_t log); /* optional */
        wc_db_handle_t (*db_open)(wc_db_url_t dburl);
        void (*db_close)(wc_db_handle_t db); /* optional */

        /* Terms of Service */
        wc_error_t (*any_terms)(wc_db_handle_t db, int *any);
        wc_error_t (*all_terms)(wc_db_handle_t db, wc_db_terms_t *terms, size_t *count);
        wc_error_t (*terms_accepted)(wc_db_handle_t db, bstring terms, wc_time_t *when);
        wc_error_t (*accept_terms)(wc_db_handle_t db, bstring terms, wc_time_t now);
} wc_storage_callbacks_t;

/* Implementation details of this structure is private to the library. */
typedef struct wc_storage *wc_storage_handle_t;

/**
 * @brief Open storage for a wallet.
 *
 * Does whatever platform-specific initialization is required to open the
 * wallet's storage interface, which includes both the database and recovery
 * log.  The callbacks parameter is a pointer to a structure containing
 * client-defined callbacks for interfacing with persistent data storage.

 * The logurl and dburl parameters are client-defined connection strings for
 * opening the log and database, respectively.  logurl will be passed to
 * callbacks->log_open, and dburl will be passed to callbacks->db_open.  Both
 * callbacks must return a non-NULL handle on success, or else wc_storage_open
 * fails returning an error code.
 *
 * When no longer needed, the storage interface should be closed with
 * wc_storage_close.
 *
 * @param storage An out parameter to be filled in with a pointer to the
 * internal storage state.  Only modified if the function returns WC_SUCCESS.
 * @param callbacks A pointer to a structure containing client-configured
 * callbacks for interfacing with persistent data storage.
 * @param logurl Some client-defined path for opening a log file.
 * @param dburl Some client-defined connection string for opening a database.
 * @return wc_error_t WC_SUCCESS, WC_ERROR_INVALID_ARGUMENT,
 * WC_ERROR_OUT_OF_MEMORY, WC_ERROR_LOG_OPEN_FAILED, or
 * WC_ERROR_DB_OPEN_FAILED.
 */
wc_error_t wc_storage_open(
        wc_storage_handle_t *storage,
        const wc_storage_callbacks_t *callbacks,
        wc_log_url_t logurl,
        wc_db_url_t dburl);

/**
 * @brief Close a wallet's storage interface, persisting data to disk and
 * freeing all associated resources.
 *
 * @param storage The wallet file to close.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_storage_close(wc_storage_handle_t storage);

/**
 * @brief Returns all terms of service that have been accepted by the user.
 *
 * The terms are returned via the passed-in array of wc_terms_t structures,
 * whose memory is managed by the caller.  The count parameter must initially
 * contain the capacity of the array pointed to by terms, and is filled in
 * with the actual number of terms returned.
 *
 * If the terms parameter is NULL or the capacity is insufficiently large
 * enough, the function returns WC_ERROR_INSUFFICIENT_CAPACITY and the count
 * parameter is filled in with the required capacity.
 *
 * @param storage The wallet storage interface.
 * @param terms An array of wc_terms_t structures to be filled in with the
 * terms of service accepted by the user.
 * @param count The capacity of the terms array, initially, and the number of
 * terms returned, on success.
 * @return wc_error_t WC_SUCCESS if the terms of service were successfully
 * stored in terms and count, WC_ERROR_INSUFFICIENT_CAPACITY if the passed-in
 * capacity is insufficient, or an error code if there was an internal problem
 * in performing this operation.
 */
wc_error_t wc_storage_enumerate_terms(
        wc_storage_handle_t storage,
        wc_terms_t *terms,
        size_t *count);

/**
 * @brief Returns whether the user has accepted any version of the Webcash
 * terms of service.
 *
 * In many cases it is sufficient to know whether the user has accepted any
 * version of the terms of service, in particular if the user will not be
 * interacting with the server (e.g. just querying the wallet balance from its
 * own internal state).
 *
 * This API stores a non-zero value in the accepted parameter if the user has
 * ever accepted any version of the terms of service, without checking if the
 * terms of service are current, or zero otherwise.
 *
 * @param storage The wallet storage interface.
 * @param accepted An out parameter to be filled in with a non-zero value if
 * the user has accepted any version of the terms of service, or zero
 * otherwise.  Only valid if the function returns WC_SUCCESS.
 * @return wc_error_t WC_SUCCESS if the terms of service acceptance was
 * successfully checked and the result stored in the out parameter accepted
 * (even if the result was negatigve), or an error code otherwise.
 */
wc_error_t wc_storage_have_accepted_terms(
        wc_storage_handle_t storage,
        int *accepted);

/**
 * @brief Returns whether the user has accepted the passed-in version of the
 * Webcash terms of service.
 *
 * The first step of connecting to the server is to fetch the current terms of
 * service, and see if the user has accepted them.  This API is used for
 * checking if the user has accepted a specific version of the terms of
 * service, passed in as a text document.
 *
 * @param storage The wallet storage interface.
 * @param accepted An out parameter to be filled in with a non-zero value if
 * the user has accepted the passed-in terms of service, or zero otherwise.
 * Only valid if the function returns WC_SUCCESS.
 * @param when An out parameter to be filled in with the time at which the
 * terms were accepted, if accepted is non-zero.  Only valid if the function
 * returns WC_SUCCESS and accepted is non-zero.
 * @param terms The specific text of the terms of service to be checked.
 * @return wc_error_t WC_SUCCESS if the terms of service acceptance was
 * successfully checked and the out parameters accepted and when have been
 * filled, or an error code otherwise.
 */
wc_error_t wc_storage_are_terms_accepted(
        wc_storage_handle_t storage,
        int *accepted,
        struct tm *when,
        bstring terms);

/**
 * @brief Accept the passed-in terms of service.
 *
 * This API is used to accept the current terms of service, which are passed
 * in as a text document.  The terms are stored in the database, along with
 * the current time at which they were accepted.  If now is NULL, the current
 * system time is used instead.
 *
 * @param storage The wallet storage interface.
 * @param terms The specific text of the terms of service to be accepted.
 * @param now The time at which the terms were accepted, or NULL to use the
 * current system time.
 * @return wc_error_t WC_SUCCESS if acceptance of the terms of service was
 * successfully saved to the wallet storage, or an error code otherwise.
 */
wc_error_t wc_storage_accept_terms(
        wc_storage_handle_t storage,
        bstring terms,
        struct tm *now);

/*****************************************************************************
 * Server connection interface
 *****************************************************************************/

/* Implementation details of these structures are specific to the client. */
typedef struct wc_conn *wc_conn_handle_t;
typedef struct wc_server_url *wc_server_url_t;

typedef struct wc_server_callbacks {
        /* Initialization */
        wc_conn_handle_t (*connect)(wc_server_url_t url);
        void (*disconnect)(wc_conn_handle_t conn);

        /* Terms of Service */
        wc_error_t (*get_terms)(wc_conn_handle_t conn, bstring *terms);
} wc_server_callbacks_t;

/* Implementation details of this structure is private to the library. */
typedef struct wc_server *wc_server_handle_t;

/**
 * @brief Open a connection to the webcash server.
 *
 * This API is optimistically named "connect" but it may not actually initiate
 * a connection.  The simplest implementation would merely set up callbacks
 * for the server object, and records the server URL.
 *
 * With upstream server support it is possible that some implementations may
 * actually open a long-lived connection to the server that can be reused for
 * multiple queries.
 *
 * @param server The server object to be filled in.
 * @param callbacks The callbacks to be used for interacting with the server.
 * @param url The URL of the server to connect to.
 * @return wc_error_t WC_SUCCESS if the server object was successfully
 * initialized, or an error code otherwise.
 */
wc_error_t wc_server_connect(
        wc_server_handle_t *server,
        const wc_server_callbacks_t *callbacks,
        wc_server_url_t url);

/**
 * @brief Close a connection to the webcash server.
 *
 * Tears down any open connections to the server, and frees any resources
 * associated with the server object.
 *
 * @param server The server object to be closed.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_server_disconnect(wc_server_handle_t server);

/**
 * @brief Get the current terms of service from the server.
 *
 * This API is used to fetch the current terms of service from the server.
 * The terms are returned as a UTF-8 encoded text/plain document.
 *
 * @param server A connected server connection.
 * @param terms An out parameter to be filled in with the current terms of
 * service.  Only valid if the function returns WC_SUCCESS.
 * @return wc_error_t WC_SUCCESS if the terms of service were successfully
 * fetched and the out parameter terms has been filled, or an error code
 * otherwise.
 */
wc_error_t wc_server_get_terms(
        wc_server_handle_t server,
        bstring *terms);

/*****************************************************************************
 * User interface callbacks
 *****************************************************************************/

/* Implementation details of these structures are specific to the client. */
typedef struct wc_window *wc_window_handle_t;
typedef struct wc_window_params *wc_window_params_t;

typedef struct wc_ui_callbacks {
        /* Initialization */
        wc_window_handle_t (*startup)(wc_window_params_t params);
        void (*shutdown)(wc_window_handle_t window);

        /* Terms of Service */
        wc_error_t (*show_terms)(wc_window_handle_t window, int *accepted, bstring terms);
} wc_ui_callbacks_t;

/* Implementation details of this structure is private to the library. */
typedef struct wc_ui *wc_ui_handle_t;

/**
 * @brief Initialize the user interface.
 *
 * This API is optimistically named "init" but it may not actually initialize
 * anything.  The simplest implementation would merely set up callbacks for
 * the UI object.
 *
 * @param ui The UI object to be filled in.
 * @param callbacks The callbacks to be used for interacting with the user
 * interface.
 * @param params The parameters to be used for initializing the user
 * interface.
 * @return wc_error_t WC_SUCCESS if the UI object was successfully
 * initialized, or an error code otherwise.
 */
wc_error_t wc_ui_startup(
        wc_ui_handle_t *ui,
        const wc_ui_callbacks_t *callbacks,
        wc_window_params_t params);

/**
 * @brief Shut down the user interface.
 *
 * Tears down any open windows, and frees any resources associated with the UI
 * object.
 *
 * @param ui The UI object to be shut down.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_ui_shutdown(wc_ui_handle_t ui);

/**
 * @brief Show the terms of service to the user, and record acceptance.
 *
 * This API is used to show the terms of service to the user, and request the
 * user to accept.  The terms are passed to the user interface as a UTF-8
 * encoded text/plain document.
 *
 * @param ui A connected user interface.
 * @param accepted An out parameter to be filled in with the user's acceptance
 * of the terms of service (non-zero indicates unacceptance).  Only valid if
 * the function returns WC_SUCCESS.
 * @param terms The terms of service to be shown to the user.
 * @return wc_error_t WC_SUCCESS if the terms of service were successfully
 * shown and the out parameter accepted has been filled, or an error code
 * otherwise.  WC_SUCCESS does not mean the terms of service were accepted,
 * but rather that the user interface was successfully shown and the user's
 * yes/no response was recorded.
 */
wc_error_t wc_ui_show_terms(
        wc_ui_handle_t ui,
        int *accepted,
        bstring terms);

/*****************************************************************************
 * Wallet context interface
 *****************************************************************************/

/* Implementation details of this structure is private to the library. */
typedef struct wc_wallet *wc_wallet_handle_t;

/**
 * @brief Setup the wallet context.
 *
 * This API is used to initialize the wallet context, which is used to
 * coordinate the user interface, the wallet storage, and the server
 * connection.
 *
 * The wallet context takes ownership of the passed in storage, server, and ui
 * interfaces.  The caller should not use these interfaces after passing them
 * to this function, nor should the caller attempt to free them.
 *
 * @param wallet An out parameter to be filled in with the wallet context.
 * @param storage The wallet storage interface.
 * @param server The server connection object.
 * @param ui The user interface object.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_OUT_OF_MEMORY.
 */
wc_error_t wc_wallet_configure(
        wc_wallet_handle_t *wallet,
        wc_storage_handle_t storage,
        wc_server_handle_t server,
        wc_ui_handle_t ui);

/**
 * @brief Shut down the wallet context.
 *
 * Tears down any open windows, closes any open connections, and frees any
 * resources associated with the wallet context.
 *
 * @param wallet The wallet context to be shut down.
 * @return wc_error_t WC_SUCCESS or WC_ERROR_INVALID_ARGUMENT.
 */
wc_error_t wc_wallet_release(wc_wallet_handle_t wallet);

/**
 * @brief Ensure the user has accepted the Webcash terms of service.
 *
 * Fetches the current Webcash terms of service from the server, checks if the
 * user has accepted this version of the terms, and if not, shows the terms to
 * the user and records their response.
 *
 * A return value of WC_SUCCESS indicates successful execution of this API,
 * not acceptance of the terms of service.  For that you must check the output
 * parameters.  accepted will be set to 1 if the user previously accepted the
 * current Webcash terms of service, or if the the terms were presented to the
 * user at this time and the user clicked "Agree."  If the user explicitly
 * rejects the terms of service, or performs any action (e.g. closing the
 * prompt window) which cannot be construed as acceptance, then the return
 * value will still be WC_SUCCESS, but accepted will be 0.
 *
 * Rejection is not permanent: calling this API multiple times will result in
 * multiple UI prompts until the current terms of service are accepted.
 *
 * Other return values are possible, and indicate an error condition.
 *
 * @param wallet A connected wallet context.
 * @param terms An out parameter to be filled in with the current terms of
 * service.  Only valid if the function returns WC_SUCCESS.
 * @param accepted An out parameter to be filled in with the user's response
 * to the terms of service (non-zero indicates acceptance).  Only valid if the
 * function returns WC_SUCCESS.
 * @param when An out parameter to be filled in with the time (possibly in the
 * past) at which the terms of service were accepted.  Only valid if the
 * function returns WC_SUCCESS.
 * @return wc_error_t WC_SUCCESS if the output parameters have been filled.
 * WC_SUCCESS does not mean the terms of service were accepted, but rather
 * that the terms of service had already been accepted and stored in the
 * wallet, or the user interface was successfully shown and the user's yes/no
 * response was recorded.  Other error codes are possible, and indicate an
 * error condition.
 */
wc_error_t wc_wallet_terms_of_service(
        wc_wallet_handle_t wallet,
        bstring *terms,
        int *accepted,
        struct tm *when);

#ifdef __cplusplus
}
#endif

#endif /* WEBCASH_H */

/* End of File
 */
