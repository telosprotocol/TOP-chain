// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xindex_upgrade.h"
#include "xdata/xblocktool.h"

#include <string>

NS_BEG2(top, xunit_service)

bool xindex_upgrade_t::is_turn_to_upgrade(const std::string & table_addr) {
    std::lock_guard<std::mutex> lck(m_mutex);
    init();
    auto iter = m_table_upgrade_flags.find(table_addr);
    xassert(iter != m_table_upgrade_flags.end());
    return iter->second.second;
}

void xindex_upgrade_t::set_upgrade_succ(const std::string & table_addr) {
    std::lock_guard<std::mutex> lck(m_mutex);
    init();
    auto iter = m_table_upgrade_flags.find(table_addr);
    xassert(iter != m_table_upgrade_flags.end());
    auto & next_table = iter->second.first;
    if (next_table.empty()) {
        return;
    }
    auto iter_next = m_table_upgrade_flags.find(next_table);
    xassert(iter_next != m_table_upgrade_flags.end());
    iter_next->second.second = true;
    xinfo("xindex_upgrade_t::set_upgrade_succ table:%s,next:%s,next flag:%d", table_addr.c_str(), next_table.c_str(), iter_next->second.second);
}

void xindex_upgrade_t::init() {
    if (inited) {
        return;
    }

    auto all_table_addrs = data::xblocktool_t::make_all_table_addresses();
    uint32_t serial_num_max = 16;
    uint32_t num = 0;

    for (uint32_t i = 0; i < all_table_addrs.size(); i++) {
        auto & table = all_table_addrs[i];
        std::string next_table = "";
        bool turn = false;

        if (base::xvaccount_t(table).get_zone_index() == base::enum_chain_zone_consensus_index) {
            if ((i + 1) < all_table_addrs.size() && (i + 1)%16 != 0) {
                next_table = all_table_addrs[i + 1];
            }

            if (i%serial_num_max == 0) {
                turn = true;
            }
        } else {
            turn = true;
        }

        m_table_upgrade_flags[table] = std::make_pair(next_table, turn);
    }
    inited = true;

    for (auto & table_upgrade_flag : m_table_upgrade_flags) {
        xdbg("xindex_upgrade_t::init table:%s,next:%s,flag:%d", table_upgrade_flag.first.c_str(), table_upgrade_flag.second.first.c_str(), table_upgrade_flag.second.second);
    }
}

NS_END2
