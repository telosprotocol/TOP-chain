// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "tests/xvnetwork/xnetwork_driver_fixture.hpp"
#include "tests/xvnetwork/xvhost_manager.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <random>

XDEFINE_MSG_CATEGORY(xmessage_category_test, 0x0001);
XDEFINE_MSG_ID(xmessage_category_test, xmessage_id_test, 0x00000001);

NS_BEG3(top, vnetwork, tests)

template <std::size_t N, std::size_t TimerDriverCount, std::size_t IoContextCount>
class xtop_vhost_fixture : public xnetwork_driver_fixture_t<N, TimerDriverCount, IoContextCount>
{
    using base_t = xnetwork_driver_fixture_t<N, TimerDriverCount, IoContextCount>;

protected:

    std::array<common::xminer_type_t, N> m_roles{};
    std::unique_ptr<xvhost_manager_t<N>> m_vhost_manager{};
    std::atomic<std::size_t> m_counter{ 0 };

    //std::array<common::xminer_type_t, 4> const predefined_roles{
    //        common::xminer_type_t::edge,
    //        common::xminer_type_t::advance,
    //        common::xminer_type_t::validator,
    //        common::xminer_type_t::archive
    //};

public:
    xtop_vhost_fixture() {
        for (auto i = 0u; i < 2; ++i) {
            m_roles[i] = common::xminer_type_t::edge;
        }

        for (auto i = 2u; i < 3 * N / 8; ++i) {
            m_roles[i] = common::xminer_type_t::advance;
        }

        for (auto i = 3 * N / 8; i < 5 * N / 8; ++i) {
            m_roles[i] = common::xminer_type_t::archive;
        }

        for (auto i = 5 * N / 8; i < N; ++i) {
            m_roles[i] = common::xminer_type_t::validator;
        }

        m_vhost_manager = top::make_unique<xvhost_manager_t<N>>(std::addressof(this->m_io_manager),
                                                                std::addressof(this->m_timer_driver_manager),
                                                                std::addressof(this->m_network_driver_manager),
                                                                m_roles,
                                                                top::common::xtestnet_id);
    }
protected:

    void
    SetUp() override {
        base_t::SetUp();

        m_counter = 0;
        m_vhost_manager->start();
    }

    void
    TearDown() override {
        m_vhost_manager->stop();
        base_t::TearDown();
    }

    void
    reset_vhost_manager(std::array<common::xminer_type_t, N> const & roles) {
        m_vhost_manager->stop();
        m_roles = roles;
        m_vhost_manager.reset();

        m_vhost_manager = top::make_unique<xvhost_manager_t<N>>(std::addressof(this->m_io_manager),
                                                                std::addressof(this->m_timer_driver_manager),
                                                                std::addressof(this->m_network_driver_manager),
                                                                m_roles,
                                                                common::xnetwork_id_t{ 0 });
        m_vhost_manager->start();
    }
};

template <std::size_t N, std::size_t TimerDriverCount, std::size_t IoContextCount>
using xvhost_fixture_t = xtop_vhost_fixture<N, TimerDriverCount, IoContextCount>;

NS_END3
