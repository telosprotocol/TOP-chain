// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xstate_mpt/xstate_mpt_cache.h"
#include "xstate_mpt/xstate_mpt_db.h"
#include "xstate_mpt/xstate_mpt_journal.h"
#include "xvledger/xaccountindex.h"
#include "xevm_common/trie/xsecure_trie.h"
namespace top {
namespace state_mpt {
class xtop_state_mpt {
public:
    xtop_state_mpt() = default;
    ~xtop_state_mpt() = default;

public:
    /// @brief Create an state MPT with specific root hash.
    /// @param table Table address of state MPT.
    /// @param root Root hash of MPT.
    /// @param db Db interface.
    /// @param cache Globle state MPT cache.
    /// @param ec Log the error code.
    /// @return MPT with given root hash. Error occured if cannot find root in db.
    static std::shared_ptr<xtop_state_mpt> create(const std::string & table, const xhash256_t & root, base::xvdbstore_t * db, xstate_mpt_cache_t * cache, std::error_code & ec);

public:
    /// @brief Get index of specific account.
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return Account index of given account. Return empty index if not find in db.
    base::xaccount_index_t get_account_index(const std::string & account, std::error_code & ec);

    /// @brief Set index of specific account.
    /// @param account Account string.
    /// @param index Index of account.
    /// @param ec Log the error code.
    void set_account_index(const std::string & account, const base::xaccount_index_t & index, std::error_code & ec);

    /// @brief Set index of specific account.
    /// @param account Account string.
    /// @param index_str Index string of account.
    /// @param ec Log the error code.
    void set_account_index(const std::string & account, const std::string & index_str, std::error_code & ec);

    /// @brief Commit all modifies to db.
    /// @param ec Log the error code.
    /// @return New root hash.
    xhash256_t commit(std::error_code & ec);

    /// @brief Update modifies to trie and calculate root hash.
    /// @param ec Log the error code.
    /// @return New root hash.
    xhash256_t get_root_hash(std::error_code & ec);

    /// @brief Get original root hash.
    /// @return Original root hash.
    xhash256_t get_original_root_hash() const;

    /// @brief Return db object to do actions relay on db.
    /// @return DB object.
    std::shared_ptr<evm_common::trie::xtrie_db_t> get_database() const;

private:
    /// @brief Internal interface to init an empty state MPT.
    /// @param table Table address of state MPT.
    /// @param root Root hash of MPT.
    /// @param db Db interface.
    /// @param cache Globle state MPT cache.
    /// @param ec Log the error code.
    /// @return MPT with given root hash. Error occured if cannot find root in db.
    void init(const std::string & table, const xhash256_t & root, base::xvdbstore_t * db, xstate_mpt_cache_t * cache, std::error_code & ec);

    /// @brief Internal interface to get index string..
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return Account index string of given account. Return empty index srting if not find in db.
    std::string get_account_index_str(const std::string & account, std::error_code & ec);

    /// @brief Internal interface to set index string of to cache.
    /// @param account Account string.
    /// @param index_str Index string of account.
    void set_account_index_str(const std::string & account, const std::string & index_str);
    
    /// @brief Move journals to pending state.
    void finalize();

    /// @brief Clear journals.
    void clear_journal();

    std::string m_table_address;

    std::shared_ptr<evm_common::trie::xsecure_trie_t> m_trie{nullptr};
    std::shared_ptr<evm_common::trie::xtrie_db_t> m_db{nullptr};
    std::shared_ptr<base::xlru_cache<std::string, std::string>> m_lru{nullptr};
    xhash256_t m_original_root;

    std::map<std::string, xbytes_t> m_indexes;
    std::map<std::string, xbytes_t> m_cache_indexes;
    std::set<std::string> m_pending_indexes;

    xstate_journal_t m_journal;
};
using xstate_mpt_t = xtop_state_mpt;

}  // namespace state_mpt
}  // namespace top