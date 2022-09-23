// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"

NS_BEG4(top, evm_common, trie, schema)

static const char * TrieSyncKey = "TrieSync";

static constexpr xbyte_t UnitPrefix{'u'};

inline xbytes_t unitKey(xhash256_t const & hash) {
    xbytes_t res{UnitPrefix};
    auto hash_bytes = hash.to_bytes();
    res.insert(res.end(), hash_bytes.begin(), hash_bytes.end());
    return res;
}

NS_END4