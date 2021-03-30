// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xnetwork/xp2p/xrouting_table.h"
#include "xnetwork/xudp_socket.h"

#include <gtest/gtest.h>

#include <memory>
#include <thread>

TEST(xnetwork, routing_table) {
    using namespace top;
    using namespace network;

    auto io_context_wrapper = std::make_shared<xasio_io_context_wrapper_t>();
    io_context_wrapper->async_start();

    auto timer_driver = std::make_shared<xtimer_driver_t>(io_context_wrapper);
    timer_driver->start();

    std::uint16_t const loop_count = 8;
    std::uint16_t const dht_port_base = 10000;
    std::uint16_t const app_port_base = 20000;

    std::vector<std::shared_ptr<p2p::xrouting_table_t>> routing_tables;
    for (std::uint16_t i = 0u; i < loop_count; ++i) {
        auto dht_port = static_cast<std::uint16_t>(dht_port_base + i);
        auto app_port = static_cast<std::uint16_t>(app_port_base + i);
        common::xnode_id_t host_id{ std::to_string(app_port) };

        std::shared_ptr<xsocket_face_t> dht_socket =
            std::make_shared<xudp_socket_t>(io_context_wrapper, dht_port);

        auto routing_table =
            std::make_shared<p2p::xrouting_table_t>(host_id,
                                                    std::move(dht_socket),
                                                    // app_port,
                                                    timer_driver.get());

        routing_table->start();

        routing_tables.push_back(std::move(routing_table));
    }

    for (std::uint16_t i = 0u; i < loop_count; ++i) {
        EXPECT_EQ(common::xnode_id_t{ std::to_string(app_port_base + i) }, routing_tables[i]->host_node_id());
    }

    for (std::uint16_t i = 1u; i < loop_count; ++i) {
        routing_tables[0]->add_node(xdht_node_t
                                    {
                                        common::xnode_id_t{ std::to_string(app_port_base + i) },
                                        xnode_endpoint_t{ "127.0.0.1", static_cast<std::uint16_t>(dht_port_base + i) }
                                    });// ,
                                    //static_cast<std::uint16_t>(app_port_base + i));
    }

    std::this_thread::sleep_for(std::chrono::seconds{20});

    for (std::uint16_t j = 1u; j < loop_count; ++j) {
        auto node_id = routing_tables[0]->node_id({ "127.0.0.1", static_cast<std::uint16_t>(dht_port_base + j) });
        EXPECT_EQ(std::to_string(app_port_base + j), node_id.to_string());

        auto node_id2 = routing_tables[j]->node_id({ "127.0.0.1", dht_port_base });
        EXPECT_EQ(routing_tables[0]->host_node_id().to_string(), node_id2.to_string());
    }

    for (std::uint16_t j = 0u; j < loop_count; ++j) {
        auto const all_known_nodes = routing_tables[j]->all_known_nodes().size();
        if (all_known_nodes != loop_count) {
            EXPECT_EQ(loop_count - 1, all_known_nodes);

            for (std::uint16_t i = 0u; i < loop_count; ++i) {
                if (i != j) {
                    auto const port = static_cast<std::uint16_t>(dht_port_base + j);
                    auto node_id = routing_tables[j]->node_id({ "127.0.0.1", port });
                    EXPECT_EQ(std::to_string(app_port_base + j), node_id.to_string());
                }
            }
        } else {
            for (std::uint16_t i = 0u; i < loop_count; ++i) {
                auto const port = static_cast<std::uint16_t>(dht_port_base + j);
                auto node_id = routing_tables[j]->node_id({ "127.0.0.1", port });
                EXPECT_EQ(std::to_string(app_port_base + i), node_id.to_string());
            }
        }
    }

    for (auto & routing_table : routing_tables) {
        routing_table->stop();
    }

    timer_driver->stop();

    io_context_wrapper->stop();
}
