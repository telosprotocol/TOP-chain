// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG1(top)

#if defined(XCXX20)
using xendian_t = std::endian;
#else

NS_BEG1(details)

enum class xtop_endian {
    little = 0,
    big    = 1,
    native = little,
};

NS_END1

using xendian_t = details::xtop_endian;

#endif

NS_END1
