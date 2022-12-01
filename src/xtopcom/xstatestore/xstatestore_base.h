// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xdata/xunit_bstate.h"
#include "xevm_common/xfixed_hash.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xvledger/xvstatestore.h"

NS_BEG2(top, statestore)

class xstatestore_base_t {
 public:
    xstatestore_base_t() {}

 public:
    void                       get_mpt_from_block(base::xvblock_t * block, std::shared_ptr<state_mpt::xstate_mpt_t> & mpt, std::error_code & ec) const;
    evm_common::xh256_t                 get_state_root_from_block(base::xvblock_t * block) const;

    uint64_t                   get_latest_executed_block_height(common::xaccount_address_t const& table_addr) const;
    void                       set_latest_executed_info(common::xaccount_address_t const& table_addr, uint64_t height,const std::string & blockhash) const;
    uint64_t                   get_latest_committed_block_height(common::xaccount_address_t const& table_addr) const;
    void                       set_lowest_executed_block_height(common::xaccount_address_t const& table_addr, uint64_t height) const;
    uint64_t                   get_lowest_executed_block_height(common::xaccount_address_t const& table_addr) const;

 public:
    base::xvblockstore_t*       get_blockstore() const;
    base::xvblkstatestore_t*    get_blkstate_store() const;
    base::xvdbstore_t*          get_dbstore() const;
};


NS_END2
