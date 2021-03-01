// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xtxpool_v2/xpending_account.h"
#include "xtxpool_v2/xtx_queue.h"
#include "xtxpool_v2/xtxpool_face.h"

#include <set>
#include <string>

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

class xtxmgr_table_t {
public:
    xtxmgr_table_t(xtxpool_table_info_t * xtable_info) : m_xtable_info(xtable_info), m_tx_queue(xtable_info), m_pending_accounts(xtable_info) {
    }

    int32_t push_tx(const xcons_transaction_ptr_t & tx, const xtx_para_t & tx_para);
    std::shared_ptr<xtx_entry> pop_tx_by_hash(const std::string & account, const uint256_t & hash, uint8_t subtype, int32_t err);
    candidate_accounts get_accounts_txs(uint32_t count);
    int32_t push_back_tx(std::shared_ptr<xtx_entry> tx_ent);
    const xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const;
    int32_t init();
    int32_t deinit();

private:
    void queue_to_pending();
    bool push_to_pending(std::vector<std::shared_ptr<xtx_entry>> & txs, uint32_t & tx_count);

    xtxpool_table_info_t * m_xtable_info;
    xtx_queue_t m_tx_queue;
    xpending_accounts_t m_pending_accounts;
};

}  // namespace xtxpool_v2
}  // namespace top
