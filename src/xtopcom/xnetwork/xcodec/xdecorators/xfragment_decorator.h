// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstddef>

#include "xbasic/xbyte_buffer.h"
#include "xbase/xns_macro.h"

NS_BEG4(top, network, codec, decorators)

struct xtop_fragment_decorator final
{
    xtop_fragment_decorator()                                            = delete;
    xtop_fragment_decorator(xtop_fragment_decorator const &)             = delete;
    xtop_fragment_decorator & operator=(xtop_fragment_decorator const &) = delete;
    xtop_fragment_decorator(xtop_fragment_decorator &&)                  = delete;
    xtop_fragment_decorator & operator=(xtop_fragment_decorator &&)      = delete;
    ~xtop_fragment_decorator()                                           = delete;

    static
    xbyte_buffer_t
    encode(xbyte_buffer_t const & in, std::size_t const fragment_size, std::size_t & offset);
};

NS_END4
