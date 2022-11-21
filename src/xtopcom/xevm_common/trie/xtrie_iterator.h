// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"
#include "xbasic/xmemory.hpp"
#include "xevm_common/trie/xtrie.h"
#include "xevm_common/xfixed_hash.h"

#include <memory>
#include <vector>

NS_BEG3(top, evm_common, trie)

class xtop_trie_simple_iterator {
public:
    static std::vector<xbytes_t> trie_leafs(xh256_t const & trie_root_hash, observer_ptr<xtrie_db_t> const & trie_db);

private:
    static void get_trie_leafs(std::shared_ptr<xtrie_node_face_t> const & node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
    static void get_hash_node_leafs(std::shared_ptr<xtrie_hash_node_t> const & hash_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
    static void get_short_node_leafs(std::shared_ptr<xtrie_short_node_t> const & short_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
    static void get_value_node_leaf(std::shared_ptr<xtrie_value_node_t> const & value_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
    static void get_full_node_leafs(std::shared_ptr<xtrie_full_node_t> const & full_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
};
using xtrie_simple_iterator_t = xtop_trie_simple_iterator;

NS_END3
