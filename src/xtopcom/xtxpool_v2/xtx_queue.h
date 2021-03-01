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
#include <vector>

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

class send_tx_queue_comp {
public:
    bool operator()(const std::shared_ptr<xtx_entry> left, const std::shared_ptr<xtx_entry> right) const {
        if (left->get_tx()->get_source_addr() == right->get_tx()->get_source_addr()) {
            return left->get_tx()->get_transaction()->get_last_nonce() < right->get_tx()->get_transaction()->get_last_nonce();
        }

        bool is_sys_l = data::is_sys_contract_address(common::xaccount_address_t{left->get_tx()->get_source_addr()});
        bool is_sys_r = data::is_sys_contract_address(common::xaccount_address_t{right->get_tx()->get_source_addr()});
        if (is_sys_l != is_sys_r) {
            return is_sys_l;
        }

        if (left->get_para().get_tx_type_score() == right->get_para().get_tx_type_score()) {
            if (left->get_para().get_charge_score() == right->get_para().get_charge_score()) {
                return left->get_para().get_timestamp() < right->get_para().get_timestamp();
            }
            return left->get_para().get_charge_score() > right->get_para().get_charge_score();
        }
        return left->get_para().get_tx_type_score() > right->get_para().get_tx_type_score();
    }
};

class xreceipt_queue_comp {
public:
    bool operator()(const std::shared_ptr<xtx_entry> left, const std::shared_ptr<xtx_entry> right) const {
        auto l_addr = left->get_tx()->is_recv_tx() ? left->get_tx()->get_target_addr() : left->get_tx()->get_source_addr();
        auto r_addr = right->get_tx()->is_recv_tx() ? right->get_tx()->get_target_addr() : right->get_tx()->get_source_addr();

        bool is_sys_l = data::is_sys_contract_address(common::xaccount_address_t{l_addr});
        bool is_sys_r = data::is_sys_contract_address(common::xaccount_address_t{r_addr});
        if (is_sys_l != is_sys_r) {
            return is_sys_l;
        }

        if (left->get_para().get_tx_type_score() == right->get_para().get_tx_type_score()) {
            if (left->get_para().get_timestamp() == right->get_para().get_timestamp()) {
                return left->get_para().get_charge_score() > right->get_para().get_charge_score();
            }
            return left->get_para().get_timestamp() < right->get_para().get_timestamp();
        }
        return left->get_para().get_tx_type_score() > right->get_para().get_tx_type_score();
    }
};

using xsend_tx_queue = std::multiset<std::shared_ptr<xtx_entry>, send_tx_queue_comp>;
using xreceipt_queue = std::multiset<std::shared_ptr<xtx_entry>, xreceipt_queue_comp>;

class xtx_queue_t {
public:
    xtx_queue_t(xtxpool_table_info_t * table_para) : m_xtable_info(table_para) {
    }
    int32_t push_tx(std::shared_ptr<xtx_entry> & tx_ent);
    std::vector<std::shared_ptr<xtx_entry>> pop_send_txs(uint32_t count = 3);
    std::vector<std::shared_ptr<xtx_entry>> pop_receipts(uint32_t count = 6);
    std::shared_ptr<xtx_entry> pop_tx_by_hash(std::string account, const uint256_t & hash, uint8_t subtype);
    const xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const;
    void set_delay_accout(const std::string & account_addr, uint8_t subtype);
    void refreash_delay_accounts();

private:
    int32_t push_receipt(const std::string & hash_str, std::shared_ptr<xtx_entry> & tx_ent);
    int32_t push_send_tx(const std::string & hash_str, std::shared_ptr<xtx_entry> & tx_ent);
    int32_t push_send_tx(const std::string & hash_str, std::shared_ptr<xtx_entry> & tx_ent, std::vector<xsend_tx_queue::iterator> & continuous_txs);
    xsend_tx_queue::iterator erase_send_tx(xsend_tx_queue::iterator send_tx_it);
    xreceipt_queue::iterator erase_receipt(xreceipt_queue::iterator receipt_it);
    void clear_old_send_txs(std::vector<xsend_tx_queue::iterator> & continuous_txs, uint32_t from_idx, const std::shared_ptr<xtx_entry> & tx_ent);
    void refreash_delay_accounts(std::unordered_map<std::string, uint8_t> & accounts_map);

    xsend_tx_queue m_send_tx_queue;
    xreceipt_queue m_receipt_queue;
    std::map<std::string, xsend_tx_queue::iterator> m_send_tx_map;                    // be easy to find send tx by hash
    std::map<std::string, xreceipt_queue::iterator> m_receipt_map;                    // be easy to find receipt by hash
    std::map<std::string, std::vector<xsend_tx_queue::iterator>> m_send_tx_accounts;  // key:account address, value:nonce continuous txs
    std::unordered_map<std::string, uint8_t> m_send_tx_delay_accounts;  // key:account address; value: delay time, initial is 3, decrease when pack , if decrease to 0, remove from
                                                                        // map.
    std::unordered_map<std::string, uint8_t> m_recv_tx_delay_accounts;
    std::unordered_map<std::string, uint8_t> m_conf_tx_delay_accounts;
    xtxpool_table_info_t * m_xtable_info;
};

}  // namespace xtxpool_v2
}  // namespace top
