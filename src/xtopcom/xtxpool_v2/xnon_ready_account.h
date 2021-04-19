// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

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

using data::xcons_transaction_ptr_t;

using xnon_ready_account_map_t = std::map<std::string, std::map<std::string, std::shared_ptr<xtx_entry>>>;  // key:account adress, value:<hash_str, transaction>

class xnon_ready_accounts_t {
public:
    xnon_ready_accounts_t(xtxpool_table_info_t * table_para) : m_xtable_info(table_para) {
    }
    int32_t push_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    const std::shared_ptr<xtx_entry> pop_tx(const tx_info_t & txinfo);
    const std::shared_ptr<xtx_entry> find_tx(const std::string & account_addr, const uint256_t & hash) const;
    const std::vector<std::string> get_accounts() const;
    const std::vector<std::shared_ptr<xtx_entry>> pop_account_txs(const std::string & account_addr);

private:
    xtxpool_table_info_t * m_xtable_info;
    xnon_ready_account_map_t m_account_map;
};

}  // namespace xtxpool_v2
}  // namespace top
