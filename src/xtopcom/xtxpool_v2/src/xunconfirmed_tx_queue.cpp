// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xunconfirmed_tx_queue.h"

#include "xdata/xblocktool.h"
#include "xmbus/xevent_behind.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xtxpool_v2/xreceipt_state_cache.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvstate.h"
#include "xdata/xtable_bstate.h"

#include <unordered_set>

NS_BEG2(top, xtxpool_v2)

#define resend_time_threshold (64)
#define too_long_time_not_confirm (300)

void xpeer_table_unconfirmed_txs_t::push_tx(const xcons_transaction_ptr_t & tx) {
    auto it = m_unconfirmed_txs.find(tx->get_last_action_receipt_id());
    if (it != m_unconfirmed_txs.end()) {
        return;
    }
    m_unconfirmed_txs[tx->get_last_action_receipt_id()] = tx;
}

void xpeer_table_unconfirmed_txs_t::erase(uint64_t receipt_id, xall_unconfirm_tx_set_t & all_unconfirm_tx_set) {
    auto it_unconfirmed_txs = m_unconfirmed_txs.find(receipt_id);
    if (it_unconfirmed_txs == m_unconfirmed_txs.end()) {
        return;
    }

    auto & tx = it_unconfirmed_txs->second;

    xtxpool_info("xpeer_table_unconfirmed_txs_t::erase tx:%s", tx->dump(true).c_str());

    auto range = all_unconfirm_tx_set.equal_range(tx);
    for (auto it_range = range.first; it_range != range.second; it_range++) {
        if (it_range->get()->get_transaction()->get_digest_str() == tx->get_transaction()->get_digest_str()) {
            all_unconfirm_tx_set.erase(it_range);
            break;
        }
    }

    m_unconfirmed_txs.erase(it_unconfirmed_txs);
}

const xcons_transaction_ptr_t xpeer_table_unconfirmed_txs_t::find(uint64_t receipt_id) const {
    auto it = m_unconfirmed_txs.find(receipt_id);
    if (it != m_unconfirmed_txs.end()) {
        return it->second;
    }
    return nullptr;
}

void xpeer_table_unconfirmed_txs_t::update_receipt_id(uint64_t max_confirm_id, xall_unconfirm_tx_set_t & all_unconfirm_tx_set) {
    if (m_max_confirm_id >= max_confirm_id) {
        return;
    }

    uint64_t front_receipt_id = m_unconfirmed_txs.begin()->first;
    for (uint64_t id = front_receipt_id; id <= max_confirm_id; id++) {
        erase(id, all_unconfirm_tx_set);
    }
    m_max_confirm_id = max_confirm_id;
}

const xcons_transaction_ptr_t xpeer_table_unconfirmed_txs_t::get_latest_receipt() const {
    if (!m_unconfirmed_txs.empty()) {
        return m_unconfirmed_txs.rbegin()->second;
    }
    return nullptr;
}

const xcons_transaction_ptr_t xpeer_table_unconfirmed_txs_t::get_first_receipt() const {
    if (!m_unconfirmed_txs.empty()) {
        return m_unconfirmed_txs.begin()->second;
    }
    return nullptr;
}

void xpeer_tables_t::push_tx(const xcons_transaction_ptr_t & tx) {
    xassert(tx->is_recv_tx());
    auto peer_table_sid = tx->get_self_tableid(); // note,here should use self tableid of origin tx target account
    xtxpool_info("xpeer_tables_t::push_tx tx:%s,peer table sid:%d,receipt id:%llu", tx->dump(true).c_str(), peer_table_sid, tx->get_last_action_receipt_id());
    auto it = m_peer_tables.find(peer_table_sid);
    if (it != m_peer_tables.end()) {
        it->second->push_tx(tx);
    } else {
        auto peer_table = std::make_shared<xpeer_table_unconfirmed_txs_t>();
        peer_table->push_tx(tx);
        m_peer_tables[peer_table_sid] = peer_table;
    }
    m_all_unconfirm_txs.insert(tx);
}

void xpeer_tables_t::erase(uint16_t peer_table_sid, uint64_t receipt_id) {
    auto it = m_peer_tables.find(peer_table_sid);
    if (it != m_peer_tables.end()) {
        it->second->erase(receipt_id, m_all_unconfirm_txs);
    }
}

const xcons_transaction_ptr_t xpeer_tables_t::find(uint16_t peer_table_sid, uint64_t receipt_id) const {
    auto it = m_peer_tables.find(peer_table_sid);
    xtxpool_dbg("xpeer_tables_t::find peer table sid:%d,receipt id:%llu", peer_table_sid, receipt_id);
    if (it != m_peer_tables.end()) {
        return it->second->find(receipt_id);
    }
    return nullptr;
}

void xpeer_tables_t::update_receiptid_state(const xtable_state_cache_t & table_state_cache) {
    for (auto peer_table : m_peer_tables) {
        auto & peer_table_sid = peer_table.first;
        auto & peer_table_txs = peer_table.second;
        peer_table_txs->update_receipt_id(table_state_cache.get_confirmid_max(peer_table_sid), m_all_unconfirm_txs);
    }
}

const std::vector<xcons_transaction_ptr_t> xpeer_tables_t::get_latest_receipts_for_resend(uint64_t now) const {
    std::vector<xcons_transaction_ptr_t> latest_receipts;
    for (auto & peer_table : m_peer_tables) {
        auto first_receipt = peer_table.second->get_first_receipt();
        if (first_receipt != nullptr && first_receipt->get_receipt_gmtime() + too_long_time_not_confirm < now) {
            xtxpool_warn("xpeer_tables_t::get_latest_receipts_for_resend tx too long time not confirm:%s,delay:%llu", first_receipt->dump().c_str(), now - first_receipt->get_receipt_gmtime());
        }
        auto receipt = peer_table.second->get_latest_receipt();
        if (receipt != nullptr && receipt->get_receipt_gmtime() + resend_time_threshold < now) {
            latest_receipts.push_back(receipt);
        }
    }
    return latest_receipts;
}

int32_t xunconfirmed_account_t::update(xblock_t * latest_committed_block, const xtable_state_cache_t & table_state_cache) {
    std::unordered_set<std::string> confirm_txs;
    std::map<std::string, xcons_transaction_ptr_t> unconfirmed_txs;
    auto account_addr = latest_committed_block->get_account();
    uint64_t latest_height = latest_committed_block->get_height();
    std::string last_block_hash;

    for (uint64_t cur_height = latest_height; cur_height > m_highest_height; cur_height--) {
        xblock_ptr_t unit_block = nullptr;
        if (cur_height == latest_height) {
            unit_block = xblock_t::raw_vblock_to_object_ptr(latest_committed_block);
        } else {
            auto _commit_block = m_para->get_vblockstore()->load_block_object(account_addr, cur_height, last_block_hash, false, metrics::blockstore_access_from_txpool_sync_status);
            if (_commit_block == nullptr) {
                base::xauto_ptr<base::xvblock_t> _block_ptr = m_para->get_vblockstore()->get_latest_connected_block(account_addr);
                uint64_t start_sync_height = _block_ptr->get_height() + 1;
                xtxpool_info("xunconfirmed_account_t::update account:%s state fall behind,try sync unit from:%llu,end:%llu", account_addr.c_str(), start_sync_height, cur_height);
                if (cur_height > start_sync_height) {
                    uint32_t sync_num = (uint32_t)(cur_height - start_sync_height);
                    mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(account_addr, start_sync_height, sync_num, true, "unit_lack");
                    m_para->get_bus()->push_event(ev);
                }
                return xtxpool_error_unitblock_lack;
            }
            m_para->get_vblockstore()->load_block_input(account_addr, _commit_block.get());

            unit_block = xblock_t::raw_vblock_to_object_ptr(_commit_block.get());
        }
        last_block_hash = unit_block->get_last_block_hash();

        // check if finish load block
        if (unit_block->is_genesis_block() || (unit_block->get_block_class() == base::enum_xvblock_class_full) ||
            (unit_block->get_block_class() == base::enum_xvblock_class_light && unit_block->get_unconfirm_sendtx_num() == 0)) {
            confirm_txs.clear();
            for (auto & tx_info : m_unconfirmed_txs) {
                // clear confirmed tx receiptid
                m_peer_tables->erase(tx_info.second->get_peer_table_sid(), tx_info.second->get_receipt_id());
                xtxpool_info("xunconfirmed_account_t::update pop tx,account=%s,tableid=%d,receiptid=%ld,tx=%s", account_addr.c_str(), tx_info.second->get_peer_table_sid(), tx_info.second->get_receipt_id(), base::xstring_utl::to_hex(tx_info.first).c_str());
            }
            m_unconfirmed_txs.clear();
            break;
        }

        // nil block do nothing
        if (unit_block->get_block_class() == base::enum_xvblock_class_nil) {
            continue;
        }

        // only light unit and has unconfirm sendtx, should update unconfirm cache
        data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(unit_block.get());
        std::vector<base::xvaction_t> unconfirm_sendtx_actions;
        for (auto & tx : lightunit->get_txs()) {
            std::string hash_str = tx->get_tx_hash();
            if (tx->is_send_tx()) {
                auto result = confirm_txs.find(hash_str);
                if (result != confirm_txs.end()) {
                    confirm_txs.erase(result);
                } else {
                    unconfirm_sendtx_actions.push_back(*tx);
                    // auto recv_tx = lightunit->create_one_txreceipt(tx->get_raw_tx().get());
                    // unconfirmed_txs[hash_str] = recv_tx;
                }
                continue;
            }

            if (tx->is_confirm_tx()) {
                confirm_txs.insert(hash_str);
                continue;
            }
        }

        // create unconfirm sendtxs and insert to cache
        if (!unconfirm_sendtx_actions.empty()) {
            auto _cert_block = m_para->get_vblockstore()->load_block_object(account_addr, cur_height + 2, 0, false, metrics::blockstore_access_from_txpool_sync_status);  // only need cert mini-block
            if (_cert_block == nullptr) {
                xtxpool_warn("xunconfirmed_account_t::update fail-load cert block.account=%s,height=%ld", account_addr.c_str(), cur_height + 2);
                return xtxpool_error_unitblock_lack;
            }
            std::vector<base::xfull_txreceipt_t> txreceipts = base::xtxreceipt_build_t::create_all_txreceipts(unit_block.get(), _cert_block.get(), unconfirm_sendtx_actions);
            if (unconfirm_sendtx_actions.size() != txreceipts.size()) {
                xtxpool_error("xunconfirmed_account_t::update fail-create txreceipts.account=%s,height=%ld", account_addr.c_str(), cur_height + 2);
                return xtxpool_error_unitblock_lack;
            }
            for (auto & receipt : txreceipts) {
                xcons_transaction_ptr_t recv_tx = make_object_ptr<xcons_transaction_t>(receipt);
                unconfirmed_txs[recv_tx->get_tx_hash()] = recv_tx;
                xassert(recv_tx->is_recv_tx());
            }
            xtxpool_dbg("xunconfirmed_account_t::update insert.account=%s,height=%ld,unconfirm_sendtxs=%zu", account_addr.c_str(), cur_height, txreceipts.size());
        }
    }

    for (auto & hash : confirm_txs) {
        auto it = m_unconfirmed_txs.find(hash);
        if (it != m_unconfirmed_txs.end()) {
            xtxpool_info("xunconfirmed_account_t::update pop tx,account=%s,tableid=%d,receiptid=%ld,tx=%s", account_addr.c_str(), it->second->get_peer_table_sid(), it->second->get_receipt_id(), base::xstring_utl::to_hex(hash).c_str());
            m_peer_tables->erase(it->second->get_peer_table_sid(), it->second->get_receipt_id());
            m_unconfirmed_txs.erase(it);
        }
    }

    if (latest_height > m_highest_height) {
        m_highest_height = latest_height;
    }

    for (auto & it_s : unconfirmed_txs) {
        auto & tx = it_s.second;
        auto peer_table_sid = tx->get_self_tableid(); // unconfirm tx is recv tx, self tablid of recv tx is for target account of origin tx
        auto confirmid_max = table_state_cache.get_confirmid_max(peer_table_sid);
        if (tx->get_last_action_receipt_id() <= confirmid_max) {
            xtxpool_warn("xunconfirmed_account_t::update account:%s,tableid=%d,confirmid_max=%ld,tx=%s already confirmed", account_addr.c_str(), peer_table_sid, confirmid_max, tx->dump(true).c_str());
            continue;
        }
        xtxpool_info("xunconfirmed_account_t::update push tx,account=%s,tableid=%d,receiptid=%ld,tx=%s", account_addr.c_str(), peer_table_sid, tx->get_last_action_receipt_id(), tx->dump().c_str());
        auto tx_info = std::make_shared<xunconfirmed_tx_info_t>(peer_table_sid, tx->get_last_action_receipt_id());
        m_unconfirmed_txs[it_s.first] = tx_info;
        m_peer_tables->push_tx(tx);
    }

    return xtxpool_success;
}

const xcons_transaction_ptr_t xunconfirmed_account_t::find(const uint256_t & hash) const {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it = m_unconfirmed_txs.find(hash_str);
    if (it != m_unconfirmed_txs.end()) {
        auto & unconfirmed_tx_info = it->second;
        return m_peer_tables->find(unconfirmed_tx_info->get_peer_table_sid(), unconfirmed_tx_info->get_receipt_id());
    }
    return nullptr;
}

xunconfirmed_tx_queue_t::~xunconfirmed_tx_queue_t() {
    uint32_t num = size();
    m_table_info->get_statistic()->dec_unconfirm_tx_cache_num(num);
    // XMETRICS_COUNTER_DECREMENT("txpool_unconfirm_txs_cache_num", num);
    // XMETRICS_COUNTER_SET("table_unconfirm_txs_num" + m_xtable_info.get_table_addr(), num);
}

void xunconfirmed_tx_queue_t::udpate_latest_confirmed_block(xblock_t * block, const xtable_state_cache_t & table_state_cache) {
    xtxpool_dbg("xunconfirmed_tx_queue_t::udpate_latest_confirmed_block block:%s", block->dump().c_str());
    auto it_account = m_unconfirmed_accounts.find(block->get_account());
    if (it_account != m_unconfirmed_accounts.end()) {
        auto & unconfirmed_account = it_account->second;
        unconfirmed_account->update(block, table_state_cache);
        if (unconfirmed_account->size() == 0) {
            xtxpool_dbg("xunconfirmed_tx_queue_t::udpate_latest_confirmed_block unconfirm pop account,block=%s", block->dump().c_str());
            m_unconfirmed_accounts.erase(it_account);
        }
    } else {
        auto unconfirmed_account = std::make_shared<xunconfirmed_account_t>(m_para, &m_peer_tables);
        unconfirmed_account->update(block, table_state_cache);
        if (unconfirmed_account->size() != 0) {
            xtxpool_dbg("xunconfirmed_tx_queue_t::udpate_latest_confirmed_block unconfirm push account,block=%s", block->dump().c_str());
            m_unconfirmed_accounts[block->get_account()] = unconfirmed_account;
        }
    }
}

const xcons_transaction_ptr_t xunconfirmed_tx_queue_t::find(const std::string & account_addr, const uint256_t & hash) const {
    auto it_account = m_unconfirmed_accounts.find(account_addr);
    if (it_account != m_unconfirmed_accounts.end()) {
        auto & unconfirmed_account = it_account->second;
        return unconfirmed_account->find(hash);
    }
    return nullptr;
}

uint64_t xunconfirmed_tx_queue_t::find_account_cache_height(const std::string & account_addr) const {
    auto it_account = m_unconfirmed_accounts.find(account_addr);
    if (it_account != m_unconfirmed_accounts.end()) {
        auto & unconfirmed_account = it_account->second;
        return unconfirmed_account->get_height();
    }
    return 0;
}

void xunconfirmed_tx_queue_t::recover(const xtable_state_cache_t & table_state_cache, const xtablestate_ptr_t & tablestate) {
    // update table-table receipt id state, if all peer_tables were complate, no need to recover, or else, get unconfirmed accounts, and load their unconfirmed txs.
    m_peer_tables.update_receiptid_state(table_state_cache);
    // receiptid_state may not be consistance with account state!
    // if (m_peer_tables.is_complated()) {
    //     xtxpool_dbg("xunconfirmed_tx_queue_t::recover table:%s, all peer tables unconfirmed txs is complate, no need to recover.", m_table_info->get_table_addr().c_str());
    //     return;
    // }

    std::set<std::string> accounts = tablestate->get_unconfirmed_accounts();

    if (m_unconfirmed_accounts.size() != 0 || accounts.size() != 0) {
        xtxpool_info("xunconfirmed_tx_queue_t::recover table=%s,height=%ld,old_unconfirm_size=%zu,new_unconfirm_size=%zu",
            m_table_info->get_table_addr().c_str(), tablestate->get_block_height(), m_unconfirmed_accounts.size(), accounts.size());
    }

    // remove unconfirmed accounts which not found from accounts.
    for (auto it_unconfirmed_account = m_unconfirmed_accounts.begin(); it_unconfirmed_account != m_unconfirmed_accounts.end();) {
        auto & account_addr = it_unconfirmed_account->first;
        auto it_accounts = accounts.find(account_addr);
        if (it_accounts == accounts.end()) {
            xtxpool_info("xunconfirmed_tx_queue_t::recover pop confirmed account.table=%s,height=%ld,account=%s", m_table_info->get_table_addr().c_str(), tablestate->get_block_height(), account_addr.c_str());
            it_unconfirmed_account = m_unconfirmed_accounts.erase(it_unconfirmed_account);
        } else {
            it_unconfirmed_account++;
        }
    }

    // recover unconfirmed txs.
    for (auto & account : accounts) {
        base::xaccount_index_t account_index;
        tablestate->get_account_index(account, account_index);

        uint64_t cache_height = find_account_cache_height(account);
        if (cache_height >= account_index.get_latest_unit_height()) {
            xtxpool_info("xunconfirmed_tx_queue_t::recover same height with index.table=%s,height=%ld,account=%s,cache h:%llu,account_index:%s", m_table_info->get_table_addr().c_str(), tablestate->get_block_height(), account.c_str(), cache_height, account_index.dump().c_str());
            continue;
        }

        xtxpool_info("xunconfirmed_tx_queue_t::recover update unconfirmed account.table=%s,height=%ld,account=%s,cache_height=%ld,index_height=%ld",
            m_table_info->get_table_addr().c_str(), tablestate->get_block_height(), account.c_str(), cache_height, account_index.get_latest_unit_height());
        base::xvaccount_t _vaccount(account);
        base::xauto_ptr<base::xvblock_t> unitblock = m_para->get_vblockstore()->load_block_object(_vaccount, account_index.get_latest_unit_height(), account_index.get_latest_unit_viewid(), false, metrics::blockstore_access_from_txpool_recover);
        if (unitblock != nullptr) {
            m_para->get_vblockstore()->load_block_input(_vaccount, unitblock.get());
            xblock_t * block = dynamic_cast<xblock_t *>(unitblock.get());
            udpate_latest_confirmed_block(block, table_state_cache);
        } else {
            base::xauto_ptr<base::xvblock_t> _block_ptr = m_para->get_vblockstore()->get_latest_connected_block(account);
            uint64_t start_sync_height = _block_ptr->get_height() + 1;
            xtxpool_warn("xunconfirmed_tx_queue_t::recover account:%s fail-state fall behind,try sync unit from:%llu,end:%llu",
                account.c_str(), start_sync_height, account_index.get_latest_unit_height());
            if (account_index.get_latest_unit_height() > start_sync_height) {
                uint32_t sync_num = (uint32_t)(account_index.get_latest_unit_height() - start_sync_height);
                mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(account, start_sync_height, sync_num, true, "unit_lack");
                m_para->get_bus()->push_event(ev);
            }
        }
    }
}

const std::vector<xcons_transaction_ptr_t> xunconfirmed_tx_queue_t::get_resend_txs(uint64_t now) {
    return m_peer_tables.get_latest_receipts_for_resend(now);
}

uint32_t xunconfirmed_tx_queue_t::size() const {
    return m_peer_tables.get_all_txs().size();
}

xcons_transaction_ptr_t xunconfirmed_tx_queue_t::get_unconfirmed_tx(const std::string & to_table_addr, uint64_t receipt_id) const {
    base::xvaccount_t vaccount(to_table_addr);
    auto peer_table_sid = vaccount.get_short_table_id();
    return m_peer_tables.find(peer_table_sid, receipt_id);
}

xcons_transaction_ptr_t xunconfirmed_tx_queue_t::get_unconfirmed_tx(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id) const {
    return m_peer_tables.find(peer_table_sid, receipt_id);
}

NS_END2
