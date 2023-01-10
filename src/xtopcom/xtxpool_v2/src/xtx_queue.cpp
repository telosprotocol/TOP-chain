// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtx_queue.h"

#include "xbasic/xmodule_type.h"
#include "xdata/xdatautil.h"
#include "xdata/xtransaction.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xstatestore/xstatestore_face.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_utl.h"
#include "xvledger/xvledger.h"

namespace top {
namespace xtxpool_v2 {

using namespace top::data;

#define account_send_tx_queue_size_max (16)

// xsend_tx_queue_internal_t::~xsend_tx_queue_internal_t() {
// #ifdef CACHE_SIZE_STATISTIC
//     int32_t size = 0;
//     for (const auto & tx_ent : m_tx_set) {
//         size += tx_ent->get_tx()->get_object_size();
//     }
//     XMETRICS_GAUGE(metrics::cachesize_send_tx_queue, -size);
//     xdbg("~xsend_tx_queue_internal_t dec num =%u", m_tx_set.size());
// #endif
// }

void xsend_tx_queue_internal_t::insert_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    tx_ent->get_tx()->set_push_pool_timestamp(now);
    tx_ent->get_tx()->get_transaction()->set_fire_timestamp_ext(now);
    auto it = m_tx_set.insert(tx_ent);
    auto it_timeout_queue = m_tx_time_order_set.insert(tx_ent);
    xsed_tx_set_iters_t iters(it, it_timeout_queue);
    m_tx_map[tx_ent->get_tx()->get_tx_hash()] = iters;
    m_xtable_info->send_tx_inc(1);
// #ifdef CACHE_SIZE_STATISTIC
//     auto size = tx_ent->get_tx()->get_object_size();
//     XMETRICS_GAUGE(metrics::cachesize_send_tx_queue, size);
// #endif
    xtxpool_info("xsend_tx_queue_internal_t::insert_tx push tx to send queue,table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
}

void xsend_tx_queue_internal_t::erase_tx(const uint256_t & hash) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it_ready = m_tx_map.find(hash_str);
    if (it_ready != m_tx_map.end()) {
        auto & tx_ent = *it_ready->second.m_send_tx_set_iter;
        uint64_t delay = xverifier::xtx_utl::get_gmttime_s() - tx_ent->get_tx()->get_push_pool_timestamp();
        xtxpool_info("xsend_tx_queue_internal_t::erase_ready_tx pop tx from send queue,table:%s,tx:%s,delay:%llu",
                     m_xtable_info->get_table_addr().c_str(),
                     tx_ent->get_tx()->dump(true).c_str(),
                     delay);
// #ifdef CACHE_SIZE_STATISTIC
//         auto size = tx_ent->get_tx()->get_object_size();
//         XMETRICS_GAUGE(metrics::cachesize_send_tx_queue, -size);
// #endif
        XMETRICS_GAUGE(metrics::txpool_tx_delay_from_push_to_commit_send, delay);
        m_tx_set.erase(it_ready->second.m_send_tx_set_iter);
        m_tx_time_order_set.erase(it_ready->second.m_send_tx_time_order_set_iter);
        m_tx_map.erase(it_ready);
        m_xtable_info->send_tx_dec(1);
        return;
    }
}

const std::shared_ptr<xtx_entry> xsend_tx_queue_internal_t::find(const std::string & hash_str) const {
    auto it_ready = m_tx_map.find(hash_str);
    if (it_ready != m_tx_map.end()) {
        return *it_ready->second.m_send_tx_set_iter;
    }
    return nullptr;
}

const std::vector<std::shared_ptr<xtx_entry>> xsend_tx_queue_internal_t::get_expired_txs() const {
    std::vector<std::shared_ptr<xtx_entry>> expired_txs;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    for (auto & tx : m_tx_time_order_set) {
        auto ret = xverifier::xtx_verifier::verify_tx_fire_expiration(tx->get_tx()->get_transaction(), now, false);
        if (ret == 0) {
            break;
        }
        expired_txs.push_back(tx);
    }

    return expired_txs;
}

const std::shared_ptr<xtx_entry> xsend_tx_queue_internal_t::pick_to_be_droped_tx() const {
    if (m_tx_set.empty()) {
        return nullptr;
    }
    return *m_tx_set.rbegin();
}

int32_t xsend_tx_account_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    uint64_t tx_nonce = tx_ent->get_tx()->get_transaction()->get_tx_nonce();
    int32_t ret = nonce_check(tx_nonce);
    if (ret != xsuccess) {
        return ret;
    }

    auto iter = m_txs.find(tx_nonce);
    if (iter == m_txs.end()) {
        m_txs[tx_nonce] = tx_ent;
        m_send_tx_queue_internal->insert_tx(tx_ent);
    } else {
        if (tx_ent->get_tx()->get_transaction()->get_fire_timestamp() <= iter->second->get_tx()->get_transaction()->get_fire_timestamp()) {
            return xtxpool_error_tx_nonce_duplicate;
        } else {
            m_send_tx_queue_internal->erase_tx(iter->second->get_tx()->get_tx_hash_256());
            m_txs[tx_nonce] = tx_ent;
            m_send_tx_queue_internal->insert_tx(tx_ent);
        }
    }

    return xsuccess;
}


int32_t xsend_tx_account_t::nonce_check(uint64_t tx_nonce) {
    if (tx_nonce <= m_latest_nonce) {
        return xtxpool_error_tx_nonce_expired;
    }
    if (tx_nonce > m_latest_nonce + account_send_tx_queue_size_max) {
        return xtxpool_error_tx_nonce_out_of_scope;
    }
    return xsuccess;
}

void xsend_tx_account_t::update_latest_nonce(uint64_t latest_nonce) {
    if (latest_nonce <= m_latest_nonce) {
        return;
    }

    for (auto iter = m_txs.begin(); iter != m_txs.end();) {
        if (iter->second->get_tx()->get_transaction()->get_last_nonce() < latest_nonce) {
            m_send_tx_queue_internal->erase_tx(iter->second->get_tx()->get_tx_hash_256());
            iter = m_txs.erase(iter);
        } else {
            break;
        }
    }

    m_latest_nonce = latest_nonce;
}

const std::vector<xcons_transaction_ptr_t> xsend_tx_account_t::get_continuous_txs(uint32_t max_num, uint64_t upper_nonce, uint64_t lower_nonce) const {
    uint64_t last_nonce = (lower_nonce == 0) ? m_latest_nonce : lower_nonce;
    std::vector<xcons_transaction_ptr_t> txs;
    for (auto & iter_tx : m_txs) {
        xtxpool_dbg("xsend_tx_account_t::get_continuous_txs tx:%s,upper_nonce:%llu,lower_nonce:%llu,m_latest_nonce:%llu", iter_tx.second->get_tx()->dump().c_str(), upper_nonce, lower_nonce, m_latest_nonce);
        auto tx_nonce = iter_tx.second->get_tx()->get_transaction()->get_tx_nonce();
        if (tx_nonce > upper_nonce) {
            break;
        }
        if (tx_nonce <= last_nonce) {
            continue;
        } else if (tx_nonce == last_nonce + 1) {
            txs.push_back(iter_tx.second->get_tx());
            if (txs.size() >= max_num) {
                break;
            }
            last_nonce++;
        } else {
            break;
        }
    }
    return txs;
}

void xsend_tx_account_t::erase(uint64_t nonce, bool clear_follower) {
    // already checked tx is exist in queue, use nonce is enough to find tx here.

    if (clear_follower) {
        for (auto iter = m_txs.begin(); iter != m_txs.end();) {
            if (iter->second->get_tx()->get_transaction()->get_tx_nonce() >= nonce) {
                m_send_tx_queue_internal->erase_tx(iter->second->get_tx()->get_tx_hash_256());
                iter = m_txs.erase(iter);
            } else {
                iter++;
            }
        }
    } else {
        auto iter = m_txs.find(nonce);
        if (iter != m_txs.end()) {
            m_send_tx_queue_internal->erase_tx(iter->second->get_tx()->get_tx_hash_256());
            m_txs.erase(iter);
        }
    }
}

int32_t xsend_tx_queue_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent, uint64_t latest_nonce) {
    clear_expired_txs();
    std::shared_ptr<xtx_entry> to_be_droped_tx = nullptr;
    int32_t err = m_send_tx_queue_internal.check_full();
    if (err != xsuccess) {
        to_be_droped_tx = m_send_tx_queue_internal.pick_to_be_droped_tx();
        if (to_be_droped_tx == nullptr) {
            return err;
        }
    }

    std::shared_ptr<xsend_tx_account_t> send_tx_account;
    auto & account_addr = tx_ent->get_tx()->get_source_addr();
    auto it = m_send_tx_accounts.find(account_addr);
    if (it == m_send_tx_accounts.end()) {
        send_tx_account = std::make_shared<xsend_tx_account_t>(&m_send_tx_queue_internal, latest_nonce);
        m_send_tx_accounts[account_addr] = send_tx_account;
    } else {
        send_tx_account = it->second;
        send_tx_account->update_latest_nonce(latest_nonce);
    }
    int32_t ret = send_tx_account->push_tx(tx_ent);
    if ((ret == xsuccess) && (to_be_droped_tx != nullptr)) {
        // in case of to_be_droped_tx maybe changed to a continuous tx, pick "to be droped tx" again and drop it,
        // if there is no uncontinuous tx, dorp "to_be_droped_tx".
        auto to_be_droped_tx_after_insert = m_send_tx_queue_internal.pick_to_be_droped_tx();
        if (to_be_droped_tx_after_insert != nullptr) {
            to_be_droped_tx = to_be_droped_tx_after_insert;
        }
        pop_tx(to_be_droped_tx->get_tx()->get_tx_hash(), true);
        if (to_be_droped_tx->get_tx()->get_tx_hash_256() == tx_ent->get_tx()->get_tx_hash_256()) {
            return err;
        }
    }
    if (send_tx_account->empty()) {
        m_send_tx_accounts.erase(account_addr);
    }
    return ret;
}

const std::vector<xcons_transaction_ptr_t> xsend_tx_queue_t::get_txs(uint32_t max_num, base::xvblock_t * cert_block, uint32_t & expired_num, uint32_t & unconituous_num) const {
    std::map<std::string, std::vector<xcons_transaction_ptr_t>> account_txs_map;
    std::vector<std::string> ordered_accounts;
    auto & send_txs = m_send_tx_queue_internal.get_queue();
    uint32_t continuous_tx_num = 0;
    // uint64_t get_index_total_cost = 0;
    uint32_t nonce_expired_num = 0;
    uint32_t nonce_unconituous_num = 0;

    for (auto it_send_tx = send_txs.begin(); (continuous_tx_num < max_num) && (it_send_tx != send_txs.end()); it_send_tx++) {
        auto & account_addr = it_send_tx->get()->get_tx()->get_source_addr();
        uint64_t nonce = it_send_tx->get()->get_tx()->get_transaction()->get_tx_nonce();
        xtxpool_dbg("xsend_tx_queue_t::get_txs tx:%s", it_send_tx->get()->get_tx()->dump().c_str());

        auto it_account_txs_map = account_txs_map.find(account_addr);
        if (it_account_txs_map != account_txs_map.end()) {
            auto & account_txs = it_account_txs_map->second;
            auto lower_nonce = account_txs.back()->get_transaction()->get_tx_nonce();
            if (nonce > lower_nonce) {
                auto iter_send_tx_account = m_send_tx_accounts.find(account_addr);
                xassert(iter_send_tx_account != m_send_tx_accounts.end());
                if (iter_send_tx_account != m_send_tx_accounts.end()) {
                    auto txs = iter_send_tx_account->second->get_continuous_txs(max_num - continuous_tx_num, nonce, lower_nonce);
                    if (!txs.empty()) {
                        account_txs.insert(account_txs.end(), txs.begin(), txs.end());
                        continuous_tx_num += (txs.size());
                        xtxpool_dbg("xsend_tx_queue_t::get_txs ordered_accounts size:%u account:%s,nonce:%llu,lower_nonce:%llu", ordered_accounts.size(), account_addr.c_str(), nonce, lower_nonce);
                    }
                }
            } else {
                auto front_nonce = account_txs.front()->get_transaction()->get_tx_nonce();
                if (nonce < front_nonce) {
                    // xtxpool_warn("xsend_tx_queue_t::get_txs expired tx:%s,cert nonce:%d", it_send_tx->get()->get_tx()->dump().c_str(), front_nonce - 1);
                    nonce_expired_num++;
                }
            }
        } else {
            base::xaccount_index_t account_index;
            // uint64_t now = base::xtime_utl::time_now_ms();
            auto ret = statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(common::xaccount_address_t(account_addr), cert_block, account_index);
            // todo: test code! remove later.
            // uint64_t now_1 = base::xtime_utl::time_now_ms();
            // get_index_total_cost += (now_1 - now);
            // if (now_1 >= now + 5) {
            //     xtxpool_info("xsend_tx_queue_t::get_txs table:%s get_accountindex_from_table_block cost too much time:%llu", m_send_tx_queue_internal.get_table_addr().c_str(), now_1 - now);
            // }
            if (!ret) {
                // xwarn("xsend_tx_queue_t::get_txs mpt get account index fail account:%s", account_addr.c_str());
                continue;
            }
            auto lower_nonce = account_index.get_latest_tx_nonce();
            if (nonce > lower_nonce) {
                auto iter_send_tx_account = m_send_tx_accounts.find(account_addr);
                xassert(iter_send_tx_account != m_send_tx_accounts.end());
                if (iter_send_tx_account != m_send_tx_accounts.end()) {
                    auto txs = iter_send_tx_account->second->get_continuous_txs(max_num - continuous_tx_num, nonce, lower_nonce);
                    if (!txs.empty()) {
                        account_txs_map[account_addr] = txs;
                        continuous_tx_num += txs.size();
                        ordered_accounts.push_back(account_addr);
                    } else {
                        xtxpool_warn("xsend_tx_queue_t::get_txs uncontinuous tx:%s,cert nonce:%d", it_send_tx->get()->get_tx()->dump().c_str(), lower_nonce);
                        nonce_unconituous_num ++;
                    }
                }
                xtxpool_dbg("xsend_tx_queue_t::get_txs ordered_accounts size:%u account:%s,nonce:%llu,lower_nonce:%llu", ordered_accounts.size(), account_addr.c_str(), nonce, lower_nonce);
            } else {
                // xtxpool_warn("xsend_tx_queue_t::get_txs expired tx:%s,cert nonce:%d", it_send_tx->get()->get_tx()->dump().c_str(), lower_nonce);
                nonce_expired_num++;
            }
        }
    }

    XMETRICS_GAUGE(metrics::txpool_pack_nonce_expired, nonce_expired_num);
    XMETRICS_GAUGE(metrics::txpool_pack_nonce_uncontinuous, nonce_unconituous_num);
    expired_num = nonce_expired_num;
    unconituous_num = nonce_unconituous_num;

    // todo: test code! remove later.
    // if (get_index_total_cost >= 10) {
    //     xtxpool_info("xsend_tx_queue_t::get_txs table:%s get_accountindex_from_table_block total cost:%llu", m_send_tx_queue_internal.get_table_addr().c_str(), get_index_total_cost);
    // }

    std::vector<xcons_transaction_ptr_t> ret_txs;
    for (auto & account_addr : ordered_accounts) {
        auto iter = account_txs_map.find(account_addr);
        xassert(iter != account_txs_map.end());
        if (iter != account_txs_map.end()) {
            ret_txs.insert(ret_txs.end(), iter->second.begin(), iter->second.end());
        }
    }

    return ret_txs;
}

const std::shared_ptr<xtx_entry> xsend_tx_queue_t::pop_tx(const std::string & tx_hash, bool clear_follower) {
    auto tx_ent = m_send_tx_queue_internal.find(tx_hash);
    if (tx_ent == nullptr) {
        return nullptr;
    }

    auto addr = tx_ent->get_tx()->get_account_addr();
    auto send_tx_account = m_send_tx_accounts.find(addr);
    xassert(send_tx_account != m_send_tx_accounts.end());
    send_tx_account->second->erase(tx_ent->get_tx()->get_transaction()->get_tx_nonce(), clear_follower);

    if (send_tx_account->second->empty()) {
        m_send_tx_accounts.erase(addr);
    }
    return tx_ent;
}

const std::shared_ptr<xtx_entry> xsend_tx_queue_t::find(const std::string & account_addr, const std::string & hash_str) const {
    return m_send_tx_queue_internal.find(hash_str);
}

void xsend_tx_queue_t::updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) {
    auto send_tx_account = m_send_tx_accounts.find(account_addr);
    if (send_tx_account != m_send_tx_accounts.end()) {
        send_tx_account->second->update_latest_nonce(latest_nonce);
        if (send_tx_account->second->empty()) {
            m_send_tx_accounts.erase(account_addr);
        }
    }
}

void xsend_tx_queue_t::updata_latest_nonce_by_hash(const std::string & tx_hash) {
    auto tx_ent = m_send_tx_queue_internal.find(tx_hash);
    if (tx_ent == nullptr) {
        return;
    }

    auto addr = tx_ent->get_tx()->get_account_addr();
    auto send_tx_account = m_send_tx_accounts.find(addr);
    xassert(send_tx_account != m_send_tx_accounts.end());
    if (send_tx_account != m_send_tx_accounts.end()) {
        send_tx_account->second->update_latest_nonce(tx_ent->get_tx()->get_transaction()->get_tx_nonce());
        if (send_tx_account->second->empty()) {
            m_send_tx_accounts.erase(addr);
        }
    }
}

void xsend_tx_queue_t::clear_expired_txs() {
    auto expired_txs = m_send_tx_queue_internal.get_expired_txs();
    for (auto & tx : expired_txs) {
        pop_tx(tx->get_tx()->get_tx_hash(), false);
    }
}

}  // namespace xtxpool_v2
}  // namespace top
