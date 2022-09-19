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

data::xtablestate_ptr_t xstatestore_base_t::get_table_state_by_block(base::xvblock_t * target_block) const {
    base::xauto_ptr<base::xvbstate_t> bstate = get_blkstate_store()->get_block_state(target_block);
    if (bstate == nullptr) {
        // TODO(jimmy) invoke sync
        xwarn("xstatestore_base_t::get_table_state_by_block fail.block=%s", target_block->dump().c_str());
        return nullptr;
    }

    data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(bstate.get());  // readonly state
    return tablestate;
}

data::xunitstate_ptr_t xstatestore_base_t::get_unit_state_by_block(base::xvblock_t * target_block) const {
    base::xauto_ptr<base::xvbstate_t> bstate = get_blkstate_store()->get_block_state(target_block);
    if (bstate == nullptr) {
        // TODO(jimmy) invoke sync
        xwarn("xstatestore_base_t::get_unit_state_by_block fail.block=%s", target_block->dump().c_str());
        return nullptr;
    }

    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
    return unitstate;
}

data::xunitstate_ptr_t xstatestore_base_t::get_unit_state_by_block_hash(common::xaccount_address_t const& account_address, base::xaccount_index_t const& index) const {
    base::xauto_ptr<base::xvbstate_t> bstate = get_blkstate_store()->get_block_state(account_address.vaccount(), index.get_latest_unit_height(), index.get_latest_unit_hash());
    if (bstate == nullptr) {
        // TODO(jimmy) invoke sync
        xwarn("xstatestore_base_t::get_unit_state_by_block_hash fail.addr=%s,index=%s", account_address.value().c_str(), index.dump().c_str());
        return nullptr;
    }

    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
    return unitstate;
}
data::xunitstate_ptr_t xstatestore_base_t::get_unit_state_by_block_viewid(common::xaccount_address_t const& account_address, base::xaccount_index_t const& index) const {
    xobject_ptr_t<base::xvblock_t> unit_block_ptr = get_blockstore()->load_block_object(account_address.vaccount(), index.get_latest_unit_height(), index.get_latest_unit_viewid(), false);
    if (unit_block_ptr == nullptr) {
        // TODO(jimmy) sync invoke?
        xwarn("xstatestore_base_t::get_unit_state_by_accountindex fail-load unit. addr=%s,index=%s", account_address.value().c_str(), index.dump().c_str());
        return nullptr;
    }
    return get_unit_state_by_block(unit_block_ptr.get());
}

data::xunitstate_ptr_t xstatestore_base_t::get_unit_state_by_accountindex(common::xaccount_address_t const& account_address, base::xaccount_index_t const& index) const {
    if (!index.get_latest_unit_hash().empty()) {
        // try to load unitstate before unitblock
        return get_unit_state_by_block_hash(account_address, index);
    } else {
        return get_unit_state_by_block_viewid(account_address, index);
    }
}

data::xtablestate_ptr_t xstatestore_base_t::get_latest_committed_table_state(common::xaccount_address_t const& table_addr) const {
    xobject_ptr_t<base::xvblock_t> _block = get_blockstore()->get_latest_committed_block(table_addr.vaccount());
    if (nullptr != _block) {
        return get_table_state_by_block(_block.get());
    }
    xerror("xstatestore_base_t::get_latest_committed_table_state fail-load block.addr=%s", table_addr.value().c_str());
    return nullptr;
}

data::xtablestate_ptr_t xstatestore_base_t::get_genesis_table_state(common::xaccount_address_t const& table_addr) const {
    xobject_ptr_t<base::xvblock_t> _block = get_blockstore()->get_genesis_block(table_addr.vaccount());
    if (nullptr != _block) {
        return get_table_state_by_block(_block.get());
    }
    xerror("xstatestore_base_t::get_genesis_table_state fail-load block.addr=%s", table_addr.value().c_str());
    return nullptr;
}

xhash256_t xstatestore_base_t::get_state_root_from_block(base::xvblock_t * block) const {
    evm_common::xh256_t state_root;
    auto ret = data::xblockextract_t::get_state_root(block, state_root);
    if (!ret) {  // should not happen
        xerror("xstatestore_base_t::get_mpt_from_block get state root fail. block:%s", block->dump().c_str());
        return xhash256_t{};
    }
    xhash256_t root_hash = xhash256_t(state_root.to_bytes());
    return root_hash;
}

void xstatestore_base_t::get_tablestate_ext_from_block(base::xvblock_t* block, statestore::xtablestate_ext_ptr_t & tablestate_ext, std::error_code & ec) const {
    std::shared_ptr<state_mpt::xtop_state_mpt> mpt = nullptr;
    get_mpt_from_block(block, mpt, ec);
    if (ec) {
        xwarn("xstatestore_base_t::get_tablestate_ext_from_block fail-get mpt. block:%s,ec=%s", block->dump().c_str(),ec.message().c_str());
        return;
    }
    auto tablestate = get_table_state_by_block(block);  // TODO(jimmy) here should only get tabletate from db without execution
    if (nullptr == tablestate) {
        ec = error::xerrc_t::statestore_tablestate_exec_fail;
        xerror("xstatestore_base_t::get_tablestate_ext_from_block fail-get table state. block:%s", block->dump().c_str());
        return;
    }
    tablestate_ext = std::make_shared<xtablestate_ext_t>(tablestate, mpt);
}

void xstatestore_base_t::get_mpt_from_block(base::xvblock_t * block, std::shared_ptr<state_mpt::xtop_state_mpt> & mpt, std::error_code & ec) const {
    xassert(!ec);
    evm_common::xh256_t state_root;
    auto ret = data::xblockextract_t::get_state_root(block, state_root);
    if (!ret) {
        ec = error::xerrc_t::statestore_extract_state_root_err;
        xerror("xstatestore_base_t::get_mpt_from_block get state root fail. block:%s", block->dump().c_str());
        return;
    }
    xhash256_t root_hash = xhash256_t(state_root.to_bytes());
    mpt = state_mpt::xtop_state_mpt::create(block->get_account(), root_hash, base::xvchain_t::instance().get_xdbstore(), state_mpt::xstate_mpt_cache_t::instance(), ec);
}

void xstatestore_base_t::set_latest_executed_info(common::xaccount_address_t const& table_addr, uint64_t height,const std::string & blockhash) const {
    base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(table_addr.vaccount()));
    account_obj->set_latest_executed_block(height, blockhash);
}
uint64_t xstatestore_base_t::get_latest_executed_block_height(common::xaccount_address_t const& table_addr) const {
    base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(table_addr.vaccount()));
    return account_obj->get_latest_executed_block_height();
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
