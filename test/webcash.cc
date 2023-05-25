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
}

int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
}

/* End of File
 */
