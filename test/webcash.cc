/* Copyright (c) 2022-2023 Mark Friedenbach
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>

#include <webcash.h>

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
                EXPECT_TRUE(bstr != NULL);
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
        EXPECT_TRUE(biseqcstr(secret.serial, ""));
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
        EXPECT_TRUE(biseqcstr(secret.serial, "abc"));
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
        EXPECT_TRUE(biseqcstr(secret.serial, "abc"));
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
        EXPECT_TRUE(biseqcstr(secret.serial, "abc"));
        EXPECT_NE(secret.serial, bstr); /* comparing pointers */
        EXPECT_NE(secret.serial->data, bstr->data);
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.amount, WC_ZERO);
        EXPECT_EQ(secret.serial, nullptr);
        EXPECT_TRUE(biseqcstr(bstr, "abc"));
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
        EXPECT_TRUE(biseqcstr(secret.serial, "abc"));
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
                EXPECT_TRUE(secret.serial->mlen > 0);
                EXPECT_EQ(secret.serial->slen, 0);
                EXPECT_NE(secret.serial->data, nullptr);
                EXPECT_TRUE(biseqcstr(secret.serial, ""));
        }
        EXPECT_EQ(wc_secret_destroy(&secret), WC_SUCCESS);
        EXPECT_EQ(secret.serial, nullptr);
        EXPECT_EQ(wc_secret_destroy(&secret), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(secret.serial, nullptr);
}

int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        assert(wc_init() == WC_SUCCESS);
        return RUN_ALL_TESTS();
}

/* End of File
 */
