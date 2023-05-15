// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#if defined(XCXX20)

#include <bit>

NS_BEG1(top)
using xendian_t = std::endian;
NS_END1

#else

NS_BEG2(top, details)

enum class xtop_endian {
    little = 0,
    big    = 1,
    native = little,
};

NS_END2

NS_BEG1(top)

using xendian_t = details::xtop_endian;

NS_END1

#endif
