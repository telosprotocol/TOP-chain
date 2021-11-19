// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xstring.h"

#include <limits>
#include <string>

#include <gtest/gtest.h>

NS_BEG3(top, tests, basic)

#define NORMAL_FROM_STRING_INT_TEST(INT)                                                                                                                                           \
    TEST(from_string, normal_##INT##_t) {                                                                                                                                          \
        std::error_code ec;                                                                                                                                                        \
                                                                                                                                                                                   \
        {                                                                                                                                                                          \
            INT##_t i{0};                                                                                                                                                          \
            std::string i_string {std::to_string(i)};                                                                                                                              \
            auto j = top::from_string<INT##_t>(i_string, ec);                                                                                                                          \
            ASSERT_TRUE(!ec);                                                                                                                                                      \
            ASSERT_EQ(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
        {                                                                                                                                                                          \
            INT##_t i{std::numeric_limits<INT##_t>::max()};                                                                                                                                \
            std::string i_string = std::to_string(i);                                                                                                                              \
                                                                                                                                                                                   \
            auto j = top::from_string<INT##_t>(i_string, ec);                                                                                                                          \
            ASSERT_TRUE(!ec);                                                                                                                                                      \
            ASSERT_EQ(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
        {                                                                                                                                                                          \
            INT##_t i{std::numeric_limits<INT##_t>::min()};                                                                                                                                \
            std::string i_string = std::to_string(i);                                                                                                                              \
                                                                                                                                                                                   \
            auto j = top::from_string<INT##_t>(i_string, ec);                                                                                                                          \
            ASSERT_TRUE(!ec);                                                                                                                                                      \
            ASSERT_EQ(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
    }

NORMAL_FROM_STRING_INT_TEST(int16)
NORMAL_FROM_STRING_INT_TEST(int32)
NORMAL_FROM_STRING_INT_TEST(int64)
NORMAL_FROM_STRING_INT_TEST(uint16)
NORMAL_FROM_STRING_INT_TEST(uint32)
NORMAL_FROM_STRING_INT_TEST(uint64)

#undef NORMAL_FROM_STRING_INT_TEST

#define NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(SHORT_INT, LONG_INT)                                                                                                         \
    TEST(from_string, normal_##SHORT_INT##_to_##LONG_INT) {                                                                                                                        \
        std::error_code ec;                                                                                                                                                        \
                                                                                                                                                                                   \
        {                                                                                                                                                                          \
            SHORT_INT##_t i{0};                                                                                                                                                    \
            std::string i_string{std::to_string(i)};                                                                                                                               \
            auto j = top::from_string<LONG_INT##_t>(i_string, ec);                                                                                                                 \
            ASSERT_TRUE(!ec);                                                                                                                                                      \
            ASSERT_EQ(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
        {                                                                                                                                                                          \
            SHORT_INT##_t i{std::numeric_limits<SHORT_INT##_t>::max()};                                                                                                            \
            std::string i_string = std::to_string(i);                                                                                                                              \
                                                                                                                                                                                   \
            auto j = top::from_string<LONG_INT##_t>(i_string, ec);                                                                                                                 \
            ASSERT_TRUE(!ec);                                                                                                                                                      \
            ASSERT_EQ(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
        {                                                                                                                                                                          \
            SHORT_INT##_t i{std::numeric_limits<SHORT_INT##_t>::min()};                                                                                                            \
            std::string i_string = std::to_string(i);                                                                                                                              \
                                                                                                                                                                                   \
            auto j = top::from_string<LONG_INT##_t>(i_string, ec);                                                                                                                 \
            ASSERT_TRUE(!ec);                                                                                                                                                      \
            ASSERT_EQ(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
    }

NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(int16, int32)
NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(int16, int64)
NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(int32, int64)
NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(uint16, uint32)
NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(uint16, uint64)
NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(uint32, uint64)
NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(uint16, int32)
NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(uint16, int64)
NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST(uint32, int64)

#undef NORMAL_FROM_STRING_SHORT_INT_TO_LONG_INT_TEST

#define ABNORMAL_FROM_STRING_ULONG_INT_TO_USHORT_INT_TEST(ULONG_INT, USHORT_INT)                                                                                                   \
    TEST(from_string, abnormal_##ULONG_INT##_to_##USHORT_INT) {                                                                                                                    \
        {                                                                                                                                                                          \
            std::error_code ec;                                                                                                                                                    \
            ULONG_INT##_t i{std::numeric_limits<ULONG_INT##_t>::max()};                                                                                                            \
            std::string i_string = std::to_string(i);                                                                                                                              \
                                                                                                                                                                                   \
            auto j = top::from_string<USHORT_INT##_t>(i_string, ec);                                                                                                               \
            ASSERT_TRUE(!!ec);                                                                                                                                                     \
            ASSERT_NE(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
        {                                                                                                                                                                          \
            std::error_code ec;                                                                                                                                                    \
            ULONG_INT##_t i{std::numeric_limits<ULONG_INT##_t>::min()};                                                                                                            \
            std::string i_string = std::to_string(i);                                                                                                                              \
                                                                                                                                                                                   \
            auto j = top::from_string<USHORT_INT##_t>(i_string, ec);                                                                                                               \
            ASSERT_TRUE(!ec);                                                                                                                                                      \
            ASSERT_EQ(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
    }

ABNORMAL_FROM_STRING_ULONG_INT_TO_USHORT_INT_TEST(uint32, uint16)
ABNORMAL_FROM_STRING_ULONG_INT_TO_USHORT_INT_TEST(uint64, uint16)
ABNORMAL_FROM_STRING_ULONG_INT_TO_USHORT_INT_TEST(uint64, uint32)

#undef ABNORMAL_FROM_STRING_ULONG_INT_TO_USHORT_INT_TEST

#define ABNORMAL_FROM_STRING_LONG_INT_TO_SHORT_INT_TEST(LONG_INT, SHORT_INT)                                                                                                       \
    TEST(from_string, abnormal_##LONG_INT##_to_##SHORT_INT) {                                                                                                                    \
        {                                                                                                                                                                          \
            std::error_code ec;                                                                                                                                                    \
            LONG_INT##_t i{std::numeric_limits<LONG_INT##_t>::max()};                                                                                                              \
            std::string i_string = std::to_string(i);                                                                                                                              \
                                                                                                                                                                                   \
            auto j = top::from_string<SHORT_INT##_t>(i_string, ec);                                                                                                                \
            ASSERT_TRUE(!!ec);                                                                                                                                                     \
            ASSERT_NE(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
        {                                                                                                                                                                          \
            std::error_code ec;                                                                                                                                                    \
            LONG_INT##_t i{std::numeric_limits<LONG_INT##_t>::min()};                                                                                                              \
            std::string i_string = std::to_string(i);                                                                                                                              \
                                                                                                                                                                                   \
            auto j = top::from_string<SHORT_INT##_t>(i_string, ec);                                                                                                                \
            ASSERT_TRUE(!!ec);                                                                                                                                                     \
            ASSERT_NE(i, j);                                                                                                                                                       \
        }                                                                                                                                                                          \
    }

ABNORMAL_FROM_STRING_LONG_INT_TO_SHORT_INT_TEST(int32, int16)
ABNORMAL_FROM_STRING_LONG_INT_TO_SHORT_INT_TEST(int64, int16)
ABNORMAL_FROM_STRING_LONG_INT_TO_SHORT_INT_TEST(int64, int32)

#undef ABNORMAL_FROM_STRING_LONG_INT_TO_SHORT_INT_TEST

NS_END3
