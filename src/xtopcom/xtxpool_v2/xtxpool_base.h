// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xlru_cache.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"

#include <map>
#include <set>
#include <string>

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

using namespace top::data;

class xtx_para_t {
public:
    xtx_para_t() {}
    xtx_para_t(uint16_t charge_score, uint16_t type_score, uint64_t timestamp, uint64_t check_unit_height, std::string check_unit_hash)
      : m_charge_score(charge_score), m_type_score(type_score), m_timestamp(timestamp), m_check_unit_height(check_unit_height), m_check_unit_hash(check_unit_hash) {
    }
    void set_charge_score(uint16_t score) {
        m_charge_score = score;
    }
    uint16_t get_charge_score() const {
        return m_charge_score;
    }
    void set_tx_type_score(uint16_t score) {
        m_type_score = score;
    }
    uint16_t get_tx_type_score() const {
        return m_type_score;
    }
    void set_timestamp(uint64_t timestamp) {
        m_timestamp = timestamp;
    }
    uint64_t get_timestamp() const {
        return m_timestamp;
    }
    void set_check_unit_height(uint64_t check_unit_height) {
        m_check_unit_height = check_unit_height;
    }
    uint64_t get_check_unit_height() const {
        return m_check_unit_height;
    }
    void set_check_unit_hash(std::string check_unit_hash) {
        m_check_unit_hash = check_unit_hash;
    }
    std::string get_check_unit_hash() const {
        return m_check_unit_hash;
    }

private:
    uint16_t m_charge_score{0};
    uint16_t m_type_score{0};
    uint64_t m_timestamp{0};
    uint64_t m_check_unit_height{0};
    std::string m_check_unit_hash;
};

class xtx_entry {
public:
    xtx_entry(const xcons_transaction_ptr_t & tx, const xtx_para_t & para) : m_tx(tx), m_para(para) {
    }
    xtx_para_t & get_para() {
        return m_para;
    }
    const xcons_transaction_ptr_t & get_tx() const {
        return m_tx;
    }

private:
    xcons_transaction_ptr_t m_tx;
    xtx_para_t m_para;
};

class xcandidate_account_entry {
public:
    xcandidate_account_entry(const std::string & account) : m_account(account) {
    }
    const std::vector<std::shared_ptr<xtx_entry>> & get_txs(enum_transaction_subtype subtype) const;
    void set_select_count(uint32_t count);
    const std::string & get_addr() const;
    int32_t add_tx(std::shared_ptr<xtx_entry> tx_ent);
    std::shared_ptr<xtx_entry> pop_tx_by_hash(const uint256_t & hash, uint8_t subtype, int32_t err);
    const xcons_transaction_ptr_t query_tx(const uint256_t & hash) const;
    uint32_t get_select_count() const;
    bool empty();

private:
    std::vector<std::shared_ptr<xtx_entry>>::iterator find_tx_ent_by_hash(std::vector<std::shared_ptr<xtx_entry>> & txs, const uint256_t & hash) const;
    bool is_duplicate(const std::vector<std::shared_ptr<xtx_entry>> & txs, std::shared_ptr<xtx_entry> tx, bool is_send_tx) const;

    std::string m_account;
    mutable std::vector<std::shared_ptr<xtx_entry>> m_send_txs;
    mutable std::vector<std::shared_ptr<xtx_entry>> m_recv_txs;
    mutable std::vector<std::shared_ptr<xtx_entry>> m_conf_txs;
    uint32_t m_selected_count{0};
};
using candidate_accounts = std::vector<std::shared_ptr<xcandidate_account_entry>>;

}  // namespace xtxpool_v2
}  // namespace top
