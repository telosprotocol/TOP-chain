// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xhash.hpp"
#include "xtrie_node.h"

#include <memory>

NS_BEG3(top, evm_common, trie)

class xtop_trie_db_face {
public:
    virtual xtrie_node_face_ptr_t node(xhash256_t hash) = 0;
};
using xtrie_db_face_t = xtop_trie_db_face;
using xtrie_db_face_ptr_t = std::shared_ptr<xtrie_db_face_t>;

NS_END3