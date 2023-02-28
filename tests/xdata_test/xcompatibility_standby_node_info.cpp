// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv0/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv1/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv2/xstandby_node_info_codec.hpp"
#include "xdata/xelection/xv0/xstandby_node_info.h"
#include "xdata/xelection/xv1/xstandby_node_info.h"
#include "xdata/xelection/xv2/xstandby_node_info.h"

#include <gtest/gtest.h>

NS_BEG4(top, data, election, tests)

TEST(compatibility, standby_node_info_v0_v1) {
    {
        data::election::v1::xstandby_node_info_t from;
        from.consensus_public_key = xpublic_key_t{"fake"};
        from.genesis = true;
        from.miner_type = common::xminer_type_t::edge | common::xminer_type_t::validator;
        from.program_version = "99.99.99";
        from.stake_container[common::xnode_type_t::consensus_validator] = 1;
        from.stake_container[common::xnode_type_t::storage_archive] = 2;
        from.stake_container[common::xnode_type_t::consensus_auditor] = 3;
        from.stake_container[common::xnode_type_t::consensus_validator] = 4;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v0::xstandby_node_info_t>::decode(from_bytes);

        ASSERT_TRUE(from.consensus_public_key == to.consensus_public_key);
        ASSERT_TRUE(from.genesis == to.genesis);
#if defined(XENABLE_MOCK_ZEC_STAKE)
        ASSERT_TRUE(from.miner_type == to.miner_type);
#endif
        ASSERT_TRUE(from.program_version == to.program_version);
        ASSERT_TRUE(from.stake_container == to.stake_container);
        ASSERT_TRUE(to == from.v0());

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v0::xstandby_node_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::decode(to_bytes);

        ASSERT_TRUE(from.consensus_public_key == from_again.consensus_public_key);
        ASSERT_TRUE(from.genesis == from_again.genesis);
#if defined(XENABLE_MOCK_ZEC_STAKE)
        ASSERT_TRUE(from.miner_type == from_again.miner_type);
#endif
        ASSERT_TRUE(from.program_version == from_again.program_version);
        ASSERT_TRUE(from.stake_container == from_again.stake_container);

        auto const from_from = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::decode(from_bytes);
        ASSERT_TRUE(from == from_from);
    }

    {
        data::election::v0::xstandby_node_info_t from;
        from.consensus_public_key = xpublic_key_t{"fake"};
        from.genesis = true;
#if defined(XENABLE_MOCK_ZEC_STAKE)
        from.miner_type = common::xminer_type_t::edge | common::xminer_type_t::validator;
#endif
        from.program_version = "99.99.99";
        from.stake_container[common::xnode_type_t::consensus_validator] = 1;
        from.stake_container[common::xnode_type_t::storage_archive] = 2;
        from.stake_container[common::xnode_type_t::consensus_auditor] = 3;
        from.stake_container[common::xnode_type_t::consensus_validator] = 4;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v0::xstandby_node_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::decode(from_bytes);

        ASSERT_TRUE(from.consensus_public_key == to.consensus_public_key);
        ASSERT_TRUE(from.genesis == to.genesis);
#if defined(XENABLE_MOCK_ZEC_STAKE)
        ASSERT_TRUE(from.miner_type == to.miner_type);
#endif
        ASSERT_TRUE(from.program_version == to.program_version);
        ASSERT_TRUE(from.stake_container == to.stake_container);

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v0::xstandby_node_info_t>::decode(to_bytes);

        ASSERT_TRUE(from == from_again);
    }
}

TEST(compatibility, standby_node_info_v1_v2) {
    {
        data::election::v2::xstandby_node_info_t from;
        from.consensus_public_key = xpublic_key_t{"fake"};
        from.genesis = true;
        from.miner_type = common::xminer_type_t::edge | common::xminer_type_t::validator;
        from.program_version = "99.99.99";
        from.stake_container[common::xnode_type_t::consensus_validator] = 1;
        from.stake_container[common::xnode_type_t::storage_archive] = 2;
        from.stake_container[common::xnode_type_t::consensus_auditor] = 3;
        from.stake_container[common::xnode_type_t::consensus_validator] = 4;
        from.raw_credit_scores[common::xnode_type_t::consensus_validator] = 100000;
        from.raw_credit_scores[common::xnode_type_t::storage_archive] = 0;
        from.raw_credit_scores[common::xnode_type_t::consensus_auditor] = 200000;
        from.raw_credit_scores[common::xnode_type_t::consensus_validator] = 300000;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v2::xstandby_node_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::decode(from_bytes);

        ASSERT_TRUE(from.consensus_public_key == to.consensus_public_key);
        ASSERT_TRUE(from.genesis == to.genesis);
        ASSERT_TRUE(from.miner_type == to.miner_type);
        ASSERT_TRUE(from.program_version == to.program_version);
        ASSERT_TRUE(from.stake_container == to.stake_container);
        ASSERT_TRUE(to == from.v1());

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v2::xstandby_node_info_t>::decode(to_bytes);

        ASSERT_TRUE(from.consensus_public_key == from_again.consensus_public_key);
        ASSERT_TRUE(from.genesis == from_again.genesis);
        ASSERT_TRUE(from.miner_type == from_again.miner_type);
        ASSERT_TRUE(from.program_version == from_again.program_version);
        ASSERT_TRUE(from.stake_container == from_again.stake_container);
        ASSERT_TRUE(from_again.raw_credit_scores.empty());

        auto const from_from = codec::xmsgpack_codec_t<data::election::v2::xstandby_node_info_t>::decode(from_bytes);
        ASSERT_TRUE(from == from_from);
    }

    {
        data::election::v1::xstandby_node_info_t from;
        from.consensus_public_key = xpublic_key_t{"fake"};
        from.genesis = true;
        from.miner_type = common::xminer_type_t::edge | common::xminer_type_t::validator;
        from.program_version = "99.99.99";
        from.stake_container[common::xnode_type_t::consensus_validator] = 1;
        from.stake_container[common::xnode_type_t::storage_archive] = 2;
        from.stake_container[common::xnode_type_t::consensus_auditor] = 3;
        from.stake_container[common::xnode_type_t::consensus_validator] = 4;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v2::xstandby_node_info_t>::decode(from_bytes);

        ASSERT_TRUE(from.consensus_public_key == to.consensus_public_key);
        ASSERT_TRUE(from.genesis == to.genesis);
        ASSERT_TRUE(from.miner_type == to.miner_type);
        ASSERT_TRUE(from.program_version == to.program_version);
        ASSERT_TRUE(from.stake_container == to.stake_container);
        ASSERT_TRUE(to.raw_credit_scores.empty());

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v2::xstandby_node_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::decode(to_bytes);

        ASSERT_TRUE(from == from_again);
    }
}

NS_END4
