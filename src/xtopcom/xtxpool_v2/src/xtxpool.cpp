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
    for (int32_t i = 0; i < enum_xtxpool_table_type_max; i++) {
        for (int32_t j = 0; j < enum_vbucket_has_tables_count; j++) {
            m_tables[i][j] = nullptr;
        }
    }
}

int32_t xtxpool_t::push_send_tx(const std::shared_ptr<xtx_entry> & tx) {
    auto table = get_txpool_table_by_addr(tx);
    if (table == nullptr) {
        return xtxpool_error_account_not_in_charge;
    }
    return table->push_send_tx(tx);
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
}

bool xtxpool_t::is_consensused_recv_receiptid(const std::string & from_addr, const std::string & to_addr, uint64_t receipt_id) const {
    auto table = get_txpool_table_by_addr(to_addr);
    if (table == nullptr) {
        return false;
    }
    return table->is_consensused_recv_receiptid(from_addr, receipt_id);
}

bool xtxpool_t::is_consensused_confirm_receiptid(const std::string & from_addr, const std::string & to_addr, uint64_t receipt_id) const {
    auto table = get_txpool_table_by_addr(from_addr);
    if (table == nullptr) {
        return false;
    }
    return table->is_consensused_confirm_receiptid(to_addr, receipt_id);
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

void xtxpool_t::subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) {
    xtxpool_info("xtxpool_t::subscribe_tables zone:%d,front_table_id:%d,back_table_id:%d", zone, front_table_id, back_table_id);
    xassert(zone < enum_xtxpool_table_type_max);
    xassert(front_table_id <= back_table_id);
    xassert(back_table_id < enum_vbucket_has_tables_count);
    std::shared_ptr<xtxpool_shard_info_t> shard = nullptr;
    std::lock_guard<std::mutex> lck(m_mutex[zone]);
    for (uint32_t i = 0; i < m_shards.size(); i++) {
        if (m_shards[i]->is_ids_match(zone, front_table_id, back_table_id)) {
            shard = m_shards[i];
            break;
        }
    }
    if (shard == nullptr) {
        shard = std::make_shared<xtxpool_shard_info_t>(zone, front_table_id, back_table_id);
        m_shards.push_back(shard);
    }

    uint32_t add_table_num = 0;
    for (uint16_t i = front_table_id; i <= back_table_id; i++) {
        std::string table_addr = data::xblocktool_t::make_address_table_account((base::enum_xchain_zone_index)zone, i);
        if (m_tables[zone][i] == nullptr) {
            m_tables[zone][i] = std::make_shared<xtxpool_table_t>(m_para.get(), table_addr, shard.get(), &m_statistic);
            add_table_num++;
        } else {
            m_tables[zone][i]->add_shard(shard.get());
        }
    }
    m_statistic.inc_table_num(add_table_num);
    shard->subscribe();
}

void xtxpool_t::unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) {
    xtxpool_info("xtxpool_t::unsubscribe_tables zone:%d,front_table_id:%d,back_table_id:%d", zone, front_table_id, back_table_id);
    xassert(zone < enum_xtxpool_table_type_max);
    xassert(front_table_id <= back_table_id);
    xassert(back_table_id < enum_vbucket_has_tables_count);
    std::lock_guard<std::mutex> lck(m_mutex[zone]);
    uint32_t remove_table_num = 0;
    for (auto it = m_shards.begin(); it != m_shards.end(); it++) {
        if ((*it)->is_ids_match(zone, front_table_id, back_table_id)) {
            (*it)->unsubscribe();
            if ((*it)->get_sub_count() == 0) {
                for (uint16_t i = front_table_id; i <= back_table_id; i++) {
                    m_tables[zone][i]->remove_shard((*it).get());
                    if (m_tables[zone][i]->no_shard()) {
                        m_tables[zone][i] = nullptr;
                        remove_table_num++;
                    }
                }
            }
            m_shards.erase(it);
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

const std::vector<xcons_transaction_ptr_t> xtxpool_t::get_resend_txs(uint8_t zone, uint16_t subaddr, uint64_t now) {
    xassert(is_table_subscribed(zone, subaddr));
    xassert(m_tables[zone][subaddr] != nullptr);
    if (m_tables[zone][subaddr] != nullptr) {
        return m_tables[zone][subaddr]->get_resend_txs(now);
    }
    return {};
}

void xtxpool_t::refresh_table(uint8_t zone, uint16_t subaddr, bool refresh_unconfirm_txs) {
    xassert(is_table_subscribed(zone, subaddr));
    xassert(m_tables[zone][subaddr] != nullptr);
    if (m_tables[zone][subaddr] != nullptr) {
        m_tables[zone][subaddr]->refresh_table(refresh_unconfirm_txs);
    }
}

// void xtxpool_t::update_non_ready_accounts(uint8_t zone, uint16_t subaddr) {
//     xassert(is_table_subscribed(zone, subaddr));
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
    table->update_table_state(table_state);
}

xcons_transaction_ptr_t xtxpool_t::get_unconfirmed_tx(const std::string & from_table_addr, const std::string & to_table_addr, uint64_t receipt_id) const {
    auto table = get_txpool_table_by_addr(from_table_addr);
    if (table == nullptr) {
        return nullptr;
    }
    return table->get_unconfirmed_tx(to_table_addr, receipt_id);
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxpool_t::get_lacking_recv_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t max_num) const {
    xassert(is_table_subscribed(zone, subaddr));
    xassert(m_tables[zone][subaddr] != nullptr);
    if (m_tables[zone][subaddr] != nullptr) {
        return m_tables[zone][subaddr]->get_lacking_recv_tx_ids(max_num);
    }
    return {};
}

const std::vector<xtxpool_table_lacking_confirm_tx_hashs_t> xtxpool_t::get_lacking_confirm_tx_hashs(uint8_t zone, uint16_t subaddr, uint32_t max_num) const {
    xassert(is_table_subscribed(zone, subaddr));
    xassert(m_tables[zone][subaddr] != nullptr);
    if (m_tables[zone][subaddr] != nullptr) {
        return m_tables[zone][subaddr]->get_lacking_confirm_tx_hashs(max_num);
    }
    return {};
}

bool xtxpool_t::need_sync_lacking_receipts(uint8_t zone, uint16_t subaddr) const {
    xassert(is_table_subscribed(zone, subaddr));
    xassert(m_tables[zone][subaddr] != nullptr);
    if (m_tables[zone][subaddr] != nullptr) {
        return m_tables[zone][subaddr]->need_sync_lacking_receipts();
    }
    return false;
}

bool xtxpool_t::is_table_subscribed(uint8_t zone, uint16_t table_id) const {
    xassert(table_id < enum_vbucket_has_tables_count);
    std::lock_guard<std::mutex> lck(m_mutex[zone]);
    for (uint32_t i = 0; i < m_shards.size(); i++) {
        if (m_shards[i]->is_id_contained(zone, table_id)) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<xtxpool_table_t> xtxpool_t::get_txpool_table_by_addr(const std::string & address) const {
    auto xid = base::xvaccount_t::get_xid_from_account(address);
    uint8_t zone = get_vledger_zone_index(xid);
    uint16_t subaddr = get_vledger_subaddr(xid);
    xassert(zone < enum_xtxpool_table_type_max);
    xassert(subaddr < enum_vbucket_has_tables_count);
    if (is_table_subscribed(zone, subaddr)) {
        xassert(m_tables[zone][subaddr] != nullptr);
        return m_tables[zone][subaddr];
    }
    xtxpool_warn("xtxpool_t::get_txpool_table_by_addr account:%s,table not found:zone:%d,subaddr:%d", address.c_str(), zone, subaddr);
    return nullptr;
}

std::shared_ptr<xtxpool_table_t> xtxpool_t::get_txpool_table_by_addr(const std::shared_ptr<xtx_entry> & tx) const {
    base::xtable_index_t tableindex = tx->get_tx()->get_self_table_index();
    uint8_t zone = tableindex.get_zone_index();
    uint8_t subaddr = tableindex.get_subaddr();
    xassert(zone < enum_xtxpool_table_type_max);
    xassert(subaddr <= (enum_vbucket_has_tables_count -1));
    if (is_table_subscribed(zone, subaddr)) {
        xassert(m_tables[zone][subaddr] != nullptr);
        return m_tables[zone][subaddr];
    }
    xtxpool_warn("xtxpool_t::get_txpool_table_by_addr table not found:zone:%d,subaddr:%d", zone, subaddr);
    return nullptr;
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

}  // namespace xtxpool_v2
}  // namespace top
