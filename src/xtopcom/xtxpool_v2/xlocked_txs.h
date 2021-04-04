// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxpool_v2/xtxpool_info.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace top {
namespace xtxpool_v2 {

class locked_tx_t {
public:
    locked_tx_t(const std::string & account_addr, enum_transaction_subtype subtype, const std::shared_ptr<xtx_entry> & tx_ent)
      : m_account_addr(account_addr), m_subtype(subtype), m_tx_ent(tx_ent) {
    }

    locked_tx_t(const tx_info_t & txinfo, const std::shared_ptr<xtx_entry> & tx_ent)
      : m_account_addr(txinfo.get_addr()), m_subtype(txinfo.get_subtype()), m_tx_ent(tx_ent) {
    }

    const std::string & get_account_addr() const {return m_account_addr;}
    enum_transaction_subtype get_subtype() const {return m_subtype;}
    const std::shared_ptr<xtx_entry> & get_tx_ent() const {return m_tx_ent;}
    void set_tx_ent(const std::shared_ptr<xtx_entry> & tx_ent) {m_tx_ent = tx_ent;}

private:
    std::string m_account_addr;
    enum_transaction_subtype m_subtype;
    std::shared_ptr<xtx_entry> m_tx_ent;
};

using locked_tx_map_t = std::unordered_map<std::string, std::shared_ptr<locked_tx_t>>;

class xlocked_txs_t {
public:
    xlocked_txs_t(xtxpool_table_info_t * table_para) : m_xtable_info(table_para) {
    }
    const std::shared_ptr<xtx_entry> pop_tx(const tx_info_t & txinfo, bool & exist);
    const bool try_push_tx(const std::shared_ptr<xtx_entry> & tx);
    void update(const locked_tx_map_t & locked_tx_map, std::vector<std::shared_ptr<xtx_entry>> & unlocked_txs);

private:
    xtxpool_table_info_t * m_xtable_info;
    locked_tx_map_t m_locked_tx_map;
};

}  // namespace xtxpool_v2
}  // namespace top
