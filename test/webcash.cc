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
        ASSERT_EQ(init, 0);
        ASSERT_EQ(defn, 0);
        ASSERT_EQ(func, 0);
        ASSERT_EQ(init, defn);
        ASSERT_EQ(init, func);
        ASSERT_EQ(defn, func);
}

TEST(gtest, wc_from_string) {
        wc_amount_t amt = WC_ZERO;
        EXPECT_EQ(wc_from_cstring(&amt, "0"), WC_SUCCESS); EXPECT_EQ(amt, 0LL);
        EXPECT_EQ(wc_from_cstring(&amt, "0.0"), WC_SUCCESS); EXPECT_EQ(amt, 0LL);
        EXPECT_EQ(wc_from_cstring(&amt, "0.00000000"), WC_SUCCESS); EXPECT_EQ(amt, 0LL);
        EXPECT_EQ(wc_from_cstring(&amt, "0.00000001"), WC_SUCCESS); EXPECT_EQ(amt, 1LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.00000000"), WC_SUCCESS); EXPECT_EQ(amt, 100000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.00000001"), WC_SUCCESS); EXPECT_EQ(amt, 100000001LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.00000010"), WC_SUCCESS); EXPECT_EQ(amt, 100000010LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.00000100"), WC_SUCCESS); EXPECT_EQ(amt, 100000100LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.00001000"), WC_SUCCESS); EXPECT_EQ(amt, 100001000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.00010000"), WC_SUCCESS); EXPECT_EQ(amt, 100010000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.00100000"), WC_SUCCESS); EXPECT_EQ(amt, 100100000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.01000000"), WC_SUCCESS); EXPECT_EQ(amt, 101000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.10000000"), WC_SUCCESS); EXPECT_EQ(amt, 110000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.1000000"), WC_SUCCESS); EXPECT_EQ(amt, 110000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.100000"), WC_SUCCESS); EXPECT_EQ(amt, 110000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.10000"), WC_SUCCESS); EXPECT_EQ(amt, 110000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.1000"), WC_SUCCESS); EXPECT_EQ(amt, 110000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.100"), WC_SUCCESS); EXPECT_EQ(amt, 110000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.10"), WC_SUCCESS); EXPECT_EQ(amt, 110000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1.1"), WC_SUCCESS); EXPECT_EQ(amt, 110000000LL);
        EXPECT_EQ(wc_from_cstring(&amt, "1."), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_from_cstring(&amt, "1"), WC_SUCCESS); EXPECT_EQ(amt, 100000000LL);
        amt = -1LL;
        EXPECT_EQ(wc_from_cstring(&amt, "1.000000000"), WC_ERROR_INVALID_ARGUMENT);
}

int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
}

/* End of File
 */
