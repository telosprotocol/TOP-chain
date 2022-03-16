// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xcodec/xmsgpack/xelection/xv1/xelection_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv0/xelection_info_codec.hpp"
#include "xdata/xelection/xv1/xelection_info.h"
#include "xdata/xelection/xv0/xelection_info.h"
#include "xcodec/xmsgpack_codec.hpp"

#include <gtest/gtest.h>

NS_BEG4(top, data, election, tests)

TEST(compatibility, election_info) {
    {
        data::election::v1::xelection_info_t from;
        from.comprehensive_stake = 42;
        from.consensus_public_key = top::xpublic_key_t{"faked public key"};
        from.genesis = true;
        from.joined_version = common::xelection_round_t{24};
        from.miner_type = top::common::xminer_type_t::advance | common::xminer_type_t::archive;
        from.stake = 4242;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v0::xelection_info_t>::decode(from_bytes);

        ASSERT_EQ(from.comprehensive_stake, to.comprehensive_stake);
        ASSERT_EQ(from.consensus_public_key, to.consensus_public_key);
        ASSERT_EQ(from.joined_version, to.joined_version);
        ASSERT_EQ(from.stake, to.stake);
        ASSERT_EQ(to, from.v0());

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v0::xelection_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::decode(to_bytes);

        ASSERT_EQ(from.comprehensive_stake, from_again.comprehensive_stake);
        ASSERT_EQ(from.consensus_public_key, from_again.consensus_public_key);
        ASSERT_EQ(from.joined_version, from_again.joined_version);
        ASSERT_EQ(from.stake, from_again.stake);
        ASSERT_FALSE(from_again.genesis);
        ASSERT_EQ(common::xminer_type_t::invalid, from_again.miner_type);
    }

    {
        data::election::v0::xelection_info_t from;
        from.comprehensive_stake = 42;
        from.consensus_public_key = top::xpublic_key_t{"faked public key"};
        from.joined_version = common::xelection_round_t{24};
        from.stake = 4242;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v0::xelection_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::decode(from_bytes);

        ASSERT_EQ(from.comprehensive_stake, to.comprehensive_stake);
        ASSERT_EQ(from.consensus_public_key, to.consensus_public_key);
        ASSERT_EQ(from.joined_version, to.joined_version);
        ASSERT_EQ(from.stake, to.stake);
        ASSERT_FALSE(to.genesis);
        ASSERT_EQ(common::xminer_type_t::invalid, to.miner_type);

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v0::xelection_info_t>::decode(to_bytes);

        ASSERT_EQ(from, from_again);
    }
}

TEST(compatibility, standby_node_info) {
    {
        data::election::v1::xstandby_node_info_t from;
        from.consensus_public_key = xpublic_key_t{"fake"};
        from.genesis = true;
        from.miner_type = common::xminer_type_t::edge | common::xminer_type_t::validator;
        from.program_version = "99.99.99";
        from.stake_container[common::xnode_type_t::validator] = 1;
        from.stake_container[common::xnode_type_t::archive] = 2;
        from.stake_container[common::xnode_type_t::consensus_auditor] = 3;
        from.stake_container[common::xnode_type_t::consensus_validator] = 4;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v0::xstandby_node_info_t>::decode(from_bytes);

        ASSERT_EQ(from.consensus_public_key, to.consensus_public_key);
        ASSERT_EQ(from.genesis, to.genesis);
#if defined(XENABLE_MOCK_ZEC_STAKE)
        ASSERT_EQ(from.miner_type, to.miner_type);
#endif
        ASSERT_EQ(from.program_version, to.program_version);
        ASSERT_EQ(from.stake_container, to.stake_container);
        ASSERT_EQ(to, from.v0());

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v0::xstandby_node_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::decode(to_bytes);

        ASSERT_EQ(from.consensus_public_key, from_again.consensus_public_key);
        ASSERT_EQ(from.genesis, from_again.genesis);
#if defined(XENABLE_MOCK_ZEC_STAKE)
        ASSERT_EQ(from.miner_type, from_again.miner_type);
#endif
        ASSERT_EQ(from.program_version, from_again.program_version);
        ASSERT_EQ(from.stake_container, from_again.stake_container);
    }

    {
        data::election::v0::xstandby_node_info_t from;
        from.consensus_public_key = xpublic_key_t{"fake"};
        from.genesis = true;
#if defined(XENABLE_MOCK_ZEC_STAKE)
        from.miner_type = common::xminer_type_t::edge | common::xminer_type_t::validator;
#endif
        from.program_version = "99.99.99";
        from.stake_container[common::xnode_type_t::validator] = 1;
        from.stake_container[common::xnode_type_t::archive] = 2;
        from.stake_container[common::xnode_type_t::consensus_auditor] = 3;
        from.stake_container[common::xnode_type_t::consensus_validator] = 4;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v0::xstandby_node_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::decode(from_bytes);

        ASSERT_EQ(from.consensus_public_key, to.consensus_public_key);
        ASSERT_EQ(from.genesis, to.genesis);
#if defined(XENABLE_MOCK_ZEC_STAKE)
        ASSERT_EQ(from.miner_type, to.miner_type);
#endif
        ASSERT_EQ(from.program_version, to.program_version);
        ASSERT_EQ(from.stake_container, to.stake_container);

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v1::xstandby_node_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v0::xstandby_node_info_t>::decode(to_bytes);

        ASSERT_EQ(from, from_again);
    }
}

NS_END4
