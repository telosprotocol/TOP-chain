// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xcodec/xdecorators/xfragment_decorator.h"

#include <iterator>

NS_BEG4(top, network, codec, decorators)

xbyte_buffer_t
xtop_fragment_decorator::encode(xbyte_buffer_t const & in,
                                std::size_t const /*fragment_size*/,
                                std::size_t & /*offset*/) {
    return in;
}


NS_END4
