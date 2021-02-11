// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xnetwork/tests/xnetwork_driver_fixture.hpp"
#include "xnetwork/xnetwork_driver.h"
#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xdht_host.h"
#include "xnetwork/xudp_socket.h"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>

using namespace top;
using namespace network;

//TEST(xnetwork, network_driver) {
//    auto io_context_wrapper{ std::make_shared<xasio_io_context_wrapper_t>() };
//    io_context_wrapper->async_run();
//
//    auto timer_driver{ std::make_shared<xtimer_driver_t>(io_context_wrapper) };
//    timer_driver->run();
//
//    xnode_id_t const node1_id{ "payton.wu@topnetwork.org" };
//    std::uint16_t const node1_dht_port = 10000;
//    std::uint16_t const node1_gossip_port = 10001;
//
//    xnode_id_t const node2_id{ "payton.wu@outlook.com" };
//    std::uint16_t const node2_dht_port = 10002;
//    std::uint16_t const node2_gossip_port = 10003;
//
//    xnode_endpoint_t const node1_dht_address{ "127.0.0.1", node1_dht_port };
//    xnode_t node1{ node1_id, { "127.0.0.1", node1_gossip_port } };
//    std::shared_ptr<xsocket_face_t> node1_dht_socket = std::make_shared<xudp_socket_t>(io_context_wrapper, node1_dht_port);
//    std::shared_ptr<xsocket_face_t> node1_gossip_socket = std::make_shared<xudp_socket_t>(io_context_wrapper, node1_gossip_port);
//
//    std::shared_ptr<xnetwork_driver_face_t> network_driver1 =
//        std::make_shared<xnetwork_driver_t>(timer_driver,
//                                            std::make_unique<p2p::xdht_host_t>(node1_id,
//                                                                               node1_dht_socket,
//                                                                               node1_gossip_port,
//                                                                               timer_driver),
//                                            node1_gossip_socket);
//
//    network_driver1->start();
//
//    xnode_t node2{ node2_id, { "127.0.0.1", node2_gossip_port} };
//    std::shared_ptr<xsocket_face_t> node2_dht_socket = std::make_shared<xudp_socket_t>(io_context_wrapper, node2_dht_port);
//    std::shared_ptr<xsocket_face_t> node2_gossip_socket = std::make_shared<xudp_socket_t>(io_context_wrapper, node2_gossip_port);
//
//    std::shared_ptr<xnetwork_driver_face_t> network_driver2 =
//        std::make_shared<xnetwork_driver_t>(timer_driver,
//                                            std::make_unique<p2p::xdht_host_t>(node2_id,
//                                                                               node2_dht_socket,
//                                                                               node2_gossip_port,
//                                                                               timer_driver),
//                                            node2_gossip_socket);
//    network_driver2->start();
//
//    std::this_thread::sleep_for(1s);
//
//    network_driver1->p2p_bootstrap(
//        {
//            xdht_node_t
//            {
//                node2_id,
//                xnode_endpoint_t{ "127.0.0.1", node2_dht_port }
//            }
//        });
//
//    std::this_thread::sleep_for(1s);
//    auto & dht_host1 = network_driver1->dht_host();
//    auto & dht_host2 = network_driver2->dht_host();
//
//    EXPECT_EQ(dht_host1.host_node_id(), node1.id());
//    EXPECT_EQ(dht_host2.host_node_id(), node2.id());
//
//    auto const node_entries = dht_host2.nearest_node_entries(dht_host1.host_node_id());
//    EXPECT_EQ(node_entries.size(), 1);
//
//    for (auto const & node_entry : node_entries) {
//        EXPECT_EQ(node_entry->id(), node1.id());
//    }
//
//    timer_driver->stop();
//    io_context_wrapper->stop();
//}

using x2network_driver_fixture_t = tests::xnetwork_driver_fixture_t<2, 1, 1>;
TEST_F(x2network_driver_fixture_t, p2p_bootstrap) {
    m_network_driver_manager.object(0).p2p_bootstrap({
        m_network_driver_manager.object(1).dht_host().host_dht_node()
    });

    std::this_thread::sleep_for(std::chrono::seconds{1});
    auto & dht_host0 = m_network_driver_manager.object(0).dht_host();
    auto & dht_host1 = m_network_driver_manager.object(1).dht_host();

    EXPECT_EQ(dht_host0.host_node_id(), m_network_driver_manager.object(0).host_node_id());
    EXPECT_EQ(dht_host1.host_node_id(), m_network_driver_manager.object(1).host_node_id());

    auto const node_entries = dht_host1.nearest_node_entries(dht_host0.host_node_id());
    EXPECT_EQ(1, node_entries.size());

    for (auto const & node_entry : node_entries) {
        EXPECT_EQ(m_network_driver_manager.object(0).host_node_id(), node_entry->id());
    }
}

using xthree_network_driver_fixture = tests::xnetwork_driver_fixture_t<3, 1, 1>;
TEST_F(xthree_network_driver_fixture, send_to) {
    auto & dht_host1 = m_network_driver_manager.object(1).dht_host();
    m_network_driver_manager.object(0).p2p_bootstrap({ dht_host1.host_dht_node() });

    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    std::ostringstream oss;
    oss << m_network_driver_manager.object(0).host_node_id().value()
        << ' '
        << m_network_driver_manager.object(1).host_node_id().value();

    auto message = oss.str();
    m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(1).host_node_id(),
                                  { std::begin(message), std::end(message) },
                                  {});

    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_FALSE(m_messages.empty());
        auto const it = m_messages.find(m_network_driver_manager.object(1).host_node_id());
        EXPECT_FALSE(it == std::end(m_messages));
        EXPECT_TRUE(it->second == m_network_driver_manager.object(0).host_node_id());
    }

    auto & dht_host2 = m_network_driver_manager.object(2).dht_host();
    m_network_driver_manager.object(0).p2p_bootstrap({ dht_host2.host_dht_node() });

    std::this_thread::sleep_for(std::chrono::milliseconds{200});

    std::ostringstream oss2;
    oss2 << m_network_driver_manager.object(1).host_node_id().value()
        << ' '
        << m_network_driver_manager.object(2).host_node_id().value();
    message = oss2.str();
    m_network_driver_manager.object(1).send_to(m_network_driver_manager.object(2).host_node_id(),
                                  { std::begin(message), std::end(message) },
                                  {});

    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_FALSE(m_messages.empty());
        auto const it = m_messages.find(m_network_driver_manager.object(2).host_node_id());
        EXPECT_FALSE(it == std::end(m_messages));
        EXPECT_TRUE(it->second == m_network_driver_manager.object(1).host_node_id());
    }
}

using xten_network_driver_fixture = tests::xnetwork_driver_fixture_t<10, 10, 10>;
TEST_F(xten_network_driver_fixture, ttl) {

    for (auto i = 0u; i < m_network_driver_manager.object_count() - 1; ++i) {
        auto const j = i + 1;
        auto & dht_host = m_network_driver_manager.object(j).dht_host();
        m_network_driver_manager.object(i).p2p_bootstrap({ dht_host.host_dht_node() });
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    ///////////////////////////////////////////////////////////////////////////
    // TTL = 0
    ///////////////////////////////////////////////////////////////////////////
    {
        std::ostringstream oss;
        oss << m_network_driver_manager.object(0).host_node_id().value()
            << ' '
            << m_network_driver_manager.object(1).host_node_id().value();

        auto message = oss.str();
        m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(1).host_node_id(),
                                                   { std::begin(message), std::end(message) },
                                                   {
                                                       xdeliver_property_t{},
                                                       xspread_property_t{ xspread_mode_t::pt2pt, 0 }
                                                   });

        std::this_thread::sleep_for(std::chrono::milliseconds{100});

        XLOCK_GUARD(m_messages_mutex) {
            EXPECT_TRUE(m_messages.empty());
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // TTL = 1
    ///////////////////////////////////////////////////////////////////////////
    {
        std::ostringstream oss;
        oss << m_network_driver_manager.object(0).host_node_id().value()
            << ' '
            << m_network_driver_manager.object(2).host_node_id().value();

        auto message = oss.str();
        m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(2).host_node_id(),
                                                   { std::begin(message), std::end(message) },
                                                   {
                                                       xdeliver_property_t{},
                                                       xspread_property_t{ xspread_mode_t::pt2pt, 1 }
                                                   });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_TRUE(m_messages.empty());
    }

    {
        std::ostringstream oss;
        oss << m_network_driver_manager.object(0).host_node_id().value()
            << ' '
            << m_network_driver_manager.object(1).host_node_id().value();

        auto message = oss.str();
        m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(1).host_node_id(),
                                                   { std::begin(message), std::end(message) },
                                                   {
                                                       xdeliver_property_t{},
                                                       xspread_property_t{ xspread_mode_t::pt2pt, 1 }
                                                   });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_FALSE(m_messages.empty());
        EXPECT_EQ(1, m_messages.size());
    }
    ///////////////////////////////////////////////////////////////////////////
    // TTL = 2
    ///////////////////////////////////////////////////////////////////////////
    {
        std::ostringstream oss;
        oss << m_network_driver_manager.object(0).host_node_id().value()
            << ' '
            << m_network_driver_manager.object(3).host_node_id().value();

        auto message = oss.str();
        m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(3).host_node_id(),
                                      { std::begin(message), std::end(message) },
                                  {
                                      xdeliver_property_t{},
                                      xspread_property_t{ xspread_mode_t::pt2pt, 2 }
                                  });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_EQ(1, m_messages.size());
    }

    {
        std::ostringstream oss;
        oss << m_network_driver_manager.object(0).host_node_id().value()
            << ' '
            << m_network_driver_manager.object(2).host_node_id().value();

        auto message = oss.str();
        m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(2).host_node_id(),
                                      { std::begin(message), std::end(message) },
                                  {
                                      xdeliver_property_t{},
                                      xspread_property_t{ xspread_mode_t::pt2pt, 2 }
                                  });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_EQ(2, m_messages.size());
    }

    ///////////////////////////////////////////////////////////////////////////
    // TTL = 8
    ///////////////////////////////////////////////////////////////////////////
    {
        std::ostringstream oss;
        oss << m_network_driver_manager.object(0).host_node_id().value()
            << ' '
            << m_network_driver_manager.object(9).host_node_id().value();

        auto message = oss.str();
        m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(9).host_node_id(),
                                      { std::begin(message), std::end(message) },
                                  {
                                      xdeliver_property_t{},
                                      xspread_property_t{ xspread_mode_t::pt2pt, 8 }
                                  });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_EQ(2, m_messages.size());
    }

    {
        std::ostringstream oss;
        oss << m_network_driver_manager.object(0).host_node_id().value()
            << ' '
            << m_network_driver_manager.object(8).host_node_id().value();

        auto message = oss.str();
        m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(8).host_node_id(),
                                                   { std::begin(message), std::end(message) },
                                                   {
                                                       xdeliver_property_t{},
                                                       xspread_property_t{ xspread_mode_t::pt2pt, 8 }
                                                   });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_EQ(3, m_messages.size());
    }

    ///////////////////////////////////////////////////////////////////////////
    // TTL = 9
    ///////////////////////////////////////////////////////////////////////////
    {
        std::ostringstream oss;
        oss << m_network_driver_manager.object(0).host_node_id().value()
            << ' '
            << m_network_driver_manager.object(9).host_node_id().value();

        auto message = oss.str();
        m_network_driver_manager.object(0).send_to(m_network_driver_manager.object(9).host_node_id(),
                                      { std::begin(message), std::end(message) },
                                  {
                                      xdeliver_property_t{},
                                      xspread_property_t{ xspread_mode_t::pt2pt, 9 }
                                  });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    XLOCK_GUARD(m_messages_mutex) {
        EXPECT_EQ(3, m_messages.size());
    }

    std::this_thread::sleep_for(std::chrono::seconds{1});
}

using x250_network_driver_fixture = tests::xnetwork_driver_fixture_t<250, 16, 16>;
TEST_F(x250_network_driver_fixture, bootstrap) {

    EXPECT_NO_FATAL_FAILURE(for (auto i = 0u; i < m_network_driver_manager.object_count() - 1; ++i) {
        auto const j = i + 1;
        auto & dht_host = m_network_driver_manager.object(j).dht_host();
        m_network_driver_manager.object(i).p2p_bootstrap({ dht_host.host_dht_node() });
        //std::this_thread::sleep_for(20ms);
    });

    std::this_thread::sleep_for(std::chrono::minutes{2});
}
