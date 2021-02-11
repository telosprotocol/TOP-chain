// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xdata/xblock.h"
#include "xdata/xchain_param.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction.h"
#include "xtxpool/xtxpool_resources_face.h"

#include <set>
#include <string>
#include <vector>

NS_BEG2(top, xtxpool)

using data::xblock_t;
using data::xcons_transaction_ptr_t;
using data::xcons_transaction_t;
using data::xtransaction_t;

using xaccount_tx_map_t = std::unordered_map<std::string, xcons_transaction_ptr_t>;

struct xsendtx_queue_entry_t {
    xcons_transaction_ptr_t m_tx{nullptr};

    explicit xsendtx_queue_entry_t(const xcons_transaction_ptr_t & tx) : m_tx(tx) {}

    bool operator<(const xsendtx_queue_entry_t & right) const {
        if (m_tx->get_transaction()->get_last_nonce() == right.m_tx->get_transaction()->get_last_nonce()) {
            return m_tx->get_transaction()->get_fire_timestamp() > right.m_tx->get_transaction()->get_fire_timestamp();
        }
        return m_tx->get_transaction()->get_last_nonce() < right.m_tx->get_transaction()->get_last_nonce();
    }
    void print() const {
        xinfo("[accountobj all txs]send from: %s to: %s fire: %lld nonce: %lld hash: %s",
              m_tx->get_transaction()->get_source_addr().c_str(),
              m_tx->get_transaction()->get_target_addr().c_str(),
              m_tx->get_transaction()->get_fire_timestamp(),
              m_tx->get_transaction()->get_last_nonce(),
              m_tx->get_transaction()->get_digest_hex_str().c_str());
    }
};

using xsendtx_queue_t = std::multiset<xsendtx_queue_entry_t>;

struct xrecvtx_queue_entry_t {
    xcons_transaction_ptr_t m_tx{nullptr};

    explicit xrecvtx_queue_entry_t(const xcons_transaction_ptr_t & tx) : m_tx(tx) {}
    bool operator<(const xrecvtx_queue_entry_t & right) const { return m_tx->get_transaction()->get_fire_timestamp() < right.m_tx->get_transaction()->get_fire_timestamp(); }
    void print() const {
        xinfo("[accountobj all txs]recv from: %s to: %s fire: %lld nonce: %lld hash: %s",
              m_tx->get_transaction()->get_source_addr().c_str(),
              m_tx->get_transaction()->get_target_addr().c_str(),
              m_tx->get_transaction()->get_fire_timestamp(),
              m_tx->get_transaction()->get_last_nonce(),
              m_tx->get_transaction()->get_digest_hex_str().c_str());
    }
};
using xrecvtx_queue_t = std::multiset<xrecvtx_queue_entry_t>;

class tx_table {
public:
    virtual void send_tx_increment(int64_t inc_num) = 0;
    virtual void recv_tx_increment(int64_t inc_num) const = 0;
    virtual void conf_tx_increment(int64_t inc_num) const = 0;
    virtual const std::string & get_table_name() const = 0;
};

class xaccountobj_t : public base::xobject_t {
public:
    explicit xaccountobj_t(const std::string & address, uint64_t account_tx_nonce, const uint256_t & account_tx_hash, tx_table * table);

protected:
    virtual ~xaccountobj_t();

private:
    xaccountobj_t(const xaccountobj_t &);
    xaccountobj_t & operator=(const xaccountobj_t &);

public:
    std::string                          get_account_address() const { return m_address; }
    int32_t                              push_send_tx(const xcons_transaction_ptr_t & tx);
    int32_t                              push_recv_tx(const xcons_transaction_ptr_t & tx);
    int32_t                              push_recv_ack_tx(const xcons_transaction_ptr_t & tx);
    void                                 on_unit_to_db(xblock_t * block, uint64_t account_tx_nonce, const uint256_t & account_tx_hash);
    xcons_transaction_ptr_t              get_tx();
    xcons_transaction_ptr_t              get_tx_by_hash(const uint256_t & hash, uint8_t subtype);
    xcons_transaction_ptr_t              pop_tx();
    xcons_transaction_ptr_t              pop_tx_by_hash(const uint256_t & hash, uint8_t subtype, int32_t result);
    void                                 clear_invalid_send_tx(uint64_t now);
    std::vector<xcons_transaction_ptr_t> get_cons_txs(uint64_t account_tx_nonce, const uint256_t & account_tx_hash);
    bool                                 empty() const { return m_tx_map.empty(); }
    size_t                               get_queue_size() const { return m_tx_map.size(); }
    uint32_t                             get_send_tx_queue_max_num() const { return m_send_tx_queue_max_num; }
    uint64_t                             get_selected_time() const { return m_selected_time; }
    void                                 clear_selected_time() { m_selected_time = 0; }
    xcons_transaction_ptr_t              query_tx(const uint256_t & hash) const;

private:
    void     clear_send_queue_by_iter(std::multiset<xsendtx_queue_entry_t>::iterator iter, int32_t result);
    void     clear_expire_txs(uint64_t now);
    bool     check_send_tx(const xcons_transaction_ptr_t & tx, const uint256_t & latest_hash);
    uint32_t get_recv_ack_txs(std::vector<xcons_transaction_ptr_t> & txs);
    uint32_t get_recv_txs(std::vector<xcons_transaction_ptr_t> & cons_txs);
    uint32_t get_send_txs(uint64_t account_tx_nonce, const uint256_t & account_tx_hash, std::vector<xcons_transaction_ptr_t> & txs);
    int32_t  check_and_erase_old_nonce_duplicate_tx(const xtransaction_t * tx);
    int32_t  check_send_queue_full_and_proc(const xtransaction_t * tx);
    bool     update_latest_send_trans_number_hash(uint64_t account_tx_nonce, const uint256_t & account_tx_hash);
    void     pop_tx_event(const xcons_transaction_ptr_t & cons_tx, bool is_send, int32_t result, bool clear_map);
    void     drop_invalid_tx(const xcons_transaction_ptr_t & cons_tx, int32_t result);

    std::string       m_address;
    uint32_t          m_send_tx_queue_max_num;  // TODO(nathan):can be static
    uint64_t          m_selected_time{0};       // selected time that when pack tx to consensus
    uint64_t          m_latest_send_trans_number{0};
    uint256_t         m_latest_send_trans_hash{};
    xsendtx_queue_t   m_send_queue;
    xrecvtx_queue_t   m_recv_queue;
    xrecvtx_queue_t   m_recv_ack_queue;
    xaccount_tx_map_t m_tx_map;
    tx_table *        m_table;
};

class accountobj_default_comp {
public:
    bool operator()(const xaccountobj_t * laccount, const xaccountobj_t * raccount) const { return laccount->get_selected_time() <= raccount->get_selected_time(); }
};

NS_END2
