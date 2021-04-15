// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, common)

template <typename T>
class xtop_fadable {
public:
    xtop_fadable() = default;
    xtop_fadable(xtop_fadable const &) = delete;
    xtop_fadable & operator=(xtop_fadable const &) = delete;
    xtop_fadable(xtop_fadable &&) = default;
    xtop_fadable & operator=(xtop_fadable &&) = default;
    virtual ~xtop_fadable() = default;

    virtual void fade() = 0;

    virtual bool faded() const noexcept = 0;
};

template <typename T>
using xfadable_t = xtop_fadable<T>;

template <typename T>
class xtop_basic_fadable : public xfadable_t<T> {
protected:
    std::atomic<bool> m_faded;

public:
    xtop_basic_fadable(xtop_basic_fadable const &) = delete;
    xtop_basic_fadable & operator=(xtop_basic_fadable const &) = delete;
    xtop_basic_fadable(xtop_basic_fadable &&) = default;
    xtop_basic_fadable & operator=(xtop_basic_fadable &&) = default;
    ~xtop_basic_fadable() override = default;

    xtop_basic_fadable() noexcept : m_faded{false} {
    }

    bool faded() const noexcept override {
        return m_faded.load(std::memory_order_acquire);
    }
};

template <typename T>
using xbasic_fadable_t = xtop_basic_fadable<T>;

NS_END2
