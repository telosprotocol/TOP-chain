// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool.h"

#include "xdata/xblocktool.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xtxpool_v2/xtxpool_para.h"
#include "xvledger/xvledger.h"

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

xtxpool_t::xtxpool_t(const std::shared_ptr<xtxpool_resources_face> & para) : m_para(para) {
    for (uint16_t i = 0; i < enum_vbucket_has_tables_count; i++) {
        base::xtable_index_t tableindex(base::enum_chain_zone_consensus_index, i);
        m_all_table_sids.insert(tableindex.to_table_shortid());
        m_tables[base::enum_chain_zone_consensus_index].push_back(nullptr);
    }
    for (uint16_t i = 0; i < MAIN_CHAIN_REC_TABLE_USED_NUM; i++) {
        base::xtable_index_t tableindex(base::enum_chain_zone_beacon_index, i);
        m_all_table_sids.insert(tableindex.to_table_shortid());
        m_tables[base::enum_chain_zone_beacon_index].push_back(nullptr);
    }
    for (uint16_t i = 0; i < MAIN_CHAIN_ZEC_TABLE_USED_NUM; i++) {
        base::xtable_index_t tableindex(base::enum_chain_zone_zec_index, i);
        m_all_table_sids.insert(tableindex.to_table_shortid());
        m_tables[base::enum_chain_zone_zec_index].push_back(nullptr);
    }
}

bool table_zone_subaddr_check(uint8_t zone, uint16_t subaddr) {
    if ((zone >= xtxpool_zone_type_max) || (zone == base::enum_chain_zone_consensus_index && subaddr >= enum_vbucket_has_tables_count) ||
        (zone == base::enum_chain_zone_beacon_index && subaddr >= MAIN_CHAIN_REC_TABLE_USED_NUM) ||
        (zone == base::enum_chain_zone_zec_index && subaddr >= MAIN_CHAIN_ZEC_TABLE_USED_NUM)) {
        xwarn("table_zone_subaddr_check zone:%d or subaddr:%d invalidate", zone, subaddr);
        return false;
    }
    return true;
}

int32_t xtxpool_t::push_send_tx(const std::shared_ptr<xtx_entry> & tx) {
    auto table = get_txpool_table_by_addr(tx);
    if (table == nullptr) {
        return xtxpool_error_account_not_in_charge;
    }
    auto ret = table->push_send_tx(tx);
    return ret;
}

int32_t xtxpool_t::push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send, bool is_pulled) {
    XMETRICS_TIME_RECORD("txpool_message_unit_receipt_push_receipt");
    auto table = get_txpool_table_by_addr(tx);
    if (table == nullptr) {
        return xtxpool_error_account_not_in_charge;
    }
    auto ret = table->push_receipt(tx, is_self_send);

    if (ret == xsuccess) {
        m_statistic.update_receipt_recv_num(tx->get_tx(), is_pulled);
    }
    return ret;
}

void xtxpool_t::print_statistic_values() const {
    m_statistic.print();

    uint32_t sender_cache_size = 0;
    uint32_t receiver_cache_size = 0;
    uint32_t height_record_size = 0;
    uint32_t table_sender_cache_size = 0;
    uint32_t table_receiver_cache_size = 0;
    uint32_t table_height_record_size = 0;

    for (uint16_t i = 0; i < enum_vbucket_has_tables_count; i++) {
        auto table = m_tables[base::enum_chain_zone_consensus_index][i];
        if (table != nullptr) {
            table->unconfirm_cache_status(table_sender_cache_size, table_receiver_cache_size, table_height_record_size);
            xinfo(
                "xtxpool_t::print_statistic_values table:%d,cache size:%u:%u:%u", table->table_sid(), table_sender_cache_size, table_receiver_cache_size, table_height_record_size);
            sender_cache_size += table_sender_cache_size;
            receiver_cache_size += table_receiver_cache_size;
            height_record_size += table_height_record_size;
        }
    }
    for (uint16_t i = 0; i < MAIN_CHAIN_REC_TABLE_USED_NUM; i++) {
        auto table = m_tables[base::enum_chain_zone_beacon_index][i];
        if (table != nullptr) {
            table->unconfirm_cache_status(table_sender_cache_size, table_receiver_cache_size, table_height_record_size);
            xinfo(
                "xtxpool_t::print_statistic_values table:%d,cache size:%u:%u:%u", table->table_sid(), table_sender_cache_size, table_receiver_cache_size, table_height_record_size);
            sender_cache_size += table_sender_cache_size;
            receiver_cache_size += table_receiver_cache_size;
            height_record_size += table_height_record_size;
        }
    }
    for (uint16_t i = 0; i < MAIN_CHAIN_ZEC_TABLE_USED_NUM; i++) {
        auto table = m_tables[base::enum_chain_zone_zec_index][i];
        if (table != nullptr) {
            table->unconfirm_cache_status(table_sender_cache_size, table_receiver_cache_size, table_height_record_size);
            xinfo(
                "xtxpool_t::print_statistic_values table:%d,cache size:%u:%u:%u", table->table_sid(), table_sender_cache_size, table_receiver_cache_size, table_height_record_size);
            sender_cache_size += table_sender_cache_size;
            receiver_cache_size += table_receiver_cache_size;
            height_record_size += table_height_record_size;
        }
    }

    XMETRICS_COUNTER_SET("txpool_sender_unconfirm_cache", sender_cache_size);
    XMETRICS_COUNTER_SET("txpool_receiver_unconfirm_cache", receiver_cache_size);
    XMETRICS_COUNTER_SET("txpool_height_record_cache", height_record_size);
}

const xcons_transaction_ptr_t xtxpool_t::pop_tx(const tx_info_t & txinfo) {
    auto table = get_txpool_table_by_addr(txinfo.get_addr());
    if (table == nullptr) {
        return nullptr;
    }
    auto tx_ent = table->pop_tx(txinfo, true);
    if (tx_ent == nullptr) {
        return nullptr;
    }
    return tx_ent->get_tx();
}

ready_accounts_t xtxpool_t::get_ready_accounts(const xtxs_pack_para_t & pack_para) {
    auto table = get_txpool_table_by_addr(pack_para.get_table_addr());
    if (table == nullptr) {
        return {};
    }
    return table->get_ready_accounts(pack_para);
}

std::vector<xcons_transaction_ptr_t> xtxpool_t::get_ready_txs(const xtxs_pack_para_t & pack_para) {
    auto table = get_txpool_table_by_addr(pack_para.get_table_addr());
    if (table == nullptr) {
        return {};
    }
    return table->get_ready_txs(pack_para);
}

const std::shared_ptr<xtx_entry> xtxpool_t::query_tx(const std::string & account_addr, const uint256_t & hash) const {
    auto table = get_txpool_table_by_addr(account_addr);
    if (table == nullptr) {
        xtxpool_warn("xtxpool_t::query_tx table not found, account:%s", account_addr.c_str());
        return nullptr;
    }
    return table->query_tx(account_addr, hash);
}

void xtxpool_t::updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) {
    auto table = get_txpool_table_by_addr(account_addr);
    if (table == nullptr) {
        return;
    }
    return table->updata_latest_nonce(account_addr, latest_nonce);
}

void xtxpool_t::subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) {
    xtxpool_info("xtxpool_t::subscribe_tables zone:%d,front_table_id:%d,back_table_id:%d", zone, front_table_id, back_table_id);
    if (front_table_id > back_table_id) {
        xerror("xtxpool_t::subscribe_tables table id invalidate front_table_id:%d back_table_id%d", front_table_id, back_table_id);
        return;
    }
    if (!table_zone_subaddr_check(zone, back_table_id)) {
        return;
    }

    std::lock_guard<std::mutex> lck(m_mutex[zone]);
    for (uint32_t i = 0; i < m_roles[zone].size(); i++) {
        if (m_roles[zone][i]->is_ids_match(zone, front_table_id, back_table_id, node_type)) {
            m_roles[zone][i]->add_sub_count();
            return;
        }
    }
    auto role = std::make_shared<xtxpool_role_info_t>(zone, front_table_id, back_table_id, node_type);
    m_roles[zone].push_back(role);
    role->add_sub_count();

    xtxpool_info("xtxpool_t::subscribe_tables sub tables:zone:%d,front_table_id:%d,back_table_id:%d", zone, front_table_id, back_table_id);

    uint32_t add_table_num = 0;
    for (uint16_t i = front_table_id; i <= back_table_id; i++) {
        std::string table_addr = data::xblocktool_t::make_address_table_account((base::enum_xchain_zone_index)zone, i);
        if (m_tables[zone][i] == nullptr) {
            m_tables[zone][i] = std::make_shared<xtxpool_table_t>(m_para.get(), table_addr, role.get(), &m_statistic, &m_all_table_sids);
            add_table_num++;
        } else {
            m_tables[zone][i]->add_role(role.get());
        }
    }
    if (add_table_num > 0) {
        m_statistic.inc_table_num(add_table_num);
        {
            std::lock_guard<std::mutex> lck(m_peer_table_height_cache_mutex);
            m_peer_table_height_cache.clear();
        }
    }
}

void xtxpool_t::unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) {
    xtxpool_info("xtxpool_t::unsubscribe_tables zone:%d,front_table_id:%d,back_table_id:%d", zone, front_table_id, back_table_id);
    if (front_table_id > back_table_id) {
        xerror("xtxpool_t::unsubscribe_tables table id invalidate front_table_id:%d back_table_id%d", front_table_id, back_table_id);
        return;
    }
    if (!table_zone_subaddr_check(zone, back_table_id)) {
        return;
    }
    std::lock_guard<std::mutex> lck(m_mutex[zone]);
    uint32_t remove_table_num = 0;
    for (auto it = m_roles[zone].begin(); it != m_roles[zone].end(); it++) {
        if ((*it)->is_ids_match(zone, front_table_id, back_table_id, node_type)) {
            (*it)->del_sub_count();
            if ((*it)->get_sub_count() != 0) {
                return;
            }
            xtxpool_info("xtxpool_t::unsubscribe_tables unsub tables zone:%d,front_table_id:%d,back_table_id:%d", zone, front_table_id, back_table_id);
            for (uint16_t i = front_table_id; i <= back_table_id; i++) {
                m_tables[zone][i]->remove_role((*it).get());
                if (m_tables[zone][i]->no_role()) {
                    xinfo("xtxpool_t::unsubscribe_tables erase table zone:%d idx:%d", zone, i);
                    m_tables[zone][i] = nullptr;
                    remove_table_num++;
                }
            }
            m_roles[zone].erase(it);
            break;
        }
    }
    m_statistic.dec_table_num(remove_table_num);
}

void xtxpool_t::on_block_confirmed(xblock_t * block) {
    if (!block->is_tableblock() || block->is_genesis_block()) {
        return;
    }

    auto table = get_txpool_table_by_addr(block->get_account());
    if (table == nullptr) {
        return;
    }

    table->on_block_confirmed(block);
}

int32_t xtxpool_t::verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) {
    auto table = get_txpool_table_by_addr(account);
    if (table == nullptr) {
        return xtxpool_error_account_not_in_charge;
    }

    return table->verify_txs(account, txs);
}

void xtxpool_t::refresh_table(uint8_t zone, uint16_t subaddr) {
    auto table = get_txpool_table(zone, subaddr);
    if (table != nullptr) {
        table->refresh_table();
    }
}

// void xtxpool_t::update_non_ready_accounts(uint8_t zone, uint16_t subaddr) {
//     xassert(m_tables[zone][subaddr] != nullptr);
//     if (m_tables[zone][subaddr] != nullptr) {
//         m_tables[zone][subaddr]->update_non_ready_accounts();
//     }
// }

void xtxpool_t::update_table_state(const data::xtablestate_ptr_t & table_state) {
    xtxpool_info("xtxpool_t::update_table_state table:%s height:%llu", table_state->get_account().c_str(), table_state->get_block_height());
    XMETRICS_TIME_RECORD("cons_tableblock_verfiy_proposal_update_receiptid_state");
    auto table = get_txpool_table_by_addr(table_state->get_account().c_str());
    if (table == nullptr) {
        return;
    }
    m_para->get_receiptid_state_cache().update_table_receiptid_state(table_state->get_receiptid_state());
    table->update_table_state(table_state);
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxpool_t::get_lacking_recv_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const {
    auto table = get_txpool_table(zone, subaddr);
    if (table != nullptr) {
        return table->get_lacking_recv_tx_ids(total_num);
    }
    return {};
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxpool_t::get_lacking_confirm_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const {
    auto table = get_txpool_table(zone, subaddr);
    if (table != nullptr) {
        return table->get_lacking_confirm_tx_ids(total_num);
    }
    return {};
}

bool xtxpool_t::need_sync_lacking_receipts(uint8_t zone, uint16_t subaddr) const {
    auto table = get_txpool_table(zone, subaddr);
    if (table != nullptr) {
        return table->need_sync_lacking_receipts();
    }
    return false;
}

std::shared_ptr<xtxpool_table_t> xtxpool_t::get_txpool_table_by_addr(const std::string & address) const {
    auto xid = base::xvaccount_t::get_xid_from_account(address);
    return get_txpool_table(get_vledger_zone_index(xid), get_vledger_subaddr(xid));
}

std::shared_ptr<xtxpool_table_t> xtxpool_t::get_txpool_table_by_addr(const std::shared_ptr<xtx_entry> & tx) const {
    base::xtable_index_t tableindex = tx->get_tx()->get_self_table_index();
    return get_txpool_table(tableindex.get_zone_index(), tableindex.get_subaddr());
}

std::shared_ptr<xtxpool_table_t> xtxpool_t::get_txpool_table(uint8_t zone, uint16_t subaddr) const {
    if (!table_zone_subaddr_check(zone, subaddr)) {
        return nullptr;
    }
    return m_tables[zone][subaddr];
}

xobject_ptr_t<xtxpool_face_t> xtxpool_instance::create_xtxpool_inst(const observer_ptr<store::xstore_face_t> & store,
                                                                    const observer_ptr<base::xvblockstore_t> & blockstore,
                                                                    const observer_ptr<base::xvcertauth_t> & certauth,
                                                                    const observer_ptr<mbus::xmessage_bus_face_t> & bus) {
    auto para = std::make_shared<xtxpool_resources>(store, blockstore, certauth, bus);
    auto xtxpool = top::make_object_ptr<xtxpool_t>(para);
    return xtxpool;
}

bool xready_account_t::put_tx(const xcons_transaction_ptr_t & tx) {
    enum_transaction_subtype new_tx_subtype = tx->get_tx_subtype();
    if (new_tx_subtype == enum_transaction_subtype_self) {
        new_tx_subtype = enum_transaction_subtype_send;
    }

    if (!m_txs.empty()) {
        enum_transaction_subtype first_tx_subtype = m_txs[0]->get_tx_subtype();
        if (first_tx_subtype == enum_transaction_subtype_self) {
            first_tx_subtype = enum_transaction_subtype_send;
        }
        if (new_tx_subtype != first_tx_subtype) {
            xtxpool_info("xready_account_t::put_tx fail tx type not same with txs already in, tx:%s,m_txs[0]:%s", tx->dump().c_str(), m_txs[0]->dump().c_str());
            return false;
        }

        if ((first_tx_subtype != enum_transaction_subtype_confirm) &&
            (m_txs[0]->get_transaction()->get_tx_type() != xtransaction_type_transfer || tx->get_transaction()->get_tx_type() != xtransaction_type_transfer)) {
            xtxpool_info("xready_account_t::put_tx fail non transfer tx, tx:%s,m_txs[0]:%s", tx->dump().c_str(), m_txs[0]->dump().c_str());
            return false;
        }
    }
    m_txs.push_back(tx);
    return true;
}

void xtxpool_t::update_peer_receipt_id_state(const base::xreceiptid_state_ptr_t & receiptid_state) {
    m_para->get_receiptid_state_cache().update_table_receiptid_state(receiptid_state);
}

void xtxpool_t::build_recv_tx(base::xtable_shortid_t from_table_sid,
                              base::xtable_shortid_t to_table_sid,
                              std::vector<uint64_t> receiptids,
                              std::vector<xcons_transaction_ptr_t> & receipts) {
    base::xtable_index_t table_idx(from_table_sid);
    auto table = get_txpool_table(table_idx.get_zone_index(), table_idx.get_subaddr());
    if (table == nullptr) {
        return;
    }
    table->build_recv_tx(to_table_sid, receiptids, receipts);
}

void xtxpool_t::build_confirm_tx(base::xtable_shortid_t from_table_sid,
                                 base::xtable_shortid_t to_table_sid,
                                 std::vector<uint64_t> receiptids,
                                 std::vector<xcons_transaction_ptr_t> & receipts) {
    base::xtable_index_t table_idx(to_table_sid);
    auto table = get_txpool_table(table_idx.get_zone_index(), table_idx.get_subaddr());
    if (table == nullptr) {
        return;
    }
    table->build_confirm_tx(from_table_sid, receiptids, receipts);
}

}  // namespace xtxpool_v2
}  // namespace top
