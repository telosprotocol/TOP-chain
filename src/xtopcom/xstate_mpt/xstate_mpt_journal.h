// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xaccountindex.h"

namespace top {
namespace state_mpt {

struct xstate_index_change_t {
    std::string account;
    std::string prev_index;

    xstate_index_change_t(const std::string & acc, const std::string & index) : account(acc), prev_index(index) {
    }
};

struct xstate_journal_t {
    std::vector<xstate_index_change_t> index_changes;
    std::map<std::string, int> dirties;

    void append(xstate_index_change_t change) {
        index_changes.emplace_back(change);
        dirties[change.account] += 1;
    }
};

}
}