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
#include <unordered_set>
#include <vector>

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

class xsend_tx_set_comp {
public:
    bool operator()(const std::shared_ptr<xtx_entry> left, const std::shared_ptr<xtx_entry> right) const {
        if (left->get_tx()->get_source_addr() == right->get_tx()->get_source_addr()) {
            return left->get_tx()->get_transaction()->get_last_nonce() < right->get_tx()->get_transaction()->get_last_nonce();
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

class xsend_tx_time_order_set_comp {
public:
    bool operator()(const std::shared_ptr<xtx_entry> left, const std::shared_ptr<xtx_entry> right) const {
        return left->get_tx()->get_transaction()->get_fire_timestamp() < right->get_tx()->get_transaction()->get_fire_timestamp();
    }
};

using xsend_tx_set_t = std::multiset<std::shared_ptr<xtx_entry>, xsend_tx_set_comp>;
using xsend_tx_time_order_set_t = std::multiset<std::shared_ptr<xtx_entry>, xsend_tx_time_order_set_comp>;

struct xsed_tx_set_iters_t {
    xsed_tx_set_iters_t() {
    }
    xsed_tx_set_iters_t(xsend_tx_set_t::iterator send_tx_set_iter, xsend_tx_time_order_set_t::iterator send_tx_time_order_set_iter)
      : m_send_tx_set_iter(send_tx_set_iter), m_send_tx_time_order_set_iter(send_tx_time_order_set_iter) {
    }
    xsend_tx_set_t::iterator m_send_tx_set_iter;
    xsend_tx_time_order_set_t::iterator m_send_tx_time_order_set_iter;
};

using xsend_tx_map_t = std::map<std::string, xsed_tx_set_iters_t>;

class xsend_tx_queue_internal_t {
public:
    xsend_tx_queue_internal_t(xtxpool_table_info_t * xtable_info) : m_xtable_info(xtable_info) {
    }
    void insert_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    void erase_tx(const uint256_t & hash);
    const std::shared_ptr<xtx_entry> find(const std::string & hash) const;
    const std::shared_ptr<xtx_entry> pick_to_be_droped_tx() const;
    const std::vector<std::shared_ptr<xtx_entry>> get_expired_txs() const;
    const xsend_tx_set_t & get_queue() const {
        return m_tx_set;
    }
    uint32_t size() const {
        return m_tx_set.size();
    }
    int32_t check_full() const {
        return m_xtable_info->check_send_tx_reached_upper_limit();
    }
    const std::string & get_table_addr() const {
        return m_xtable_info->get_table_addr();
    }

private:
    xsend_tx_set_t m_tx_set;
    xsend_tx_time_order_set_t m_tx_time_order_set;  // for easily traverse and erase timeout send txs
    xsend_tx_map_t m_tx_map;                        // for easily find send tx by hash
    xtxpool_table_info_t * m_xtable_info;
};

class xsend_tx_account_t {
public:
    xsend_tx_account_t(xsend_tx_queue_internal_t * send_tx_queue_internal, uint64_t latest_nonce) : m_send_tx_queue_internal(send_tx_queue_internal), m_latest_nonce(latest_nonce) {
    }
    int32_t push_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    void update_latest_nonce(uint64_t latest_nonce);
    const std::vector<xcons_transaction_ptr_t> get_continuous_txs(uint32_t max_num, uint64_t upper_nonce, uint64_t lower_nonce) const;
    void erase(uint64_t nonce, bool clear_follower);
    bool empty() const {
        return m_txs.empty();
    }
    uint64_t get_latest_nonce() const {
        return m_latest_nonce;
    }

private:
    int32_t nonce_check(uint64_t last_nonce);
    std::map<uint64_t, std::shared_ptr<xtx_entry>> m_txs;
    xsend_tx_queue_internal_t * m_send_tx_queue_internal;
    uint64_t m_latest_nonce;
};

class xsend_tx_queue_t {
public:
    xsend_tx_queue_t(xtxpool_table_info_t * xtable_info) : m_send_tx_queue_internal(xtable_info) {
    }
    int32_t push_tx(const std::shared_ptr<xtx_entry> & tx_ent, uint64_t latest_nonce);
    const std::vector<xcons_transaction_ptr_t> get_txs(uint32_t max_num, base::xvblock_t * cert_block, uint32_t & expired_num, uint32_t & unconituous_num) const;
    const std::shared_ptr<xtx_entry> pop_tx(const std::string & tx_hash, bool clear_follower);
    const std::shared_ptr<xtx_entry> find(const std::string & hash) const;
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce);
    void updata_latest_nonce_by_hash(const std::string & tx_hash);
    void clear_expired_txs();
    uint32_t size() const {
        return m_send_tx_queue_internal.size();
    }

private:
    xsend_tx_queue_internal_t m_send_tx_queue_internal;
    std::map<std::string, std::shared_ptr<xsend_tx_account_t>> m_send_tx_accounts;
};

}  // namespace xtxpool_v2
}  // namespace top
