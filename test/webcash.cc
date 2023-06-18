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
        wc_secret_t secret;
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
        wc_secret_t secret;
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
        wc_secret_t secret;
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
        wc_secret_t secret;
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
        wc_secret_t secret;
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
        wc_secret_t secret;
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
        wc_secret_t secret;
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
        wc_secret_t secret;
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

TEST(gtetst, wc_derive_serials) {
        char buf1[64*20 + 1] = {0};
        char buf2[64*20 + 1] = {
                "be835897e85381905634f8bcc5db1eaa384d363c326335f4e9d89d119e78b0c5"
                "1f8e224c65115ce8eaf98b47457b0e5da0fcfcc480f0b3aafc516d5677eb24c1"
                "e7b87e9e263d6496888e252c67292637deb691cbf1f4894c9cfa7bfc440ffa05"
                "5a9ecb6cbe5ce83f15fc36ec8891fc6cc85c73099920721868934b0b934fba1d"
                "e10419abfca5e06d931a4faf4d6231ae2de6179459d2d58d1cfdcd0feb2b89b1"
                "9da3e943eda843e67d927b4a048095c57eecd8aeda7167e67c00f338031e179c"
                "178ab1df04e28f95d062fddb69babcf1f6d939a8fe27968a3fb54a77137b89a3"
                "71cf21df71b545440c2ca6cc4942ff4d81f2958e897741d403d2d7a3593a1cb8"
                "98bb2cc75c9a479b98bc432e9a75e395ea17fcccd0191c0b7fcee5f39e6cbecb"
                "1f108b5d962b985b7f61ba79b228b8a91d51fd6e3f4cb2fb751fa9f13d55aa35"
                "2c1037c9a2c301ee2d061a708968bcc76b71f7b872908bf979a7433f782ea880"
                "f474ad4dfc83771371cb650cb5b5fab0bda7cb8fd914abc607729ad65c192e83"
                "0830a4f79de40c476cd56ce317233873c27bdb5a92f11e24a12dbbe2dac2b43a"
                "c58fec454214e4e6cca720077070ee92da82e1058538559fb31aa5c7238f706d"
                "fa941605fe5f750d26cdc8de10f8ddb9fb80acfc06f7f782de265c865d3789bd"
                "452dda0c8268cacca437490086c29afc326f4611c8843d5d4454dd0b50ce7cea"
                "0979fd3d964093cc34f66de4d7e7dab6c2e5573c9cc4fae7d8b2b24308c6e886"
                "822ab78f6fbf7e556dca72368084c2764602c24aad0c791309ab2130c99a265b"
                "e0958fff040e6908eeea4f5f8a729b15b5ae4bf44e07e62911e5e5ef92420751"
                "b6c25321889b1a9dc7d0058ec98f223f8bd42af49a6eb103d4a53e97bd9c9ecf"
        };
        struct sha256 hdroot = {{
                0x40, 0x7c, 0x95, 0x0b, 0x3d, 0xe6, 0x00, 0x64,
                0xd7, 0xff, 0x74, 0x4b, 0x9b, 0x47, 0x43, 0xb8,
                0xde, 0x58, 0xe9, 0x43, 0xe7, 0xc5, 0x37, 0xdf,
                0x3d, 0x3a, 0x8a, 0x29, 0xa3, 0x2e, 0x1d, 0x0f
        }};
        uint64_t chaincode = 1; /* PAY */
        uint64_t depth = 0;
        size_t count = 20;
        bstring bstr = nullptr;
        size_t n = 0;
        wc_derive_serials(buf1, &hdroot, chaincode, depth, count);
        EXPECT_EQ(memcmp(buf1, buf2, sizeof(buf1)), 0);
        for (; n < count; ++n) {
                EXPECT_EQ(wc_derive_serial(&bstr, &hdroot, chaincode, depth + n), WC_SUCCESS);
                EXPECT_NE(bstr, nullptr);
                if (bstr) {
                        EXPECT_EQ(bstr->slen, 64);
                        EXPECT_EQ(bisstemeqblk(bstr, buf2 + n*64, 64), 1);
                }
                bdestroy(bstr);
                bstr = nullptr;
        }
}

TEST(gtest, wc_storage_open_close) {
        wc_storage_callbacks_t incompletecb = {
                .log_open = [](wc_log_url_t logurl) -> wc_log_handle_t {
                        return (wc_log_handle_t)1;
                },
        };
        wc_storage_callbacks_t cb = {
                .log_open = [](wc_log_url_t logurl) -> wc_log_handle_t {
                        return (wc_log_handle_t)nullptr;
                },
                .db_open = [](wc_db_url_t dburl) -> wc_db_handle_t {
                        return (wc_db_handle_t)nullptr;
                },
        };
        wc_storage_handle_t w = nullptr;
        EXPECT_EQ(wc_storage_open(nullptr, nullptr, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(w, nullptr);
        EXPECT_EQ(wc_storage_open(&w, nullptr, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(w, nullptr);
        EXPECT_EQ(wc_storage_open(nullptr, &cb, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(w, nullptr);
        EXPECT_EQ(wc_storage_open(&w, &incompletecb, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(w, nullptr);
        EXPECT_EQ(wc_storage_open(&w, &cb, nullptr, nullptr), WC_ERROR_LOG_OPEN_FAILED);
        EXPECT_EQ(w, nullptr);
        cb.log_open = [](wc_log_url_t logurl) -> wc_log_handle_t {
                return (wc_log_handle_t)1;
        };
        EXPECT_EQ(wc_storage_open(&w, &cb, nullptr, nullptr), WC_ERROR_DB_OPEN_FAILED);
        EXPECT_EQ(w, nullptr);
        cb.db_open = [](wc_db_url_t dburl) -> wc_db_handle_t {
                return (wc_db_handle_t)2;
        };
        EXPECT_EQ(wc_storage_open(&w, &cb, nullptr, nullptr), WC_SUCCESS);
        ASSERT_NE(w, nullptr);
        EXPECT_EQ(wc_storage_close(nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_storage_close(w), WC_SUCCESS);
}

std::map<std::string, time_t> g_terms;
wc_storage_callbacks_t g_storage_callbacks = {
        .log_open = [](wc_log_url_t logurl) -> wc_log_handle_t {
                return (wc_log_handle_t)1;
        },
        .db_open = [](wc_db_url_t dburl) -> wc_db_handle_t {
                return (wc_db_handle_t)2;
        },
        .any_terms = [](wc_db_handle_t db, int *any) -> wc_error_t {
                if (!any) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
                *any = !g_terms.empty();
                return WC_SUCCESS;
        },
        .all_terms = [](wc_db_handle_t db, wc_db_terms_t *terms, size_t *count) {
                if (!count) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
                if (!terms || *count < g_terms.size()) {
                        *count = g_terms.size();
                        return WC_ERROR_INSUFFICIENT_CAPACITY;
                }
                size_t i = 0;
                for (auto it = g_terms.begin(); it != g_terms.end(); ++it, ++i) {
                        terms[i].when = it->second;
                        terms[i].text = blk2bstr(it->first.c_str(), it->first.size());
                        if (!terms[i].text) {
                                for (size_t j = 0; j < i; ++j) {
                                        bdestroy(terms[j].text);
                                }
                                return WC_ERROR_OUT_OF_MEMORY;
                        }
                }
                *count = i;
                return WC_SUCCESS;
        },
        .terms_accepted = [](wc_db_handle_t db, bstring bstr, wc_time_t *when) -> wc_error_t {
                if (!bstr || !when) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
                std::string str((const char*)bstr->data, (size_t)bstr->slen);
                auto it = g_terms.find(str);
                if (it != g_terms.end()) {
                        *when = it->second;
                } else {
                        *when = 0;
                }
                return WC_SUCCESS;
        },
        .accept_terms = [](wc_db_handle_t db, bstring bstr, wc_time_t now) -> wc_error_t {
                if (!bstr) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
                std::string str((const char*)bstr->data, (size_t)bstr->slen);
                g_terms[str] = now;
                return WC_SUCCESS;
        },
};

TEST(gtest, wc_storage_terms) {
        // The internal implementation of wc_storage_enumerate_terms assumes
        // that wc_db_terms_t is sufficiently smaller than wc_terms_t that the
        // same buffer can be used for both and rewritten in-place.  This is a
        // reasonable assumption, but just in case we check it here.
        ASSERT_LT(sizeof(wc_db_terms_t) + sizeof(bstring), sizeof(wc_terms_t) + 1);

        wc_storage_handle_t w = nullptr;
        EXPECT_EQ(wc_storage_open(&w, &g_storage_callbacks, nullptr, nullptr), WC_SUCCESS);
        ASSERT_NE(w, nullptr);

        std::vector<wc_terms_t> vec;
        size_t count = 0;
        int accepted = 0;

        g_terms.clear();
        ASSERT_EQ(g_terms.size(), 0);

        vec.resize(1);
        count = std::numeric_limits<size_t>::max();
        EXPECT_EQ(wc_storage_enumerate_terms(w, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_storage_enumerate_terms(w, vec.data(), nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_storage_enumerate_terms(w, nullptr, &count), WC_ERROR_INSUFFICIENT_CAPACITY);
        EXPECT_EQ(count, 0); count = std::numeric_limits<size_t>::max();
        EXPECT_EQ(wc_storage_enumerate_terms(w, vec.data(), &count), WC_SUCCESS);
        EXPECT_EQ(count, 0);

        accepted = -1;
        EXPECT_EQ(wc_storage_have_accepted_terms(w, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_storage_have_accepted_terms(w, &accepted), WC_SUCCESS);
        EXPECT_EQ(accepted, 0);

        bstring bstr = cstr2bstr("foo");
        ASSERT_NE(bstr, nullptr);
        accepted = -1;
        struct tm when = {0};
        EXPECT_EQ(wc_storage_are_terms_accepted(w, &accepted, &when, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_storage_are_terms_accepted(w, nullptr, nullptr, bstr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_storage_are_terms_accepted(w, nullptr, &when, bstr), WC_SUCCESS);
        EXPECT_EQ(wc_storage_are_terms_accepted(w, &accepted, nullptr, bstr), WC_SUCCESS);
        EXPECT_EQ(accepted, 0);

        EXPECT_EQ(wc_storage_accept_terms(w, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_storage_accept_terms(w, bstr, nullptr), WC_SUCCESS);

        count = std::numeric_limits<size_t>::max();
        EXPECT_EQ(wc_storage_enumerate_terms(w, vec.data(), &count), WC_SUCCESS);
        EXPECT_EQ(count, 1);
        EXPECT_EQ(vec[0].text->slen, bstr->slen);
        EXPECT_EQ(memcmp(vec[0].text->data, bstr->data, bstr->slen), 0);
        EXPECT_EQ(biseq(vec[0].text, bstr), 1);

        accepted = -1;
        EXPECT_EQ(wc_storage_have_accepted_terms(w, &accepted), WC_SUCCESS);
        EXPECT_EQ(accepted, 1);

        accepted = -1;
        EXPECT_EQ(wc_storage_are_terms_accepted(w, &accepted, nullptr, bstr), WC_SUCCESS);
        EXPECT_EQ(accepted, 1);

        accepted = -1;
        bstr->data[1] = 'a';
        EXPECT_EQ(wc_storage_are_terms_accepted(w, &accepted, nullptr, bstr), WC_SUCCESS);
        EXPECT_EQ(accepted, 0);

        EXPECT_EQ(wc_storage_close(w), WC_SUCCESS);
}

TEST(gtest, wc_server_connect) {
        wc_server_callbacks_t incompletecb = {};
        wc_server_callbacks_t cb = {
                .connect = [](wc_server_url_t url) -> wc_conn_handle_t {
                        return (wc_conn_handle_t)nullptr;
                },
        };
        wc_server_handle_t c = nullptr;
        EXPECT_EQ(wc_server_connect(nullptr, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(c, nullptr);
        EXPECT_EQ(wc_server_connect(&c, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(c, nullptr);
        EXPECT_EQ(wc_server_connect(nullptr, &cb, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(c, nullptr);
        EXPECT_EQ(wc_server_connect(&c, &incompletecb, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(c, nullptr);
        EXPECT_EQ(wc_server_connect(&c, &cb, nullptr), WC_ERROR_CONNECT_FAILED);
        EXPECT_EQ(c, nullptr);
        cb.connect = [](wc_server_url_t url) -> wc_conn_handle_t {
                return (wc_conn_handle_t)1;
        };
        EXPECT_EQ(wc_server_connect(&c, &cb, nullptr), WC_SUCCESS);
        ASSERT_NE(c, nullptr);
        EXPECT_EQ(wc_server_disconnect(nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_server_disconnect(c), WC_SUCCESS);
}

std::string g_terms_of_service = "foo";
wc_server_callbacks_t g_server_callbacks = {
        .connect = [](wc_server_url_t url) -> wc_conn_handle_t {
                return (wc_conn_handle_t)1;
        },
        .get_terms = [](wc_conn_handle_t conn, bstring *terms) -> wc_error_t {
                if (!terms) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
                bstring bstr = blk2bstr(g_terms_of_service.c_str(), g_terms_of_service.size());
                if (!bstr) {
                        return WC_ERROR_OUT_OF_MEMORY;
                }
                *terms = bstr;
                return WC_SUCCESS;
        },
};

TEST(gtest, wc_server_terms) {
        wc_server_handle_t c = nullptr;
        EXPECT_EQ(wc_server_connect(&c, &g_server_callbacks, nullptr), WC_SUCCESS);
        ASSERT_NE(c, nullptr);
        EXPECT_EQ(wc_server_get_terms(c, nullptr), WC_ERROR_INVALID_ARGUMENT);
        bstring terms = nullptr;
        EXPECT_EQ(wc_server_get_terms(c, &terms), WC_SUCCESS);
        ASSERT_NE(terms, nullptr);
        EXPECT_EQ(terms->slen, 3);
        EXPECT_EQ(memcmp(terms->data, "foo", 3), 0);
        EXPECT_EQ(biseqcstr(terms, "foo"), 1);
        EXPECT_EQ(wc_server_disconnect(c), WC_SUCCESS);
}

TEST(gtest, wc_ui_startup) {
        wc_ui_callbacks_t incompletecb = {};
        wc_ui_callbacks_t cb = {
                .startup = [](wc_window_params_t params) -> wc_window_handle_t {
                        return (wc_window_handle_t)nullptr;
                },
        };
        wc_ui_handle_t ui = nullptr;
        EXPECT_EQ(wc_ui_startup(nullptr, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(ui, nullptr);
        EXPECT_EQ(wc_ui_startup(&ui, nullptr, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(ui, nullptr);
        EXPECT_EQ(wc_ui_startup(nullptr, &cb, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(ui, nullptr);
        EXPECT_EQ(wc_ui_startup(&ui, &incompletecb, nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(ui, nullptr);
        EXPECT_EQ(wc_ui_startup(&ui, &cb, nullptr), WC_ERROR_STARTUP_FAILED);
        EXPECT_EQ(ui, nullptr);
        cb.startup = [](wc_window_params_t params) -> wc_window_handle_t {
                return (wc_window_handle_t)1;
        };
        EXPECT_EQ(wc_ui_startup(&ui, &cb, nullptr), WC_SUCCESS);
        ASSERT_NE(ui, nullptr);
        EXPECT_EQ(wc_ui_shutdown(nullptr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_ui_shutdown(ui), WC_SUCCESS);
}

bool g_should_accept_terms = false;
wc_ui_callbacks_t g_ui_callbacks = {
        .startup = [](wc_window_params_t params) -> wc_window_handle_t {
                return (wc_window_handle_t)1;
        },
        .show_terms = [](wc_window_handle_t window, int *accepted,  bstring terms) -> wc_error_t {
                if (!terms) {
                        return WC_ERROR_INVALID_ARGUMENT;
                }
                *accepted = !!g_should_accept_terms;
                return WC_SUCCESS;
        },
};

TEST(gtest, wc_ui_terms) {
        wc_ui_handle_t ui = nullptr;
        int accepted = -1;
        bstring bstr = blk2bstr(g_terms_of_service.c_str(), g_terms_of_service.size());
        ASSERT_NE(bstr, nullptr);
        EXPECT_EQ(wc_ui_startup(&ui, &g_ui_callbacks, nullptr), WC_SUCCESS);
        ASSERT_NE(ui, nullptr);
        EXPECT_EQ(wc_ui_show_terms(nullptr, &accepted, bstr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_ui_show_terms(ui, nullptr, bstr), WC_ERROR_INVALID_ARGUMENT);
        EXPECT_EQ(wc_ui_show_terms(ui, &accepted, nullptr), WC_ERROR_INVALID_ARGUMENT);
        g_should_accept_terms = false;
        EXPECT_EQ(wc_ui_show_terms(ui, &accepted, bstr), WC_SUCCESS);
        EXPECT_EQ(accepted, 0);
        g_should_accept_terms = true;
        EXPECT_EQ(wc_ui_show_terms(ui, &accepted, bstr), WC_SUCCESS);
        EXPECT_EQ(accepted, 1);
        EXPECT_EQ(wc_ui_shutdown(ui), WC_SUCCESS);
}

int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        assert(wc_init() == WC_SUCCESS);
        return RUN_ALL_TESTS();
}

/* End of File
 */
