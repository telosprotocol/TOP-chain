// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xuncommit_txs.h"

#include "xtxpool_v2/xtxpool_log.h"

NS_BEG2(top, xtxpool_v2)

const std::string & xuncommit_block_txs_t::get_block_hash() const {
    return m_block_hash;
}

const std::vector<xcons_transaction_ptr_t> xuncommit_block_txs_t::get_send_txs() const {
    std::vector<xcons_transaction_ptr_t> send_txs;
    for (auto & tx : m_send_txs) {
        send_txs.push_back(tx.second);
    }
    return send_txs;
}
const std::vector<xcons_transaction_ptr_t> xuncommit_block_txs_t::get_receipts() const {
    std::vector<xcons_transaction_ptr_t> receipts;
    for (auto & tx : m_receipts) {
        receipts.push_back(tx.second);
    }
    return receipts;
}

xcons_transaction_ptr_t xuncommit_block_txs_t::query_tx(const std::string tx_hash) const {
    auto it_send = m_send_txs.find(tx_hash);
    if (it_send != m_send_txs.end()) {
        return it_send->second;
    }

    auto it_receipt = m_receipts.find(tx_hash);
    if (it_receipt != m_receipts.end()) {
        return it_receipt->second;
    }
    return nullptr;
}

enum_need_update_block_t xuncommit_txs_t::pop_recovered_block_txs(uint64_t cert_height,
                                                                  const std::string & cert_block_hash,
                                                                  const std::string & lock_block_hash,
                                                                  std::vector<xcons_transaction_ptr_t> & send_txs,
                                                                  std::vector<xcons_transaction_ptr_t> & receipts) {
    std::lock_guard<std::mutex> lck(m_mutex);
    if (m_block_txs_map.empty()) {
        return update_cert_and_lock;
    }
    auto iter_cert = m_block_txs_map.rbegin();
    uint64_t local_cert_height = iter_cert->first;
    auto & local_cert_hash = iter_cert->second.get_block_hash();

    if (cert_height > local_cert_height + 1) {
        xdbg("xuncommit_block_txs_t::pop_recovered_block_txs table:%s cert height jumped.cert height:%llu, local cert height:%llu",
             m_table_addr.c_str(),
             cert_height,
             local_cert_height);
        m_block_txs_map.clear();
        return update_cert_and_lock;
    } else if (cert_height == local_cert_height + 1) {
        if (lock_block_hash == local_cert_hash) {
            // no fork happened
            xdbg("xuncommit_block_txs_t::pop_recovered_block_txs table:%s cert height normally update.cert height:%llu", m_table_addr.c_str(), cert_height, local_cert_height);
            for (auto iter = m_block_txs_map.begin(); iter != m_block_txs_map.end();) {
                auto & height = iter->first;
                if (height < local_cert_height) {
                    iter = m_block_txs_map.erase(iter);
                } else {
                    break;
                }
            }
            return update_cert_only;
        } else {
            // block forked
            xdbg("xuncommit_block_txs_t::pop_recovered_block_txs table:%s lock forked.cert height:%llu", m_table_addr.c_str(), cert_height);
            send_txs = iter_cert->second.get_send_txs();
            receipts = iter_cert->second.get_receipts();
            m_block_txs_map.clear();
            return update_cert_and_lock;
        }
    } else if (cert_height == local_cert_height) {
        if (cert_block_hash == local_cert_hash) {
            return no_need_update;
        } else {
            // block forked
            uint64_t lock_height = local_cert_height - 1;
            auto iter_lock = m_block_txs_map.find(lock_height);
            if (iter_lock != m_block_txs_map.end()) {
                auto & local_lock_hash = iter_lock->second.get_block_hash();
                if (local_lock_hash == lock_block_hash) {
                    // cert forked.
                    xdbg("xuncommit_block_txs_t::pop_recovered_block_txs table:%s cert forked.cert height %llu", m_table_addr.c_str(), local_cert_height);
                    send_txs = iter_cert->second.get_send_txs();
                    receipts = iter_cert->second.get_receipts();
                    m_block_txs_map.erase(local_cert_height);
                    return update_cert_only;
                } else {
                    // cert and lock forked.
                    xdbg("xuncommit_block_txs_t::pop_recovered_block_txs table:%s cert and lock forked.cert height %llu", m_table_addr.c_str(), local_cert_height);
                    send_txs = iter_cert->second.get_send_txs();
                    receipts = iter_cert->second.get_receipts();
                    auto & lock_send_txs = iter_lock->second.get_send_txs();
                    auto & lock_receipts = iter_lock->second.get_receipts();
                    send_txs.insert(send_txs.begin(), lock_send_txs.begin(), lock_send_txs.end());
                    receipts.insert(receipts.begin(), lock_receipts.begin(), lock_receipts.end());
                    m_block_txs_map.clear();
                    return update_cert_and_lock;
                }
            } else {
                // cert forked, has no lock cache
                xdbg("xuncommit_block_txs_t::pop_recovered_block_txs table:%s cert forked.cert height %llu", m_table_addr.c_str(), local_cert_height);
                send_txs = iter_cert->second.get_send_txs();
                receipts = iter_cert->second.get_receipts();
                m_block_txs_map.clear();
                return update_cert_and_lock;
            }
        }
    } else {
        // should not happen: cert height is lower than local cert height
        xwarn("xuncommit_block_txs_t::pop_recovered_block_txs table:%s cert height lower than local. cert:%llu local:%llu", m_table_addr.c_str(), cert_height, local_cert_height);
        return no_need_update;
    }
}

void xuncommit_txs_t::update_block_txs(uint64_t height,
                                       const std::string & block_hash,
                                       const std::map<std::string, xcons_transaction_ptr_t> & send_txs,
                                       const std::map<std::string, xcons_transaction_ptr_t> & receipts) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_block_txs_map.emplace(height, xuncommit_block_txs_t(block_hash, send_txs, receipts));
}

xcons_transaction_ptr_t xuncommit_txs_t::query_tx(const std::string tx_hash) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    for (auto & block_txs : m_block_txs_map) {
        auto tx = block_txs.second.query_tx(tx_hash);
        if (tx != nullptr) {
            return tx;
        }
    }
    return nullptr;
}

NS_END2
