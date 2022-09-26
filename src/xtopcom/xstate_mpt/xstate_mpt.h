// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xnode_id.h"
#include "xevm_common/trie/xsecure_trie.h"
#include "xstate_mpt/xstate_mpt_cache.h"
#include "xstate_mpt/xstate_mpt_journal.h"
#include "xstate_mpt/xstate_object.h"
#include "xvledger/xvdbstore.h"

namespace top {
namespace state_mpt {

struct xaccount_info_t {
public:
    xaccount_info_t() = default;
    ~xaccount_info_t() = default;

    std::string encode();
    void decode(const std::string & str);

    std::string m_account;
    base::xaccount_index_t m_index;
};

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
    static std::shared_ptr<xtop_state_mpt> create(const common::xaccount_address_t & table, const xhash256_t & root, base::xvdbstore_t * db, xstate_mpt_cache_t * cache, std::error_code & ec);

public:
    /// @brief Get index of specific account.
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return Account index of given account. Return empty index if not find in db.
    base::xaccount_index_t get_account_index(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Get unit data of specific account.
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return Unit data of given account. Return empty data if not find in db.
    xbytes_t get_unit(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Set index of specific account.
    /// @param account Account string.
    /// @param index Index of account.
    /// @param ec Log the error code.
    void set_account_index(common::xaccount_address_t const & account, const base::xaccount_index_t & index, std::error_code & ec);

    /// @brief Set index of specific account.
    /// @param account Account string.
    /// @param index_str Index string of account.
    /// @param ec Log the error code.
    void set_account_index(common::xaccount_address_t const & account, const std::string & index_str, std::error_code & ec);

    /// @brief Set index and unit data of specific account.
    /// @param account Account string.
    /// @param index Index of account.
    /// @param unit unit data of account.
    /// @param ec Log the error code.
    void set_account_index_with_unit(common::xaccount_address_t const & account, const base::xaccount_index_t & index, const xbytes_t & unit, std::error_code & ec);

    /// @brief Set index and unit data of specific account.
    /// @param account Account string.
    /// @param index_str Index string of account.
    /// @param unit unit data of account.
    /// @param ec Log the error code.
    void set_account_index_with_unit(common::xaccount_address_t const & account, const std::string & index_str, const xbytes_t & unit, std::error_code & ec);

    /// @brief Prune unit data in db.
    /// @param account Account string.
    /// @param ec Log the error code.
    void prune_unit(const std::string & account, std::error_code & ec);

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
    const xhash256_t & get_original_root_hash() const;

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
    void init(const common::xaccount_address_t & table, const xhash256_t & root, base::xvdbstore_t * db, xstate_mpt_cache_t * cache, std::error_code & ec);
    
    /// @brief Move journals to pending state.
    void finalize();

    /// @brief Clear journals.
    void clear_journal();

    /// @brief Get or create state object of specific account.
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return State object of account.
    std::shared_ptr<xstate_object_t> get_or_new_state_object(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Get state object for specific account.
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return State object of account.
    std::shared_ptr<xstate_object_t> get_state_object(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Create state object for specific account.
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return State object of account.
    std::shared_ptr<xstate_object_t> create_object(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Get state object from db.
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return State object of account.
    std::shared_ptr<xstate_object_t> get_deleted_state_object(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Set state object to cache.
    /// @param obj State object.
    void set_state_object(std::shared_ptr<xstate_object_t> obj);

    /// @brief Update state object to db.
    /// @param obj State object.
    /// @param ec Log the error code.
    void update_state_object(std::shared_ptr<xstate_object_t> obj, std::error_code & ec);

    common::xaccount_address_t m_table_address;

    std::shared_ptr<evm_common::trie::xsecure_trie_t> m_trie{nullptr};
    std::shared_ptr<evm_common::trie::xtrie_db_t> m_db{nullptr};
    std::shared_ptr<base::xlru_cache<std::string, std::string>> m_lru{nullptr};
    xhash256_t m_original_root;

    std::map<common::xaccount_address_t, xbytes_t> m_cache_indexes;

    std::map<common::xaccount_address_t, std::shared_ptr<xstate_object_t>> m_state_objects;
    std::set<common::xaccount_address_t> m_state_objects_pending;
    std::set<common::xaccount_address_t> m_state_objects_dirty;

    xstate_journal_t m_journal;
};
using xstate_mpt_t = xtop_state_mpt;

}  // namespace state_mpt
}  // namespace top