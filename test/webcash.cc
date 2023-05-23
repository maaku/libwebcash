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

int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
}

/* End of File
 */
