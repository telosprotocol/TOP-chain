// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include <iosfwd>

#include "xbasic/xbyte.h"
#include "xbase/xns_macro.h"

NS_BEG1(top)

using xbyte_buffer_t = std::vector<xbyte_t>;

xbyte_buffer_t
random_bytes(std::size_t const size);

xbyte_buffer_t
random_base58_bytes(std::size_t const size);

NS_END1
