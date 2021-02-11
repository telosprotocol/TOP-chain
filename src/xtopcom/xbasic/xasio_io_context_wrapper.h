// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xthread.h"
#include "xbasic/xasio_config.h"
#include "xbasic/xobject_ptr.h"
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

/**
 * @brief A simple wrapper on base::xiothread_t.  The start / stop / running are fake.  Basically,
 *        it's not correct to call start after calling stop, since this class is a wrapper of
 *        thread object.  The thread object cannot be restarted after it is closed.
 *        This wrapper is just to make the code in a uniform style when introducing the libxbase.
 */
template <typename IoThreadT, typename std::enable_if<std::is_base_of<base::xiothread_t, IoThreadT>::value>::type * = nullptr>
class xtop_base_iothread_wrapper final : public xrunnable_t<xtop_base_iothread_wrapper<IoThreadT>> {
private:
    xobject_ptr_t<IoThreadT> m_iothread_ptr{};

public:
    xtop_base_iothread_wrapper(xtop_base_iothread_wrapper const &) = delete;
    xtop_base_iothread_wrapper & operator=(xtop_base_iothread_wrapper const &) = delete;
    xtop_base_iothread_wrapper(xtop_base_iothread_wrapper &&) = default;
    xtop_base_iothread_wrapper & operator=(xtop_base_iothread_wrapper &&) = default;
    ~xtop_base_iothread_wrapper() = default;

    xtop_base_iothread_wrapper() {
        m_iothread_ptr.attach(base::xiothread_t::create_thread(base::xcontext_t::instance(), 0, -1));
    }

    void start() override {
    }

    void stop() override {
    }

    bool running() const noexcept override {
        return m_iothread_ptr->is_running();
    }

    void running(bool const) noexcept override {
        // do nothing
    }

    base::xcontext_t & context() const noexcept {
        return *m_iothread_ptr->get_context();
    }

    std::int32_t thread_id() const noexcept {
        return m_iothread_ptr->get_thread_id();
    }
};
using xbase_iothread_wrapper_t = xtop_base_iothread_wrapper<base::xiothread_t>;

NS_END1
