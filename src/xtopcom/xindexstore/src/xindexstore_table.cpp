// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xbasic/xmemory.hpp"
#include "xstore/xstore_face.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xfull_tableblock.h"
#include "xindexstore/src/xindexstore_table.h"

NS_BEG2(top, store)

xindexstore_table_t::xindexstore_table_t(const std::string & account, const xindexstore_resources_ptr_t & resources)
: xindexstore_face_t(account), m_resources(resources) {
    xdbg("xindexstore_table_t::xindexstore_table_t create,this=%p,account=%s", this, account.c_str());
}
xindexstore_table_t::~xindexstore_table_t() {
    xdbg("xindexstore_table_t::xindexstore_table_t destroy,this=%p", this);
}

void xindexstore_table_t::set_cache_state(uint64_t block_height, const std::string & block_hash, const xtablestate_ptr_t & state) {
    xassert(block_height == state->get_height());

    auto iter = m_cache_tablestate.find(block_height);
    if (iter == m_cache_tablestate.end()) {
        std::map<std::string, xtablestate_ptr_t> hash_map;
        hash_map[block_hash] = state;
        m_cache_tablestate[block_height] = hash_map;
    } else {
        std::map<std::string, xtablestate_ptr_t> & hash_map = iter->second;
        xassert(hash_map.size() < 4);  // should not has too many different blocks in same height
        auto iter2 = hash_map.find(block_hash);
        if (iter2 == hash_map.end()) {
            hash_map[block_hash] = state;
        } else {
            return;
        }
    }
    xdbg("xindexstore_table_t::set_cache_state table=%s,height=%" PRIu64 ",hash=%s",
        get_account().c_str(), state->get_height(), base::xstring_utl::to_hex(block_hash).c_str());
    // try to clear old state
    clear_old_cache_state();
}

void xindexstore_table_t::set_cache_state(const xblock_ptr_t & block, const xtablestate_ptr_t & state) {
    return set_cache_state(block->get_height(), block->get_block_hash(), state);
}

xtablestate_ptr_t xindexstore_table_t::get_cache_state(uint64_t block_height, const std::string & block_hash) const {
    auto iter = m_cache_tablestate.find(block_height);
    if (iter != m_cache_tablestate.end()) {
        const std::map<std::string, xtablestate_ptr_t> & hash_map = iter->second;
        auto iter2 = hash_map.find(block_hash);
        if (iter2 != hash_map.end()) {
            return iter2->second;
        }
    }
    return nullptr;
}

xtablestate_ptr_t xindexstore_table_t::get_cache_state(const xblock_ptr_t & block) const {
    return get_cache_state(block->get_height(), block->get_block_hash());
}

void xindexstore_table_t::clear_old_cache_state() {
    if (m_cache_tablestate.size() > 4) {  // only cache 4 height blocks
        // clear lowest height state
        auto iter = m_cache_tablestate.begin();
        xdbg("xindexstore_table_t::clear_old_cache_state table=%s,height=%" PRIu64 "",
            get_account().c_str(), iter->first);
        m_cache_tablestate.erase(iter);
    }
    xassert(m_cache_tablestate.size() <= 4);
}

xtablestate_ptr_t xindexstore_table_t::execute_block_to_new_state(const xtablestate_ptr_t & prev_state, const xblock_ptr_t & current_block) {
    xassert(prev_state->get_height() + 1 == current_block->get_height());
    xtablestate_ptr_t new_state;
    if (current_block->get_block_class() == base::enum_xvblock_class_full) {
        new_state = prev_state->clone();
    } else {
        new_state = prev_state->clone_with_fulldata();
    }
    xdbg("xindexstore_table_t::execute_block_to_new_state account=%s,height=%ld,prev_state=%p,old=%p,new=%p,old=%p,new=%p",
        current_block->get_account().c_str(),
        current_block->get_height(),
        prev_state.get(),
        prev_state->get_accountindex_state()->get_last_full_state().get(),
        new_state->get_accountindex_state()->get_last_full_state().get(),
        prev_state->get_accountindex_state()->get_binlog().get(),
        new_state->get_accountindex_state()->get_binlog().get());
    new_state->execute_block(current_block.get());
    xassert(new_state->get_height() == current_block->get_height());
    return new_state;
}

xtablestate_ptr_t xindexstore_table_t::rebuild_tablestate(const xtablestate_ptr_t & old_state, const std::map<uint64_t, xblock_ptr_t> & latest_blocks) {
    xtablestate_ptr_t new_state = old_state;
    for (auto & v : latest_blocks) {
        new_state = execute_block_to_new_state(new_state, v.second);
        set_cache_state(v.second, new_state);
    }
    return new_state;
}

xtablestate_ptr_t xindexstore_table_t::load_latest_tablestate_from_db() {
    xaccount_ptr_t account_state = get_store()->query_account(get_account());
    if (account_state == nullptr) {
        xdbg("xindexstore_table_t::load_latest_tablestate_from_db account null.table=%s",
            get_account().c_str());
        return make_object_ptr<xtablestate_t>();  // create genensis state
    }

    xobject_ptr_t<base::xvboffdata_t> full_offdata = nullptr;
    uint64_t last_full_height = account_state->get_last_full_unit_height();
    if (last_full_height != 0) {
        base::xauto_ptr<base::xvblock_t> last_full_block = get_blockstore()->load_block_object(*this, last_full_height, base::enum_xvblock_flag_committed, true);
        if (nullptr == last_full_block) {
            xerror("xindexstore_table_t::load_latest_tablestate_from_db fail-load full block. table=%s,height=%" PRIu64 "",
                get_account().c_str(), last_full_height);
            return nullptr;
        }
        if (nullptr == last_full_block->get_offdata()) {
            xerror("xindexstore_table_t::load_latest_tablestate_from_db fail-full block has no offdata. table=%s,height=%" PRIu64 "",
                get_account().c_str(), last_full_height);
            return nullptr;
        }
        last_full_block->get_offdata()->add_ref();
        full_offdata.attach(last_full_block->get_offdata());
        xdbg("xindexstore_table_t::load_latest_tablestate_from_db load new fulldata.account=%s,height=%ld", get_account().c_str(), last_full_height);
    }

    std::string binlog_str = account_state->get_extend_data(xblockchain2_t::enum_blockchain_ext_type_binlog);
    xtablestate_ptr_t new_tablestate = make_object_ptr<xtablestate_t>(full_offdata, last_full_height, binlog_str, account_state->get_last_height());
    set_cache_state(account_state->get_last_height(), account_state->get_last_block_hash(), new_tablestate);
    return new_tablestate;
}

xtablestate_ptr_t xindexstore_table_t::create_target_tablestate(const xblock_ptr_t & block) {
    std::map<uint64_t, xblock_ptr_t> latest_blocks;
    xblock_ptr_t current_block = block;
    latest_blocks[current_block->get_height()] = current_block;
    uint32_t count = 0;
    uint32_t max_count = 6;  // TODO(jimmy)
    xtablestate_ptr_t db_state = nullptr;

    while (count < max_count) {
        // find prev state in cache
        xtablestate_ptr_t prev_state = get_cache_state(current_block->get_height() - 1, current_block->get_last_block_hash());
        if (nullptr != prev_state) {
            xdbg("xindexstore_table_t::create_target_tablestate rebuild state from cache state.table=%s,prev_height=%ld,block_height=%ld",
                get_account().c_str(), prev_state->get_height(), block->get_height());
            return rebuild_tablestate(prev_state, latest_blocks);
        }

        // find prev state in db
        if (nullptr == db_state) {
            db_state = load_latest_tablestate_from_db();
            if (nullptr == db_state) {
                xwarn("xindexstore_table_t::create_target_tablestate fail-load db state.table=%s,block=%s",
                    get_account().c_str(), block->dump().c_str());
                return nullptr;
            }
        }
        if (db_state->get_height() == current_block->get_height()) {
            xdbg("xindexstore_table_t::create_target_tablestate match db state.table=%s,block_height=%ld",
                get_account().c_str(), block->get_height());
            return db_state;
        }
        if (db_state->get_height() == (current_block->get_height() - 1)) {
            xdbg_info("xindexstore_table_t::create_target_tablestate rebuild state from db state.table=%s,dbstate_height=%ld,block_height=%ld",
                get_account().c_str(), db_state->get_height(), block->get_height());
            return rebuild_tablestate(db_state, latest_blocks);
        }

        // check if block is too old
        if (block->get_height() < db_state->get_height()) {
            xwarn("xindexstore_table_t::create_target_tablestate fail-block is too old.table=%s,dbstate_height=%ld,block=%s",
                get_account().c_str(), db_state->get_height(), block->dump().c_str());
            return nullptr;
        }

        // load prev block
        if (current_block->get_height() <= 1) {
            xerror("xindexstore_table_t::create_target_tablestate fail-prev block is genesis.table=%s,dbstate_height=%ld,block=%s",
                get_account().c_str(), db_state->get_height(), block->dump().c_str());
            return nullptr;
        }
        base::xauto_ptr<base::xvblock_t> prev_block = get_blockstore()->load_block_object(*this, current_block->get_height() - 1, current_block->get_last_block_hash(), true);
        if (nullptr == prev_block) {
            xwarn("xindexstore_table_t::create_target_tablestate fail-load block. table=%s,height=%ld,block=%s",
                get_account().c_str(), current_block->get_height() - 1, block->dump().c_str());
            return nullptr;
        }
        current_block = xblock_t::raw_vblock_to_object_ptr(prev_block.get());
        latest_blocks[current_block->get_height()] = current_block;
    }
    xwarn("xindexstore_table_t::create_target_tablestate fail-exceed max count limit. table=%s,block=%s",
        get_account().c_str(), block->dump().c_str());
    return nullptr;
}

xtablestate_ptr_t xindexstore_table_t::get_target_tablestate(const xblock_ptr_t & block) {
    // firstly, try to find in cache
    xtablestate_ptr_t state = get_cache_state(block);
    if (state != nullptr) {
        xdbg("xindexstore_table_t::get_target_tablestate succ-block height match cache tablestate.table=%s,block=%s",
            get_account().c_str(), block->dump().c_str());
        return state;
    }
    if (block->is_genesis_block()) {
        return make_object_ptr<xtablestate_t>();  // TODO(jimmy) create genesis tablestate
    }
    // secondly, try to create new state
    return create_target_tablestate(block);
}

xtablestate_ptr_t xindexstore_table_t::clone_tablestate(const xblock_ptr_t & block) {
    std::lock_guard<std::mutex> l(m_lock);
    xtablestate_ptr_t tablestate = get_target_tablestate(block);
    if (tablestate == nullptr) {
        return nullptr;
    }
    xassert(tablestate->get_height() == block->get_height());
    return tablestate;
}

bool  xindexstore_table_t::get_account_index(const xblock_ptr_t & committed_block, const std::string & account, base::xaccount_index_t & account_index) {
    std::lock_guard<std::mutex> l(m_lock);
    xtablestate_ptr_t tablestate = get_target_tablestate(committed_block);
    if (tablestate == nullptr) {
        return false;
    }
    xassert(tablestate->get_height() == committed_block->get_height());
    return tablestate->get_account_index(account, account_index);
}

bool  xindexstore_table_t::get_account_index(const std::string & account, base::xaccount_index_t & account_index) {
    // query latest table
    auto latest_table = get_blockstore()->get_latest_connected_block(*this);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());
    return get_account_index(committed_block, account, account_index);
}

bool  xindexstore_table_t::get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_index_info) {
    // query latest table
    auto latest_table = get_blockstore()->get_latest_connected_block(*this);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());

    base::xaccount_index_t account_index;
    if (false == get_account_index(committed_block, account, account_index)) {
        return false;
    }

    base::xvaccount_t _account_vaddress(account);
    // base::xblock_vector blocks = get_blockstore()->load_block_object(_account_vaddress, account_index.get_latest_unit_height());
    // if (blocks.get_vector().empty()) {
    //     xerror("xindexstore_table_t::get_account_basic_info fail-load block,account=%s,height=%" PRIu64 "",
    //         account.c_str(), account_index.get_latest_unit_height());
    //     return false;
    // }
    // for (auto & block : blocks.get_vector()) {
    //     if (account_index.is_match_unit_hash(block->get_block_hash())) {
    //         latest_unit = xblock_t::raw_vblock_to_object_ptr(block);
    //         break;
    //     }
    // }
    // if (latest_unit == nullptr) {
    //     xerror("xindexstore_table_t::get_account_basic_info fail-find hash match block,account=%s,height=%" PRIu64 "",
    //         account.c_str(), account_index.get_latest_unit_height());
    //     return false;
    // }

    base::xauto_ptr<base::xvblock_t> _block_ptr = get_blockstore()->get_latest_cert_block(_account_vaddress);
    xblock_ptr_t latest_cert_unit = xblock_t::raw_vblock_to_object_ptr(_block_ptr.get());

    // if (account_state->get_last_height() != account_index.get_latest_unit_height()) {
    //     xwarn("xindexstore_table_t::get_account_basic_info fail-state block unmatch.account=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
    //         account.c_str(), account_state->get_last_height(), account_index.get_latest_unit_height());
    //     return false;
    // }
    // TODO(jimmy)
    // if (account_state->get_last_height() != latest_unit->get_height() || account_state->get_last_height() + 2 < account_index.get_latest_unit_height()) {
    //     xwarn("xindexstore_table_t::get_account_basic_info fail-state block unmatch.account=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 ",index_height=%" PRIu64 "",
    //         account.c_str(), account_state->get_last_height(), latest_unit->get_height(), account_index.get_latest_unit_height());
    //     account_index_info.set_sync_from_height(account_state->get_last_height() + 1);
    //     account_index_info.set_sync_to_height(account_index.get_latest_unit_height());
    //     return false;
    // }

    if (latest_cert_unit->get_height() < account_index.get_latest_unit_height()) {
        base::xauto_ptr<base::xvblock_t> _start_block_ptr = get_blockstore()->get_latest_connected_block(_account_vaddress);
        if (account_index.get_latest_unit_height() > _start_block_ptr->get_height()) {
            account_index_info.set_sync_height_start(_start_block_ptr->get_height() + 1);
            account_index_info.set_sync_num(account_index.get_latest_unit_height() - _start_block_ptr->get_height());
        }
        return false;
    }

    xaccount_ptr_t account_state = get_store()->query_account(account);
    if (account_state == nullptr) {
        account_state = make_object_ptr<xblockchain2_t>(get_account());
    }
    // account_index_info.set_latest_block(latest_unit);
    account_index_info.set_latest_state(account_state);
    account_index_info.set_account_index(account_index);
    return true;
}

NS_END2
