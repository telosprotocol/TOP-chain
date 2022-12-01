// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address_fwd.h"
#include "xevm_common/trie/xtrie_sync.h"
#include "xevm_common/xfixed_hash.h"
#include "xvledger/xvdbstore.h"

namespace top {
namespace state_mpt {

std::shared_ptr<evm_common::trie::Sync> new_state_sync(const common::xaccount_address_t & table, evm_common::xh256_t const & root, base::xvdbstore_t * db, bool sync_unit);

}  // namespace state_mpt
}  // namespace top