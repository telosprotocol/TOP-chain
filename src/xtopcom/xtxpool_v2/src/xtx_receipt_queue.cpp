// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtx_receipt_queue.h"

#include "xbasic/xmodule_type.h"
#include "xdata/xdatautil.h"
#include "xdata/xtransaction.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_utl.h"

namespace top {
namespace xtxpool_v2 {

using namespace top::data;

xreceipt_set_t m_tx_queue;
xreceipt_map_t m_tx_map;
xtxpool_table_info_t * m_xtable_info;

void xreceipt_queue_internal_t::insert_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    auto it = m_tx_queue.insert(tx_ent);
    m_tx_map[tx_ent->get_tx()->get_transaction()->get_digest_str()] = it;
    m_xtable_info->tx_inc(tx_ent->get_tx()->get_tx_subtype(), 1);
    xtxpool_info("xreceipt_queue_internal_t::insert_tx table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
}

void xreceipt_queue_internal_t::erase_tx(const uint256_t & hash) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it_tx_map = m_tx_map.find(hash_str);
    if (it_tx_map != m_tx_map.end()) {
        auto & tx_ent = *it_tx_map->second;
        xtxpool_info("xreceipt_queue_internal_t::erase_ready_tx from ready txs,table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
        m_xtable_info->tx_dec(tx_ent->get_tx()->get_tx_subtype(), 1);
        m_tx_queue.erase(it_tx_map->second);
        m_tx_map.erase(it_tx_map);
        return;
    }
}

const std::shared_ptr<xtx_entry> xreceipt_queue_internal_t::find(const uint256_t & hash) const {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it_tx_map = m_tx_map.find(hash_str);
    if (it_tx_map != m_tx_map.end()) {
        return *it_tx_map->second;
    }
    return nullptr;
}

int32_t xpeer_table_receipts_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent, uint64_t latest_receipt_id) {
    uint64_t new_receipt_id = tx_ent->get_tx()->get_last_action_receipt_id();
    update_latest_id(latest_receipt_id);
    if (new_receipt_id <= m_latest_receipt_id) {
        xtxpool_warn("xpeer_table_receipts_t::push_tx duplicate receipt:%s,id:%llu:%llu", tx_ent->get_tx()->dump().c_str(), new_receipt_id, m_latest_receipt_id);
        return xtxpool_error_tx_duplicate;
    }
    auto it = m_txs.find(new_receipt_id);
    if (it != m_txs.end()) {
        return xtxpool_error_request_tx_repeat;
    }

    m_txs[new_receipt_id] = tx_ent;
    m_receipt_queue_internal->insert_tx(tx_ent);
    return xsuccess;
}

void xpeer_table_receipts_t::update_latest_id(uint64_t latest_receipt_id) {
    if (latest_receipt_id <= m_latest_receipt_id) {
        return;
    }
    for (auto it = m_txs.begin(); it != m_txs.end();) {
        if (it->first <= latest_receipt_id) {
            m_receipt_queue_internal->erase_tx(it->second->get_tx()->get_transaction()->digest());
            it = m_txs.erase(it);
        } else {
            break;
        }
    }
    m_latest_receipt_id = latest_receipt_id;
}

const std::vector<xcons_transaction_ptr_t> xpeer_table_receipts_t::get_txs(uint64_t upper_receipt_id, uint32_t max_num) const {
    std::vector<xcons_transaction_ptr_t> ret_txs;
    for (auto it = m_txs.begin(); it != m_txs.end(); it++) {
        if (it->first <= upper_receipt_id) {
            ret_txs.push_back(it->second->get_tx());
        } else {
            break;
        }
    }
    if (ret_txs.size() <= max_num) {
        return ret_txs;
    }
    return {};
}

void xpeer_table_receipts_t::erase(uint64_t receipt_id) {
    auto it = m_txs.find(receipt_id);
    if (it != m_txs.end()) {
        m_receipt_queue_internal->erase_tx(it->second->get_tx()->get_transaction()->digest());
        m_txs.erase(it);
    }
}

int32_t xreceipt_queue_new_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent, uint64_t latest_receipt_id) {
    auto & peer_table_map = get_peer_table_map(tx_ent->get_tx()->is_recv_tx());
    std::shared_ptr<xpeer_table_receipts_t> peer_table_receipts;
    auto & account_addr = (tx_ent->get_tx()->is_recv_tx()) ? tx_ent->get_tx()->get_source_addr() : tx_ent->get_tx()->get_target_addr();
    base::xvaccount_t vaccount(account_addr);
    auto peer_table_sid = vaccount.get_short_table_id();

    auto it = peer_table_map.find(peer_table_sid);
    if (it == peer_table_map.end()) {
        peer_table_receipts = std::make_shared<xpeer_table_receipts_t>(&m_receipt_queue_internal);
        peer_table_map[peer_table_sid] = peer_table_receipts;
    } else {
        peer_table_receipts = it->second;
    }
    return peer_table_receipts->push_tx(tx_ent, latest_receipt_id);
}

const std::vector<xcons_transaction_ptr_t> xreceipt_queue_new_t::get_txs(uint32_t recv_txs_max_num,
                                                                         uint32_t confirm_txs_max_num,
                                                                         const base::xreceiptid_state_ptr_t & receiptid_state) const {
    std::map<base::xtable_shortid_t, std::vector<xcons_transaction_ptr_t>> recv_peer_table_map;
    std::map<base::xtable_shortid_t, std::vector<xcons_transaction_ptr_t>> confirm_peer_table_map;
    // uint32_t max_num, const base::xreceiptid_state_ptr_t & receiptid_state
    uint32_t txs_count = 0;
    uint32_t max_num = (recv_txs_max_num + confirm_txs_max_num) * 2;
    auto & ready_txs = m_receipt_queue_internal.get_queue();
    for (auto it_tx_map_tx = ready_txs.begin(); (txs_count < max_num) && (it_tx_map_tx != ready_txs.end()); it_tx_map_tx++) {
        // use receipt id ascending order peculiarity to deduplicate, if tx's peer table is found from map and receipt id is bigger, update it in map.
        bool is_recv_tx = it_tx_map_tx->get()->get_tx()->is_recv_tx();
        auto & account_addr = is_recv_tx ? it_tx_map_tx->get()->get_tx()->get_source_addr() : it_tx_map_tx->get()->get_tx()->get_target_addr();
        base::xvaccount_t vaccount(account_addr);
        auto peer_table_sid = vaccount.get_short_table_id();
        uint64_t receipt_id = it_tx_map_tx->get()->get_tx()->get_last_action_receipt_id();
        std::map<base::xtable_shortid_t, std::vector<xcons_transaction_ptr_t>> & tx_peer_table_map = is_recv_tx ? recv_peer_table_map : confirm_peer_table_map;

        uint32_t old_tx_num = 0;
        auto it_tx_peer_table_map = tx_peer_table_map.find(peer_table_sid);
        if (it_tx_peer_table_map != tx_peer_table_map.end()) {
            if (receipt_id <= it_tx_peer_table_map->second.back()->get_last_action_receipt_id()) {
                continue;
            } else {
                old_tx_num = it_tx_peer_table_map->second.size();
            }
        }

        std::shared_ptr<xpeer_table_receipts_t> peer_table_receipts = nullptr;
        if (is_recv_tx) {
            auto it = m_recv_tx_peer_table_map.find(peer_table_sid);
            xassert(it != m_recv_tx_peer_table_map.end());
            peer_table_receipts = it->second;
        } else {
            auto it = m_confirm_tx_peer_table_map.find(peer_table_sid);
            xassert(it != m_confirm_tx_peer_table_map.end());
            peer_table_receipts = it->second;
        }
        auto txs_tmp = peer_table_receipts->get_txs(receipt_id, max_num - txs_count);
        if (txs_tmp.size() <= old_tx_num) {
            continue;
        }
        tx_peer_table_map[peer_table_sid] = txs_tmp;
        xtxpool_dbg("xreceipt_queue_new_t::get_txs receipt_id:%llu,old_tx_num:%u,txs_tmp size=%u", receipt_id, old_tx_num, txs_tmp.size());
        txs_count += (txs_tmp.size() - old_tx_num);
    }

    // get receipts continuous with min sendid
    std::vector<xcons_transaction_ptr_t> ret_txs;
    for (auto & it_confirm_peer_table_map : confirm_peer_table_map) {
        auto peer_table_sid = it_confirm_peer_table_map.first;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_confirmid_max();
        auto & confirm_txs = it_confirm_peer_table_map.second;
        for (auto confirm_tx : confirm_txs) {
            if (ret_txs.size() >= confirm_txs_max_num) {
                break;
            }
            if (confirm_tx->get_last_action_receipt_id() <= min_receipt_id) {
                // receipt id less than min receipt id, skip it
                continue;
            } else if (confirm_tx->get_last_action_receipt_id() == (min_receipt_id + 1)) {
                // continuous receipt id
                ret_txs.push_back(confirm_tx);
                min_receipt_id++;
            } else {
                // not continuous, stop.
                break;
            }
        }
        if (ret_txs.size() >= confirm_txs_max_num) {
            break;
        }
    }

    auto confirm_txs_num = ret_txs.size();

    for (auto & it_recv_peer_table_map : recv_peer_table_map) {
        auto peer_table_sid = it_recv_peer_table_map.first;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_recvid_max();
        auto & recv_txs = it_recv_peer_table_map.second;
        for (auto recv_tx : recv_txs) {
            if (ret_txs.size() >= confirm_txs_num + recv_txs_max_num) {
                break;
            }
            if (recv_tx->get_last_action_receipt_id() <= min_receipt_id) {
                // receipt id less than min receipt id, skip it
                continue;
            } else if (recv_tx->get_last_action_receipt_id() == (min_receipt_id + 1)) {
                // continuous receipt id
                ret_txs.push_back(recv_tx);
                min_receipt_id++;
            } else {
                // not continuous, stop.
                break;
            }
        }
        if (ret_txs.size() >= confirm_txs_num + recv_txs_max_num) {
            break;
        }
    }

    return ret_txs;
}

const std::shared_ptr<xtx_entry> xreceipt_queue_new_t::pop_tx(const tx_info_t & txinfo) {
    auto tx_ent = m_receipt_queue_internal.find(txinfo.get_hash());
    if (tx_ent == nullptr || tx_ent->get_tx()->get_tx_subtype() != txinfo.get_subtype()) {
        return nullptr;
    }

    auto & account_addr = (tx_ent->get_tx()->is_recv_tx()) ? tx_ent->get_tx()->get_source_addr() : tx_ent->get_tx()->get_target_addr();
    base::xvaccount_t vaccount(account_addr);
    auto peer_table_sid = vaccount.get_short_table_id();
    auto & peer_table_map = get_peer_table_map(tx_ent->get_tx()->is_recv_tx());

    auto it_peer_table_receipts = peer_table_map.find(peer_table_sid);
    xassert(it_peer_table_receipts != peer_table_map.end());
    it_peer_table_receipts->second.get()->erase(tx_ent->get_tx()->get_last_action_receipt_id());

    return tx_ent;
}

const std::shared_ptr<xtx_entry> xreceipt_queue_new_t::find(const std::string & account_addr, const uint256_t & hash) const {
    return m_receipt_queue_internal.find(hash);
}

void xreceipt_queue_new_t::update_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state) {
    for (auto & it : m_recv_tx_peer_table_map) {
        auto & peer_table_sid = it.first;
        auto & peer_table_tx_queue = it.second;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        peer_table_tx_queue->update_latest_id(receiptid_pair.get_recvid_max());
    }

    for (auto & it : m_confirm_tx_peer_table_map) {
        auto & peer_table_sid = it.first;
        auto & peer_table_tx_queue = it.second;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        peer_table_tx_queue->update_latest_id(receiptid_pair.get_confirmid_max());
    }
}

}  // namespace xtxpool_v2
}  // namespace top
