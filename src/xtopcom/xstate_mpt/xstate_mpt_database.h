// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcommon/xtable_address.h"
#include "xevm_common/trie/xtrie_db.h"
#include "xvledger/xvdbstore.h"

NS_BEG2(top, state_mpt)

class xtop_state_mpt_caching_db {
public:
    xtop_state_mpt_caching_db(xtop_state_mpt_caching_db const &) = delete;
    xtop_state_mpt_caching_db & operator=(xtop_state_mpt_caching_db const &) = delete;
    xtop_state_mpt_caching_db(xtop_state_mpt_caching_db &&) = default;
    xtop_state_mpt_caching_db & operator=(xtop_state_mpt_caching_db &&) = default;
    ~xtop_state_mpt_caching_db() = default;

    explicit xtop_state_mpt_caching_db(base::xvdbstore_t * db);

    observer_ptr<evm_common::trie::xtrie_db_t> trie_db(common::xtable_address_t const & table) const;

private:
    std::map<common::xtable_address_t, std::shared_ptr<evm_common::trie::xtrie_db_t>> table_caches_;
};
using xstate_mpt_caching_db_t = xtop_state_mpt_caching_db;

NS_END2
