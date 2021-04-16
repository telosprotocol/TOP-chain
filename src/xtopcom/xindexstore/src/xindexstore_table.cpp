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

xtable_mbt_binlog_ptr_t xindexstore_table_t::query_mbt_binlog(const xblock_ptr_t & committed_block) {
    uint64_t latest_commit_height = committed_block->get_height();
    xaccount_ptr_t account_state = get_store()->query_account(get_account());
    if (account_state == nullptr) {
        account_state = make_object_ptr<xblockchain2_t>(get_account());
    }
    if (account_state->get_last_height() != latest_commit_height) {
        xwarn("xindexstore_table_t::query_binlog fail-load commmit account.table=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
            get_account().c_str(), account_state->get_last_height(), latest_commit_height);
        return nullptr;
    }
    xtable_mbt_binlog_ptr_t commit_mbt_binlog = account_state->get_table_mbt_binlog();
    commit_mbt_binlog->set_height(latest_commit_height);
    return commit_mbt_binlog;
}

xtable_mbt_ptr_t xindexstore_table_t::query_last_mbt(const xblock_ptr_t & committed_block) {
    uint64_t last_full_height;
    if (committed_block->get_block_class() == base::enum_xvblock_class_full) {
        last_full_height = committed_block->get_height();
    } else {
        last_full_height = committed_block->get_last_full_block_height();
    }

    xblock_ptr_t last_full_block;
    if (committed_block->get_block_class() == base::enum_xvblock_class_full) {
        last_full_block = committed_block;
    } else {
        base::xauto_ptr<base::xvblock_t> latest_full_block = get_blockstore()->load_block_object(*this, last_full_height, base::enum_xvblock_flag_committed, true);
        if (latest_full_block == nullptr) {
            xerror("xindexstore_table_t::get_account_index fail-load full block.table=%s,full_height=%" PRIu64 "",
                get_account().c_str(), last_full_height);
            return nullptr;
        }
        last_full_block = xblock_t::raw_vblock_to_object_ptr(latest_full_block.get());
    }
    data::xtable_mbt_ptr_t last_full_table_mbt;
    if (last_full_block->get_height() != 0) {
        xassert(false); // TODO(jimmy)
        // last_full_table_mbt = last_full_block->get_full_offstate();
        // if (last_full_table_mbt == nullptr) {
        //     xerror("xindexstore_table_t::get_account_index fail-load offstate from block.block=%s",
        //         last_full_block->dump().c_str());
        //     return nullptr;
        // }
    } else {
        last_full_table_mbt = make_object_ptr<base::xtable_mbt_t>();
    }
    last_full_table_mbt->set_height(last_full_height);
    return last_full_table_mbt;
}

bool xindexstore_table_t::update_mbt_state(const xblock_ptr_t & committed_block) {
    uint64_t latest_commit_height = committed_block->get_height();
    if (m_mbt_new_state.get_mbt_binlog()->get_height() != latest_commit_height) {
        xtable_mbt_binlog_ptr_t mbt_binlog = query_mbt_binlog(committed_block);
        if (nullptr == mbt_binlog) {
            return false;
        }
        xassert(mbt_binlog->get_height() == latest_commit_height);
        m_mbt_new_state.set_mbt_binlog(mbt_binlog);
    }

    uint64_t last_full_height = (committed_block->get_block_class() == base::enum_xvblock_class_full) ? committed_block->get_height() : committed_block->get_last_full_block_height();
    if (m_mbt_new_state.get_last_full_state()->get_height() != last_full_height) {
        xtable_mbt_ptr_t mbt = query_last_mbt(committed_block);
        if (nullptr == mbt) {
            return false;
        }
        m_mbt_new_state.set_last_full_state(mbt);
    }
    return true;
}

bool  xindexstore_table_t::get_account_index(const xblock_ptr_t & committed_block, const std::string & account, base::xaccount_index_t & account_index) {
    std::lock_guard<std::mutex> l(m_lock);
    if (false == update_mbt_state(committed_block)) {
        return false;
    }
    return m_mbt_new_state.get_account_index(account, account_index);
}

base::xtable_mbt_new_state_ptr_t xindexstore_table_t::get_mbt_new_state(const xblock_ptr_t & committed_block) {
    std::lock_guard<std::mutex> l(m_lock);
    if (false == update_mbt_state(committed_block)) {
        return nullptr;
    }
    base::xtable_mbt_new_state_ptr_t state = std::make_shared<base::xtable_mbt_new_state_t>(m_mbt_new_state);
    return state;
}

base::xtable_mbt_new_state_ptr_t  xindexstore_table_t::get_mbt_new_state() {
    // query latest table
    auto latest_table = get_blockstore()->get_latest_committed_block(*this);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());
    return get_mbt_new_state(committed_block);
}


bool  xindexstore_table_t::get_account_index(const std::string & account, base::xaccount_index_t & account_index) {
    // query latest table
    auto latest_table = get_blockstore()->get_latest_committed_block(*this);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());
    return get_account_index(committed_block, account, account_index);
}

bool  xindexstore_table_t::get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_index_info) {
    // query latest table
    auto latest_table = get_blockstore()->get_latest_committed_block(*this);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());

    base::xaccount_index_t account_index;
    if (false == get_account_index(committed_block, account, account_index)) {
        return false;
    }

    base::xvaccount_t _account_vaddress(account);
    auto _load_block = get_blockstore()->load_block_object(_account_vaddress, account_index.get_latest_unit_height(), base::enum_xvblock_flag_committed, true);
    if (_load_block == nullptr) {
        xwarn("xindexstore_table_t::get_account_basic_info fail-load block,account=%s,height=%" PRIu64 "",
            account.c_str(), account_index.get_latest_unit_height());
        return false;
    }

    xaccount_ptr_t account_state = get_store()->query_account(account);
    if (account_state == nullptr) {
        account_state = make_object_ptr<xblockchain2_t>(get_account());
    }
    if (account_state->get_last_height() != _load_block->get_height()) {
        xwarn("xindexstore_table_t::get_account_basic_info fail-state block unmatch.account=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
            account.c_str(), account_state->get_last_height(), _load_block->get_height());
        return false;
    }

    xblock_ptr_t latest_unit = xblock_t::raw_vblock_to_object_ptr(_load_block.get());
    account_index_info.set_latest_block(latest_unit);
    account_index_info.set_latest_state(account_state);
    account_index_info.set_account_index(account_index);
    return true;
}

NS_END2
