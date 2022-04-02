// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv0/xelection_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv1/xelection_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv2/xelection_info_codec.hpp"
#include "xdata/xelection/xv0/xelection_info.h"
#include "xdata/xelection/xv1/xelection_info.h"
#include "xdata/xelection/xv2/xelection_info.h"

#include <gtest/gtest.h>

NS_BEG4(top, data, election, tests)

TEST(compatibility, election_info_v0_v1) {
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

        auto const from_from = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::decode(from_bytes);
        ASSERT_EQ(from, from_from);
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

TEST(compatibility, election_info_v1_v2) {
    {
        data::election::v2::xelection_info_t from;
        from.comprehensive_stake = 42;
        from.consensus_public_key = top::xpublic_key_t{"faked public key"};
        from.genesis = true;
        from.joined_version = common::xelection_round_t{24};
        from.miner_type = top::common::xminer_type_t::advance | common::xminer_type_t::archive;
        from.stake = 4242;
        from.raw_credit_score = 100000;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v2::xelection_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::decode(from_bytes);

        ASSERT_EQ(from.comprehensive_stake, to.comprehensive_stake);
        ASSERT_EQ(from.consensus_public_key, to.consensus_public_key);
        ASSERT_EQ(from.joined_version, to.joined_version);
        ASSERT_EQ(from.stake, to.stake);
        ASSERT_EQ(from.genesis, to.genesis);
        ASSERT_EQ(from.miner_type, to.miner_type);
        ASSERT_EQ(to, from.v1());

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v2::xelection_info_t>::decode(to_bytes);

        ASSERT_EQ(from.comprehensive_stake, from_again.comprehensive_stake);
        ASSERT_EQ(from.consensus_public_key, from_again.consensus_public_key);
        ASSERT_EQ(from.joined_version, from_again.joined_version);
        ASSERT_EQ(from.stake, from_again.stake);
        ASSERT_EQ(from.genesis, from_again.genesis);
        ASSERT_EQ(from.miner_type, from_again.miner_type);
        ASSERT_EQ(0, from_again.raw_credit_score);

        auto const from_from = codec::xmsgpack_codec_t<data::election::v2::xelection_info_t>::decode(from_bytes);
        ASSERT_EQ(from, from_from);
    }

    {
        data::election::v1::xelection_info_t from;
        from.comprehensive_stake = 42;
        from.consensus_public_key = top::xpublic_key_t{"faked public key"};
        from.joined_version = common::xelection_round_t{24};
        from.stake = 4242;
        from.genesis = true;
        from.miner_type = common::xminer_type_t::advance | common::xminer_type_t::archive | common::xminer_type_t::edge;

        auto const from_bytes = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::encode(from);
        auto const to = codec::xmsgpack_codec_t<data::election::v2::xelection_info_t>::decode(from_bytes);

        ASSERT_EQ(from.comprehensive_stake, to.comprehensive_stake);
        ASSERT_EQ(from.consensus_public_key, to.consensus_public_key);
        ASSERT_EQ(from.joined_version, to.joined_version);
        ASSERT_EQ(from.stake, to.stake);
        ASSERT_EQ(from.genesis, to.genesis);
        ASSERT_EQ(from.miner_type, to.miner_type);
        ASSERT_EQ(0, to.raw_credit_score);

        auto const to_bytes = codec::xmsgpack_codec_t<data::election::v2::xelection_info_t>::encode(to);
        auto const from_again = codec::xmsgpack_codec_t<data::election::v1::xelection_info_t>::decode(to_bytes);

        ASSERT_EQ(from, from_again);
    }
}

NS_END4
