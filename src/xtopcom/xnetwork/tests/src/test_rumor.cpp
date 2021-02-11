// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/tests/xrumor_fixture.hpp"
#include "xnetwork/tests/xdelay_socket.h"
#include "xnetwork/tests/xdrop_socket.h"
#include "xnetwork/xnetwork_driver.h"

#include <gtest/gtest.h>

#include <thread>

using namespace top;        // NOLINT
using namespace network;    // NOLINT
using namespace tests;      // NOLINT

using x64_rumor_fixture = top::network::tests::xrumor_fixture_t<64, 8, 8>;
TEST_F(x64_rumor_fixture, statistic) {
    for (auto i = 0u; i < m_network_driver_manager.object_count() - 1; ++i) {
        auto const j = i + 1;
        auto & dht_host = m_network_driver_manager.object(j).dht_host();
        m_network_driver_manager.object(i).p2p_bootstrap({ dht_host.host_dht_node() });
    }

    std::this_thread::sleep_for(std::chrono::seconds{ 30 });

    //for (auto i = 0u; i < m_network_driver_manager.object_count(); ++i) {
    //    auto const & node_id_value = m_network_driver_manager.object(i).host_node_id().value();
    //    xbyte_buffer_t const rumor{ std::begin(node_id_value), std::end(node_id_value) };
    //    m_network_driver_manager.object(i).spread_rumor(rumor);
    //}
    // std::printf("%ld\n", std::time(nullptr));
    // std::fflush(stdout);
    rumors.clear();
    rumors.push_back(u8"rumor");
    m_network_driver_manager.object(0).spread_rumor(xbyte_buffer_t{ std::begin(rumors[0]), std::end(rumors[0]) });

    std::this_thread::sleep_for(std::chrono::minutes{1});

    EXPECT_EQ(63, m_received_msg_count);
}

XINLINE_CONSTEXPR std::size_t node_count{ 12 };
using x128_random_delay_rumor_fixture = top::network::tests::xrumor_fixture_t<node_count, 12, 12, xtrip_delay_socket_t<xudp_socket_t, 1, 400>>;
TEST_F(x128_random_delay_rumor_fixture, gossip) {
    if (!delay_executer.running()) {
        delay_executer.start();
    }

    for (auto i = 0u; i < m_network_driver_manager.object_count() - 1; ++i) {
        auto const j = i + 1;
        auto & dht_host = m_network_driver_manager.object(j).dht_host();
        m_network_driver_manager.object(i).p2p_bootstrap({ dht_host.host_dht_node() });
    }

    std::this_thread::sleep_for(std::chrono::minutes{1});

    //for (auto i = 0u; i < m_network_driver_manager.object_count(); ++i) {
    //    auto const & node_id_value = m_network_driver_manager.object(i).host_node_id().value();
    //    xbyte_buffer_t const rumor{ std::begin(node_id_value), std::end(node_id_value) };
    //    m_network_driver_manager.object(i).spread_rumor(rumor);
    //}
    std::printf("%ld\n", std::time(nullptr));
    std::fflush(stdout);

    std::size_t const rumor_count = 1000;
    rumors.clear();
    for (auto i = 0u; i < rumor_count; ++i) {
        rumors.push_back(rumor_base + std::to_string(i));
    }

    for (auto i = 0u; i < rumors.size(); ++i) {
        m_network_driver_manager.object(i % m_network_driver_manager.object_count()).spread_rumor(xbyte_buffer_t{ std::begin(rumors[i]), std::end(rumors[i]) });
    }

    std::this_thread::sleep_for(std::chrono::minutes{1});

    EXPECT_EQ((node_count - 1) * rumor_count, m_received_msg_count);

    if (delay_executer.running()) {
        delay_executer.stop();
    }
}

XINLINE_CONSTEXPR std::size_t node_count1{ 128 };
using x128_random_drop_rumor_fixture = top::network::tests::xrumor_fixture_t<node_count1, 12, 12, xdrop_socket_t<xudp_socket_t, 40>>; // 40% drop rate
TEST_F(x128_random_drop_rumor_fixture, gossip_drop) {
    for (auto i = 0u; i < m_network_driver_manager.object_count() - 1; ++i) {
        auto const j = i + 1;
        auto & dht_host = m_network_driver_manager.object(j).dht_host();
        m_network_driver_manager.object(i).p2p_bootstrap({ dht_host.host_dht_node() });
    }

    std::this_thread::sleep_for(std::chrono::minutes{1});

    std::printf("%ld\n", std::time(nullptr));
    std::fflush(stdout);

    std::size_t const rumor_count = 100;
    rumors.clear();
    for (auto i = 0u; i < rumor_count; ++i) {
        rumors.push_back(rumor_base + std::to_string(i));
    }

    for (auto i = 0u; i < rumors.size(); ++i) {
        m_network_driver_manager.object(i % m_network_driver_manager.object_count()).spread_rumor(xbyte_buffer_t{ std::begin(rumors[i]), std::end(rumors[i]) });
    }

    std::this_thread::sleep_for(std::chrono::minutes{ 2 });

    EXPECT_EQ((node_count1 - 1) * rumor_count, m_received_msg_count);
}
