// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xdata/xcons_transaction.h"

#include <inttypes.h>

NS_BEG2(top, xtxpool_v2)

using data::xcons_transaction_ptr_t;

class xuncommit_block_txs_t {
public:
    xuncommit_block_txs_t(const std::string & block_hash,
                          const std::map<std::string, xcons_transaction_ptr_t> & send_txs,
                          const std::map<std::string, xcons_transaction_ptr_t> & receipts)
      : m_block_hash(block_hash), m_send_txs(send_txs), m_receipts(receipts) {
    }
    const std::string & get_block_hash() const;
    const std::vector<xcons_transaction_ptr_t> get_send_txs() const;
    const std::vector<xcons_transaction_ptr_t> get_receipts() const;
    xcons_transaction_ptr_t query_tx(const std::string tx_hash) const;

private:
    std::string m_block_hash;
    std::map<std::string, xcons_transaction_ptr_t> m_send_txs;
    std::map<std::string, xcons_transaction_ptr_t> m_receipts;
};

enum enum_need_update_block_t {
    no_need_update = 0,
    update_cert_only = 1,
    update_cert_and_lock = 2,
};

class xuncommit_txs_t {
public:
    xuncommit_txs_t(const std::string & table_addr) : m_table_addr(table_addr) {
    }

public:
    enum_need_update_block_t pop_recovered_block_txs(uint64_t cert_height,
                                                     const std::string & cert_block_hash,
                                                     const std::string & lock_block_hash,
                                                     std::vector<xcons_transaction_ptr_t> & send_txs,
                                                     std::vector<xcons_transaction_ptr_t> & receipts);
    void update_block_txs(uint64_t height,
                          const std::string & block_hash,
                          const std::map<std::string, xcons_transaction_ptr_t> & send_txs,
                          const std::map<std::string, xcons_transaction_ptr_t> & receipts);

    xcons_transaction_ptr_t query_tx(const std::string tx_hash) const;

private:
    std::string m_table_addr;
    mutable std::mutex m_mutex;
    std::map<uint64_t, xuncommit_block_txs_t> m_block_txs_map;
};

NS_END2
