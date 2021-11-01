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

class xready_send_tx_queue_comp {
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

class xready_receipt_queue_comp {
public:
    bool operator()(const std::shared_ptr<xtx_entry> left, const std::shared_ptr<xtx_entry> right) const {
        if (left->get_para().get_tx_type_score() == right->get_para().get_tx_type_score()) {
            if (left->get_para().get_timestamp() == right->get_para().get_timestamp()) {
                return left->get_para().get_charge_score() > right->get_para().get_charge_score();
            }
            return left->get_para().get_timestamp() < right->get_para().get_timestamp();
        }
        return left->get_para().get_tx_type_score() > right->get_para().get_tx_type_score();
    }
};

class xnon_ready_send_tx_queue_comp {
public:
    bool operator()(const std::shared_ptr<xtx_entry> left, const std::shared_ptr<xtx_entry> right) const {
        return left->get_tx()->get_transaction()->get_fire_timestamp() < right->get_tx()->get_transaction()->get_fire_timestamp();
    }
};

using xready_send_tx_queue_t = std::multiset<std::shared_ptr<xtx_entry>, xready_send_tx_queue_comp>;
using xready_receipt_queue_t = std::multiset<std::shared_ptr<xtx_entry>, xready_receipt_queue_comp>;
using xnon_ready_send_tx_queue_t = std::multiset<std::shared_ptr<xtx_entry>, xnon_ready_send_tx_queue_comp>;
using xready_send_tx_map_t = std::map<std::string, xready_send_tx_queue_t::iterator>;
using xready_receipt_map_t = std::map<std::string, xready_receipt_queue_t::iterator>;
using xnon_ready_send_tx_map_t = std::map<std::string, xnon_ready_send_tx_queue_t::iterator>;

class xsend_tx_queue_internal_t {
public:
    xsend_tx_queue_internal_t(xtxpool_table_info_t * xtable_info) : m_xtable_info(xtable_info) {
    }
    void insert_ready_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    void insert_non_ready_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    void erase_ready_tx(const uint256_t & hash);
    void erase_non_ready_tx(const uint256_t & hash);
    const std::shared_ptr<xtx_entry> find(const uint256_t & hash) const;
    const std::shared_ptr<xtx_entry> pick_to_be_droped_tx() const;
    const std::vector<std::shared_ptr<xtx_entry>> get_expired_txs() const;
    const xready_send_tx_queue_t & get_ready_queue() const {
        return m_ready_tx_queue;
    }
    uint32_t size() const {
        return m_ready_tx_queue.size() + m_non_ready_tx_queue.size();
    }
    uint32_t non_ready_size() const {
        return m_non_ready_tx_queue.size();
    }
    int32_t check_full() const {
        return m_xtable_info->check_send_tx_reached_upper_limit();
    }

private:
    xready_send_tx_queue_t m_ready_tx_queue;
    xready_send_tx_map_t m_ready_tx_map;  // be easy to find send tx by hash
    xnon_ready_send_tx_queue_t m_non_ready_tx_queue;
    xnon_ready_send_tx_map_t m_non_ready_tx_map;  // be easy to find send tx by hash
    xtxpool_table_info_t * m_xtable_info;
};

class xcontinuous_txs_t {
public:
    // continuous txs must always keep nonce and hash continuity!
    xcontinuous_txs_t(xsend_tx_queue_internal_t * send_tx_queue_internal, uint64_t latest_nonce)
      : m_send_tx_queue_internal(send_tx_queue_internal), m_latest_nonce(latest_nonce) {
    }
    uint64_t get_back_nonce() const;
    void update_latest_nonce(uint64_t latest_nonce);
    int32_t insert(std::shared_ptr<xtx_entry> tx_ent);
    const std::vector<std::shared_ptr<xtx_entry>> get_txs(uint64_t upper_nonce, uint32_t max_num) const;
    // must call pop_uncontinuous_txs after call erase to keep m_txs continue with m_latest_nonce
    void erase(uint64_t nonce, bool clear_follower);
    const std::vector<std::shared_ptr<xtx_entry>> pop_uncontinuous_txs();
    bool empty() const {
        return m_txs.empty();
    }
    uint64_t get_latest_nonce() const {
        return m_latest_nonce;
    }

private:
    int32_t nonce_check(uint64_t last_nonce);
    void batch_erase(uint32_t from_idx, uint32_t to_idx);  // erase scope: [from_idx, to_idx), not include to_idx
    std::vector<std::shared_ptr<xtx_entry>> m_txs;
    xsend_tx_queue_internal_t * m_send_tx_queue_internal;
    uint64_t m_latest_nonce;
};

class xuncontinuous_txs_t {
public:
    xuncontinuous_txs_t(xsend_tx_queue_internal_t * send_tx_queue_internal) : m_send_tx_queue_internal(send_tx_queue_internal) {
    }
    int32_t insert(std::shared_ptr<xtx_entry> tx_ent);
    const std::shared_ptr<xtx_entry> pop_by_last_nonce(uint64_t last_nonce);
    void erase(uint64_t nonce);
    bool empty() const {
        return m_txs.empty();
    }

private:
    std::map<uint64_t, std::shared_ptr<xtx_entry>> m_txs;
    xsend_tx_queue_internal_t * m_send_tx_queue_internal;
};

class xsend_tx_account_t {
public:
    xsend_tx_account_t(xsend_tx_queue_internal_t * send_tx_queue_internal, uint64_t latest_nonce)
      : m_continuous_txs(send_tx_queue_internal, latest_nonce), m_uncontinuous_txs(send_tx_queue_internal) {
    }
    int32_t push_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    void update_latest_nonce(uint64_t latest_nonce);
    void refresh();
    const std::vector<std::shared_ptr<xtx_entry>> get_continuous_txs(uint64_t upper_nonce, uint32_t max_num) const;
    void erase(uint64_t nonce, bool clear_follower);
    bool empty() const {
        return m_continuous_txs.empty() && m_uncontinuous_txs.empty();
    }
    bool need_update() const {
        return m_continuous_txs.empty() && (!m_uncontinuous_txs.empty());
    }
    uint64_t get_latest_nonce() const {
        return m_continuous_txs.get_latest_nonce();
    }

private:
    void try_continue();
    xcontinuous_txs_t m_continuous_txs;
    xuncontinuous_txs_t m_uncontinuous_txs;
};

#define xtxpool_account_nonce_cache_lru_size (512)

class xsend_tx_queue_t {
public:
    xsend_tx_queue_t(xtxpool_table_info_t * xtable_info) : m_send_tx_queue_internal(xtable_info), m_account_nonce_lru(xtxpool_account_nonce_cache_lru_size) {
    }
    int32_t push_tx(const std::shared_ptr<xtx_entry> & tx_ent, uint64_t latest_nonce);
    const std::vector<std::shared_ptr<xtx_entry>> get_txs(uint32_t max_num) const;
    const std::shared_ptr<xtx_entry> pop_tx(const tx_info_t & txinfo, bool clear_follower);
    const std::shared_ptr<xtx_entry> find(const std::string & account_addr, const uint256_t & hash) const;
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce);
    bool is_account_need_update(const std::string & account_addr) const;
    void clear_expired_txs();
    bool get_account_nonce_cache(const std::string & account_addr, uint64_t & latest_nonce) const;
    uint32_t size() const {
        return m_send_tx_queue_internal.size();
    }
    uint32_t non_ready_size() const {
        return m_send_tx_queue_internal.non_ready_size();
    }

private:
    xsend_tx_queue_internal_t m_send_tx_queue_internal;
    std::map<std::string, std::shared_ptr<xsend_tx_account_t>> m_send_tx_accounts;
    mutable basic::xlru_cache<std::string, uint64_t> m_account_nonce_lru;
};

}  // namespace xtxpool_v2
}  // namespace top
