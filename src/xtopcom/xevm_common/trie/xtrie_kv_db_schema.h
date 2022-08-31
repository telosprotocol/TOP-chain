// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"

NS_BEG4(top, evm_common, trie, schema)

static constexpr xbyte_t CodePrefix{'c'};

inline xbytes_t codeKey(xhash256_t const & hash) {
    xbytes_t res{CodePrefix};
    res.insert(res.end(), hash.to_bytes().begin(), hash.to_bytes().end());
    return res;
}

NS_END4