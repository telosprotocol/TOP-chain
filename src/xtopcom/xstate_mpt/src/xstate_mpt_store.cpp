// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_mpt_store.h"

#include "xstate_mpt/xstate_mpt.h"
#include "xvledger/xvdbstore.h"

NS_BEG2(top, state_mpt)

xtop_state_mpt_store::xtop_state_mpt_store(common::xaccount_address_t table_address) : table_account_address_ {std::move(table_address)} {
}

void xtop_state_mpt_store::load_state(evm_common::xh256_t const & root_hash, base::xvdbstore_t * db, std::error_code & ec) {
    auto state = xstate_mpt_t::create(table_account_address_, root_hash, db, ec);
}

NS_END2
