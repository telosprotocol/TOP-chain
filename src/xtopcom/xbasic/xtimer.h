// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xtimer.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xobject_ptr.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"
#include "xbasic/xtimer_fwd.h"

#include <asio/steady_timer.hpp>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <system_error>

NS_BEG1(top)

class xtop_timer final : public std::enable_shared_from_this<xtop_timer> {
public:
    using timeout_callback_t = std::function<void(std::error_code const &)>;

private:
    mutable std::mutex m_timer_mutex{};
    asio::steady_timer m_timer;

public:
    xtop_timer(xtop_timer const &) = delete;
    xtop_timer operator=(xtop_timer const &) = delete;
    xtop_timer(xtop_timer && other) = delete;
    xtop_timer & operator=(xtop_timer && other) = delete;
    ~xtop_timer() noexcept;

    xtop_timer(std::shared_ptr<xasio_io_context_wrapper_t> io_object, std::chrono::milliseconds const & ms_in_future, timeout_callback_t callback);

    bool expired() const noexcept;

    void wait();
};

class xtop_base_timer_wrapper final
  : public top::base::xxtimer_t
  , public xbasic_runnable_t<xtop_base_timer_wrapper> {

public:
    using timeout_callback_t = std::function<void(std::chrono::milliseconds)>;

private:
    using base_t = top::base::xxtimer_t;
    timeout_callback_t m_callback;
    std::chrono::milliseconds m_timeout;

public:
    xtop_base_timer_wrapper(xtop_base_timer_wrapper const &) = delete;
    xtop_base_timer_wrapper & operator=(xtop_base_timer_wrapper const &) = delete;
    xtop_base_timer_wrapper(xtop_base_timer_wrapper &&) = default;
    xtop_base_timer_wrapper & operator=(xtop_base_timer_wrapper &&) = default;

    explicit xtop_base_timer_wrapper(observer_ptr<xbase_io_context_wrapper_t> io_context, std::chrono::milliseconds ms_in_future, timeout_callback_t callback);

    bool expired() const noexcept;

    void start() override;

    void stop() override;

protected:
    ~xtop_base_timer_wrapper() override = default;

private:
    bool on_timer_fire(int32_t const thread_id,
                       int64_t const timer_id,
                       int64_t const current_time_ms,
                       int32_t const start_timeout_ms,
                       int32_t & /*in_out_cur_interval_ms*/) override;
};
using xbase_timer_wrapper_t = xtop_base_timer_wrapper;

class xtop_base_timer final /*: public std::enable_shared_from_this<xtop_base_timer>*/ {
public:
    using timeout_callback_t = xbase_timer_wrapper_t::timeout_callback_t;

private:
    mutable std::mutex m_timer_mutex{};
    xobject_ptr_t<xbase_timer_wrapper_t> m_timer;

public:
    xtop_base_timer(xtop_base_timer const &) = delete;
    xtop_base_timer operator=(xtop_base_timer const &) = delete;
    xtop_base_timer(xtop_base_timer &&) = delete;
    xtop_base_timer & operator=(xtop_base_timer &&) = delete;
    ~xtop_base_timer() noexcept;

    xtop_base_timer(observer_ptr<xbase_io_context_wrapper_t> io_object, std::chrono::milliseconds ms_in_future, timeout_callback_t callback);

    bool expired() const noexcept;

    void wait();
};

using xbase_timer_t = xtop_base_timer;

NS_END1
