// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xbase.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xsharding_info.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xrouter/xrouter.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>


using top::router::xrouter_face_t;
using top::router::xrouter_t;

TEST(xtop_router, address_of_book_id) {
    std::unique_ptr<xrouter_face_t> router{ top::make_unique<xrouter_t>() };
    auto const & config_register = top::config::xconfig_register_t::get_instance();

    auto auditor_group_count = XGET_CONFIG(auditor_group_count);
    auto validator_group_count = XGET_CONFIG(validator_group_count);

    for (auto i = 0u; i < enum_vbucket_has_books_count; ++i) {
        auto const empty_address = router->address_of_book_id(static_cast<std::uint16_t>(i), top::common::xnode_type_t::edge, top::common::xtopchain_network_id);
        EXPECT_TRUE(empty_address.empty());

        auto const archive_address = router->address_of_book_id(static_cast<std::uint16_t>(i), top::common::xnode_type_t::storage_archive, top::common::xtopchain_network_id);
        EXPECT_TRUE(top::common::build_archive_sharding_address(top::common::xarchive_group_id, top::common::xtopchain_network_id) == archive_address);

        auto const rec_address = router->address_of_book_id(static_cast<std::uint16_t>(i), top::common::xnode_type_t::committee, top::common::xbeacon_network_id);
        EXPECT_TRUE(top::common::build_committee_sharding_address(top::common::xbeacon_network_id) == rec_address);

        auto const zec_address = router->address_of_book_id(static_cast<std::uint16_t>(i), top::common::xnode_type_t::zec, top::common::xtopchain_network_id);
        EXPECT_TRUE(top::common::build_zec_sharding_address(top::common::xtopchain_network_id) == zec_address);

        auto const auditor_address = router->address_of_book_id(static_cast<std::uint16_t>(i), top::common::xnode_type_t::consensus_auditor, top::common::xtopchain_network_id);

        top::common::xcluster_address_t expected_auditor_address{
            top::common::xtopchain_network_id,
            top::common::xdefault_zone_id,
            top::common::xdefault_cluster_id,
            top::common::xgroup_id_t{ top::common::xauditor_group_id_value_begin + i / (enum_vbucket_has_books_count / auditor_group_count) },
        };

        EXPECT_TRUE(expected_auditor_address == auditor_address);

        auto const validator_address = router->address_of_book_id(static_cast<std::uint16_t>(i), top::common::xnode_type_t::consensus_validator, top::common::xtopchain_network_id);
            top::common::xcluster_address_t expected_validator_address{
            top::common::xtopchain_network_id,
            top::common::xdefault_zone_id,
            top::common::xdefault_cluster_id,
            top::common::xgroup_id_t{ top::common::xvalidator_group_id_value_begin + i / (enum_vbucket_has_books_count / validator_group_count) },
        };
        EXPECT_TRUE(expected_validator_address == validator_address);
    }
}
