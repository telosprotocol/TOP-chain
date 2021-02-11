// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xthreading/xbackend_thread.hpp"

NS_BEG1(top)

void
xtop_asio_io_context_wrapper::start() {
    assert(m_work_guard == nullptr);

#if !defined NDEBUG
    m_thread_id = std::this_thread::get_id();
#endif

    assert(!running());
    running(true);

    m_work_guard = top::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(m_io_context.get_executor());
    m_io_context.restart();
    m_io_context.run();
}

void
xtop_asio_io_context_wrapper::async_start() {
    auto self = shared_from_this();
    threading::xbackend_thread::spawn([this, self] {
        running(true);
        while (running()) {
            try {
                m_work_guard = top::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(m_io_context.get_executor());
                m_io_context.restart();
                m_io_context.run();
            } catch (std::exception const & eh) {
                xerror("[io context] unhandled exception %s", eh.what());
            }
        }
    });
}

void
xtop_asio_io_context_wrapper::stop() {
    assert(!m_io_context.stopped());

    m_work_guard.reset();
    m_io_context.stop();
#if !defined NDEBUG
    m_thread_id = std::thread::id{};
#endif

    running(false);
}

#if !defined NDEBUG
std::thread::id const &
xtop_asio_io_context_wrapper::thread_id() const noexcept {
    return m_thread_id;
}
#endif

NS_END1
