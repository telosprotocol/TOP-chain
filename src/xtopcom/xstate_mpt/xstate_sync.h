// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_sync.h"
#include "xvledger/xvdbstore.h"

namespace top {
namespace state_mpt {

std::shared_ptr<evm_common::trie::Sync> new_state_sync(const std::string & table, const xhash256_t & root, base::xvdbstore_t * db);

}  // namespace state_mpt
}  // namespace top