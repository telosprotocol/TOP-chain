// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, common)

template <typename T>
class xtop_enable_synchronization {
public:
    xtop_enable_synchronization()                                                = default;
    xtop_enable_synchronization(xtop_enable_synchronization const &)             = delete;
    xtop_enable_synchronization & operator=(xtop_enable_synchronization const &) = delete;
    xtop_enable_synchronization(xtop_enable_synchronization &&)                  = default;
    xtop_enable_synchronization & operator=(xtop_enable_synchronization &&)      = default;
    virtual ~xtop_enable_synchronization()                                       = default;

    virtual
    void
    synchronize() = 0;
};

template <typename T>
using xenable_synchronization_t = xtop_enable_synchronization<T>;
NS_END2
