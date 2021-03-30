// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <atomic>

NS_BEG1(top)

/**
 * @brief Defines basic interface for a runnable object.
 *
 * @tparam T Object type that enables the runnable trait.
 */
template <typename T>
class xtop_runnable {
public:
    xtop_runnable() = default;
    xtop_runnable(xtop_runnable const &) = delete;
    xtop_runnable & operator=(xtop_runnable const &) = delete;
    xtop_runnable(xtop_runnable &&) = default;
    xtop_runnable & operator=(xtop_runnable &&) = default;
    virtual ~xtop_runnable() = default;

    virtual void start() = 0;

    virtual void stop() = 0;

    virtual bool running() const noexcept = 0;

    virtual void running(bool const value) noexcept = 0;
};

template <typename T>
using xrunnable_t = xtop_runnable<T>;

/**
 * @brief Implement runnable interface by enabling atomic running status change.
 *
 * @tparam T Object type that enables the runnable trait.
 */
template <typename T>
class xtop_basic_runnable : public xrunnable_t<T> {
    std::atomic<bool> m_running{false};

public:
    xtop_basic_runnable(xtop_basic_runnable const &) = default;
    xtop_basic_runnable & operator=(xtop_basic_runnable const &) = default;
    xtop_basic_runnable(xtop_basic_runnable &&) = default;
    xtop_basic_runnable & operator=(xtop_basic_runnable &&) = default;
    ~xtop_basic_runnable() = default;

#if !defined(XCXX14_OR_ABOVE)
    xtop_basic_runnable() noexcept {
    }
#else
    xtop_basic_runnable() = default;
#endif

    bool running() const noexcept override {
        return m_running.load(std::memory_order_acquire);
    }

    void running(bool const value) noexcept override {
        m_running.store(value, std::memory_order_release);
    }
};

template <typename T>
using xbasic_runnable_t = xtop_basic_runnable<T>;

template <typename T>
class xtop_trival_runnable : public xrunnable_t<T> {
public:
    xtop_trival_runnable(xtop_trival_runnable const &) = default;
    xtop_trival_runnable & operator=(xtop_trival_runnable const &) = default;
    xtop_trival_runnable(xtop_trival_runnable &&) = default;
    xtop_trival_runnable & operator=(xtop_trival_runnable &&) = default;
    ~xtop_trival_runnable() = default;

#if !defined XCXX14_OR_ABOVE
    xtop_trival_runnable() noexcept {
    }
#else
    xtop_trival_runnable() = default;
#endif
    bool running() const noexcept override {
        return true;
    }

    void running(bool const) noexcept override {
    }
};

template <typename T>
using xtrival_runnable_t = xtop_trival_runnable<T>;

NS_END1
