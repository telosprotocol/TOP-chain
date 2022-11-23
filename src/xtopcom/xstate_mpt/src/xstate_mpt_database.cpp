// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt_database.h"

#include "xevm_common/trie/xtrie_kv_db.h"

namespace top {
namespace state_mpt {

xtop_state_mpt_caching_db::xtop_state_mpt_caching_db(base::xvdbstore_t * db) : m_db(db) {
}

std::shared_ptr<evm_common::trie::xtrie_db_t> xtop_state_mpt_caching_db::trie_db(common::xtable_address_t const & table) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::shared_ptr<evm_common::trie::xtrie_db_t> trie_db{nullptr};
    if (!m_table_caches.count(table)) {
        auto const kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, table);
        trie_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
        m_table_caches.insert({table, trie_db});
        xinfo("xstate_mpt_caching_container_t::open_trie new trie db from table: %s", table.to_string().c_str());
    } else {
        trie_db = m_table_caches.at(table);
    }
    return trie_db;
}

std::shared_ptr<evm_common::trie::xtrie_face_t> xtop_state_mpt_caching_db::open_trie(common::xtable_address_t const & table, evm_common::xh256_t const & hash, std::error_code & ec) {
    return evm_common::trie::xsecure_trie_t::build_from(hash, trie_db(table), ec);
}

}  // namespace state_mpt
}  // namespace top