// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, threading)

class xtop_dummy_mutex {
public:
    constexpr xtop_dummy_mutex() noexcept = default;
    xtop_dummy_mutex(xtop_dummy_mutex const &) = delete;
    xtop_dummy_mutex & operator=(xtop_dummy_mutex const &) = delete;
    ~xtop_dummy_mutex() = default;

    inline  void lock() {
    }

    inline bool try_lock() {
        return true;
    }

    inline void unlock() {
    }
};
using xdummy_mutex_t = xtop_dummy_mutex;

NS_END2
