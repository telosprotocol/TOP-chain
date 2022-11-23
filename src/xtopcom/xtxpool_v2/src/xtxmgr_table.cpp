// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxmgr_table.h"

#include "xmetrics/xmetrics.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_utl.h"

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

#define send_txs_num_pop_from_queue_max (100)
#define receipts_num_pop_from_queue_max (200)
#define send_txs_num_pop_from_queue_batch_num (100)
#define receipts_num_pop_from_queue_batch_num (200)

int32_t xtxmgr_table_t::push_send_tx(const std::shared_ptr<xtx_entry> & tx, uint64_t latest_nonce) {
    auto & account_addr = tx->get_tx()->get_transaction()->get_source_addr();

    if (nullptr != query_tx(account_addr, tx->get_tx()->get_tx_hash())) {
        xtxpool_warn("xtxmgr_table_t::push_send_tx tx repeat tx:%s", tx->get_tx()->dump().c_str());
        return xtxpool_error_request_tx_repeat;
    }

    int32_t ret = m_send_tx_queue.push_tx(tx, latest_nonce);
    if (ret != xsuccess) {
        xtxpool_warn("xtxmgr_table_t::push_tx fail.table %s(send queue size:%u,all send tx counter:%d),tx:%s,last nonce:%u,ret:%s",
                     m_xtable_info->get_table_addr().c_str(),
                     m_send_tx_queue.size(),
                     m_xtable_info->get_send_tx_count(),
                     tx->get_tx()->dump(true).c_str(),
                     latest_nonce,
                     xtxpool_error_to_string(ret).c_str());
    } else {
        xtxpool_info("xtxmgr_table_t::push_tx success.table %s(send queue size:%u,all send tx ccounter:%d),tx:%s,last nonce:%u",
                     m_xtable_info->get_table_addr().c_str(),
                     m_send_tx_queue.size(),
                     m_xtable_info->get_send_tx_count(),
                     tx->get_tx()->dump(true).c_str(),
                     latest_nonce);
    }
    return ret;
}

int32_t xtxmgr_table_t::push_receipt(const std::shared_ptr<xtx_entry> & tx) {
    XMETRICS_TIME_RECORD("txpool_message_unit_receipt_push_receipt_table_push_receipt");
    auto account_addr = tx->get_tx()->get_account_addr();
    auto tx_inside = query_tx(account_addr, tx->get_tx()->get_tx_hash());
    if (tx_inside != nullptr) {
        if (tx_inside->get_tx_subtype() < tx->get_tx()->get_tx_subtype()) {
            xtxpool_dbg("xtxmgr_table_t::push_receipt same tx hash, new tx:%s replace old tx:%s", tx->get_tx()->dump().c_str(), tx_inside->dump().c_str());
            pop_tx(tx_inside->get_tx_hash(), tx_inside->get_tx_subtype(), false);
        } else {
            xtxpool_warn("xtxmgr_table_t::push_receipt tx repeat tx:%s", tx->get_tx()->dump().c_str());
            // XMETRICS_COUNTER_INCREMENT("txpool_receipt_repeat", 1);
            m_xtable_info->get_statistic()->inc_receipt_repeat_num(1);
            return xtxpool_error_request_tx_repeat;
        }
    }

    int32_t ret = m_new_receipt_queue.push_tx(tx);
    if (ret != xsuccess) {
        xtxpool_warn("xtxmgr_table_t::push_receipt fail.table %s(receipt queue size:%u, receipt counter:%d recv:%d,confirm:%d),tx:%s,ret:%s",
                     m_xtable_info->get_table_addr().c_str(),
                     m_new_receipt_queue.size(),
                     m_xtable_info->get_recv_tx_count() + m_xtable_info->get_conf_tx_count(),
                     m_xtable_info->get_recv_tx_count(),
                     m_xtable_info->get_conf_tx_count(),
                     tx->get_tx()->dump(true).c_str(),
                     xtxpool_error_to_string(ret).c_str());
    } else {
        xtxpool_info("xtxmgr_table_t::push_receipt success.table %s(receipt queue size:%u, receipt counter:%d recv:%d,confirm:%d),tx:%s",
                     m_xtable_info->get_table_addr().c_str(),
                     m_new_receipt_queue.size(),
                     m_xtable_info->get_recv_tx_count() + m_xtable_info->get_conf_tx_count(),
                     m_xtable_info->get_recv_tx_count(),
                     m_xtable_info->get_conf_tx_count(),
                     tx->get_tx()->dump(true).c_str());
    }
    return ret;
}

data::xcons_transaction_ptr_t xtxmgr_table_t::pop_tx(const std::string & tx_hash, base::enum_transaction_subtype subtype, bool clear_follower) {
    // maybe m_tx_queue m_pending_accounts both contains the tx
    std::shared_ptr<xtx_entry> tx_ent = nullptr;
    if (subtype == enum_transaction_subtype_self || subtype == enum_transaction_subtype_send) {
        tx_ent = m_send_tx_queue.pop_tx(tx_hash, clear_follower);
    } else {
        tx_ent = m_new_receipt_queue.pop_tx(tx_hash, subtype);
    }
    if (tx_ent == nullptr) {
        return nullptr;
    }
    return tx_ent->get_tx();
}

void xtxmgr_table_t::update_id_state(const tx_info_t & txinfo, base::xtable_shortid_t table_sid, uint64_t receiptid, uint64_t nonce) {
    if (txinfo.get_subtype() == enum_transaction_subtype_self || txinfo.get_subtype() == enum_transaction_subtype_send) {
        updata_latest_nonce(txinfo.get_addr(), nonce);
    }

    // only send and self tx push to pending accounts queue.so as pop.
    m_new_receipt_queue.update_receipt_id_by_confirmed_tx(txinfo, table_sid, receiptid);
}

std::vector<xcons_transaction_ptr_t> xtxmgr_table_t::get_ready_txs(const xtxs_pack_para_t & pack_para, const xunconfirm_id_height & unconfirm_id_height) {
    uint32_t confirm_tx_num = 0;
    uint32_t recv_tx_num = 0;
    xtxpool_info("xtxmgr_table_t::get_ready_txs table:%s in", m_xtable_info->get_table_addr().c_str());
    std::vector<xcons_transaction_ptr_t> ready_txs = m_new_receipt_queue.get_txs(pack_para.get_confirm_and_recv_txs_max_num(),
                                                                                 pack_para.get_confirm_txs_max_num(),
                                                                                 pack_para.get_table_state_highqc()->get_receiptid_state(),
                                                                                 unconfirm_id_height,
                                                                                 confirm_tx_num);
    recv_tx_num = ready_txs.size() - confirm_tx_num;

    auto send_txs = m_send_tx_queue.get_txs(pack_para.get_all_txs_max_num() - ready_txs.size(), pack_para.get_cert_block());
    uint32_t send_tx_num = send_txs.size();
    ready_txs.insert(ready_txs.end(), send_txs.begin(), send_txs.end());

    XMETRICS_GAUGE(metrics::cons_table_leader_get_txpool_sendtx_count, send_tx_num);
    XMETRICS_GAUGE(metrics::cons_table_leader_get_txpool_recvtx_count, recv_tx_num);
    XMETRICS_GAUGE(metrics::cons_table_leader_get_txpool_confirmtx_count, confirm_tx_num);

    xtxpool_info("xtxmgr_table_t::get_ready_txs table:%s,ready_txs size:%u,send:%u,recv:%u,confirm:%u,sendq:%u,recvq:%u,confirmq:%u",
                 m_xtable_info->get_table_addr().c_str(),
                 ready_txs.size(),
                 send_tx_num,
                 recv_tx_num,
                 confirm_tx_num,
                 m_send_tx_queue.size(),
                 m_xtable_info->get_recv_tx_count(),
                 m_xtable_info->get_conf_tx_count());
    if (ready_txs.size() > pack_para.get_all_txs_max_num()) {
        xtxpool_error("xtxmgr_table_t::get_ready_txs-txs num too much table:%s,ready_txs size:%u,send:%u,recv:%u,confirm:%u,sendq:%u,recvq:%u,confirmq:%u",
                 m_xtable_info->get_table_addr().c_str(),
                 ready_txs.size(),
                 send_tx_num,
                 recv_tx_num,
                 confirm_tx_num,
                 m_send_tx_queue.size(),
                 m_xtable_info->get_recv_tx_count(),
                 m_xtable_info->get_conf_tx_count());
    }
    for (auto & tx : ready_txs) {
        xtxpool_dbg("xtxmgr_table_t::get_ready_txs table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx->dump().c_str());
    }

    return ready_txs;
}

data::xcons_transaction_ptr_t xtxmgr_table_t::query_tx(const std::string & account_addr, const std::string & hash_str) const {
    auto tx = m_send_tx_queue.find(account_addr, hash_str);
    if (tx == nullptr) {
        tx = m_new_receipt_queue.find(account_addr, hash_str);
    }
    if (tx == nullptr) {
        return nullptr;
    }
    return tx->get_tx();
}

void xtxmgr_table_t::updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) {
    xtxpool_info("xtxmgr_table_t::updata_latest_nonce.table %s,account:%s,last nonce:%u", m_xtable_info->get_table_addr().c_str(), account_addr.c_str(), latest_nonce);
    m_send_tx_queue.updata_latest_nonce(account_addr, latest_nonce);
}

bool xtxmgr_table_t::is_repeat_tx(const std::shared_ptr<xtx_entry> & tx) const {
    auto account_addr = tx->get_tx()->get_account_addr();
    auto tx_inside = query_tx(account_addr, tx->get_tx()->get_tx_hash());
    if (tx_inside != nullptr) {
        if (tx_inside->get_tx_subtype() < tx->get_tx()->get_tx_subtype()) {
            return false;
        } else {
            return true;
        }
    }
    return false;
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxmgr_table_t::get_lacking_recv_tx_ids(const std::set<base::xtable_shortid_t> & all_table_sids, uint32_t & total_num) const {
    return m_new_receipt_queue.get_lacking_recv_tx_ids(all_table_sids, total_num);
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxmgr_table_t::get_lacking_confirm_tx_ids(uint32_t & total_num) const {
    return m_new_receipt_queue.get_lacking_confirm_tx_ids(total_num);
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxmgr_table_t::get_lacking_discrete_confirm_tx_ids(
    const std::map<base::xtable_shortid_t, xneed_confirm_ids> & need_confirm_ids_map,
    uint32_t & total_num) const {
    return m_new_receipt_queue.get_lacking_discrete_confirm_tx_ids(need_confirm_ids_map, total_num);
}

void xtxmgr_table_t::clear_expired_txs() {
#ifdef ENABLE_METRICS
    auto queue_size_before = m_send_tx_queue.size();
#endif
    m_send_tx_queue.clear_expired_txs();

#ifdef ENABLE_METRICS
    auto queue_size_after = m_send_tx_queue.size();
    if (queue_size_after < queue_size_before) {
        XMETRICS_GAUGE(metrics::txpool_send_tx_timeout, queue_size_before - queue_size_after);
    }
#endif
}

void xtxmgr_table_t::update_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state) {
    m_new_receipt_queue.update_receiptid_state(receiptid_state);
}

}  // namespace xtxpool_v2
}  // namespace top
