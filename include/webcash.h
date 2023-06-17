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
        WC_SUCCESS = 0,                 /**< Success */
        WC_ERROR_INVALID_ARGUMENT = -1, /**< Invalid argument */
        WC_ERROR_OUT_OF_MEMORY = -2,    /**< Out of memory */
        WC_ERROR_OVERFLOW = -3,         /**< Overflow */
        WC_ERROR_DB_OPEN_FAILED = -4,   /**< Database open failed */
        WC_ERROR_LOG_OPEN_FAILED = -5,  /**< Recovery log open failed */
        WC_ERROR_CONNECT_FAILED = -6    /**< Connection to server failed */
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

/*****************************************************************************
 * Storage interface (database and recovery log)
 *****************************************************************************/

/* Implementation details of these structures are specific to the client. */
typedef struct wc_log *wc_log_handle_t;
typedef struct wc_log_url *wc_log_url_t;
typedef struct wc_db *wc_db_handle_t;
typedef struct wc_db_url *wc_db_url_t;

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
} wc_storage_callbacks_t;

/* Implementation details of these structures are private to the library. */
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
} wc_server_callbacks_t;

/* Implementation details of these structures are private to the library. */
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

#ifdef __cplusplus
}
#endif

#endif /* WEBCASH_H */

/* End of File
 */
