// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xrole_type.h"

#include <gtest/gtest.h>

#include <cstring>

NS_BEG3(top, common, tests)

TEST(xcommon, miner_type_simple) {
    {
        common::xminer_type_t const miner_type{common::xminer_type_t::advance};
        EXPECT_EQ(common::XMINER_TYPE_ADVANCE, common::to_string(miner_type));
    }

// #if defined(XENABLE_MOCK_ZEC_STAKE)
    {
        common::xminer_type_t const miner_type{common::xminer_type_t::archive};
        EXPECT_EQ(common::XMINER_TYPE_ARCHIVE, common::to_string(miner_type));
    }
// #endif

    {
        common::xminer_type_t const miner_type{common::xminer_type_t::edge};
        EXPECT_EQ(common::XMINER_TYPE_EDGE, common::to_string(miner_type));
    }

    {
        common::xminer_type_t const miner_type{common::xminer_type_t::exchange};
        EXPECT_EQ(common::XMINER_TYPE_EXCHANGE, common::to_string(miner_type));
    }

    {
        common::xminer_type_t const miner_type{common::xminer_type_t::validator};
        EXPECT_EQ(common::XMINER_TYPE_VALIDATOR, common::to_string(miner_type));
    }
}

TEST(xcommon, miner_type_compound) {
    {
        common::xminer_type_t const miner_type{common::xminer_type_t::advance | common::xminer_type_t::edge};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_EQ(std::strlen(XMINER_TYPE_ADVANCE) + std::strlen(XMINER_TYPE_EDGE) + 1, miner_string.length());
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_ADVANCE) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_EDGE) != std::string::npos);
    }

// #if defined(XENABLE_MOCK_ZEC_STAKE)
    {
        common::xminer_type_t const miner_type{common::xminer_type_t::archive | common::xminer_type_t::advance};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_EQ(std::strlen(XMINER_TYPE_ADVANCE) + std::strlen(XMINER_TYPE_ARCHIVE) + 1, miner_string.length());
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_ADVANCE) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_ARCHIVE) != std::string::npos);
    }
// #endif

    {
        common::xminer_type_t const miner_type{common::xminer_type_t::edge | common::xminer_type_t::exchange};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_EQ(std::strlen(XMINER_TYPE_EDGE) + std::strlen(XMINER_TYPE_EXCHANGE) + 1, miner_string.length());
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_EDGE) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_EXCHANGE) != std::string::npos);
    }

    {
        common::xminer_type_t const miner_type{common::xminer_type_t::exchange | common::xminer_type_t::validator};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_EQ(std::strlen(XMINER_TYPE_EXCHANGE) + std::strlen(XMINER_TYPE_VALIDATOR) + 1, miner_string.length());
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_EXCHANGE) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_VALIDATOR) != std::string::npos);
    }

    {
        common::xminer_type_t const miner_type{common::xminer_type_t::validator | common::xminer_type_t::advance};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_EQ(std::strlen(XMINER_TYPE_ADVANCE) + std::strlen(XMINER_TYPE_VALIDATOR) + 1, miner_string.length());
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_ADVANCE) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_VALIDATOR) != std::string::npos);
    }

    {
        common::xminer_type_t const miner_type{common::xminer_type_t::validator | common::xminer_type_t::advance | common::xminer_type_t::edge};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_EQ(std::strlen(XMINER_TYPE_ADVANCE) + std::strlen(XMINER_TYPE_VALIDATOR) + std::strlen(XMINER_TYPE_EDGE) + 2, miner_string.length());
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_ADVANCE) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_VALIDATOR) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_EDGE) != std::string::npos);
    }

    {
        common::xminer_type_t const miner_type{common::xminer_type_t::validator | common::xminer_type_t::advance | common::xminer_type_t::edge | common::xminer_type_t::invalid};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_EQ(std::strlen(XMINER_TYPE_ADVANCE) + std::strlen(XMINER_TYPE_VALIDATOR) + std::strlen(XMINER_TYPE_EDGE) + 2, miner_string.length());
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_ADVANCE) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_VALIDATOR) != std::string::npos);
        EXPECT_TRUE(miner_string.find(common::XMINER_TYPE_EDGE) != std::string::npos);
    }
}

TEST(xcommon, miner_type_invalid) {
    {
        common::xminer_type_t const miner_type{common::xminer_type_t::invalid};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_FALSE(miner_string.empty());
        EXPECT_EQ(0, miner_string.find(common::XMINER_TYPE_INVALID));
    }

    {
        std::underlying_type<common::xminer_type_t>::type miner_type_value{0x1000};
        common::xminer_type_t const miner_type{static_cast<common::xminer_type_t>(miner_type_value)};
        auto const miner_string = common::to_string(miner_type);

        EXPECT_FALSE(miner_string.empty());
        EXPECT_EQ(0, miner_string.find(common::XMINER_TYPE_INVALID));
        EXPECT_TRUE(miner_string.find(std::to_string(miner_type_value)) != std::string::npos);
    }
}

NS_END3
