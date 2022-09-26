// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xaccountindex.h"

namespace top {
namespace state_mpt {

struct xstate_index_change_t {
    common::xaccount_address_t account;
    base::xaccount_index_t prev_index;

    xstate_index_change_t(common::xaccount_address_t const & acc, const base::xaccount_index_t & index) : account(acc), prev_index(index) {
    }
};

struct xstate_journal_t {
    std::vector<xstate_index_change_t> index_changes;
    std::map<common::xaccount_address_t, int> dirties;

    void append(xstate_index_change_t change) {
        index_changes.emplace_back(change);
        dirties[change.account] += 1;
    }
};

}
}