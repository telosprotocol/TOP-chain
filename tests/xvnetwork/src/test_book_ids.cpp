// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchain_timer/xchain_timer.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xgenesis_data.h"
#include "xconfig/xpredefined_configurations.h"
#include "tests/xnetwork/xdummy_network_driver.h"
#include "tests/xvnetwork/xdummy_data_accessor.h"
#include "xvnetwork/xvhost.h"
#include "xvnetwork/xvnetwork_driver.h"

#include <gtest/gtest.h>

constexpr std::uint8_t book_id_count{ 128 };

TEST(test_table_id, table_id) {
    //std::vector<std::uint8_t> ids;
    //ids.reserve(book_id_count);

    //for (auto i = 0u; i < book_id_count; ++i) {
    //    ids.push_back(i);
    //}

    //auto result = top::vnetwork::filter_book_ids(ids, 0, 2);
    //EXPECT_EQ(book_id_count / 2, result.size());

    //for (auto i = 0u; i < result.size(); ++i) {
    //    EXPECT_EQ(result[i], static_cast<std::uint16_t>(i));
    //}

    //result = top::vnetwork::filter_book_ids(ids, 3, 4);
    //EXPECT_EQ(book_id_count / 4, result.size());

    //for (auto i = 0u; i < result.size(); ++i) {
    //    EXPECT_EQ(result[i], static_cast<std::uint16_t>(i + (book_id_count / 4) * 3));
    //}

    //result = top::vnetwork::filter_book_ids(ids, 0, 1);
    //result = top::vnetwork::filter_book_ids(result, 0, 1);
    //result = top::vnetwork::filter_book_ids(result, 1, 2);
    //result = top::vnetwork::filter_book_ids(result, 2, 4);
    //EXPECT_EQ(book_id_count / 2 / 4, result.size());

    //for (auto i = 0u; i < result.size(); ++i) {
    //    EXPECT_EQ(result[i], static_cast<std::uint16_t>(book_id_count / 2 + i + 2 * book_id_count / 2 / 4));
    //}
    auto& config_register = top::config::xconfig_register_t::get_instance();
    config_register.load();

    top::common::xnetwork_id_t test_net_id{1};
    // top::vnetwork::tests::xdummy_election_data_accessor_t dummy_election_cache_data_accessor{ top::common::xtestnet_id };
    // top::network::tests::xdummy_network_driver_t dummy_network_driver{};
    auto auditor_group_count = XGET_CONFIG(auditor_group_count);

    auto const zone_count = XGET_CONFIG(zone_count);
    auto const cluster_count = XGET_CONFIG(cluster_count);

    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    top::xobject_ptr_t<top::time::xchain_timer_t> chain_timer = top::make_object_ptr<top::time::xchain_timer_t>(timer_driver);
    auto vhost = std::make_shared<top::vnetwork::xvhost_t>(top::make_observer(&top::tests::network::xdummy_network_driver),
                                                           top::make_observer(chain_timer),
                                                           test_net_id,
                                                           top::make_observer(&top::tests::vnetwork::xdummy_network_data_accessor));
    for (auto i = 0u; i < cluster_count; ++i) {
        for (auto j = 0u; j < auditor_group_count; ++j) {
            auto vnet_driver = std::make_shared<top::vnetwork::xvnetwork_driver_t>(make_observer(vhost.get()),
                                                                                   top::common::xnode_address_t{
                                                                                       top::common::xcluster_address_t{
                                                                                           test_net_id,
                                                                                           top::common::xdefault_zone_id,
                                                                                           top::common::xcluster_id_t{ static_cast<top::common::xcluster_id_t::value_type>(1 + i) },
                                                                                           top::common::xgroup_id_t{ static_cast<top::common::xgroup_id_t::value_type>(top::common::xauditor_group_id_value_begin + j) }
                                                                                       },
                                                                                       top::common::xaccount_election_address_t{ top::common::xnode_id_t{ "adv" }, top::common::xslot_id_t{ 0 } }
                                                                                   },
                                                                                   top::common::xelection_round_t{0});
            auto tids = vnet_driver->table_ids();
            ASSERT_EQ(enum_vbucket_has_books_count / zone_count / cluster_count / auditor_group_count * 8, tids.size());
        }
    }
}
