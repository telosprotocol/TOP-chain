// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/tests/xnetwork_driver_manager.hpp"
#include "xnetwork/tests/xtimer_driver_fixture.hpp"

#include <cstddef>

NS_BEG3(top, vnetwork, tests)

template <std::size_t NetworkDriverCount, std::size_t TimerDriverCount, std::size_t IoContextCount>
class xtop_network_driver_fixture : public network::tests::xtimer_driver_fixture_t<TimerDriverCount, IoContextCount>
{
    using base_t = network::tests::xtimer_driver_fixture_t<TimerDriverCount, IoContextCount>;
protected:
    network::tests::xnetwork_driver_manager_t<NetworkDriverCount> m_network_driver_manager
    {
        static_cast<network::tests::xobject_manager_t<xasio_io_context_wrapper_t> *>(std::addressof(this->m_io_manager)),
        static_cast<network::tests::xobject_manager_t<xtimer_driver_t> *>(std::addressof(this->m_timer_driver_manager))
    };

public:
    xtop_network_driver_fixture()                                                = default;
    xtop_network_driver_fixture(xtop_network_driver_fixture const &)             = delete;
    xtop_network_driver_fixture & operator=(xtop_network_driver_fixture const &) = delete;
    xtop_network_driver_fixture(xtop_network_driver_fixture &&)                  = default;
    xtop_network_driver_fixture & operator=(xtop_network_driver_fixture &&)      = default;
    ~xtop_network_driver_fixture() override                                      = default;

protected:
    void
    SetUp() override {
        base_t::SetUp();
        m_network_driver_manager.start();
    }

    void
    TearDown() override {
        m_network_driver_manager.stop();
        base_t::TearDown();
    }
};

template <std::size_t NetworkDriverCount, std::size_t TimerDriverCount, std::size_t IoContextCount>
using xnetwork_driver_fixture_t = xtop_network_driver_fixture<NetworkDriverCount, TimerDriverCount, IoContextCount>;

NS_END3
