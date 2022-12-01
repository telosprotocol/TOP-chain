// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt_database.h"

#include "xdata/xnative_contract_address.h"
#include "xevm_common/trie/xtrie_kv_db.h"

namespace top {
namespace state_mpt {

xtop_state_mpt_caching_db::xtop_state_mpt_caching_db(base::xvdbstore_t * db) {
    for (auto i = 0; i < enum_vbucket_has_tables_count; ++i) {
        auto const table_address = common::xtable_address_t::build_from(sharding_table_address.to_string() + '@' + std::to_string(i));
        m_table_caches[table_address] = evm_common::trie::xtrie_db_t::NewDatabase(std::make_shared<evm_common::trie::xkv_db_t>(db, table_address));
    }

    for (auto i = 0u; i < MAIN_CHAIN_REC_TABLE_USED_NUM; ++i) {
        auto const table_address = common::xtable_address_t::build_from(rec_table_address.to_string() + '@' + std::to_string(i));
        m_table_caches[table_address] = evm_common::trie::xtrie_db_t::NewDatabase(std::make_shared<evm_common::trie::xkv_db_t>(db, table_address));
    }

    for (auto i = 0u; i < MAIN_CHAIN_ZEC_TABLE_USED_NUM; ++i) {
        auto const table_address = common::xtable_address_t::build_from(zec_table_address.to_string() + '@' + std::to_string(i));
        m_table_caches[table_address] = evm_common::trie::xtrie_db_t::NewDatabase(std::make_shared<evm_common::trie::xkv_db_t>(db, table_address));
    }

    for (auto i = 0u; i < MAIN_CHAIN_EVM_TABLE_USED_NUM; ++i) {
        auto const table_address = common::xtable_address_t::build_from(eth_table_address.to_string() + '@' + std::to_string(i));
        m_table_caches[table_address] = evm_common::trie::xtrie_db_t::NewDatabase(std::make_shared<evm_common::trie::xkv_db_t>(db, table_address));
    }

    for (auto i = 0u; i < MAIN_CHAIN_RELAY_TABLE_USED_NUM; ++i) {
        auto const table_address = common::xtable_address_t::build_from(relay_table_base_address.to_string() + '@' + std::to_string(i));
        m_table_caches[table_address] = evm_common::trie::xtrie_db_t::NewDatabase(std::make_shared<evm_common::trie::xkv_db_t>(db, table_address));
    }
}

observer_ptr<evm_common::trie::xtrie_db_t> xtop_state_mpt_caching_db::trie_db(common::xtable_address_t const & table) {
    return make_observer(m_table_caches.at(table));
}

std::shared_ptr<evm_common::trie::xtrie_face_t> xtop_state_mpt_caching_db::open_trie(common::xtable_address_t const & table, evm_common::xh256_t const & hash, std::error_code & ec) {
    return evm_common::trie::xsecure_trie_t::build_from(hash, trie_db(table), ec);
}

}  // namespace state_mpt
}  // namespace top
