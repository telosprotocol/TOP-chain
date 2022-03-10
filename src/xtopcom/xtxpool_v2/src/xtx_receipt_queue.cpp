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

void xreceipt_queue_internal_t::insert_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    tx_ent->get_tx()->set_push_pool_timestamp(now);
    auto it = m_tx_queue.insert(tx_ent);
    m_tx_map[tx_ent->get_tx()->get_tx_hash()] = it;
    m_xtable_info->tx_inc(tx_ent->get_tx()->get_tx_subtype(), 1);
    xtxpool_info("xreceipt_queue_internal_t::insert_tx table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
}

void xreceipt_queue_internal_t::erase_tx(const uint256_t & hash) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it_tx_map = m_tx_map.find(hash_str);
    if (it_tx_map != m_tx_map.end()) {
        auto & tx_ent = *it_tx_map->second;
        uint64_t delay = xverifier::xtx_utl::get_gmttime_s() - tx_ent->get_tx()->get_push_pool_timestamp();
        xtxpool_info("xreceipt_queue_internal_t::erase_ready_tx from ready txs,table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
        if (tx_ent->get_tx()->is_confirm_tx()) {
            XMETRICS_GAUGE(metrics::txpool_tx_delay_from_push_to_commit_recv, delay);
        } else {
            XMETRICS_GAUGE(metrics::txpool_tx_delay_from_push_to_commit_confirm, delay);
        }

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

int32_t xpeer_table_receipts_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    uint64_t new_receipt_id = tx_ent->get_tx()->get_last_action_receipt_id();

    if (new_receipt_id <= m_latest_receipt_id) {
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

void xpeer_table_receipts_t::update_latest_receipt_id(uint64_t latest_receipt_id) {
    if (latest_receipt_id <= m_latest_receipt_id) {
        return;
    }
    for (auto it = m_txs.begin(); it != m_txs.end();) {
        if (it->first <= latest_receipt_id) {
            m_receipt_queue_internal->erase_tx(it->second->get_tx()->get_tx_hash_256());
            it = m_txs.erase(it);
        } else {
            break;
        }
    }
    m_latest_receipt_id = latest_receipt_id;
}

const std::vector<xcons_transaction_ptr_t> xpeer_table_receipts_t::get_txs(uint64_t lower_receipt_id, uint64_t upper_receipt_id, uint32_t max_num) const {
    std::vector<xcons_transaction_ptr_t> ret_txs;
    for (auto it = m_txs.begin(); it != m_txs.end(); it++) {
        if (it->first < lower_receipt_id) {
            continue;
        }
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
        m_receipt_queue_internal->erase_tx(it->second->get_tx()->get_tx_hash_256());
        m_txs.erase(it);
    }
}

void xpeer_table_receipts_t::get_lacking_ids(std::vector<uint64_t> & lacking_ids, uint64_t max_pull_id) const {
    uint64_t last_receipt_id = m_latest_receipt_id;

    if (m_txs.size() == max_pull_id - m_latest_receipt_id) {
        return;
    }

    for (auto & tx_pair : m_txs) {
        auto & receipt_id = tx_pair.first;
        if (receipt_id != last_receipt_id + 1) {
            xdbg("xpeer_table_receipts_t::get_lacking_ids receipt_id:%llu", receipt_id);
            for (auto id = last_receipt_id + 1; id < receipt_id; id++) {
                xdbg("xpeer_table_receipts_t::get_lacking_ids add id:%llu,m_latest_receipt_id:%llu,max_pull_id:%llu", id, m_latest_receipt_id, max_pull_id);
                lacking_ids.push_back(id);
            }
        }
        last_receipt_id = receipt_id;
    }

    auto iter = m_txs.rbegin();
    if (iter != m_txs.rend()) {
        last_receipt_id = iter->first;
    } else {
        last_receipt_id = m_latest_receipt_id;
    }
    for (uint64_t id = last_receipt_id + 1; id <= max_pull_id; id++) {
        xdbg("xpeer_table_receipts_t::get_lacking_ids add id:%llu,m_latest_receipt_id:%llu,max_pull_id:%llu", id, m_latest_receipt_id, max_pull_id);
        lacking_ids.push_back(id);
    }
}

const std::shared_ptr<xtx_entry> xpeer_table_receipts_t::get_tx_by_receipt_id(uint64_t receipt_id) const {
    auto it = m_txs.find(receipt_id);
    if (it != m_txs.end()) {
        return it->second;
    }
    return nullptr;
}

int32_t xreceipt_queue_new_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    if ((tx_ent->get_tx()->is_recv_tx() && m_receipt_queue_internal.recv_tx_full()) || (tx_ent->get_tx()->is_confirm_tx() && m_receipt_queue_internal.confirm_tx_full())) {
        // just warn, not return. for receipts should not be dropped.
        xwarn("xreceipt_queue_new_t::push_tx receipt tx full,tx:%s", tx_ent->get_tx()->dump().c_str());
    }

    auto & peer_table_map = get_peer_table_map(tx_ent->get_tx()->is_recv_tx());
    std::shared_ptr<xpeer_table_receipts_t> peer_table_receipts;
    auto peer_table_sid = tx_ent->get_tx()->get_peer_tableid();

    auto it = peer_table_map.find(peer_table_sid);
    if (it == peer_table_map.end()) {
        peer_table_receipts = std::make_shared<xpeer_table_receipts_t>(&m_receipt_queue_internal);
        peer_table_map[peer_table_sid] = peer_table_receipts;
    } else {
        peer_table_receipts = it->second;
    }
    return peer_table_receipts->push_tx(tx_ent);
}

const std::vector<xcons_transaction_ptr_t> xreceipt_queue_new_t::get_txs(uint32_t confirm_and_recv_txs_max_num,
                                                                         uint32_t confirm_txs_max_num,
                                                                         const base::xreceiptid_state_ptr_t & receiptid_state,
                                                                         uint32_t & confirm_txs_num) const {
    std::map<base::xtable_shortid_t, std::vector<xcons_transaction_ptr_t>> recv_peer_table_map;
    std::map<base::xtable_shortid_t, std::vector<xcons_transaction_ptr_t>> confirm_peer_table_map;
    uint32_t txs_count = 0;
    // get more recv and confirm txs from ordered receipt queue first, because we don't know if there is enough continuous confirm txs or recv txs in top max num of the queue
    uint32_t max_num = confirm_and_recv_txs_max_num;
    auto & ready_txs = m_receipt_queue_internal.get_queue();
    for (auto it_tx_map_tx = ready_txs.begin(); (txs_count < max_num) && (it_tx_map_tx != ready_txs.end()); it_tx_map_tx++) {
        // use receipt id ascending order peculiarity to deduplicate, if tx's peer table is found from map and receipt id is bigger, update it in map.
        bool is_recv_tx = it_tx_map_tx->get()->get_tx()->is_recv_tx();
        auto peer_table_sid = it_tx_map_tx->get()->get_tx()->get_peer_tableid();
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
        uint64_t min_receipt_id = 0;
        if (is_recv_tx) {
            base::xreceiptid_pair_t receiptid_pair;
            receiptid_state->find_pair(peer_table_sid, receiptid_pair);
            min_receipt_id = receiptid_pair.get_recvid_max();
            auto it = m_recv_tx_peer_table_map.find(peer_table_sid);
            xassert(it != m_recv_tx_peer_table_map.end());
            peer_table_receipts = it->second;
        } else {
            base::xreceiptid_pair_t receiptid_pair;
            receiptid_state->find_pair(peer_table_sid, receiptid_pair);
            min_receipt_id = receiptid_pair.get_confirmid_max();
            auto it = m_confirm_tx_peer_table_map.find(peer_table_sid);
            xassert(it != m_confirm_tx_peer_table_map.end());
            peer_table_receipts = it->second;
        }
        auto txs_tmp = peer_table_receipts->get_txs(min_receipt_id + 1, receipt_id, max_num - txs_count);
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
        for (auto & confirm_tx : confirm_txs) {
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

    confirm_txs_num = ret_txs.size();

    for (auto & it_recv_peer_table_map : recv_peer_table_map) {
        auto peer_table_sid = it_recv_peer_table_map.first;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_recvid_max();
        auto & recv_txs = it_recv_peer_table_map.second;
        for (auto & recv_tx : recv_txs) {
            if (ret_txs.size() >= confirm_and_recv_txs_max_num) {
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
        if (ret_txs.size() >= confirm_and_recv_txs_max_num) {
            break;
        }
    }

    return ret_txs;
}

const std::vector<xcons_transaction_ptr_t> xreceipt_queue_new_t::get_txs(uint32_t confirm_and_recv_txs_max_num,
                                                                         uint32_t confirm_txs_max_num,
                                                                         const base::xreceiptid_state_ptr_t & receiptid_state,
                                                                         const xunconfirm_id_height & unconfirm_id_height,
                                                                         uint32_t & confirm_txs_num) const {
    std::map<base::xtable_shortid_t, std::vector<xcons_transaction_ptr_t>> recv_peer_table_map;
    std::map<base::xtable_shortid_t, uint64_t> confirm_peer_table_max_id_map;
    uint32_t txs_count = 0;
    // get more recv and confirm txs from ordered receipt queue first, because we don't know if there is enough continuous confirm txs or recv txs in top max num of the queue
    uint32_t max_num = confirm_and_recv_txs_max_num * 2;
    auto & ready_txs = m_receipt_queue_internal.get_queue();
    for (auto it_tx_map_tx = ready_txs.begin(); (txs_count < max_num) && (it_tx_map_tx != ready_txs.end()); it_tx_map_tx++) {
        // use receipt id ascending order peculiarity to deduplicate, if tx's peer table is found from map and receipt id is bigger, update it in map.
        bool is_recv_tx = it_tx_map_tx->get()->get_tx()->is_recv_tx();
        auto peer_table_sid = it_tx_map_tx->get()->get_tx()->get_peer_tableid();
        uint64_t receipt_id = it_tx_map_tx->get()->get_tx()->get_last_action_receipt_id();

        if (is_recv_tx) {
            uint32_t old_tx_num = 0;
            auto it_tx_peer_table_map = recv_peer_table_map.find(peer_table_sid);
            if (it_tx_peer_table_map != recv_peer_table_map.end()) {
                if (receipt_id <= it_tx_peer_table_map->second.back()->get_last_action_receipt_id()) {
                    continue;
                } else {
                    old_tx_num = it_tx_peer_table_map->second.size();
                }
            }
            base::xreceiptid_pair_t receiptid_pair;
            receiptid_state->find_pair(peer_table_sid, receiptid_pair);
            auto min_receipt_id = receiptid_pair.get_recvid_max();

            auto it = m_recv_tx_peer_table_map.find(peer_table_sid);
            xassert(it != m_recv_tx_peer_table_map.end());
            std::shared_ptr<xpeer_table_receipts_t> peer_table_receipts = it->second;

            auto txs_tmp = peer_table_receipts->get_txs(min_receipt_id + 1, receipt_id, max_num - txs_count);
            if (txs_tmp.size() <= old_tx_num) {
                continue;
            }
            recv_peer_table_map[peer_table_sid] = txs_tmp;
            xtxpool_dbg("xreceipt_queue_new_t::get_txs receipt_id:%llu,old_tx_num:%u,txs_tmp size=%u", receipt_id, old_tx_num, txs_tmp.size());
            txs_count += (txs_tmp.size() - old_tx_num);
        } else {
            uint64_t old_receipt_id = 0;
            auto it_confirm_peer_table_max_id = confirm_peer_table_max_id_map.find(peer_table_sid);
            if (it_confirm_peer_table_max_id != confirm_peer_table_max_id_map.end()) {
                if (it_confirm_peer_table_max_id->second < receipt_id) {
                    confirm_peer_table_max_id_map[peer_table_sid] = receipt_id;
                }
            } else {
                confirm_peer_table_max_id_map[peer_table_sid] = receipt_id;
            }
            txs_count++;
        }
    }

    // get receipts continuous with min sendid
    std::vector<xcons_transaction_ptr_t> ret_txs;
    for (auto & it_confirm_peer_table_max_id_map : confirm_peer_table_max_id_map) {
        auto & peer_table_sid = it_confirm_peer_table_max_id_map.first;
        auto & max_receipt_id = it_confirm_peer_table_max_id_map.second;

        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_confirmid_max();

        if (max_receipt_id <= min_receipt_id) {
            continue;
        }

        auto it_confirm_tx_peer_table = m_confirm_tx_peer_table_map.find(peer_table_sid);
        if (it_confirm_tx_peer_table == m_confirm_tx_peer_table_map.end()) {
            xtxpool_error("xreceipt_queue_new_t::get_txs confirm tx table sid not found in cache.");
            continue;
        } else {
            auto confirm_txs = it_confirm_tx_peer_table->second->get_txs(min_receipt_id + 1, max_receipt_id, max_receipt_id - min_receipt_id);
            std::vector<uint64_t> receipt_ids;
            for (auto & confirm_tx : confirm_txs) {
                receipt_ids.push_back(confirm_tx->get_last_action_receipt_id());
            }
            if (receipt_ids.empty()) {
                xtxpool_error("xreceipt_queue_new_t::get_txs receipt_ids is empty.");
                continue;
            }

            auto filted_ids = unconfirm_id_height.filter_sender_continuous_need_confirm_ids(peer_table_sid, min_receipt_id + 1, receipt_ids);

            if (filted_ids.empty()) {
                continue;
            }
            uint64_t max_filted_id = filted_ids[filted_ids.size() - 1];

            for (auto & confirm_tx : confirm_txs) {
                if (confirm_tx->get_last_action_receipt_id() > max_filted_id) {
                    break;
                }
                ret_txs.push_back(confirm_tx);
            }
        }

        if (ret_txs.size() >= confirm_txs_max_num) {
            break;
        }
    }

    confirm_txs_num = ret_txs.size();

    for (auto & it_recv_peer_table_map : recv_peer_table_map) {
        auto peer_table_sid = it_recv_peer_table_map.first;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_recvid_max();
        auto & recv_txs = it_recv_peer_table_map.second;
        for (auto & recv_tx : recv_txs) {
            if (ret_txs.size() >= confirm_and_recv_txs_max_num) {
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
        if (ret_txs.size() >= confirm_and_recv_txs_max_num) {
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

    auto peer_table_sid = tx_ent->get_tx()->get_peer_tableid();
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
        xdbg("xreceipt_queue_new_t::update_receiptid_state recv self:%d,peer:%d,id pair:%s", receiptid_state->get_self_tableid(), peer_table_sid, receiptid_pair.dump().c_str());
        peer_table_tx_queue->update_latest_receipt_id(receiptid_pair.get_recvid_max());
    }

    for (auto & it : m_confirm_tx_peer_table_map) {
        auto & peer_table_sid = it.first;
        auto & peer_table_tx_queue = it.second;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        xdbg("xreceipt_queue_new_t::update_receiptid_state confirm self:%d,peer:%d,id pair:%s", receiptid_state->get_self_tableid(), peer_table_sid, receiptid_pair.dump().c_str());
        peer_table_tx_queue->update_latest_receipt_id(receiptid_pair.get_confirmid_max());
    }
}

void xreceipt_queue_new_t::update_receipt_id_by_confirmed_tx(const tx_info_t & txinfo, base::xtable_shortid_t peer_table_sid, uint64_t receiptid) {
    if (txinfo.get_subtype() == enum_transaction_subtype_self) {
        return;
    }

    xdbg("xreceipt_queue_new_t::update_receipt_id_by_confirmed_tx table:%s peer table sid:%d subtype:%d receiptid:%llu",
         m_receipt_queue_internal.get_table_addr().c_str(),
         peer_table_sid,
         txinfo.get_subtype(),
         receiptid);

    std::shared_ptr<xpeer_table_receipts_t> peer_table_receipts;
    if (txinfo.get_subtype() == enum_transaction_subtype_recv) {
        auto it = m_recv_tx_peer_table_map.find(peer_table_sid);
        if (it == m_recv_tx_peer_table_map.end()) {
            peer_table_receipts = std::make_shared<xpeer_table_receipts_t>(&m_receipt_queue_internal);
            m_recv_tx_peer_table_map[peer_table_sid] = peer_table_receipts;
        } else {
            peer_table_receipts = it->second;
        }
        peer_table_receipts->update_latest_receipt_id(receiptid);
    }

    if (txinfo.get_subtype() == enum_transaction_subtype_confirm) {
        auto it = m_confirm_tx_peer_table_map.find(peer_table_sid);
        if (it == m_confirm_tx_peer_table_map.end()) {
            peer_table_receipts = std::make_shared<xpeer_table_receipts_t>(&m_receipt_queue_internal);
            m_confirm_tx_peer_table_map[peer_table_sid] = peer_table_receipts;
        } else {
            peer_table_receipts = it->second;
        }

        peer_table_receipts->update_latest_receipt_id(receiptid);
    }
}

// uint64_t xreceipt_queue_new_t::get_latest_recv_receipt_id(base::xtable_shortid_t peer_table_sid) const {
//     auto it = m_recv_tx_peer_table_map.find(peer_table_sid);
//     if (it != m_recv_tx_peer_table_map.end()) {
//         return it->second->get_latest_receipt_id();
//     }
//     return 0;
// }

// uint64_t xreceipt_queue_new_t::get_latest_confirm_receipt_id(base::xtable_shortid_t peer_table_sid) const {
//     auto it = m_confirm_tx_peer_table_map.find(peer_table_sid);
//     if (it != m_confirm_tx_peer_table_map.end()) {
//         return it->second->get_latest_receipt_id();
//     }
//     return 0;
// }

const std::vector<xtxpool_table_lacking_receipt_ids_t> xreceipt_queue_new_t::get_lacking_recv_tx_ids(uint32_t & total_num) const {
    std::vector<xtxpool_table_lacking_receipt_ids_t> table_lacking_ids_vec;
    auto self_table_sid = m_receipt_queue_internal.get_table_info()->get_short_table_id();
    auto table_receiptid_state = m_para->get_receiptid_state_cache().get_table_receiptid_state(self_table_sid);

    for (auto & it : m_recv_tx_peer_table_map) {
        std::vector<uint64_t> lacking_ids;
        auto & peer_table_sid = it.first;
        auto & table_receipts = it.second;
        uint64_t recvid_max = 0;
        if (table_receiptid_state != nullptr) {
            base::xreceiptid_pair_t pair;
            table_receiptid_state->find_pair(peer_table_sid, pair);
            recvid_max = pair.get_recvid_max();
        }

        uint64_t max_pull_id = m_para->get_receiptid_state_cache().get_sendid_max(peer_table_sid, self_table_sid);

        table_receipts->update_latest_receipt_id(recvid_max);
        table_receipts->get_lacking_ids(lacking_ids, max_pull_id);
        if (!lacking_ids.empty()) {
            table_lacking_ids_vec.push_back(xtxpool_table_lacking_receipt_ids_t(peer_table_sid, lacking_ids));
            total_num += lacking_ids.size();
            xdbg("xreceipt_queue_new_t::get_lacking_recv_tx_ids, self:%d,peer:%d,lacking_ids size:%d", self_table_sid, peer_table_sid, lacking_ids.size());
        }
    }
    return table_lacking_ids_vec;
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xreceipt_queue_new_t::get_lacking_confirm_tx_ids(uint32_t & total_num) const {
    std::vector<xtxpool_table_lacking_receipt_ids_t> table_lacking_ids_vec;
    auto self_table_sid = m_receipt_queue_internal.get_table_info()->get_short_table_id();
    auto table_receiptid_state = m_para->get_receiptid_state_cache().get_table_receiptid_state(self_table_sid);

    for (auto & it : m_confirm_tx_peer_table_map) {
        std::vector<uint64_t> lacking_ids;
        auto & peer_table_sid = it.first;
        auto & table_receipts = it.second;
        uint64_t confirmid_max = 0;
        if (table_receiptid_state != nullptr) {
            base::xreceiptid_pair_t pair;
            table_receiptid_state->find_pair(peer_table_sid, pair);
            confirmid_max = pair.get_confirmid_max();
        }

        uint64_t max_pull_id = m_para->get_receiptid_state_cache().get_recvid_max(peer_table_sid, self_table_sid);

        table_receipts->update_latest_receipt_id(confirmid_max);
        table_receipts->get_lacking_ids(lacking_ids, max_pull_id);
        if (!lacking_ids.empty()) {
            table_lacking_ids_vec.push_back(xtxpool_table_lacking_receipt_ids_t(peer_table_sid, lacking_ids));
            total_num += lacking_ids.size();
            xdbg("xreceipt_queue_new_t::get_lacking_confirm_tx_ids, self:%d,peer:%d,lacking_ids size:%d", self_table_sid, peer_table_sid, lacking_ids.size());
        }
    }
    return table_lacking_ids_vec;
}

uint32_t xreceipt_queue_new_t::size() const {
    return m_receipt_queue_internal.size();
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xreceipt_queue_new_t::get_lacking_discrete_confirm_tx_ids(
    const std::map<base::xtable_shortid_t, xneed_confirm_ids> & need_confirm_ids_map,
    uint32_t & total_num) const {
    std::vector<xtxpool_table_lacking_receipt_ids_t> lacking_ids_vec_ret;
    for (auto & need_confirm_ids_pair : need_confirm_ids_map) {
        std::vector<uint64_t> lacking_receipt_ids;
        auto & peer_table_sid = need_confirm_ids_pair.first;
        auto & need_confirm_ids = need_confirm_ids_pair.second;
        auto it = m_confirm_tx_peer_table_map.find(peer_table_sid);
        if (it == m_confirm_tx_peer_table_map.end()) {
            lacking_receipt_ids.insert(lacking_receipt_ids.end(), need_confirm_ids.m_ids.begin(), need_confirm_ids.m_ids.end());
            total_num += need_confirm_ids.m_ids.size();
        } else {
            for (auto & id : need_confirm_ids.m_ids) {
                auto confirm_tx = it->second->get_tx_by_receipt_id(id);
                if (confirm_tx == nullptr) {
                    lacking_receipt_ids.push_back(id);
                    total_num++;
                }
            }
        }
    }

    return lacking_ids_vec_ret;
}

}  // namespace xtxpool_v2
}  // namespace top
