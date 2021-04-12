// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>

NS_BEG2(top, common)

enum class xenum_major_sideline_type : std::uint64_t {
    invalid,
    Bitcoin    = 0x0000000000000001,
    Ethereum   = 0x0000000000000002,
    EOS        = 0x0000000000000004,
};

NS_END2
