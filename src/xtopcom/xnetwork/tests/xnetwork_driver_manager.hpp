// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xnetwork/tests/xloopback_utility.h"
#include "xnetwork/tests/xtimer_driver_manager.hpp"
#include "xnetwork/xnetwork_driver.h"
#include "xnetwork/xp2p/xdht_host.h"
#include "xnetwork/xudp_socket.h"

#include <array>
#include <memory>
#include <string>

NS_BEG3(top, network, tests)

template <std::size_t N, typename AppSocketT, typename DhtSocketT>
class xtop_network_driver_manager final : public xobject_manager_t<xnetwork_driver_face_t>
{

public:
    static constexpr std::uint16_t dht_port{ 50000 };
    static constexpr std::uint16_t app_port{ 60000 };
    static constexpr std::size_t size{ N };

private:
    observer_ptr<xobject_manager_t<xasio_io_context_wrapper_t>> m_io_manager;
    observer_ptr<xobject_manager_t<xtimer_driver_t>> m_timer_driver_manager;
    std::array<std::shared_ptr<xnetwork_driver_face_t>, N> m_net_drivers{};

public:
    xtop_network_driver_manager(xtop_network_driver_manager const &)             = delete;
    xtop_network_driver_manager & operator=(xtop_network_driver_manager const &) = delete;
    xtop_network_driver_manager(xtop_network_driver_manager &&)                  = default;
    xtop_network_driver_manager & operator=(xtop_network_driver_manager &&)      = default;
    ~xtop_network_driver_manager()                                               = default;

    explicit
    xtop_network_driver_manager(xobject_manager_t<xasio_io_context_wrapper_t> * io_manager,
                                xobject_manager_t<xtimer_driver_t> * timer_driver_manager)
        : m_io_manager{ make_observer(io_manager) }
        , m_timer_driver_manager{ make_observer(timer_driver_manager) }
    {
        for (auto i = 0u; i < N; ++i) {
            auto const io_object_index = i % m_io_manager->object_count();
            auto const timer_driver_index = i % m_timer_driver_manager->object_count();

            auto & io_object = m_io_manager->object(io_object_index);
            auto & timer_driver = m_timer_driver_manager->object(timer_driver_index);

            auto dht_socket = std::make_shared<DhtSocketT>(io_object.shared_from_this(), loopback_endpoint(i + 2, dht_port));
            auto app_socket = std::make_shared<AppSocketT>(io_object.shared_from_this(), loopback_endpoint(i + 2, app_port));

            // xnode_id_t node_id;
            // node_id.random();

            common::xnode_id_t const id{ std::to_string(app_port + i) };

            m_net_drivers[i] = std::make_shared<xnetwork_driver_t>(timer_driver.shared_from_this(),
                                                                   top::make_unique<p2p::xdht_host_t>(id,
                                                                                                      std::move(dht_socket),
                                                                                                      make_observer(std::addressof(timer_driver))),
                                                                   std::move(app_socket));
        }
    }

    void
    start() override {
        for (std::uint16_t i = 0u; i < N; ++i) {
            m_net_drivers[i]->start();
        }
    }

    void
    stop() override {
        for (auto i = 0u; i < N; ++i) {
            m_net_drivers[i]->stop();
        }
    }

    std::size_t
    object_count() const noexcept override {
        return N;
    }

    xtop_network_driver_face &
    object(std::size_t const index) override {
        return *m_net_drivers.at(index);
    }

    xtop_network_driver_face const &
    object(std::size_t const index) const override {
        return *m_net_drivers.at(index);
    }
};

template <std::size_t N, typename AppSocketT = xudp_socket_t, typename DhtSocketT = xudp_socket_t>
using xnetwork_driver_manager_t = xtop_network_driver_manager<N, AppSocketT, DhtSocketT>;

NS_END3
