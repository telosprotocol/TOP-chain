// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xbasic/xmemory.hpp"
#include "xstore/xstore_face.h"
#include "xbase/xvledger.h"
#include "xdata/xfull_tableblock.h"
#include "xindexstore/src/xindexstore_table.h"

NS_BEG2(top, store)

bool xindexstore_table_t::update_state_binlog(const xblock_ptr_t & committed_block) {
    uint64_t latest_commit_height = committed_block->get_height();
    if (m_commit_mbt_binlog_height == UINT64_MAX || m_commit_mbt_binlog_height < latest_commit_height) {
        xaccount_ptr_t account_state = get_store()->query_account(get_account());
        if (account_state == nullptr) {
            account_state = make_object_ptr<xblockchain2_t>(get_account());
        }
        if (account_state->get_last_height() != latest_commit_height) {
            xwarn("xindexstore_table_t::update_state_binlog fail-load commmit account.table=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
                get_account().c_str(), account_state->get_last_height(), latest_commit_height);
            return false;
        }
        m_commit_mbt_binlog = account_state->get_table_mbt_binlog();
        m_commit_mbt_binlog_height = latest_commit_height;
        xdbg("xindexstore_table_t::update_state_binlog cache new binlog state.account=%s,height=%" PRIu64 "", get_account().c_str(), latest_commit_height);
    } else if (m_commit_mbt_binlog_height > latest_commit_height) {
        xwarn("xindexstore_table_t::update_state_binlog fail-committed block height less than state.table=%s,binlog_height=%" PRIu64 ",block_height=%" PRIu64 "",
            get_account().c_str(), m_commit_mbt_binlog_height, latest_commit_height);
        return false;
    }
    return true;
}

bool xindexstore_table_t::update_state_full(const xblock_ptr_t & committed_block) {
    uint64_t last_full_height;
    if (committed_block->get_block_class() == base::enum_xvblock_class_full) {
        last_full_height = committed_block->get_height();
    } else {
        last_full_height = committed_block->get_last_full_block_height();
    }
    if (m_last_full_table_mbt_height == UINT64_MAX || m_last_full_table_mbt_height != last_full_height) {
        xblock_ptr_t last_full_block;
        if (committed_block->get_block_class() == base::enum_xvblock_class_full) {
            last_full_block = committed_block;
        } else {
            base::xauto_ptr<base::xvblock_t> latest_full_block = get_blockstore()->load_block_object(*this, last_full_height);
            if (latest_full_block == nullptr) {
                xerror("xindexstore_table_t::get_account_index fail-load full block.table=%s,full_height=%" PRIu64 "",
                    get_account().c_str(), last_full_height);
                return false;
            }
            last_full_block = xblock_t::raw_vblock_to_object_ptr(latest_full_block.get());
        }
        data::xtable_mbt_ptr_t last_full_table_mbt;
        if (last_full_block->get_height() != 0) {
            last_full_table_mbt = last_full_block->get_full_offstate();
            if (last_full_table_mbt == nullptr) {
                xerror("xindexstore_table_t::get_account_index fail-load offstate from block.block=%s",
                    last_full_block->dump().c_str());
                return false;
            }
        } else {
            last_full_table_mbt = make_object_ptr<xtable_mbt_t>();
        }
        m_last_full_table_mbt = last_full_table_mbt;
        m_last_full_table_mbt_height = last_full_height;
        xdbg("xindexstore_table_t::get_account_index cache new full state.account=%s,height=%" PRIu64 "", get_account().c_str(), last_full_height);
    }
    return true;
}

bool  xindexstore_table_t::get_account_index(const xblock_ptr_t & committed_block, const std::string & account, data::xaccount_index_t & account_index) {
    std::lock_guard<std::mutex> l(m_lock);

    // firstly, cache index binlog and try to find account index from binlog
    if (false == update_state_binlog(committed_block)) {
        return false;
    }
    if (m_commit_mbt_binlog->get_account_index(account, account_index)) {
        return true;
    }

    // secondly, cache last full index and try to find accout index from last full index
    if (false == update_state_full(committed_block)) {
        return false;
    }
    m_last_full_table_mbt->get_account_index(account, account_index);
    return true;
}

bool  xindexstore_table_t::get_account_index(const std::string & account, data::xaccount_index_t & account_index) {
    // query latest table
    auto latest_table = get_blockstore()->get_latest_committed_block(*this);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());
    return get_account_index(committed_block, account, account_index);
}

bool  xindexstore_table_t::get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_index_info) {
    // query latest table
    auto latest_table = get_blockstore()->get_latest_committed_block(*this);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());

    data::xaccount_index_t account_index;
    if (false == get_account_index(committed_block, account, account_index)) {
        return false;
    }

    auto _load_block = get_blockstore()->load_block_object(account, account_index.get_latest_unit_height());
    if (_load_block == nullptr) {
        xwarn("xindexstore_table_t::get_account_basic_info fail-load block,account=%s,height=%" PRIu64 "",
            account.c_str(), account_index.get_latest_unit_height());
        return false;
    }
    if (!_load_block->check_block_flag(base::enum_xvblock_flag_committed)) {
        xwarn("xindexstore_table_t::get_account_basic_info fail-not execute block,block=%s",
            _load_block->dump().c_str());
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
