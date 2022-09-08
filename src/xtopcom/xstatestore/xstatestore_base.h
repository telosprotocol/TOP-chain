// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xcommon/xaccount_address.h"

NS_BEG2(top, statestore)

class xstatestore_base_t {
 public:
    xstatestore_base_t() {}

 public:
    data::xtablestate_ptr_t    get_latest_connectted_table_state(common::xaccount_address_t const& table_addr) const;
    data::xtablestate_ptr_t    get_latest_committed_table_state(common::xaccount_address_t const& table_addr) const;
    data::xtablestate_ptr_t    get_genesis_table_state(common::xaccount_address_t const& table_addr) const;
    data::xtablestate_ptr_t    get_table_state_by_block(base::xvblock_t * target_block) const;

    data::xunitstate_ptr_t     get_unit_state_by_block(base::xvblock_t * target_block) const;
    data::xunitstate_ptr_t     get_unit_state_by_accountindex(common::xaccount_address_t const& account_address, base::xaccount_index_t const& index) const;

 private:
   data::xunitstate_ptr_t      get_unit_state_by_block_hash(common::xaccount_address_t const& account_address, base::xaccount_index_t const& index) const;
   data::xunitstate_ptr_t      get_unit_state_by_block_viewid(common::xaccount_address_t const& account_address, base::xaccount_index_t const& index) const;
 public:
    base::xvblockstore_t*       get_blockstore() const;
    base::xvblkstatestore_t*    get_blkstate_store() const;
};


NS_END2
