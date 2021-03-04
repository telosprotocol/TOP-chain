// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool.h"

#include "xdata/xblocktool.h"

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

int32_t xtxpool_t::push_tx(const xcons_transaction_ptr_t & tx, const xtx_para_t & tx_para) {
    std::string account_addr;
    if (tx->is_recv_tx()) {
        account_addr = tx->get_target_addr();
    } else {
        account_addr = tx->get_source_addr();
    }
    auto table = get_txpool_table_by_addr(account_addr);
    return table->push_tx(tx, tx_para);
}

const xcons_transaction_ptr_t xtxpool_t::pop_tx_by_hash(const std::string & account_addr, const uint256_t & hash, uint8_t subtype, int32_t err) {
    auto table = get_txpool_table_by_addr(account_addr);
    auto tx_ent = table->pop_tx_by_hash(account_addr, hash, subtype, err);
    if (tx_ent == nullptr) {
        return nullptr;
    }
    return tx_ent->get_tx();
}

candidate_accounts xtxpool_t::get_candidate_accounts(const std::string & table_addr, uint32_t count) {
    auto table = get_txpool_table_by_addr(table_addr);
    return table->get_accounts_txs(count);
}

int32_t xtxpool_t::push_back_tx(std::shared_ptr<xtx_entry> tx_ent) {
    std::string account_addr;
    if (tx_ent->get_tx()->is_recv_tx()) {
        account_addr = tx_ent->get_tx()->get_target_addr();
    } else {
        account_addr = tx_ent->get_tx()->get_source_addr();
    }

    auto table = get_txpool_table_by_addr(account_addr);
    return table->push_back_tx(tx_ent);
}

const xcons_transaction_ptr_t xtxpool_t::query_tx(const std::string & account, const uint256_t & hash) const {
    auto table = get_txpool_table_by_addr(account);
    return table->query_tx(account, hash);
}

void xtxpool_t::subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) {
    xassert(zone < enum_xtxpool_table_type_max);
    xassert(front_table_id <= back_table_id);
    xassert(back_table_id < enum_vbucket_has_tables_count);
    std::shared_ptr<xtxpool_shard_info_t> shard = nullptr;
    for (auto & it : m_shards) {
        if (it->is_ids_match(zone, front_table_id, back_table_id)) {
            shard = it;
            break;
        }
    }
    if (shard == nullptr) {
        shard = std::make_shared<xtxpool_shard_info_t>(zone, front_table_id, back_table_id);
        for (uint16_t i = front_table_id; i <= back_table_id; i++) {
            std::string table_addr = data::xblocktool_t::make_address_table_account((base::enum_xchain_zone_index)zone, i);
            m_tables[zone][i] = std::make_shared<xtxpool_table_t>(table_addr, shard.get());
            m_tables[zone][i]->init();
        }
    }
    shard->subscribe();
}

void xtxpool_t::unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) {
    xassert(zone < enum_xtxpool_table_type_max);
    xassert(front_table_id <= back_table_id);
    xassert(back_table_id < enum_vbucket_has_tables_count);
    for (auto it = m_shards.begin(); it != m_shards.end(); it++) {
        if ((*it)->is_ids_match(zone, front_table_id, back_table_id)) {
            (*it)->unsubscribe();
            if ((*it)->get_sub_count() == 0) {
                for (uint16_t i = front_table_id; i <= back_table_id; i++) {
                    m_tables[zone][i]->deinit();
                }
            }
            m_shards.erase(it);
            break;
        }
    }
}

std::shared_ptr<xtxpool_table_t> xtxpool_t::get_txpool_table_by_addr(const std::string & address) const {
    auto xid = base::xvaccount_t::get_xid_from_account(address);
    uint8_t zone = get_vledger_zone_index(xid);
    uint16_t subaddr = get_vledger_subaddr(xid);
    xassert(zone < enum_xtxpool_table_type_max);
    xassert(subaddr < enum_vbucket_has_tables_count);
    xassert(m_tables[zone][subaddr] != nullptr);
    return m_tables[zone][subaddr];
}

}  // namespace xtxpool_v2
}  // namespace top
