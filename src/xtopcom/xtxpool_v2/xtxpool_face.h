// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xtxpool_v2/xtxpool_base.h"

#include <string>
#include <vector>

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;
using namespace top::data;

// txpool does not consider pack strategy, unit service does
// enum enum_pack_strategy_type {
//     pack_strategy_common = 0,
//     pack_strategy_fast = 1,
//     pack_strategy_immediately = 2,
// };

class xtxpool_face_t {
public:
    virtual int32_t push_tx(const xcons_transaction_ptr_t & tx, const xtx_para_t & tx_para);
    virtual const xcons_transaction_ptr_t pop_tx_by_hash(const std::string & account_addr, const uint256_t & hash, uint8_t subtype, int32_t err);
    virtual candidate_accounts get_candidate_accounts(const std::string & table_addr, uint32_t count);
    virtual int32_t push_back_tx(std::shared_ptr<xtx_entry> tx_ent);
    virtual const xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const;
    virtual void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id);
    virtual void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id);
};

}  // namespace xtxpool_v2
}  // namespace top
