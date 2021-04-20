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
    m_tablestate = make_object_ptr<xtablestate_t>();
}

bool xindexstore_table_t::update_tablestate(const xblock_ptr_t & block) {
    if (m_tablestate->get_binlog_height() == block->get_height()) {
        return true;
    }

    xaccount_ptr_t account_state = get_store()->query_account(get_account());
    if (account_state == nullptr) {
        xwarn("xindexstore_table_t::get_tablestate fail-account null.table=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
            get_account().c_str(), account_state->get_last_height(), block->get_height());
        return false;
    }
    if (account_state->get_last_height() != block->get_height()) {
        xwarn("xindexstore_table_t::get_tablestate fail-account height not match.table=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
            get_account().c_str(), account_state->get_last_height(), block->get_height());
        return false;
    }

    uint64_t last_full_height = (block->get_block_class() == base::enum_xvblock_class_full) ? block->get_height() : block->get_last_full_block_height();
    if (m_tablestate->get_full_height() != last_full_height) {
        xassert(m_tablestate->get_full_height() < last_full_height);
        base::xauto_ptr<base::xvblock_t> last_full_block = get_blockstore()->load_block_object(*this, last_full_height, base::enum_xvblock_flag_committed, true);
        if (nullptr == last_full_block) {
            xwarn("xindexstore_table_t::update_tablestate fail-load full block. table=%s,height=%" PRIu64 "",
                get_account().c_str(), last_full_height);
            return false;
        }
        xassert(last_full_block->get_offdata() != nullptr);
        std::string old_full_data_str;
        last_full_block->get_offdata()->serialize_to_string(old_full_data_str);
        m_tablestate->serialize_from_full_offdata(old_full_data_str);
        m_tablestate->set_full_height(last_full_height);
    }

    std::string binlog_str = account_state->get_extend_data(xblockchain2_t::enum_blockchain_ext_type_binlog);
    m_tablestate->serialize_from_binlog(binlog_str);
    m_tablestate->set_binlog_height(block->get_height());
    return true;
}

xtablestate_ptr_t xindexstore_table_t::clone_tablestate(const xblock_ptr_t & block) {
    std::lock_guard<std::mutex> l(m_lock);
    if (false == update_tablestate(block)) {
    return nullptr;
    }
    return m_tablestate->clone();
}

xtablestate_ptr_t xindexstore_table_t::clone_tablestate() {
    auto latest_table = get_blockstore()->get_latest_committed_block(*this);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());
    return clone_tablestate(committed_block);
}

bool  xindexstore_table_t::get_account_index(const xblock_ptr_t & committed_block, const std::string & account, base::xaccount_index_t & account_index) {
    std::lock_guard<std::mutex> l(m_lock);
    if (false == update_tablestate(committed_block)) {
        return false;
    }
    return m_tablestate->get_account_index(account, account_index);
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

    base::xauto_ptr<base::xvblock_t> _block_ptr = get_blockstore()->get_latest_committed_block(_account_vaddress);
    xblock_ptr_t latest_unit = xblock_t::raw_vblock_to_object_ptr(_block_ptr.get());

    xaccount_ptr_t account_state = get_store()->query_account(account);
    if (account_state == nullptr) {
        account_state = make_object_ptr<xblockchain2_t>(get_account());
    }
    // if (account_state->get_last_height() != account_index.get_latest_unit_height()) {
    //     xwarn("xindexstore_table_t::get_account_basic_info fail-state block unmatch.account=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 "",
    //         account.c_str(), account_state->get_last_height(), account_index.get_latest_unit_height());
    //     return false;
    // }
    // TODO(jimmy)
    if (account_state->get_last_height() != latest_unit->get_height() || account_state->get_last_height() + 2 < account_index.get_latest_unit_height()) {
        xwarn("xindexstore_table_t::get_account_basic_info fail-state block unmatch.account=%s,state_height=%" PRIu64 ",block_height=%" PRIu64 ",index_height=%" PRIu64 "",
            account.c_str(), account_state->get_last_height(), latest_unit->get_height(), account_index.get_latest_unit_height());
        return false;
    }
    account_index_info.set_latest_block(latest_unit);
    account_index_info.set_latest_state(account_state);
    account_index_info.set_account_index(account_index);
    return true;
}

NS_END2
