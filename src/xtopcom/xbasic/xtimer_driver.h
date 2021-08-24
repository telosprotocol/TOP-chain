// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbase/xobject_ptr.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"
#include "xbasic/xtimer.h"
#include "xbasic/xtimer_driver_fwd.h"

#include <functional>
#include <memory>
#include <mutex>

NS_BEG1(top)

class xtop_timer_driver final
  : public std::enable_shared_from_this<xtop_timer_driver>
  , public xbasic_runnable_t<xtop_timer_driver> {
private:
    using runnable_base_type = xbasic_runnable_t<xtop_timer_driver>;

    std::mutex m_timers_mutex{};
    std::vector<std::shared_ptr<top::xtimer_t>> m_timers{};

    std::chrono::milliseconds m_reap_interval;
    std::weak_ptr<xasio_io_context_wrapper_t> m_io_object;

public:
    xtop_timer_driver(xtop_timer_driver const &) = delete;
    xtop_timer_driver & operator=(xtop_timer_driver const &) = delete;
    xtop_timer_driver(xtop_timer_driver &&) = default;
    xtop_timer_driver & operator=(xtop_timer_driver &&) = default;
    ~xtop_timer_driver() = default;

    explicit xtop_timer_driver(std::shared_ptr<xasio_io_context_wrapper_t> io_object, std::chrono::milliseconds reap_interval_ms = std::chrono::milliseconds{100});

    void start() override;

    void stop() override;

    void schedule(std::chrono::milliseconds const & ms_in_future, top::xtimer_t::timeout_callback_t callback);

private:
    void do_reap();
};

class xtop_base_timer_driver final
  : public std::enable_shared_from_this<xtop_base_timer_driver>
  , public xbasic_runnable_t<xtop_base_timer_driver> {
private:
    using runnable_base_type = xbasic_runnable_t<xtop_base_timer_driver>;

    std::mutex m_timers_mutex{};
    std::vector<std::unique_ptr<top::xbase_timer_t>> m_timers{};

    std::chrono::milliseconds m_reap_interval;
    observer_ptr<xbase_io_context_wrapper_t> m_io_object;

public:
    xtop_base_timer_driver(xtop_base_timer_driver const &) = delete;
    xtop_base_timer_driver & operator=(xtop_base_timer_driver const &) = delete;
    xtop_base_timer_driver(xtop_base_timer_driver &&) = default;
    xtop_base_timer_driver & operator=(xtop_base_timer_driver &&) = default;
    ~xtop_base_timer_driver() = default;

    explicit xtop_base_timer_driver(std::shared_ptr<xbase_io_context_wrapper_t> const & io_object, std::chrono::milliseconds reap_interval_ms = std::chrono::minutes{2});

    void start() override;

    void stop() override;

    void schedule(std::chrono::milliseconds const & ms_in_future, top::xbase_timer_t::timeout_callback_t callback);

private:
    void do_reap();
};
using xbase_timer_driver_t = xtop_base_timer_driver;

NS_END1
