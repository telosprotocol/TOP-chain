// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/tests/xtrival_socket.h"
#include "xnetwork/tests/xdht_host_fixture.hpp"
#include "xnetwork/xp2p/xdht_host.h"

#include <gtest/gtest.h>

#include <memory>

using namespace top;
using namespace network;
using namespace tests;

TEST(xnetwork, bootstrap_empty_seeds) {
    auto io_context_wrapper = std::make_shared<xasio_io_context_wrapper_t>();
    auto timer_driver = std::make_shared<xtimer_driver_t>(io_context_wrapper);

    //std::unique_ptr<p2p::xdht_host_face_t> const dht_host =
    auto const dht_host =
        top::make_unique<p2p::xdht_host_t>(common::xnode_id_t{},
                                           std::make_shared<tests::xtrival_socket_t>(),
                                           // 10000,
                                           timer_driver.get());

    EXPECT_NO_THROW(dht_host->bootstrap({}));
    common::xnode_id_t node_id;
    node_id.random();

    auto neighbors = dht_host->nearest_node_entries(node_id);
    EXPECT_TRUE(neighbors.empty());

    std::this_thread::sleep_for(std::chrono::seconds{10});

    node_id.random();
    neighbors = dht_host->nearest_node_entries(node_id);
    EXPECT_TRUE(neighbors.empty());
}

using xsingle_dht_host_fixture = xtop_dht_host_fixture<1>;
TEST_F(xsingle_dht_host_fixture, bootstrap_non_exist_seed) {
    common::xnode_id_t seed_node_id;
    seed_node_id.random();

    assert(m_dht_hosts[0]);
    EXPECT_NO_THROW(m_dht_hosts[0]->bootstrap({ {seed_node_id, xnode_endpoint_t{ "127.0.0.1", 65530 } } }));

    std::this_thread::sleep_for(std::chrono::seconds{10});

    common::xnode_id_t node_id;
    node_id.random();

    auto neighbors = m_dht_hosts[0]->nearest_node_entries(node_id);
    EXPECT_TRUE(neighbors.empty());
}

using xfour_dht_hosts_fixture = xtop_dht_host_fixture<4>;
TEST_F(xfour_dht_hosts_fixture, bootstrap_two_seeds) {
    for (auto const & dht_host : m_dht_hosts) {
        ASSERT_NE(dht_host, nullptr);
    }

    m_dht_hosts[0]->bootstrap({ m_dht_hosts[2]->host_dht_node(), m_dht_hosts[3]->host_dht_node() });
    m_dht_hosts[1]->bootstrap({ m_dht_hosts[2]->host_dht_node(), m_dht_hosts[3]->host_dht_node() });

    std::this_thread::sleep_for(std::chrono::seconds{10});

    auto const neighbors0 = m_dht_hosts[0]->nearest_node_entries(m_dht_hosts[0]->host_node_id());
    auto const neighbors1 = m_dht_hosts[1]->nearest_node_entries(m_dht_hosts[1]->host_node_id());
    auto const neighbors2 = m_dht_hosts[2]->nearest_node_entries(m_dht_hosts[2]->host_node_id());
    auto const neighbors3 = m_dht_hosts[3]->nearest_node_entries(m_dht_hosts[3]->host_node_id());

    EXPECT_EQ(neighbors0.size(), neighbors1.size());
    EXPECT_EQ(neighbors1.size(), neighbors2.size());
    EXPECT_EQ(neighbors2.size(), neighbors3.size());
    EXPECT_EQ(neighbors3.size(), neighbors0.size());

    EXPECT_EQ(m_dht_host_count - 1, neighbors0.size());

    for (auto const & neighbor : neighbors0) {
        EXPECT_TRUE(neighbor->id() == m_dht_hosts[1]->host_node_id() ||
                    neighbor->id() == m_dht_hosts[2]->host_node_id() ||
                    neighbor->id() == m_dht_hosts[3]->host_node_id());
    }

    for (auto const & neighbor : neighbors1) {
        EXPECT_TRUE(neighbor->id() == m_dht_hosts[0]->host_node_id() ||
                    neighbor->id() == m_dht_hosts[2]->host_node_id() ||
                    neighbor->id() == m_dht_hosts[3]->host_node_id());
    }

    for (auto const & neighbor : neighbors2) {
        EXPECT_TRUE(neighbor->id() == m_dht_hosts[0]->host_node_id() ||
                    neighbor->id() == m_dht_hosts[1]->host_node_id() ||
                    neighbor->id() == m_dht_hosts[3]->host_node_id());
    }

    for (auto const & neighbor : neighbors3) {
        EXPECT_TRUE(neighbor->id() == m_dht_hosts[0]->host_node_id() ||
                    neighbor->id() == m_dht_hosts[1]->host_node_id() ||
                    neighbor->id() == m_dht_hosts[2]->host_node_id());
    }
}

using xnine_dht_hosts_fixture = xtop_dht_host_fixture<9>;
TEST_F(xnine_dht_hosts_fixture, bootstrap_seeds_has_intersection) {
    for (auto const & dht_host : m_dht_hosts) {
        ASSERT_NE(dht_host, nullptr);
    }

    std::vector<xdht_node_t> const seeds1
    {
        m_dht_hosts[0]->host_dht_node(),
        m_dht_hosts[1]->host_dht_node(),
        m_dht_hosts[2]->host_dht_node(),
    };

    std::vector<xdht_node_t> const seeds2
    {
        m_dht_hosts[1]->host_dht_node(),
        m_dht_hosts[2]->host_dht_node(),
        m_dht_hosts[3]->host_dht_node(),
    };

    std::vector<xdht_node_t> const seeds3
    {
        m_dht_hosts[3]->host_dht_node(),
        m_dht_hosts[4]->host_dht_node(),
        m_dht_hosts[5]->host_dht_node(),
    };

    m_dht_hosts[6]->bootstrap(seeds1);
    m_dht_hosts[7]->bootstrap(seeds2);
    m_dht_hosts[8]->bootstrap(seeds3);


    std::this_thread::sleep_for(std::chrono::seconds{10});

    std::array<std::vector<std::shared_ptr<p2p::xnode_entry_t>>, m_dht_host_count> neighbors;
    for (auto i = 0u; i < m_dht_host_count; ++i) {
        neighbors[i] = m_dht_hosts[i]->nearest_node_entries(m_dht_hosts[i]->host_node_id());
    }

    for (auto i = 0u; i < m_dht_host_count; ++i) {
        auto const j = i + 1;
        if (j < m_dht_host_count) {
            EXPECT_EQ(neighbors[i].size(), neighbors[j].size());
            EXPECT_EQ(m_dht_host_count - 1, neighbors[i].size());
        }
    }

    for (auto i = 0u; i < m_dht_host_count; ++i) {
        for (auto const & neighbor : neighbors[i]) {
            auto exist = false;

            for (auto j = 0u; j < m_dht_host_count; ++j) {
                if (exist) {
                    break;
                }

                if (i == j) {
                    continue;
                }

                exist = neighbor->id() == m_dht_hosts[j]->host_node_id();
            }
            EXPECT_TRUE(exist);
        }
    }
}

using xeight_dht_hosts_fixture = xtop_dht_host_fixture<8>;
TEST_F(xeight_dht_hosts_fixture, bootstrap_seeds_has_no_intersection) {
    for (auto const & dht_host : m_dht_hosts) {
        ASSERT_NE(dht_host, nullptr);
    }

    std::vector<xdht_node_t> const seeds1
    {
        m_dht_hosts[0]->host_dht_node(),
        m_dht_hosts[1]->host_dht_node(),
        m_dht_hosts[2]->host_dht_node(),
    };

    std::vector<xdht_node_t> const seeds2
    {
        m_dht_hosts[4]->host_dht_node(),
        m_dht_hosts[5]->host_dht_node(),
        m_dht_hosts[6]->host_dht_node(),
    };

    m_dht_hosts[3]->bootstrap(seeds1);
    m_dht_hosts[7]->bootstrap(seeds2);

    std::this_thread::sleep_for(std::chrono::seconds{10});

    std::array<std::vector<std::shared_ptr<p2p::xnode_entry_t>>, m_dht_host_count> neighbors;
    for (auto i = 0u; i < m_dht_host_count; ++i) {
        neighbors[i] = m_dht_hosts[i]->nearest_node_entries(m_dht_hosts[i]->host_node_id());
    }

    constexpr auto middle_index = m_dht_host_count / 2;
    for (auto i = 0u; i < m_dht_host_count; ++i) {
        auto const j = i + 1;
        if (j < m_dht_host_count) {
            EXPECT_EQ(neighbors[i].size(), neighbors[j].size());
            EXPECT_EQ(middle_index - 1, neighbors[i].size());
        }
    }

    for (auto i = 0u; i < m_dht_host_count; ++i) {
        for (auto const & neighbor : neighbors[i]) {
            if (i < middle_index) {
                auto exist = false;
                for (auto j = 0u; j < middle_index; ++j) {
                    if (exist) {
                        break;
                    }

                    if (i == j) {
                        continue;
                    }

                    exist = neighbor->id() == m_dht_hosts[j]->host_node_id();
                }

                EXPECT_TRUE(exist);

                for (auto j = middle_index; j < m_dht_host_count; ++j) {
                    EXPECT_FALSE(neighbor->id() == m_dht_hosts[j]->host_node_id());
                }
            } else {
                for (auto j = 0u; j < middle_index; ++j) {
                    EXPECT_FALSE(neighbor->id() == m_dht_hosts[j]->host_node_id());
                }

                auto exist = false;
                for (auto j = middle_index; j < m_dht_host_count; ++j) {
                    if (exist) {
                        break;
                    }

                    if (i == j) {
                        continue;
                    }

                    exist = neighbor->id() == m_dht_hosts[j]->host_node_id();
                }

                EXPECT_TRUE(exist);
            }
        }
    }
}
