// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>

NS_BEG1(top)

using xbyte_t = std::uint8_t;

template <typename T>
xbyte_t to_byte(T const & input);

template <typename T>
T from_byte(xbyte_t byte);

NS_END1
