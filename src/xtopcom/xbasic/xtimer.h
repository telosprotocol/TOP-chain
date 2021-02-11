// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xtimer.h"
#include "xbasic/xasio_io_context_wrapper.h"
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

class xtop_timer final : public std::enable_shared_from_this<xtop_timer>
{
public:
    using timeout_callback_t = std::function<void(std::error_code const &)>;

private:
    mutable std::mutex m_timer_mutex{};
    asio::steady_timer m_timer;

public:
    xtop_timer(xtop_timer const &)              = delete;
    xtop_timer operator=(xtop_timer const &)    = delete;
    xtop_timer(xtop_timer && other)             = delete;
    xtop_timer & operator=(xtop_timer && other) = delete;
    ~xtop_timer() noexcept;

    xtop_timer(std::shared_ptr<xasio_io_context_wrapper_t> io_object,
               std::chrono::milliseconds const & ms_in_future,
               timeout_callback_t callback);

    bool
    expired() const noexcept;

    void
    wait();
};

template <typename TimerImpT>
class xtop_timer2 final : private TimerImpT
                       , public xbasic_runnable_t<xtop_timer2<TimerImpT>>
                       , public std::enable_shared_from_this<xtop_timer2<TimerImpT>>
{
    XSTATIC_ASSERT((std::is_base_of<top::base::xxtimer_t, TimerImpT>::value));

public:
    using timeout_callback_t = std::function<void()>;

private:
    using base_t = TimerImpT;
    timeout_callback_t m_callback;
    std::chrono::milliseconds m_timeout;

public:
    xtop_timer2(xtop_timer2 const &)             = delete;
    xtop_timer2 & operator=(xtop_timer2 const &) = delete;
    xtop_timer2(xtop_timer2 &&)                  = default;
    xtop_timer2 & operator=(xtop_timer2 &&)      = default;

    xtop_timer2(base::xcontext_t & context,
                std::int32_t const thread_id,
                std::chrono::milliseconds const & ms_in_future,
                timeout_callback_t callback)
        : base_t(context, thread_id), m_callback{ std::move(callback) }, m_timeout{ ms_in_future } {
    }

    bool
    expired() const noexcept {
        return !const_cast<xtop_timer2 *>(this)->is_active();
    }

    void
    start() override {
        assert(!this->running());
        this->running(true);
        base_t::start(m_timeout.count(), 0);
    }

    void
    stop() override {
        base_t::stop();
        assert(this->running());
        this->running(false);
    }

protected:
    ~xtop_timer2() override = default;

private:
    bool
    on_timer_fire(int32_t const, int64_t const, int64_t const, int32_t const, int32_t &) override {
        assert(m_callback);
        m_callback();
        return true;
    }
};

template <typename TimerImpT>
using xtimer2_t = xtop_timer2<TimerImpT>;

NS_END1
