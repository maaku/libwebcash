/* Copyright (c) 2022-2023 Mark Friedenbach
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>

#include <webcash.h>

#include <bstraux.h>

TEST(gtest, wc_zero) {
        wc_amount_t init = 0;
        wc_amount_t defn = WC_ZERO;
        wc_amount_t func = wc_zero();
        EXPECT_EQ(init, 0);
        EXPECT_EQ(defn, 0);
        EXPECT_EQ(func, 0);
        EXPECT_EQ(init, defn);
        EXPECT_EQ(init, func);
        EXPECT_EQ(defn, func);
}

static void test_cstring(
        const char *str,
        wc_error_t err,
        wc_amount_t amt = 0,
        bool noncanonical = false
) {
        wc_amount_t amt2 = -1;
        int noncanonical2 = -1;
        bstring bstr = NULL;
        EXPECT_EQ(wc_from_cstring(&amt2, &noncanonical2, str), err);
        if (err == WC_SUCCESS) {
                EXPECT_EQ(amt2, amt);
                EXPECT_EQ(noncanonical2, !!noncanonical);
                bstr = wc_to_bstring(amt);
                EXPECT_NE(bstr, nullptr);
                if (bstr) {
                        EXPECT_EQ(bstr->slen == strlen(str) && !memcmp(bstr->data, str, bstr->slen), !noncanonical);
                        bdestroy(bstr); bstr = NULL;
                }
        } else {
                EXPECT_EQ(amt2, -1);
                EXPECT_EQ(noncanonical2, -1);
        }
}

TEST(gtest, wc_from_string) {
        test_cstring("0", WC_SUCCESS, 0);
        test_cstring("0.", WC_SUCCESS, 0, true);
        test_cstring("0.0", WC_SUCCESS, 0, true);
        test_cstring("0.00", WC_SUCCESS, 0, true);
        test_cstring("0.000", WC_SUCCESS, 0, true);
        test_cstring("0.0000", WC_SUCCESS, 0, true);
        test_cstring("0.00000", WC_SUCCESS, 0, true);
        test_cstring("0.000000", WC_SUCCESS, 0, true);
        test_cstring("0.0000000", WC_SUCCESS, 0, true);
        test_cstring("0.00000000", WC_SUCCESS, 0, true);
        test_cstring("0.000000001", WC_ERROR_INVALID_ARGUMENT);
        test_cstring("0.00000001", WC_SUCCESS, 1);
        test_cstring("1.00000000", WC_SUCCESS, 100000000, true);
        test_cstring("1.00000001", WC_SUCCESS, 100000001);
        test_cstring("1.00000010", WC_SUCCESS, 100000010, true);
        test_cstring("1.00000100", WC_SUCCESS, 100000100, true);
        test_cstring("1.00001000", WC_SUCCESS, 100001000, true);
        test_cstring("1.00010000", WC_SUCCESS, 100010000, true);
        test_cstring("1.00100000", WC_SUCCESS, 100100000, true);
        test_cstring("1.01000000", WC_SUCCESS, 101000000, true);
        test_cstring("1.10000000", WC_SUCCESS, 110000000, true);
        test_cstring("1.1000000", WC_SUCCESS, 110000000, true);
        test_cstring("1.100000", WC_SUCCESS, 110000000, true);
        test_cstring("1.10000", WC_SUCCESS, 110000000, true);
        test_cstring("1.1000", WC_SUCCESS, 110000000, true);
        test_cstring("1.100", WC_SUCCESS, 110000000, true);
        test_cstring("1.10", WC_SUCCESS, 110000000, true);
        test_cstring("1.1", WC_SUCCESS, 110000000);
        test_cstring("1", WC_SUCCESS, 100000000);
        test_cstring("1.", WC_SUCCESS, 100000000, true);
        test_cstring("1.000000000", WC_SUCCESS, 100000000, true);
        test_cstring("\"1.0\"", WC_ERROR_INVALID_ARGUMENT);
}

TEST(gtest, wc_secret_new) {
        wc_secret_t secret = {0};
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_ERROR_INVALID_ARGUMENT);
        ASSERT_EQ(wc_secret_new(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_EQ(biseqcstr(secret.serial, ""), 1);
        secret.amount = INT64_C(1);
        EXPECT_EQ(secret.amount, INT64_C(1));
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_EQ(secret.serial, nullptr);
}

TEST(gtest, wc_secret_from_cstring) {
        wc_secret_t secret = {0};
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_ERROR_INVALID_ARGUMENT);
        ASSERT_EQ(wc_secret_from_cstring(&secret, INT64_C(1), "abc"), WC_SUCCESS);
        EXPECT_EQ(secret.amount, INT64_C(1));
        ASSERT_NE(secret.serial, nullptr);
        EXPECT_EQ(biseqcstr(secret.serial, "abc"), 1);
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_EQ(secret.serial, nullptr);
}

TEST(gtest, wc_secret_from_bstring) {
        wc_secret_t secret = {0};
        bstring bstr = nullptr;
        unsigned char *cstr = nullptr;
        bstr = bfromcstr("abc");
        ASSERT_NE(bstr, nullptr);
        cstr = bstr->data;
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_ERROR_INVALID_ARGUMENT);
        ASSERT_EQ(wc_secret_from_bstring(&secret, INT64_C(1), &bstr), WC_SUCCESS);
        EXPECT_EQ(bstr, nullptr);
        EXPECT_EQ(secret.amount, INT64_C(1));
        EXPECT_EQ(biseqcstr(secret.serial, "abc"), 1);
        EXPECT_EQ(secret.serial->data, cstr);
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_EQ(secret.serial, nullptr);
}

TEST(gtest, wc_secret_from_bstring_copy) {
        wc_secret_t secret = {0};
        bstring bstr = nullptr;
        bstr = bfromcstr("abc");
        ASSERT_NE(bstr, nullptr);
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_ERROR_INVALID_ARGUMENT);
        ASSERT_EQ(wc_secret_from_bstring_copy(&secret, INT64_C(1), bstr), WC_SUCCESS);
        EXPECT_EQ(secret.amount, INT64_C(1));
        EXPECT_EQ(biseqcstr(secret.serial, "abc"), 1);
        EXPECT_NE(secret.serial, bstr); /* comparing pointers */
        EXPECT_NE(secret.serial->data, bstr->data);
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_EQ(secret.serial, nullptr);
        EXPECT_EQ(biseqcstr(bstr, "abc"), 1);
        EXPECT_EQ(bdestroy(bstr), BSTR_OK);
}

TEST(gtest, wc_secret_is_valid) {
        wc_secret_t secret = {0};
        /* Must pass in a secret value. */
        EXPECT_EQ(wc_secret_is_valid(nullptr), WC_ERROR_INVALID_ARGUMENT);
        /* Zero-initialized secret has both .amount and .serial invalid. */
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_ERROR_INVALID_ARGUMENT);
        /* Secret with valid .amount and invalid .serial is invalid. */
        secret.amount = INT64_C(1);
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_ERROR_INVALID_ARGUMENT);
        /* Secret with invalid .amount and valid .serial is invalid. */
        secret.amount = WC_ZERO;
        secret.serial = bfromcstr("abc");
        ASSERT_NE(secret.serial, nullptr);
        EXPECT_EQ(biseqcstr(secret.serial, "abc"), 1);
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_ERROR_INVALID_ARGUMENT);
        /* Secret with valid .amount and valid .serial is valid. */
        secret.amount = INT64_C(1);
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_SUCCESS);
        /* Secret with a NUL character in .serial is invalid. */
        secret.serial->data[1] = '\0';
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_ERROR_INVALID_ARGUMENT);
        /* Cleanup works on invalid secrets. */
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_EQ(secret.serial, nullptr);
}

TEST(gtest, wc_secret_destroy) {
        wc_secret_t secret = {0};
        EXPECT_EQ(wc_secret_destroy(&secret), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(secret.serial, nullptr);
        EXPECT_EQ(wc_secret_new(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_NE(secret.serial, nullptr);
        if (secret.serial) {
                EXPECT_GT(secret.serial->mlen, 0);
                EXPECT_EQ(secret.serial->slen, 0);
                EXPECT_NE(secret.serial->data, nullptr);
                EXPECT_EQ(biseqcstr(secret.serial, ""), 1);
        }
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.serial, nullptr);
        EXPECT_EQ(wc_secret_destroy(&secret), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(secret.serial, nullptr);
}

TEST(gtest, wc_secret_string) {
        wc_secret_t secret = {0};
        bstring bstr = nullptr;
        int noncanonical = -1;
        ASSERT_EQ(wc_secret_from_cstring(nullptr, INT64_C(1234567800), "abc"), WC_ERROR_INVALID_ARGUMENT);
        ASSERT_EQ(wc_secret_from_cstring(&secret, INT64_C(1234567800), "abc"), WC_SUCCESS);
        EXPECT_EQ(secret.amount, INT64_C(1234567800));
        ASSERT_NE(secret.serial, nullptr);
        EXPECT_EQ(biseqcstr(secret.serial, "abc"), 1);
        EXPECT_EQ(wc_secret_to_string(&bstr, &secret), WC_SUCCESS);
        EXPECT_NE(bstr, nullptr);
        EXPECT_EQ(biseqcstr(bstr, "e12.345678:secret:abc"), 1);
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_EQ(secret.serial, nullptr);
        EXPECT_EQ(wc_secret_parse(&secret, nullptr, bstr), WC_SUCCESS);
        EXPECT_EQ(secret.amount, INT64_C(1234567800));
        EXPECT_EQ(biseqcstr(secret.serial, "abc"), 1);
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(bdestroy(bstr), BSTR_OK);
}

TEST(gtest, wc_public_init) {
        wc_public_t pub = WC_PUBLIC_INIT;
        struct sha256 hash = {0};
        EXPECT_EQ(pub.amount, WC_ZERO);
        EXPECT_EQ(memcmp(&pub.hash.u8, &hash.u8, sizeof(hash.u8)), 0);
}

TEST(gtest, wc_public_from_secret) {
        wc_secret_t secret = {0};
        wc_public_t pub = WC_PUBLIC_INIT;
        struct sha256 hash = { /* sha256(b"abc").digest() */
                0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
                0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
                0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
                0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
        };
        EXPECT_EQ(wc_secret_from_cstring(&secret, INT64_C(1), "abc"), WC_SUCCESS);
        EXPECT_EQ(wc_secret_is_valid(&secret), WC_SUCCESS);
        pub = wc_public_from_secret(&secret);
        EXPECT_EQ(pub.amount, INT64_C(1));
        EXPECT_EQ(memcmp(&pub.hash.u8, &hash.u8, sizeof(hash.u8)), 0);
        /* Cleanup */
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
}

TEST(gtest, wc_public_is_valid) {
        wc_public_t pub = WC_PUBLIC_INIT;
        EXPECT_EQ(wc_public_is_valid(nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_public_is_valid(&pub), WC_ERROR_INVALID_ARGUMENT);
        pub.amount = INT64_C(1);
        EXPECT_EQ(wc_public_is_valid(&pub), WC_SUCCESS);
}

TEST(gtest, wc_public_string) {
        const struct sha256 zero = {0};
        const struct sha256 hash = { /* sha256(b"abc").digest() */
                0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
                0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
                0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
                0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
        };
        wc_public_t pub = {
                INT64_C(1234567800), /* 12.345678 */
                hash
        };
        bstring bstr = nullptr;
        int noncanonical = -1;
        EXPECT_EQ(wc_public_to_string(&bstr, &pub), WC_SUCCESS);
        EXPECT_NE(bstr, nullptr);
        EXPECT_EQ(biseqcstr(bstr, "e12.345678:public:ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), 1);
        pub = WC_PUBLIC_INIT;
        EXPECT_EQ(pub.amount, WC_ZERO);
        EXPECT_EQ(memcmp(&pub.hash.u8, &zero.u8, sizeof(zero.u8)), 0);
        EXPECT_EQ(wc_public_parse(&pub, &noncanonical, bstr), WC_SUCCESS);
        EXPECT_EQ(noncanonical, 0);
        EXPECT_EQ(pub.amount, INT64_C(1234567800));
        EXPECT_EQ(memcmp(&pub.hash.u8, &hash.u8, sizeof(hash.u8)), 0);
}

TEST(gtest, wc_mining_nonces) {
        tagbstring b64 = {0};
        bstring dec = nullptr;
        int error = 0;
        int i = 0, u = 0, t = 0, h = 0;
        blk2tbstr(b64, wc_mining_nonces, sizeof(wc_mining_nonces));
        dec = bBase64DecodeEx(&b64, &error);
        EXPECT_EQ(error, BSTR_OK);
        ASSERT_NE(dec, nullptr);
        EXPECT_EQ(dec->slen, 3*1000);
        for (; i < 1000; ++i) {
                u = i % 10;
                t = (i / 10) % 10;
                h = (i / 100) % 10;
                EXPECT_EQ(dec->data[3*i], '0' + h);
                EXPECT_EQ(dec->data[3*i + 1], '0' + t);
                EXPECT_EQ(dec->data[3*i + 2], '0' + u);
        }
        EXPECT_EQ(dec->data[3*i], '\0');
}

TEST(gtest, wc_mining_final) {
        tagbstring b64 = {0};
        bstring dec = nullptr;
        int error = 0;
        blk2tbstr(b64, wc_mining_final, sizeof(wc_mining_final));
        dec = bBase64DecodeEx(&b64, &error);
        EXPECT_EQ(error, BSTR_OK);
        ASSERT_NE(dec, nullptr);
        EXPECT_EQ(dec->slen, 1);
        EXPECT_EQ(dec->data[0], '}');
}

TEST(gtest, wc_mining_8way) {
        const char *nonces =
                "abcd" "efgh" "ijkl" "mnop" "qrst" "uvwx" "yz01" "2345";
        struct sha256 hashes1[8] = {0};
        struct sha256 hashes2[8] = {0};
        struct sha256_ctx ctx = SHA256_INIT;
        int i = 0;
        for (; i < 8; ++i) {
                sha256_update(&ctx, nonces, 4);
                sha256_update(&ctx, &nonces[4*i], 4);
                sha256_update(&ctx, nonces, 4);
                sha256_done(&hashes1[i], &ctx);
                ctx = SHA256_INIT;
        }
        EXPECT_EQ(hashes1[0].u8[0], 0x88);
        EXPECT_EQ(hashes1[0].u8[1], 0x7f);
        EXPECT_EQ(hashes1[7].u8[30], 0x86);
        EXPECT_EQ(hashes1[7].u8[31], 0x50);
        wc_mining_8way(hashes2[0].u8, &ctx, (const unsigned char*)nonces, (const unsigned char*)nonces, (const unsigned char*)nonces);
        EXPECT_EQ(memcmp(hashes1, hashes2, sizeof(hashes1)), 0);
}

int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        assert(wc_init() == WC_SUCCESS);
        return RUN_ALL_TESTS();
}

/* End of File
 */
