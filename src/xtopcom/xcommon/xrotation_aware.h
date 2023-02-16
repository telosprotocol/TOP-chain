// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlog.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xthreading/xutility.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xlogic_time.h"

#include <atomic>
#include <cassert>
#include <exception>
#include <memory>
#include <mutex>

NS_BEG2(top, common)

enum class xenum_rotation_status : std::uint8_t { invalid, started, faded, outdated };
using xrotation_status_t = xenum_rotation_status;

template <typename T>
class xtop_rotation_aware {
public:
    xtop_rotation_aware() = default;
    xtop_rotation_aware(xtop_rotation_aware const &) = delete;
    xtop_rotation_aware & operator=(xtop_rotation_aware const &) = delete;
    xtop_rotation_aware(xtop_rotation_aware &&) = default;
    xtop_rotation_aware & operator=(xtop_rotation_aware &&) = default;
    virtual ~xtop_rotation_aware() = default;

    virtual void rotation_status(xrotation_status_t const status, xlogic_time_t const when) noexcept = 0;

    virtual xrotation_status_t rotation_status(xlogic_time_t const logic_time) const noexcept = 0;

    virtual xlogic_time_t start_time() const noexcept = 0;

    virtual xlogic_time_t fade_time() const noexcept = 0;

    virtual xlogic_time_t outdate_time() const noexcept = 0;
};

template <typename T>
using xrotation_aware_t = xtop_rotation_aware<T>;

template <typename T>
class xtop_basic_rotation_aware : public xrotation_aware_t<T> {
protected:
    std::atomic<xlogic_time_t> m_start_time{xjudgement_day};
    std::atomic<xlogic_time_t> m_fade_time{xjudgement_day};
    std::atomic<xlogic_time_t> m_outdate_time{xjudgement_day};

public:
    xtop_basic_rotation_aware(xtop_basic_rotation_aware const &) = delete;
    xtop_basic_rotation_aware & operator=(xtop_basic_rotation_aware const &) = delete;
    xtop_basic_rotation_aware(xtop_basic_rotation_aware &&) = default;
    xtop_basic_rotation_aware & operator=(xtop_basic_rotation_aware &&) = default;
    ~xtop_basic_rotation_aware() override = default;

#if !defined(XCXX14)
    xtop_basic_rotation_aware() noexcept {
    }
#else
    xtop_basic_rotation_aware() = default;
#endif

    void rotation_status(xrotation_status_t const status, xlogic_time_t const when) noexcept override {
        switch (status) {
        case common::xrotation_status_t::started: {
            m_start_time.store(when, std::memory_order_release);
            break;
        }

        case common::xrotation_status_t::faded: {
            assert(when >= m_start_time.load(std::memory_order_acquire));
            m_fade_time.store(when, std::memory_order_release);
            break;
        }

        case common::xrotation_status_t::outdated: {
            // assert(when >= m_fade_time.load(std::memory_order_relaxed)  &&
            //        when >= m_start_time.load(std::memory_order_relaxed) &&
            //        m_fade_time.load(std::memory_order_relaxed) >= m_start_time.load(std::memory_order_relaxed));
            // comment out the above assert.
            // when node exists and restarts later, it may receive
            // an old data from sync module.
            // last data, pre data => last data, synced data, pre data,
            // the pre data's has already been faded before, but since
            // the synced data arrives, the current time may not exceed the pre fade time.
            // so just ignore this assertion, and this node will be later stopped.
            m_outdate_time.store(when, std::memory_order_release);
            break;
        }

        default: {
            assert(false);
            break;
        }
        }
    }

    xrotation_status_t rotation_status(xlogic_time_t const logic_time) const noexcept override {
        if (m_outdate_time.load(std::memory_order_acquire) <= logic_time) {
            return common::xrotation_status_t::outdated;
        }

        if (m_fade_time.load(std::memory_order_acquire) <= logic_time) {
            return common::xrotation_status_t::faded;
        }

        if (m_start_time.load(std::memory_order_acquire) <= logic_time) {
            return common::xrotation_status_t::started;
        }

        return common::xrotation_status_t::invalid;
    }

    xlogic_time_t start_time() const noexcept override {
        return m_start_time.load(std::memory_order_acquire);
    }

    xlogic_time_t fade_time() const noexcept override {
        return m_fade_time.load(std::memory_order_acquire);
    }

    xlogic_time_t outdate_time() const noexcept override {
        return m_outdate_time.load(std::memory_order_acquire);
    }
};

template <typename T>
using xbasic_rotation_aware_t = xtop_basic_rotation_aware<T>;

NS_END2
