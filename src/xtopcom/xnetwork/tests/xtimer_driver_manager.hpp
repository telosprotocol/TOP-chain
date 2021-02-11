// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver.h"
#include "xnetwork/tests/xio_context_manager.hpp"

#include <array>
#include <cstddef>
#include <memory>

NS_BEG3(top, network, tests)

template <std::size_t TimerDriverCount>
class xtop_timer_driver_manager final : public xobject_manager_t<xtimer_driver_t>
{
protected:
    observer_ptr<xobject_manager_t<xasio_io_context_wrapper_t>> m_io_manager;
    std::array<std::shared_ptr<xtimer_driver_t>, TimerDriverCount> m_timer_drivers{};

public:
    xtop_timer_driver_manager(xtop_timer_driver_manager const &)             = delete;
    xtop_timer_driver_manager & operator=(xtop_timer_driver_manager const &) = delete;
    xtop_timer_driver_manager(xtop_timer_driver_manager &&)                  = default;
    xtop_timer_driver_manager & operator=(xtop_timer_driver_manager &&)      = default;
    ~xtop_timer_driver_manager() override                                    = default;

    explicit
    xtop_timer_driver_manager(xobject_manager_t<xasio_io_context_wrapper_t> * io_manager)
        : m_io_manager{ make_observer(io_manager) }
    {
        assert(m_io_manager);

        for (auto i = 0u; i < TimerDriverCount; ++i) {
            m_timer_drivers[i] = std::make_shared<xtimer_driver_t>(io_manager->object(i % io_manager->object_count()).shared_from_this());
        }
    }

    void
    start() override {
        for (auto i = 0u; i < TimerDriverCount; ++i) {
            m_timer_drivers[i]->start();
        }
    }

    void
    stop() override {
        for (auto i = 0u; i < TimerDriverCount; ++i) {
            m_timer_drivers[i]->stop();
        }
    }

    std::size_t
    object_count() const noexcept override {
        return TimerDriverCount;
    }

    xtimer_driver_t &
    object(std::size_t const index) override {
        return *m_timer_drivers.at(index);
    }

    xtimer_driver_t const &
    object(std::size_t const index) const override {
        return *m_timer_drivers.at(index);
    }
};

template <std::size_t TimerDriverCount>
using xtimer_driver_manager_t = xtop_timer_driver_manager<TimerDriverCount>;

NS_END3
