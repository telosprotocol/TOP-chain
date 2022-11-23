// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash.hpp"
#include "xcommon/xaccount_address.h"
#include "xstate_mpt/xstate_mpt_store_fwd.h"
#include "xevm_common/xfixed_hash.h"

#include <map>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

NS_BEG2(top, base)

class xvdbstore_t;

NS_END2

NS_BEG2(top, state_mpt)

class xtop_state_mpt_store {
private:
    common::xaccount_address_t table_account_address_;
    // std::unordered_map<evm_common::xh256_t, uint64_t> trie_root_hash_to_table_height_map_;
    std::map<uint64_t, std::vector<evm_common::xh256_t>> table_height_to_root_hashes_map_;
    std::unordered_map<evm_common::xh256_t, std::unordered_set<evm_common::xh256_t>> trie_node_hashes_;

public:
    xtop_state_mpt_store() = default;
    xtop_state_mpt_store(xtop_state_mpt_store const &) = delete;
    xtop_state_mpt_store & operator=(xtop_state_mpt_store const &) = delete;
    xtop_state_mpt_store(xtop_state_mpt_store &&) = default;
    xtop_state_mpt_store & operator=(xtop_state_mpt_store &&) = default;
    ~xtop_state_mpt_store() = default;

    explicit xtop_state_mpt_store(common::xaccount_address_t table_address);

    void load_state(evm_common::xh256_t const & root_hash, base::xvdbstore_t * db, std::error_code & ec);
};
using xstate_mpt_store_t = xtop_state_mpt_store;

NS_END2
