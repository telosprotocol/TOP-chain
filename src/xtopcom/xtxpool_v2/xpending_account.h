// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xlru_cache.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xdata/xgenesis_data.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxpool_v2/xtxpool_info.h"

#include <map>
#include <set>
#include <string>

namespace top {
namespace xtxpool_v2 {

#define xtxpool_pending_accounts_num_max (128)
#define xtxpool_pending_account_selected_count_lru_size (1024)

using data::xcons_transaction_ptr_t;

class xaccount_entry_comp {
public:
    bool operator()(const std::shared_ptr<xcandidate_account_entry> left, const std::shared_ptr<xcandidate_account_entry> right) const {
        bool is_sys_l = data::is_sys_contract_address(common::xaccount_address_t{left->get_addr()});
        bool is_sys_r = data::is_sys_contract_address(common::xaccount_address_t{right->get_addr()});
        if (is_sys_l == is_sys_r) {
            return left->get_select_count() < right->get_select_count();
        }

        return is_sys_l;
    }
};

using accounts_set = std::multiset<std::shared_ptr<xcandidate_account_entry>, xaccount_entry_comp>;

class xpending_accounts_t {
public:
    xpending_accounts_t(xtxpool_table_info_t * table_para) : m_xtable_info(table_para), m_account_selected_lru(xtxpool_pending_account_selected_count_lru_size) {
    }
    int32_t push_tx(std::shared_ptr<xtx_entry> tx_ent);
    std::shared_ptr<xtx_entry> pop_tx_by_hash(const std::string & account_addr, const uint256_t & hash, uint8_t subtype, int32_t err);
    candidate_accounts pop_accounts(uint32_t count);
    const xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const;

private:
    void tx_count_inc(uint8_t subtype, int32_t count);
    xtxpool_table_info_t * m_xtable_info;
    accounts_set m_accounts_set;                                      // accounts ordered by account type and selected number
    std::map<std::string, accounts_set::iterator> m_account_map;      // be easy to find account from m_accounts_set by address
    basic::xlru_cache<std::string, uint16_t> m_account_selected_lru;  // record selected count of accounts, for order
};

}  // namespace xtxpool_v2
}  // namespace top
