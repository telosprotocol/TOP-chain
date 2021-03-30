// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_tableunit.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

using namespace mbus;
using namespace data;

xsync_tableunit_t::xsync_tableunit_t(std::string vnode_id, const std::string &address,
        mbus::xmessage_bus_face_t *mbus):
m_vnode_id(vnode_id),
m_address(address),
m_mbus(mbus) {
}

#if 0
void xsync_tableunit_t::on_history_unit(const data::xblock_ptr_t &block, const syncbase::xdata_mgr_ptr_t &data_mgr) {

    //xsync_dbg("[account][tableunit] on_history_unit %s %lu", block->get_block_owner().c_str(), block->get_height());
#if 0
    uint64_t height = block->get_height();
    uint64_t block_time = block->get_timerblock_height();

    mbus::xevent_account_ptr_t e =
            std::make_shared<mbus::xevent_account_finish_block_t>(m_address, height, block_time, data_mgr);
    e->err = mbus::xevent_t::succ;
    m_mbus->push_event(e);
#endif
}

bool xsync_tableunit_t::on_receive_history_unit(const std::string &address, uint64_t height, const xdata_mgr_ptr_t &data_mgr) {
    auto it = m_waiting_accounts.find(address);
    if (it == m_waiting_accounts.end())
        return false;

    if (data_mgr->is_contain(it->second)) {
        xsync_dbg("[account][tableunit][notify] on_account_complete %s %lu expect %s %lu notify:%lu is_exist",
                m_address.c_str(), m_history_waiting_height, address.c_str(), it->second, height);
        m_waiting_accounts.erase(it);
        if (m_waiting_accounts.empty()) {
            m_history_waiting_height = 0;
            return true;
        }
    } else {
        xsync_dbg("[account][tableunit][notify] on_account_complete %s %lu expect %s %lu notify:%lu no_exist",
                m_address.c_str(), m_history_waiting_height, address.c_str(), it->second, height);
    }

    return false;
}

void xsync_tableunit_t::on_active_tableblock(const data::xblock_ptr_t &block) {
#ifdef SYNC_UNIT
    xsync_dbg("[account][tableunit] on_active_tableblock %s %lu", block->get_block_owner().c_str(), block->get_height());

    if (block->is_emptyblock())
        return;

    data::xtable_block_t * tableblock = dynamic_cast<data::xtable_block_t *>(block.get());
    const auto & account_units = tableblock->get_input_units();
    uint64_t block_height = block->get_height();
    uint64_t block_time = block->get_timerblock_height();

    std::vector<xaccount_info_t> account_list;

    for (auto it: account_units) {
        const std::string &address = it->get_unit_account();
        uint64_t height = it->get_unit_height();
        if (m_sub_account_pool.update(address, height)) {
            xsync_dbg("[account][tableunit][resolve] %s %lu find_latest_height %s %lu", m_address.c_str(), block_height, address.c_str(), height);
            xaccount_info_t info(address, height, block_time);
            account_list.push_back(info);
        }
    }

    table_find_unit(account_list);
#endif
}

void xsync_tableunit_t::on_history_tableblock(const data::xblock_ptr_t &block) {

    //xsync_dbg("[account][tableunit] on_history_tableblock %s %lu", block->get_block_owner().c_str(), block->get_height());

    if (block->is_emptyblock()) {
        return;
    }

    data::xtable_block_t * tableblock = dynamic_cast<data::xtable_block_t *>(block.get());
    const auto & account_units = tableblock->get_input_units();
    uint64_t block_height = block->get_height();
    uint64_t block_time = block->get_timerblock_height();

    std::vector<xaccount_info_t> account_list;
#ifdef SYNC_UNIT
    for (auto it: account_units) {
        const std::string &address = it->get_unit_account();
        uint64_t height = it->get_unit_height();

        //xsync_dbg("[account][tableunit] %s %lu find_history_height %s %lu", m_address.c_str(), block_height, address.c_str(), height);

        xdata_mgr_ptr_t data_mgr = m_account_queue->get_data_mgr(address);

        if (data_mgr!=nullptr && data_mgr->is_contain(height)) {
            xsync_dbg("[account][tableunit][resolve] %s %lu find_history_height %s %lu is_exist", m_address.c_str(), block_height, address.c_str(), height);
            continue;
        }

        xsync_dbg("[account][tableunit][resolve] %s %lu find_history_height %s %lu no_exist", m_address.c_str(), block_height, address.c_str(), height);

        m_waiting_accounts[address] = height;

        xaccount_info_t info(address, height, block_time);
        account_list.push_back(info);
    }

    table_find_unit(account_list);
#endif

    if (!m_waiting_accounts.empty()) {
        m_history_waiting_height = block_height;
    }
}

bool xsync_tableunit_t::is_idle() {
    return m_history_waiting_height==0 && m_waiting_accounts.empty();
}

void xsync_tableunit_t::clear() {
    m_history_waiting_height = 0;
    m_waiting_accounts.clear();
    m_sub_account_pool.clear();
}

void xsync_tableunit_t::table_find_unit(std::vector<xaccount_info_t> &account_list) {
    for (auto &it: account_list) {
        mbus::xevent_account_ptr_t e =
                    std::make_shared<mbus::xevent_account_find_block_t>(it.address, it.height, it.block_time);
        e->err = mbus::xevent_t::succ;
        m_mbus->push_event(e);
    }
}
#endif

NS_END2
