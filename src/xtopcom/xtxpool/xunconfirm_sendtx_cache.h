// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction.h"
#include "xstore/xstore_face.h"
#include "xtxpool/xtxpool_resources_face.h"
#include "xtxpool/xtxpool_receipt_receiver_counter.h"

#include <map>
#include <set>
#include <string>
#include <unordered_map>

NS_BEG2(top, xtxpool)

using data::xcons_transaction_ptr_t;
using store::xstore_face_ptr_t;

const uint32_t MAX_ONCE_PROCESS_RETRY_SENDTX_RECEIPT = 50;

class xunconfirm_sendtx_account_cache_t {
public:
    void     push_receipt(const std::string & table_addr, const std::string & address, const uint64_t & unit_height, const uint256_t & txhash, const xcons_transaction_ptr_t & receipt);
    void     pop_receipt(const std::string & table_addr, const std::string & address, const uint64_t & unit_height, const uint256_t & txhash);
    void     pop_all_receipt(const std::string & table_addr, const std::string & address);
    void     set_cache_max_height(uint64_t height);
    uint64_t get_cache_max_height() const { return m_cache_max_height; }
    void     set_account_max_height(uint64_t height);
    uint64_t get_account_max_height() const { return m_account_max_height; }
    bool     cache_complete() const { return m_cache_max_height == m_account_max_height; }
    bool     empty() const { return m_receipts.empty(); }
    bool     has_txhash_receipt(const uint256_t & txhash);
    size_t   cache_size() { return m_receipts.size(); }
    const xcons_transaction_ptr_t get_tx(const uint256_t & txhash) const;

private:
    uint64_t                                     m_account_max_height{0};
    uint64_t                                     m_cache_max_height{0};
    std::map<uint256_t, xcons_transaction_ptr_t> m_receipts;
};

using xunconfirm_sendtx_account_cache_ptr_t = std::shared_ptr<xunconfirm_sendtx_account_cache_t>;

struct xunconfirm_sendtx_retry_cache_entry_t {
    xcons_transaction_ptr_t m_receipt{nullptr};
    uint256_t               m_txhash;
    mutable uint64_t        m_retry_send_timeout_timestamp{0};

    explicit xunconfirm_sendtx_retry_cache_entry_t(const xcons_transaction_ptr_t & receipt, const uint256_t & txhash) : m_receipt(receipt), m_txhash(txhash) {}
    bool     receipt_has_tx() const { return m_receipt->get_transaction() != nullptr; }
    bool     operator<(const xunconfirm_sendtx_retry_cache_entry_t & right) const { return m_retry_send_timeout_timestamp < right.m_retry_send_timeout_timestamp; }
    uint64_t get_timestamp() const { return m_retry_send_timeout_timestamp; }
    void     set_timestamp(uint64_t timestamp) const { m_retry_send_timeout_timestamp = timestamp; }
};

using repeat_cache_t = std::unordered_map<std::string, xunconfirm_sendtx_account_cache_ptr_t>;
using retry_cache_t = std::multiset<xunconfirm_sendtx_retry_cache_entry_t>;

class xunconfirm_sendtx_cache_t {
public:
    explicit xunconfirm_sendtx_cache_t(const std::shared_ptr<xtxpool_resources_face> &para, std::string & table_addr);

    int32_t                               has_txhash_receipt(const std::string & address, const uint256_t & txhash, uint64_t now, uint64_t unit_height = 0);
    size_t                                cache_account_size() const { return m_accounts_cache.size(); }  // for test
    xunconfirm_sendtx_account_cache_ptr_t get_account_cache(const std::string & address) const;           // for test
    std::vector<xcons_transaction_ptr_t>  on_timer_check_cache(uint64_t now, xtxpool_receipt_receiver_counter & counter);
    uint64_t                              get_account_cache_height(const std::string & address) const;  // for test
    void                                  on_unit_update(const std::string & address, uint64_t now);
    void                                  clear();
    int32_t                               get_unconfirm_tx_num() const { return m_unconfirm_tx_num; }
    xcons_transaction_ptr_t               get_unconfirm_tx(const std::string source_addr, const uint256_t & hash, uint64_t now);
    void                                  accounts_update(std::set<std::string> accounts, uint64_t now);

private:
    int32_t check_and_update_cache(const std::string & address, uint64_t now, uint64_t unit_height);
    void    update_cache(xunconfirm_sendtx_account_cache_ptr_t & account_cache, const std::string & address, uint64_t now, base::xauto_ptr<base::xvblock_t> & last_committed_unit);
    void    traverse_and_get_unconfirm_units(const std::string & address, uint64_t begin_height, std::vector<xblock_t *> & units, base::xauto_ptr<base::xvblock_t> & last_committed_unit);
    void    traverse_units_and_update_receipt_cache(const std::vector<xblock_t *> & units, xunconfirm_sendtx_account_cache_ptr_t & account_cache, uint64_t now);
    void    insert_retry_cache(const xcons_transaction_ptr_t & receipt, const uint256_t & txhash, uint64_t now);
    void    refresh_receipt_retry_timestamp(const xunconfirm_sendtx_retry_cache_entry_t & entry, uint64_t now);
    void    clear_account_cache(const std::string & address);
    bool    is_account_cache_complete(const std::string & address);
    xunconfirm_sendtx_account_cache_ptr_t find_and_create_account_cache(const std::string & address, uint64_t account_height);

    std::shared_ptr<xtxpool_resources_face> m_para;
    repeat_cache_t                          m_accounts_cache;  // for repeat check
    retry_cache_t                           m_retry_cache;     // for retry send, order by timestamp
    std::atomic<int32_t>                    m_unconfirm_tx_num{0};
    std::mutex                              m_cache_mutex;
    std::string                             m_table_addr;
};
using xunconfirm_sendtx_cache_ptr_t = std::shared_ptr<xunconfirm_sendtx_cache_t>;

NS_END2
