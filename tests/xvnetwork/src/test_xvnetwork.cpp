// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xasio_io_context_wrapper.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xversion.h"
#include "tests/xvnetwork/xvhost_fixture.hpp"
#include "xvnetwork/xaddress.h"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xvnode_address_codec.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>           // NOLINT
#include <sstream>
#include <unordered_set>

using namespace top;        // NOLINT
using namespace vnetwork;   // NOLINT
using namespace network;    // NOLINT
using namespace common;     // NOLINT

#define DEFINE_VERSION_AND_NET_ID(NUM)                      \
    common::xelection_round_t const version ## NUM {             \
        static_cast<common::xelection_round_t::value_type>(NUM)  \
    };                                                      \
    auto const network_id ## NUM = m_network_info.network_id

#if 0
using xsingle_vhost_fixture = top::vnetwork::tests::xvhost_fixture_t<1, 1, 1>;
TEST_F(xsingle_vhost_fixture, version) {
    common::xelection_round_t const version{
        static_cast<common::xelection_round_t::value_type>(0)
    };

    auto vnetwork = m_vhost_manager->object(0).add_vnetwork(m_network_info.network_id);
    auto zone = vnetwork->add_zone_vnode(common::xzone_id_t{ 1 });
    auto cluster = zone->add_cluster_vnode(common::xcluster_id_t{ 1 });
    auto advance_group = cluster->add_group_vnode(common::xgroup_id_t{ 1 }, common::xnode_type_t::consensus_auditor, version, 0, 0, 0);
    auto advance_vnode = advance_group.added->add_vnode(common::xnode_id_t{ u8"T-test" }, xslot_id_t{ 0 }, version, 0);
    auto consensus_group = cluster->add_group_vnode(common::xgroup_id_t{ 64 }, common::xnode_type_t::consensus_validator, version, 0, 0, 0);
    auto consensus_vnode = consensus_group.added->add_vnode(common::xnode_id_t{ u8"T-consensus" }, xslot_id_t{ 0 }, version, 0);

    EXPECT_EQ(consensus_vnode->version(), version);
    EXPECT_EQ(consensus_group.added->version(), version);
    EXPECT_EQ(advance_vnode->version(), version);
    EXPECT_EQ(advance_group.added->version(), version);
    // EXPECT_EQ(zone->version(), version);
    EXPECT_EQ(m_roles.at(0), m_vhost_manager->object(0).role());
    EXPECT_EQ(false, advance_vnode->address().empty());
    //std::printf("%s\n", oss.str().c_str());
}
#endif

TEST(xvnetwork, xcodec) {
    common::xnode_id_t node_id;
    node_id.random();

    top::common::xnode_address_t expected{
        top::common::xcluster_address_t{
            xnetwork_id_t{ 0 },
            xzone_id_t{ 1 },
            xcluster_id_t{ 1 },
            xgroup_id_t{ 1 }
        },
        xaccount_election_address_t{ node_id, top::common::xslot_id_t{} },
        common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(0) },
        std::uint16_t{1024},
        std::uint64_t{0}
    };

    auto bytes = ::top::codec::msgpack_encode(expected);
    auto actual = top::codec::msgpack_decode<top::common::xnode_address_t>(bytes);
    EXPECT_EQ(expected, actual);

    xvnetwork_message_t msg_expected{
        expected,
        expected,
        vnetwork::xmessage_t{ xbyte_buffer_t{}, common::xmessage_id_t::invalid },
        0
    };

    bytes = top::codec::msgpack_encode(msg_expected);
    auto msg_actual = top::codec::msgpack_decode<xvnetwork_message_t>(bytes);
    EXPECT_EQ(msg_expected.hash(), msg_actual.hash());
}

using xsix_vhost_fixture = top::vnetwork::tests::xvhost_fixture_t<6, 2, 2>;

#if 0
TEST_F(xsix_vhost_fixture, send_data) {
    for (auto i = 0u; i < m_network_driver_manager.object_count() - 1; ++i) {
        auto const j = i + 1;
        auto & dht_host = m_network_driver_manager.object(j).dht_host();
        m_network_driver_manager.object(i).p2p_bootstrap({ dht_host.host_dht_node() });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });

    auto const size = m_vhost_manager->object_count();
    for (auto i = 0u; i < size; ++i) {
        m_vhost_manager->object(i).register_message_ready_notify(top::common::xnode_address_t{
                                                                     top::common::xcluster_address_t{ m_network_info.network_id }
                                                                 },
                                                                 [this, i](xvnode_address_t const & src,
                                                                           vnetwork::xmessage_t const & msg,
                                                                           std::uint64_t const /*timer_height*/) {
            ++m_counter;
            EXPECT_EQ(top::common::get_message_category(msg.id()), xmessage_category_test);
            EXPECT_EQ(m_vhost_manager->object(0).host_node_id().value(), src.account_address().value());
            // EXPECT_EQ(m_vhost_manager->object(i).host_node_id(), static_cast<top::common::xnode_id_t>(dst.account_address()));
        });

        EXPECT_EQ(m_roles.at(i), m_vhost_manager->object(i).role());
    }

    std::vector<common::xnode_id_t> node_ids{ size };
    for (auto i = 0u; i < size; ++i) {
        node_ids[i] = m_network_driver_manager.object(i).host_node_id();
    }

    std::vector<std::shared_ptr<xvnetwork_t>> vnetworks{ size };
    common::xelection_round_t const version{
        static_cast<common::xelection_round_t::value_type>(0)
    };

    for (auto i = 0u; i < size; ++i) {
        auto vnetwork = m_vhost_manager->object(i).add_vnetwork(m_vhost_manager->object(i).network_id());
        auto zone = vnetwork->add_zone_vnode(top::common::xzone_id_t{ 1 });
        auto cluster = zone->add_cluster_vnode(common::xcluster_id_t{ 1 });
        auto advance_group = cluster->add_group_vnode(common::xgroup_id_t{ 1 }, common::xnode_type_t::consensus_auditor, version, 0, 0, 0);
        advance_group.added->add_vnode(node_ids[0], xslot_id_t{ 0 }, version, 0);
        auto consensus_group = cluster->add_group_vnode(common::xgroup_id_t{ 64 }, common::xnode_type_t::consensus_validator, version, 0, 0, 0);
        consensus_group.added->add_vnode(node_ids[1], xslot_id_t{ 0 }, version, 0);
        consensus_group.added->add_vnode(node_ids[2], xslot_id_t{ 1 }, version, 0);
        consensus_group.added->add_vnode(node_ids[3], xslot_id_t{ 2 }, version, 0);
        consensus_group.added->add_vnode(node_ids[4], xslot_id_t{ 3 }, version, 0);
        consensus_group.added->add_vnode(node_ids[5], xslot_id_t{ 4 }, version, 0);


        advance_group.added->associate_child_group(consensus_group.added);
        auto const & consensus_groups = advance_group.added->associated_child_groups(0);
        std::unordered_set<common::xgroup_id_t> consensus_group_ids;
        consensus_group_ids.reserve(consensus_groups.size());
        for (auto const & cluster : consensus_groups) {
            consensus_group_ids.insert(cluster->group_id());
        }
        auto const it = consensus_group_ids.find(consensus_group.added->group_id());
        EXPECT_NE(std::end(consensus_group_ids), it);
        // EXPECT_EQ(advance_group->group_id(), consensus_cluster->associated_parent_cluster()->cluster_id());
    }

    auto & vhost = m_vhost_manager->object(0);
    for (auto i = 1u; i < size; ++i) {
        vnetwork::xmessage_t msg{
            top::random_bytes(32),
            xmessage_id_test
        };

        vhost.send(msg,
                   top::common::xnode_address_t{
                       top::common::xcluster_address_t{
                           m_network_info.network_id,
                           common::xzone_id_t{ 1 },
                           common::xcluster_id_t{ 1 },
                           common::xgroup_id_t{ 1 }
                       },
                       xaccount_election_address_t{ node_ids[0], top::common::xslot_id_t{} },
                       common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(0) }
                   },
                   top::common::xnode_address_t{
                       top::common::xcluster_address_t{
                           m_network_info.network_id,
                           common::xzone_id_t{ 1 },
                           common::xcluster_id_t{ 1 },
                           common::xgroup_id_t{ 64 }
                       },
                       xaccount_election_address_t{ node_ids[i], top::common::xslot_id_t{} },
                       common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(0) }
                   },
                   {});
    }

    std::this_thread::sleep_for(std::chrono::seconds{ 5 });

    EXPECT_EQ(size - 1, m_counter);
}
#endif

// using x24_vhost_fixture = top::vnetwork::tests::xvhost_fixture_t<24, 12, 12>;
// TEST_F(x24_vhost_fixture, bootstrap) {
//     DEFINE_VERSION_AND_NET_ID(0);

//     std::size_t const zone_count{ 3 };
//     std::size_t const advance_cluster_count{ 2 };
//     std::size_t const consensus_cluster_count{ 4 };

//     /*
//      * +--------------------------+
//      * | zone 0 (for edge nodes)  |
//      * |+-----------+             |
//      * || cluster 0 |             |
//      * |+-----------+             |
//      * +--------------------------+
//      * |
//      * +---------------------------------------------------------------------------------------------+
//      * | zone 1 (for advance / consensus / archive nodes)                                            |
//      * |+-----------------+                                                                          |
//      * ||    cluster 0    |                                                                          |
//      * ||-----------------|                                                                          |
//      * || archive cluster |                                                                          |
//      * |+-----------------+                                                                          |
//      * |                                                                                             |
//      * |                                                                                             |
//      * |            +-------------------+                           +-------------------+            |
//      * |            |     cluster x     |                           |   cluster x + 1   |            |
//      * |            |-------------------|                           |-------------------|            |
//      * |            |  advance cluster  |                           |  advance cluster  |            |
//      * |            +-------------------+                           +-------------------+            |
//      * |                    ^   ^                                           ^   ^                    |
//      * |                    |   |                                           |   |                    |
//      * |            +-------+   +-------+                           +-------+   +-------+            |
//      * |            |                   |                           |                   |            |
//      * |            |                   |                           |                   |            |
//      * |+-----------+-------+   +-------+-----------+   +-----------+-------+   +-------+-----------+|
//      * ||     cluster 1     |   |     cluster 3     |   |     cluster 2     |   |     cluster 4     ||
//      * ||-------------------|   |-------------------|   |-------------------|   |-------------------||
//      * || consensus cluster |   | consensus cluster |   | consensus cluster |   | consensus cluster ||
//      * |+-------------------+   +-------------------+   +-------------------+   +-------------------+|
//      * +---------------------------------------------------------------------------------------------+
//      */

//     // in this case, first build up the initial virtual network.  the edge node the REC node.
//     top::vnetwork::xvnetwork_construction_data_t construction_data{ network_id0, version0 };
//     for (auto i = 0u; i < zone_count; ++i) {
//         switch (i) {
//             // zone 0 edge_cluster_id is the edge network
//             case 0: {
//                 xcluster_address_t edge_cluster_address{ network_id0, xedge_zone_id, xdefault_cluster_id, xdefault_group_id, xvnode_type_t::edge };
//                 construction_data.add_group_info(edge_cluster_address, 0, version0, common::xelection_round_t{}, xgroup_id_t{});
//                 construction_data.insert(m_network_driver_manager.object(0).host_node_id(), xslot_id_t{ static_cast<xslot_id_t::value_type>(i) }, edge_cluster_address, version0);
//                 break;
//             }

//             // other zones are working zones. each zone has a archive cluster.
//             default: {
//                 xcluster_address_t archive_cluster_address{ network_id0, xzone_id_t{ static_cast<xzone_id_t::value_type>(i) }, xtop_archive_cid, xarchive_group_id, xvnode_type_t::archive };
//                 construction_data.add_group_info(archive_cluster_address, 0, version0, common::xelection_round_t{}, xgroup_id_t{});

//                 for (auto j = 0u; j < advance_cluster_count; ++j) {
//                     xcluster_address_t advance_cluster_address{
//                         network_id0,
//                         xzone_id_t{ static_cast<xzone_id_t::value_type>(i) },
//                         common::xdefault_cluster_id,
//                         common::xgroup_id_t{ static_cast<common::xgroup_id_t::value_type>(common::xauditor_group_id_value_begin + j) },
//                         common::xnode_type_t::consensus_auditor
//                     };
//                     construction_data.add_group_info(advance_cluster_address, 0, version0, common::xelection_round_t{}, common::xgroup_id_t{});
//                 }

//                 for (auto j = 0u; j < consensus_cluster_count; ++j) {
//                     common::xgroup_id_t consensus_cluster_id{ static_cast<common::xgroup_id_t::value_type>(common::xvalidator_group_id_value_begin + j) };

//                     xcluster_address_t consensus_cluster_address{
//                         network_id0,
//                         xzone_id_t{ static_cast<xzone_id_t::value_type>(i) },
//                         common::xdefault_cluster_id,
//                         consensus_cluster_id,
//                         common::xnode_type_t::consensus_validator
//                     };
//                     construction_data.add_group_info(consensus_cluster_address, 0, version0, version0, xgroup_id_t{static_cast<common::xgroup_id_t::value_type>(common::xauditor_group_id_value_begin + j)});
//                 }

//                 break;
//             }
//         }
//     }

//     xcluster_address_t edge_cluster_address{
//         network_id0,
//         xedge_zone_id,
//         xdefault_cluster_id,
//         xdefault_group_id
//     };
//     xvnode_address_t const edge_address{
//         edge_cluster_address,
//         xaccount_address_t{ m_network_driver_manager.object(0).host_node_id() },
//         version0
//     };

//     // on the first node (which is edge), builds the initial virtual network.
//     m_vhost_manager->object(0).build_vnetwork(construction_data);
//     // EXPECT_EQ(edge_address, m_vhost_manager->object(0).address(xvnode_address_query_condition_t{ edge_cluster_address, m_network_driver_manager.object(0).host_node_id() }));
//     {
//         auto vnetwork = m_vhost_manager->object(0).vnetwork(network_id0);
//         auto zone_vnodes = vnetwork->zone_vnodes();
//         EXPECT_EQ(zone_count, zone_vnodes.size());    // we have 3 zones for testing

//         for (auto const & zone_vnode : zone_vnodes) {
//             if (zone_vnode->zone_id() == xedge_zone_id) {
//                 continue;
//             }



//             // EXPECT_EQ(advance_cluster_count, zone_vnode->children<common::xnode_type_t::consensus_auditor>().size());
//             // EXPECT_EQ(consensus_cluster_count, zone_vnode->children<common::xnode_type_t::consensus_validator>().size());
//         }
//     }

//     for (auto i = 1u; i < m_vhost_manager->object_count(); ++i) {
//         //m_vhost_manager->object(i).bootstrap(edge_address,
//         //                                     m_network_driver_manager.object(0).host_node(),
//         //                                     { m_network_driver_manager.object(0).dht_host().host_dht_node() });
//         std::this_thread::sleep_for(std::chrono::seconds{ 1 });
//     }

//     std::this_thread::sleep_for(std::chrono::minutes{ 1 });

//     for (auto i = 0u; i < m_vhost_manager->object_count(); ++i) {
//         // EXPECT_EQ(false, m_vhost_manager->object(i).last_address().empty());
//         // std::printf("%s last address %s\n", m_vhost_manager->object(i).host_node_id().to_string().c_str(), m_vhost_manager->object(i).last_address().to_string().c_str());
//         // std::fflush(stdout);
//     }

//     for (auto i = 0u; i < m_network_driver_manager.object_count(); ++i) {
//         auto size = m_network_driver_manager.object(i).dht_host().all_node_entries().size();
//         EXPECT_GE(size, 1);
//         std::printf("%s neighbors count %zu\n", m_network_driver_manager.object(i).host_node_id().to_string().c_str(), size);
//         std::fflush(stdout);
//     }

//     for (auto i = 0u; i < m_vhost_manager->object_count(); ++i) {
//         auto vnetwork = m_vhost_manager->object(i).vnetwork(network_id0);
//         auto const zone_vnodes = vnetwork->zone_vnodes();
//         EXPECT_EQ(zone_count, zone_vnodes.size());    // we have 3 zones for testing

//         for (auto const & zone_vnode : zone_vnodes) {
//             if (zone_vnode->zone_id() == xedge_zone_id) {
//                 continue;
//             }

//             //EXPECT_EQ(advance_cluster_count, zone_vnode->children<common::xnode_type_t::consensus_auditor>().size());
//             //EXPECT_EQ(consensus_cluster_count, zone_vnode->children<common::xnode_type_t::consensus_validator>().size());
//         }
//     }

//     for (auto i = 0u; i < m_vhost_manager->object_count(); ++i) {
//         //auto & vhost = m_vhost_manager->object(i);
//         //xcluster_address_t cluster_addr{
//         //    vhost.network_id(),
//         //    vhost.
//         //};
//         //auto addr = vhost.address(xvnode_address_query_condition_t{vhost.network_id(), vhost.});
//         //m_vhost_manager->object(i).register_message_ready_notify(xmessage_category_test,
//         //                                                         [this, last_address](top::vnetwork::xvnode_address_t const & sender,
//         //                                                                              top::vnetwork::xvnode_address_t const & receiver,
//         //                                                                              top::vnetwork::xmessage_t const & message) {
//         //    m_counter++;
//         //    std::string message_string{ std::begin(message.payload()), std::end(message.payload()) };
//         //    EXPECT_EQ(message_string, sender.to_string() + last_address.to_string());
//         //});
//     }

//     for (auto i = m_vhost_manager->object_count() - 1; i > 0; --i) {
//         //for (auto j = 0u; j < i; ++j) {
//         //    std::string message_string = m_vhost_manager->object(i).last_address().to_string();
//         //    message_string += m_vhost_manager->object(j).last_address().to_string();
//         //    xbyte_buffer_t payload{ std::begin(message_string), std::end(message_string) };
//         //    vnetwork::xmessage_t message{
//         //        payload,
//         //        xmessage_id_test
//         //    };
//         //    m_vhost_manager->object(i).send(m_vhost_manager->object(j).last_address(), message, {});

//         //    std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
//         //}
//     }

//     // std::this_thread::sleep_for( std::chrono::seconds{ 10 });
//     // auto n = m_vhost_manager->object_count();

//     // EXPECT_EQ((n * n - n) / 2, m_counter);
// }

using x16_8_8_vhost_fixture = top::vnetwork::tests::xvhost_fixture_t<16, 8, 8>;
TEST_F(x16_8_8_vhost_fixture, gossip) {
    // DEFINE_VERSION_AND_NET_ID(0);

    // std::array<common::xminer_type_t, 16> roles{
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge,
    //     common::xminer_type_t::edge
    // };

    // reset_vhost_manager(roles);

    // xvnetwork_construction_data_t construction_data0{ network_id0, version0 };
    // xcluster_address_t cluster_addr{
    //     network_id0,
    //     xedge_zone_id,
    //     xdefault_cluster_id,
    //     xdefault_group_id
    // };
    // construction_data0.add_group_info(cluster_addr, 0, version0, common::xelection_round_t{}, common::xgroup_id_t{});
    // for (auto i = 0u; i < m_vhost_manager->object_count(); ++i) {
    //     construction_data0.insert(m_vhost_manager->object(i).host_node_id(), xslot_id_t{ static_cast<xslot_id_t::value_type>(i) }, cluster_addr, version0);
    // }

    // for (auto i = 0u; i < m_vhost_manager->object_count(); ++i) {
    //     m_vhost_manager->object(i).build_vnetwork(construction_data0);
    // }

    // xvnode_address_t const edge_address{
    //     cluster_addr,
    //     xaccount_address_t{ m_network_driver_manager.object(0).host_node_id() },
    //     version0
    // };

    // for (auto i = 1u; i < m_vhost_manager->object_count(); ++i) {
    //     m_vhost_manager->object(i).bootstrap({ m_network_driver_manager.object(0).dht_host().host_dht_node() });
    //     std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
    // }

    // std::this_thread::sleep_for(std::chrono::seconds{ 10 });

    // for (auto i = 0u; i < m_vhost_manager->object_count(); ++i) {
    //     m_vhost_manager->object(i).register_message_ready_notify(xvnode_address_t{
    //                                                                  cluster_addr,
    //                                                                  xaccount_address_t{ m_network_driver_manager.object(i).host_node_id() },
    //                                                                  version0
    //                                                              },
    //                                                              [this, i](xvnode_address_t const & src,
    //                                                                        vnetwork::xmessage_t const & msg,
    //                                                                        std::uint64_t const /*timer_height*/) {
    //          ++m_counter;
    //          EXPECT_EQ(top::common::get_message_category(msg.id()), xmessage_category_test);
    //          EXPECT_EQ(m_vhost_manager->object(0).host_node_id(), static_cast<top::common::xnode_id_t>(src.account_address()));
    //          // EXPECT_EQ(m_vhost_manager->object(i).host_node_id(), static_cast<top::common::xnode_id_t>(dst.account_address()));
    //          std::string const rumor{ u8"rumor" };
    //          std::string msg_payload{ std::begin(msg.payload()), std::end(msg.payload()) };
    //          EXPECT_EQ(rumor, msg_payload);
    //      });

    //     EXPECT_EQ(m_roles.at(i), m_vhost_manager->object(i).role());
    // }

    // std::string msg{ u8"rumor" };
    // vnetwork::xmessage_t rumor{ xbyte_buffer_t{ std::begin(msg), std::end(msg), }, xmessage_id_test };
    // auto addr = m_vhost_manager->object(0).address(cluster_addr, version0);
    // m_vhost_manager->object(0).broadcast(rumor,
    //                                      addr);

    // std::this_thread::sleep_for(std::chrono::seconds{ 30 });

    // EXPECT_EQ(m_vhost_manager->object_count() - 1, m_counter);
}

#undef DEFINE_VERSION_AND_NET_ID
