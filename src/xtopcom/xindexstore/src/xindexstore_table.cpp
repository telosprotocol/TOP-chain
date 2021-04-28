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
    m_tablestate = make_object_ptr<xtablestate_t>();  // create genensis state
}

void xindexstore_table_t::set_cache_state(const xblock_ptr_t & block, const xtablestate_ptr_t & state) {
    xassert(block->get_height() == state->get_height());

    auto iter = m_cache_tablestate.find(block->get_block_hash());
    if (iter == m_cache_tablestate.end()) {
        m_cache_tablestate[block->get_block_hash()] = state;
        xdbg("xindexstore_table_t::set_cache_state table=%s,tablestate_height=%" PRIu64 "",
        get_account().c_str(), state->get_height());
    }

    // try to clear old state
    clear_old_cache_state();
}

xtablestate_ptr_t xindexstore_table_t::get_cache_state(const xblock_ptr_t & block) const {
    auto iter = m_cache_tablestate.find(block->get_block_hash());
    if (iter != m_cache_tablestate.end()) {
        return iter->second;
    }
    return nullptr;
}

void xindexstore_table_t::clear_old_cache_state() {
    if (m_cache_tablestate.size() > 6) {
        for (auto iter = m_cache_tablestate.begin(); iter != m_cache_tablestate.end();) {
            const xtablestate_ptr_t & state = iter->second;
            if (state->get_height() + 5 <= max_highest_height) {
                xdbg("xindexstore_table_t::clear_old_cache_state table=%s,tablestate_height=%" PRIu64 "",
                    get_account().c_str(), state->get_height());
                iter = m_cache_tablestate.erase(iter);
            } else {
                iter++;
            }
        }
    }
}

xtablestate_ptr_t xindexstore_table_t::rebuild_tablestate(const xtablestate_ptr_t & old_state, const std::map<uint64_t, xblock_ptr_t> & latest_blocks) {
    xtablestate_ptr_t new_state = old_state->clone();  // must clone before execute
    for (auto & v : latest_blocks) {
        xassert(new_state->get_height() + 1 == v.second->get_height());
        auto ret = new_state->execute_block(v.second.get());
        xassert(ret);
        xassert(new_state->get_height() == v.second->get_height());
    }
    return new_state;
}

xtablestate_ptr_t xindexstore_table_t::get_target_block_state(const xtablestate_ptr_t & old_state, const xblock_ptr_t & block) {
    if (old_state->get_height() == block->get_height()) {
        return old_state;
    }

    if (old_state->get_height() > block->get_height()) {
        xerror("xindexstore_table_t::get_target_block_state block height less than state.block_height=%ld,state_height=%ld", block->get_height(), old_state->get_height());
        return nullptr;
    }

    uint64_t begin_height = old_state->get_height() + 1;
    std::map<uint64_t, xblock_ptr_t> latest_blocks;
    xblock_ptr_t current_block = block;
    latest_blocks[current_block->get_height()] = current_block;
    while (current_block->get_height() > begin_height) {
        base::xauto_ptr<base::xvblock_t> prev_block = get_blockstore()->load_block_object(*this, current_block->get_height() - 1, current_block->get_last_block_hash(), true);
        if (nullptr == prev_block) {
            // TODO(jimmy)
            xwarn("xindexstore_table_t::get_target_block_state fail-load block. table=%s,height=%" PRIu64 "",
                get_account().c_str(), current_block->get_height() - 1);
            return nullptr;
        }
        prev_block->add_ref();
        current_block.attach(dynamic_cast<xblock_t*>(prev_block.get()));
        latest_blocks[current_block->get_height()] = current_block;
    }

    xtablestate_ptr_t new_tablestate = rebuild_tablestate(old_state, latest_blocks);
    xassert(new_tablestate != nullptr);
    return new_tablestate;
}

xtablestate_ptr_t xindexstore_table_t::load_base_tablestate_from_db(const xtablestate_ptr_t & old_tablestate) {
    xaccount_ptr_t account_state = get_store()->query_account(get_account());
    if (account_state == nullptr) {
        xerror("xindexstore_table_t::load_base_tablestate_from_db fail-account null.table=%s",
            get_account().c_str());
        return nullptr;
    }

    if (old_tablestate->get_height() >= account_state->get_last_height()) {
        xdbg("xindexstore_table_t::load_base_tablestate_from_db no need reload base.table=%s,tablestate_height=%" PRIu64 ",account_height=%" PRIu64 "",
            get_account().c_str(), old_tablestate->get_height(), account_state->get_last_height());
        return old_tablestate;
    }

    xobject_ptr_t<base::xvboffdata_t> full_offdata = nullptr;
    uint64_t last_full_height = account_state->get_last_full_unit_height();
    if (last_full_height != 0) {
        if (last_full_height == old_tablestate->get_full_height()) {
            full_offdata = old_tablestate->get_block_full_data();
        } else {
            base::xauto_ptr<base::xvblock_t> last_full_block = get_blockstore()->load_block_object(*this, last_full_height, base::enum_xvblock_flag_committed, true);
            if (nullptr == last_full_block) {
                xerror("xindexstore_table_t::load_tablestate_from_db fail-load full block. table=%s,height=%" PRIu64 "",
                    get_account().c_str(), last_full_height);
                return nullptr;
            }
            if (nullptr == last_full_block->get_offdata()) {
                xerror("xindexstore_table_t::load_tablestate_from_db fail-full block has no offdata. table=%s,height=%" PRIu64 "",
                    get_account().c_str(), last_full_height);
                return nullptr;
            }
            last_full_block->get_offdata()->add_ref();
            full_offdata.attach(last_full_block->get_offdata());
        }
    }

    std::string binlog_str = account_state->get_extend_data(xblockchain2_t::enum_blockchain_ext_type_binlog);

    xtablestate_ptr_t new_tablestate = make_object_ptr<xtablestate_t>(full_offdata, last_full_height, binlog_str, account_state->get_last_height());
    return new_tablestate;
}

xtablestate_ptr_t xindexstore_table_t::get_target_tablestate(const xblock_ptr_t & block) {
    xtablestate_ptr_t state = get_cache_state(block);
    if (state != nullptr) {
        xdbg("xindexstore_table_t::get_target_tablestate succ-block height match cache tablestate.table=%s,block_height=%" PRIu64 "",
            get_account().c_str(), block->get_height());
        return state;
    }
    if (m_tablestate->get_height() == block->get_height()) {
        return m_tablestate;
    }

    if (m_tablestate->get_height() > block->get_height()) {
        xwarn("xindexstore_table_t::get_target_tablestate fail-block height less than cache tablestate.table=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
            get_account().c_str(), m_tablestate->get_height(), block->get_height());
        return nullptr;
    }

    xtablestate_ptr_t tablestate_base = load_base_tablestate_from_db(m_tablestate);
    if (nullptr == tablestate_base) {
        xerror("xindexstore_table_t::get_target_tablestate fail-load_base_tablestate_from_db.table=%s,block_height=%" PRIu64 "",
            get_account().c_str(), block->get_height());
        return nullptr;
    }
    if (m_tablestate->get_height() < tablestate_base->get_height()) {
        m_tablestate = tablestate_base;
        xdbg("xindexstore_table_t::get_target_tablestate succ-cache new tablestate.table=%s,block_height=%" PRIu64 ",full_height=%" PRIu64 "",
            get_account().c_str(), m_tablestate->get_height(), m_tablestate->get_full_height());
    }

    if (tablestate_base->get_height() < block->get_height()) {
        tablestate_base = get_target_block_state(tablestate_base, block);
        if (nullptr == tablestate_base) {
            xwarn("xindexstore_table_t::get_target_tablestate fail-get_target_block_state.table=%s,old_state_height=%" PRIu64 ",block_height=%" PRIu64 "",
                get_account().c_str(), m_tablestate->get_height(), block->get_height());
            return nullptr;
        }
        xassert(tablestate_base->get_height() == block->get_height());
    }

    if (tablestate_base->get_height() == block->get_height()) {
        set_cache_state(block, tablestate_base);
        xdbg("xindexstore_table_t::get_target_tablestate succ.table=%s,old_state_height=%" PRIu64 ",block_height=%" PRIu64 "",
            get_account().c_str(), m_tablestate->get_height(), block->get_height());
        return tablestate_base;
    }
    xwarn("xindexstore_table_t::get_target_tablestate fail. binlog height larger than block.table=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
        get_account().c_str(), tablestate_base->get_height(), block->get_height());
    return nullptr;
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
        account_index_info.set_sync_height_start(_start_block_ptr->get_height() + 1);
        account_index_info.set_sync_num(account_index.get_latest_unit_height() - _start_block_ptr->get_height());
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
