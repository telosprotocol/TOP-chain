// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver.h"
#include "xnetwork/tests/xtimer_driver_fixture.hpp"
#include "xnetwork/tests/xloopback_utility.h"
#include "xnetwork/xp2p/xdht_host.h"
#include "xnetwork/xudp_socket.h"

#include <array>
#include <memory>

NS_BEG3(top, network, tests)

template <std::size_t DhtHostCount>
class xtop_dht_host_fixture : public xtimer_driver_fixture_t<1, 1>
{
    using base_t = xtimer_driver_fixture_t<1, 1>;

protected:
    std::array<std::shared_ptr<xsocket_face_t>, DhtHostCount> m_sockets{};
    std::array<std::unique_ptr<p2p::xdht_host_face_t>, DhtHostCount> m_dht_hosts{};
    static constexpr std::size_t m_dht_host_count{ DhtHostCount };

public:
    static constexpr std::uint16_t dht_port{ 10000 };
    static constexpr std::uint16_t app_port{ 20000 };

    xtop_dht_host_fixture(xtop_dht_host_fixture const &)             = delete;
    xtop_dht_host_fixture & operator=(xtop_dht_host_fixture const &) = delete;
    xtop_dht_host_fixture(xtop_dht_host_fixture &&)                  = default;
    xtop_dht_host_fixture & operator=(xtop_dht_host_fixture &&)      = default;
    ~xtop_dht_host_fixture() override                                = default;

    xtop_dht_host_fixture() {
        for (auto i = 0u; i < DhtHostCount; ++i) {
            m_sockets[i] = std::make_shared<xudp_socket_t>(m_io_manager.object(0).shared_from_this(),
                                                           loopback_endpoint(i + 2, dht_port));

            common::xnode_id_t random_node_id;
            random_node_id.random();

            m_dht_hosts[i] = top::make_unique<p2p::xdht_host_t>(random_node_id,
                                                                m_sockets[i],
                                                                std::addressof(m_timer_driver_manager.object(0)));
        }
    }

    std::array<std::unique_ptr<p2p::xdht_host_face_t>, DhtHostCount> const &
    dht_hosts() const noexcept {
        return m_dht_hosts;
    }

    std::array<std::unique_ptr<p2p::xdht_host_face_t>, DhtHostCount> &
    dht_hosts() noexcept {
        return m_dht_hosts;
    }

protected:
    void
    SetUp() override {
        base_t::SetUp();

        for (auto i = 0u; i < DhtHostCount; ++i) {
            m_dht_hosts[i]->start();
        }
    }

    void
    TearDown() override {
        for (auto & dht_host : m_dht_hosts) {
            dht_host->stop();
        }
        base_t::TearDown();
    }
};

NS_END3
