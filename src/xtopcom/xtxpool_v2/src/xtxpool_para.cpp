// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool_para.h"

#include "xconfig/xconfig_register.h"

NS_BEG2(top, xtxpool_v2)

#define TIMER_HEIGHT_PER_DAY (24 * 360)

xtxpool_resources::xtxpool_resources(const observer_ptr<store::xstore_face_t> & store,
                                     const observer_ptr<base::xvblockstore_t> & blockstore,
                                     const observer_ptr<base::xvcertauth_t> & certauth,
                                     const observer_ptr<mbus::xmessage_bus_face_t> & bus)
  : m_store(store), m_blockstore(blockstore), m_certauth(certauth), m_bus(bus) {
}

xtxpool_resources::~xtxpool_resources() {
}

store::xstore_face_t * xtxpool_resources::get_store() const {
    return m_store.get();
}
base::xvblockstore_t * xtxpool_resources::get_vblockstore() const {
    return m_blockstore.get();
}
base::xvcertauth_t * xtxpool_resources::get_certauth() const {
    return m_certauth.get();
}

mbus::xmessage_bus_face_t * xtxpool_resources::get_bus() const {
    return m_bus.get();
}

xreceiptid_state_cache_t & xtxpool_resources::get_receiptid_state_cache() {
    return m_receiptid_state_cache;
}

void xtxpool_resources::update_send_ids_after_add_rsp_id(const base::xreceiptid_state_ptr_t & receiptid_state, const std::set<base::xtable_shortid_t> & all_table_sids) {
    auto self_sid = receiptid_state->get_self_tableid();
    auto it = m_send_ids_after_add_rsp_id.find(self_sid);
    if (it != m_send_ids_after_add_rsp_id.end()) {
        return;
    }
    std::map<base::xtable_shortid_t, uint64_t> table_send_id_map;
    for (auto & peer_sid : all_table_sids) {
        base::xreceiptid_pair_t pair;
        receiptid_state->find_pair(peer_sid, pair);
        table_send_id_map[peer_sid] = pair.get_sendid_max();
    }
    m_send_ids_after_add_rsp_id[self_sid] = table_send_id_map;
}

bool xtxpool_resources::get_send_id_after_add_rsp_id(base::xtable_shortid_t self_sid, base::xtable_shortid_t peer_sid, uint64_t & send_id) const {
    auto it = m_send_ids_after_add_rsp_id.find(self_sid);
    if (it == m_send_ids_after_add_rsp_id.end()) {
        return false;
    }
    auto & table_send_id_map = it->second;
    auto it_table_send_id_map = table_send_id_map.find(peer_sid);
    if (it_table_send_id_map == table_send_id_map.end()) {
        xerror("xtxpool_resources::get_send_id_after_add_rsp_id send id not found self_sid:%d,peer_sid:%d", self_sid, peer_sid);
        return false;
    }
    send_id = it_table_send_id_map->second;
    return true;
}

NS_END2
