// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xvledger/xvledger.h"
#include "xstatestore/xstatestore_base.h"
#include "xstatestore/xerror.h"
#include "xdata/xblockextract.h"

NS_BEG2(top, statestore)

evm_common::xh256_t xstatestore_base_t::get_state_root_from_block(base::xvblock_t * block) const {
    std::error_code ec;
    auto state_root = data::xblockextract_t::get_state_root(block, ec);
    if (ec) {  // should not happen
        xerror("xstatestore_base_t::get_mpt_from_block get state root fail. block:%s", block->dump().c_str());
        return evm_common::xh256_t{};
    }

    return state_root;
}

void xstatestore_base_t::get_mpt_from_block(base::xvblock_t * block, std::shared_ptr<state_mpt::xstate_mpt_t> & mpt, std::error_code & ec) const {
    xassert(!ec);
    auto const & state_root = data::xblockextract_t::get_state_root(block, ec);
    if (ec) {
        // ec = error::xerrc_t::statestore_extract_state_root_err;
        xerror("xstatestore_base_t::get_mpt_from_block get state root fail. block:%s", block->dump().c_str());
        return;
    }

    mpt = state_mpt::xstate_mpt_t::create(common::xaccount_address_t{block->get_account()}, state_root, base::xvchain_t::instance().get_xdbstore(), ec);
}

void xstatestore_base_t::set_latest_executed_info(common::xaccount_address_t const& table_addr, uint64_t height,const std::string & blockhash) const {
    base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(table_addr.vaccount()));
    account_obj->set_latest_executed_block(height, blockhash);
}
uint64_t xstatestore_base_t::get_latest_executed_block_height(common::xaccount_address_t const& table_addr) const {
    base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(table_addr.vaccount()));
    return account_obj->get_latest_executed_block_height();
}

void xstatestore_base_t::set_lowest_executed_block_height(common::xaccount_address_t const& table_addr, uint64_t height) const {
    base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(table_addr.vaccount()));
    account_obj->set_lowest_executed_block_height(height);
}
uint64_t xstatestore_base_t::get_lowest_executed_block_height(common::xaccount_address_t const& table_addr) const {
    base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(table_addr.vaccount()));
    return account_obj->get_lowest_executed_block_height();
}

uint64_t xstatestore_base_t::get_latest_committed_block_height(common::xaccount_address_t const& table_addr) const {
    uint64_t _highest_commit_block_height = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block_height(table_addr.vaccount());
    return _highest_commit_block_height;
}

base::xvblockstore_t*  xstatestore_base_t::get_blockstore() const {
    return base::xvchain_t::instance().get_xblockstore();
}

base::xvblkstatestore_t* xstatestore_base_t::get_blkstate_store() const {
    return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();
}
base::xvdbstore_t* xstatestore_base_t::get_dbstore() const {
    return base::xvchain_t::instance().get_xdbstore();
}


NS_END2
