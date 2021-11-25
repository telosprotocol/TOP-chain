// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer.h"
#include "xnetwork/tests/xnetwork_driver_manager.hpp"
#include "xvnetwork/xvhost.h"
#include "tests/xvnetwork/xdummy_election_cache_data_accessor.h"
#include "xchain_timer/xchain_timer.h"

#include <array>
#include <cassert>
#include <memory>

NS_BEG3(top, vnetwork, tests)

template <std::size_t N>
class xtop_vhost_manager final : public network::tests::xobject_manager_t<xvhost_face_t>
{
private:
    observer_ptr<network::tests::xobject_manager_t<xasio_io_context_wrapper_t>> m_io_manager;
    observer_ptr<network::tests::xobject_manager_t<xtimer_driver_t>> m_timer_driver_manager;
    observer_ptr<network::tests::xobject_manager_t<network::xnetwork_driver_face_t>> m_network_driver_manager;
    std::array<std::shared_ptr<xvhost_face_t>, N> m_vhosts;

    std::shared_ptr<top::xbase_io_context_wrapper_t> m_io_object{std::make_shared<top::xbase_io_context_wrapper_t>()};
    std::shared_ptr<top::xbase_timer_driver_t> m_timer_driver{std::make_shared<top::xbase_timer_driver_t>(m_io_object)};
    top::xobject_ptr_t<time::xchain_time_face_t> m_chain_timer{ top::make_object_ptr<time::xchain_timer_t>(m_timer_driver) };

    xdummy_election_data_accessor_t m_election_data_accessor;

public:
    xtop_vhost_manager(xtop_vhost_manager const &)             = delete;
    xtop_vhost_manager & operator=(xtop_vhost_manager const &) = delete;
    xtop_vhost_manager(xtop_vhost_manager &&)                  = default;
    xtop_vhost_manager & operator=(xtop_vhost_manager &&)      = default;
    ~xtop_vhost_manager()                                      = default;

    xtop_vhost_manager(network::tests::xobject_manager_t<xasio_io_context_wrapper_t> * io_manager,
                       network::tests::xobject_manager_t<xtimer_driver_t> * timer_driver_manager,
                       network::tests::xobject_manager_t<network::xnetwork_driver_face_t> * network_driver_manager,
                       std::array<common::xminer_type_t, N> const & roles,
                       common::xnetwork_id_t const & network_id)
        : m_io_manager{ make_observer(io_manager) }
        , m_timer_driver_manager{ make_observer(timer_driver_manager) }
        , m_network_driver_manager{ make_observer(network_driver_manager) }
        , m_election_data_accessor{ network_id }
    {
        assert(m_io_manager);
        assert(m_timer_driver_manager);
        assert(m_network_driver_manager);

        for (auto i = 0u; i < N; ++i) {
            auto const network_driver_index = i % m_network_driver_manager->object_count();
            auto const timer_driver_index = i % m_timer_driver_manager->object_count();

            m_vhosts[i] = std::make_shared<xvhost_t>(make_observer(std::addressof(network_driver_manager->object(network_driver_index))),
                                                     make_observer(m_chain_timer),
                                                     network_id,
                                                     make_observer(&m_election_data_accessor));
        }
    }

    void
    start() override {
        for (auto i = 0u; i < N; ++i) {
            m_vhosts[i]->start();
        }
    }

    void
    stop() override {
        for (auto i = 0u; i < N; ++i) {
            m_vhosts[i]->stop();
        }
    }

    std::size_t
    object_count() const noexcept override {
        return N;
    }

    xvhost_face_t &
    object(std::size_t const index) override {
        return *m_vhosts[index];
    }

    xvhost_face_t const &
    object(std::size_t const index) const override {
        return *m_vhosts[index];
    }
};

template <std::size_t N>
using xvhost_manager_t = xtop_vhost_manager<N>;

NS_END3
