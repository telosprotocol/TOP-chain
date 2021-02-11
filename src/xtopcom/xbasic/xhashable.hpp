// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <cstdint>

NS_BEG1(top)

template <typename T, typename HashT>
class xtop_hashable
{
public:
    xtop_hashable()                                  = default;
    xtop_hashable(xtop_hashable const &)             = default;
    xtop_hashable & operator=(xtop_hashable const &) = default;
    xtop_hashable(xtop_hashable &&)                  = default;
    xtop_hashable & operator=(xtop_hashable &&)      = default;
    virtual ~xtop_hashable()                         = default;

    using hash_result_type = HashT;

    virtual
    hash_result_type
    hash() const = 0;
};

template <typename T, typename HashT = std::uint64_t>
using xhashable_t = xtop_hashable<T, HashT>;

NS_END1
