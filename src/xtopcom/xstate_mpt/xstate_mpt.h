// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie.h"
#include "xstate_mpt/xstate_mpt_db.h"
#include "xvledger/xaccountindex.h"

namespace top {
namespace state_mpt {

class xtop_state_mpt {
public:
    xtop_state_mpt() = default;

public:
    static std::shared_ptr<xtop_state_mpt> create(xhash256_t root, base::xvdbstore_t * db, std::error_code & ec);

public:
    base::xaccount_index_t get_account_index(const std::string & account, std::error_code & ec) const;
    void set_account_index(const std::string & account, const base::xaccount_index_t & index, std::error_code & ec);
    xhash256_t get_root_hash() const;
    xhash256_t commit(std::error_code & ec);

private:
    void init(xhash256_t root, base::xvdbstore_t * db, std::error_code & ec);

    std::shared_ptr<evm_common::trie::xtrie_t> m_trie;
    std::shared_ptr<evm_common::trie::xtrie_db_t> m_db;
};
using xstate_mpt_t = xtop_state_mpt;

}  // namespace state_mpt
}  // namespace top