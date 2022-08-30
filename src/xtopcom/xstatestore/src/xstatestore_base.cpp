// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xvledger/xvledger.h"
#include "xstatestore/xstatestore_base.h"

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

data::xunitstate_ptr_t xstatestore_base_t::get_unit_state_by_accountindex(common::xaccount_address_t const& account_address, base::xaccount_index_t const& index) const {
    xobject_ptr_t<base::xvblock_t> unit_block_ptr;
    if (!index.get_latest_unit_hash().empty()) {
        unit_block_ptr = get_blockstore()->load_block_object(account_address.vaccount(), index.get_latest_unit_height(), index.get_latest_unit_hash(), false);
    } else {
        unit_block_ptr = get_blockstore()->load_block_object(account_address.vaccount(), index.get_latest_unit_height(), index.get_latest_unit_viewid(), false);
    }
    if (unit_block_ptr == nullptr) {
        // TODO(jimmy) sync invoke?
        xwarn("xstatestore_base_t::get_unit_state_by_accountindex fail-load unit. addr=%s,index=%s", account_address.value().c_str(), index.dump().c_str());
        return nullptr;
    }
    return get_unit_state_by_block(unit_block_ptr.get());
}

data::xtablestate_ptr_t xstatestore_base_t::get_latest_connectted_table_state(common::xaccount_address_t const& table_addr) const {
    xobject_ptr_t<base::xvblock_t> _block = get_blockstore()->get_latest_connected_block(table_addr.vaccount());
    if (nullptr != _block) {
        return get_table_state_by_block(_block.get());
    }
    xerror("xstatestore_base_t::get_latest_connectted_table_state fail-load block.addr=%s", table_addr.value().c_str());
    return nullptr;
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




base::xvblockstore_t*  xstatestore_base_t::get_blockstore() const {
    return base::xvchain_t::instance().get_xblockstore();
}

base::xvblkstatestore_t* xstatestore_base_t::get_blkstate_store() const {
    return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();
}


NS_END2
