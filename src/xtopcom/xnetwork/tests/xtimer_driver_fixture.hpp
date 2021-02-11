// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/tests/xio_context_fixture.hpp"
#include "xnetwork/tests/xtimer_driver_manager.hpp"

#include <cstddef>
#include <memory>

NS_BEG3(top, network, tests)

template <std::size_t TimerDriverCount, std::size_t IoContextCount>
class xtop_timer_driver_fixture : public xio_context_fixture_t<IoContextCount>
{
    using base_t = xio_context_fixture_t<IoContextCount>;

protected:
    xtimer_driver_manager_t<TimerDriverCount> m_timer_driver_manager;

public:
    // xtop_timer_driver_fixture()                                              = default;
    xtop_timer_driver_fixture(xtop_timer_driver_fixture const &)             = delete;
    xtop_timer_driver_fixture & operator=(xtop_timer_driver_fixture const &) = delete;
    xtop_timer_driver_fixture(xtop_timer_driver_fixture &&)                  = default;
    xtop_timer_driver_fixture & operator=(xtop_timer_driver_fixture &&)      = default;
    ~xtop_timer_driver_fixture() override                                    = default;

    xtop_timer_driver_fixture()
        : m_timer_driver_manager{ static_cast<xobject_manager_t<xasio_io_context_wrapper_t> *>(std::addressof(this->m_io_manager)) }
    {}

protected:
    void
    SetUp() override {
        base_t::SetUp();
        m_timer_driver_manager.start();
    }

    void
    TearDown() override {
        m_timer_driver_manager.stop();
        base_t::TearDown();
    }
};

template <std::size_t TimerDriverCount, std::size_t IoContextCount>
using xtimer_driver_fixture_t = xtop_timer_driver_fixture<TimerDriverCount, IoContextCount>;

NS_END3
