// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xtable_address.h"
#include "xevm_common/trie/xsecure_trie.h"
#include "xcommon/xfixed_hash.h"
#include "xstate_mpt/xstate_mpt_store_fwd.h"
#include "xstate_mpt/xstate_object.h"
#include "xvledger/xvdbstore.h"

#include <memory>
#include <mutex>
#include <string>

NS_BEG3(top, state_mpt, details)

struct xtop_account_info {
    std::string encode() const;
    void decode(const std::string & str);

    common::xaccount_address_t account;
    base::xaccount_index_t index;
};

class xtop_state_mpt {
private:
    common::xtable_address_t m_table_address;

    std::unique_ptr<evm_common::trie::xtrie_face_t> m_trie{nullptr};
    observer_ptr<evm_common::trie::xtrie_db_t> m_trie_db{nullptr};
    // evm_common::xh256_t m_original_root;

    mutable std::mutex m_state_objects_lock;
    mutable std::mutex m_trie_lock;

    std::map<common::xaccount_address_t, std::shared_ptr<xstate_object_t>> m_state_objects;
    std::set<common::xaccount_address_t> m_state_objects_pending;

public:
    /// @brief Create an state MPT with specific root hash.
    /// @param table Table address of state MPT.
    /// @param root Root hash of MPT.
    /// @param db Db interface.
    /// @param ec Log the error code.
    /// @return MPT with given root hash. Error occurred if cannot find root in db.
    static std::shared_ptr<xtop_state_mpt> create(common::xtable_address_t const & table, const evm_common::xh256_t & root, base::xvdbstore_t * db, std::error_code & ec);

    /// @brief Get index of specific account.
    /// @param account Account string.
    /// @param ec Log the error code.
    /// @return Account index of given account. Return empty index if not find in db.
    base::xaccount_index_t get_account_index(common::xaccount_address_t const & account, std::error_code & ec);

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

    /// @brief Prune unit data in db.
    /// @param account Account string.
    /// @param ec Log the error code.
    void prune_unit(common::xaccount_address_t const & account, std::error_code & ec);

    /// @brief Commit all modifies to db.
    /// @param ec Log the error code.
    /// @return New root hash.
    evm_common::xh256_t commit(std::error_code & ec);

    void prune(evm_common::xh256_t const & old_trie_root_hash, std::unordered_set<evm_common::xh256_t> & pruned_hashes, std::error_code & ec) const;
    void commit_pruned(std::unordered_set<evm_common::xh256_t> const & pruned_hashes, std::error_code & ec) const;

    void prune(std::error_code & ec);
    void commit_pruned(std::vector<evm_common::xh256_t> pruned_keys, std::error_code & ec) const;
    void clear_pruned(evm_common::xh256_t const & pruned_key, std::error_code & ec) const;
    void clear_pruned(std::error_code & ec) const;

    /// @brief Update modifies to trie and calculate root hash.
    /// @param ec Log the error code.
    /// @return New root hash.
    evm_common::xh256_t get_root_hash(std::error_code & ec);

    /// @brief Get original root hash.
    /// @return Original root hash.
    evm_common::xh256_t const & original_root_hash() const noexcept;

private:
    /// @brief Internal interface to init an empty state MPT.
    /// @param table Table address of state MPT.
    /// @param root Root hash of MPT.
    /// @param db Db interface.
    /// @param ec Log the error code.
    void init(common::xtable_address_t const & table, const evm_common::xh256_t & root, base::xvdbstore_t * db, std::error_code & ec);

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

    std::shared_ptr<xstate_object_t> query_state_object(common::xaccount_address_t const& account) const;
};


NS_END3

NS_BEG2(top, state_mpt)
using xstate_mpt_t = details::xtop_state_mpt;
using xaccount_info_t = details::xtop_account_info;
NS_END2
