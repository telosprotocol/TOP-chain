// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xtimer_driver.h"

#include "xbase/xlog.h"

#include <cassert>

NS_BEG1(top)

xtop_timer_driver::xtop_timer_driver(std::shared_ptr<xasio_io_context_wrapper_t> io_object, std::chrono::milliseconds reap_interval_ms)
  : m_reap_interval{std::move(reap_interval_ms)}, m_io_object{io_object} {
    assert(io_object);
}

void xtop_timer_driver::start() {
    assert(!running());

    running(true);

    do_reap();
}

void xtop_timer_driver::stop() {
    assert(running());
    running(false);
}

void xtop_timer_driver::schedule(std::chrono::milliseconds const & ms_in_future, top::xtimer_t::timeout_callback_t callback) {
    if (!running()) {
        xwarn("[timer driver] timer driver not run");
        return;
    }

    std::lock_guard<std::mutex> lock{m_timers_mutex};
    auto io_object = m_io_object.lock();
    if (io_object) {
        m_timers.push_back(std::make_shared<top::xtimer_t>(io_object, ms_in_future, std::move(callback)));
    }
}

void xtop_timer_driver::do_reap() {
    if (!running()) {
        xwarn("[timer driver] timer driver not run");
        return;
    }

    {
        std::lock_guard<std::mutex> lock{m_timers_mutex};

        auto it = std::begin(m_timers);
        while (it != std::end(m_timers)) {
            if ((*it)->expired()) {
                (*it)->wait();
                it = m_timers.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (!running()) {
        xwarn("[timer driver] timer driver not run");
        return;
    }

    auto io_object = m_io_object.lock();
    if (io_object == nullptr) {
        return;
    }

    auto self = shared_from_this();
    m_timers.push_back(std::make_shared<top::xtimer_t>(io_object, m_reap_interval, [this, self](asio::error_code const & ec) {
        if (ec && ec == asio::error::operation_aborted) {
            xwarn("[timer driver] timer driver cancelled");
            return;
        }

        if (!running()) {
            xwarn("[timer driver] timer driver not run");
            return;
        }

        do_reap();
    }));
}

xtop_base_timer_driver::xtop_base_timer_driver(std::shared_ptr<xbase_io_context_wrapper_t> const & io_object, std::chrono::milliseconds reap_interval_ms) : m_reap_interval {reap_interval_ms}, m_io_object {make_observer(io_object.get())} {
}

void xtop_base_timer_driver::start() {
    assert(!running());
    running(true);
    do_reap();
}

void xtop_base_timer_driver::stop() {
    assert(running());
    running(false);
}

void xtop_base_timer_driver::schedule(std::chrono::milliseconds const & ms_in_future, top::xbase_timer_t::timeout_callback_t callback) {
    if (!running()) {
        xwarn("[xbase timer driver] timer driver not run");
        return;
    }

    assert(m_io_object != nullptr);
    std::lock_guard<std::mutex> lock{m_timers_mutex};
    m_timers.push_back(top::make_unique<top::xbase_timer_t>(m_io_object, ms_in_future, std::move(callback)));
}

void xtop_base_timer_driver::do_reap() {
    if (!running()) {
        xwarn("[xbase timer driver] timer driver not run");
        return;
    }

    {
        std::lock_guard<std::mutex> lock{m_timers_mutex};

        auto it = std::begin(m_timers);
        while (it != std::end(m_timers)) {
            if ((*it)->expired()) {
                it = m_timers.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (!running()) {
        xwarn("[xbase timer driver] timer driver not run");
        return;
    }
    auto self = shared_from_this();
    m_timers.push_back(top::make_unique<top::xbase_timer_t>(m_io_object, m_reap_interval, [this, self](std::chrono::milliseconds) -> bool {
        if (!running()) {
            xwarn("[xbase timer driver] timer driver not run");
            return false;
        }

        do_reap();
        return true;
    }));
}


NS_END1
