// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xthread.h"
#include "xbasic/xasio_config.h"
#include "xbase/xobject_ptr.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>

#include <memory>
#include <thread>

NS_BEG1(top)

class xtop_asio_io_context_wrapper final
  : public std::enable_shared_from_this<xtop_asio_io_context_wrapper>
  , public xbasic_runnable_t<xtop_asio_io_context_wrapper> {
    asio::io_context m_io_context{};
    std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_work_guard{};
#if !defined NDEBUG
    std::thread::id m_thread_id{};
#endif

    using runnable_base_type = xbasic_runnable_t<xtop_asio_io_context_wrapper>;

public:
    void start() override;

    void async_start();

    void stop() override;

    template <typename IoObjectT>
    IoObjectT create() {
        return IoObjectT{m_io_context};
    }

    template <typename TimerT>
    TimerT create_timer() {
        return TimerT{m_io_context};
    }

    template <typename TimerT>
    TimerT create_timer(typename TimerT::time_point const & tp) {
        return TimerT{m_io_context, tp};
    }

    template <typename TimerT, typename RepT, typename PeriodT>
    TimerT create_timer(std::chrono::duration<RepT, PeriodT> const & duration) {
        return TimerT{m_io_context, duration};
    }

#if !defined NDEBUG
    std::thread::id const & thread_id() const noexcept;
#endif
};
using xasio_io_context_wrapper_t = xtop_asio_io_context_wrapper;

/// @brief A simple wrapper on base::xiothread_t.  The start / stop / running are fake. Basically,
///        it's wrong to restart the io context by calling start() after stop(), since xbase's
///        thead doesn't support restart. The thread object cannot be restarted after it is closed.
///        This wrapper is just to make the code in a uniform style (like ASIO) when introducing
///        the libxbase.
class xtop_base_io_context_wrapper final : public xrunnable_t<xtop_base_io_context_wrapper> {
private:
    xobject_ptr_t<base::xiothread_t> m_iothread_ptr{};

public:
    xtop_base_io_context_wrapper(xtop_base_io_context_wrapper const &) = delete;
    xtop_base_io_context_wrapper & operator=(xtop_base_io_context_wrapper const &) = delete;
    xtop_base_io_context_wrapper(xtop_base_io_context_wrapper &&) = default;
    xtop_base_io_context_wrapper & operator=(xtop_base_io_context_wrapper &&) = default;
    ~xtop_base_io_context_wrapper() = default;

    xtop_base_io_context_wrapper();
    explicit xtop_base_io_context_wrapper(xobject_ptr_t<base::xiothread_t> io_thread);

    void start() override;
    void stop() override;
    bool running() const noexcept override;
    void running(bool const) noexcept override;

    base::xcontext_t & context() const noexcept;
    std::int32_t thread_id() const noexcept;
};
using xbase_io_context_wrapper_t = xtop_base_io_context_wrapper;

NS_END1
