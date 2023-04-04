// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_gossip.h"
#include "xsync/xsync_log.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xnative_contract_address.h"

NS_BEG2(top, sync)

const uint32_t FROZEN_TIME_INTERVAl = 180;
const uint32_t COMMON_TIME_INTERVAl = 180;
const uint32_t BEHIND_DUMP_INTERVAl = 180;

const uint32_t frozen_add_role_gossip_factor = 20;
const uint32_t common_add_role_gossip_factor = 3;

const uint32_t frozen_timer_gossip_factor = 1;
const uint32_t common_timer_gossip_factor = 1;

const uint32_t frozen_behind_gossip_factor = 10;
const uint32_t common_behind_gossip_factor = 3;

const uint32_t max_peer_behind_count = 10000;

xsync_gossip_t::xsync_gossip_t(std::string vnode_id,  xsync_store_face_t* sync_store,
        xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_role_chains_mgr(role_chains_mgr),
m_role_xips_mgr(role_xips_mgr),
m_sync_sender(sync_sender) {
}

xsync_gossip_t::~xsync_gossip_t() {
}

void xsync_gossip_t::on_timer() {

    if (m_time_rejecter.reject()){
        return;
    }
    
    if (m_frozen_count % FROZEN_TIME_INTERVAl == 0) {
        process_timer(true);
        m_frozen_count = 1;
    } else {
        m_frozen_count++;
    }

    if (m_count % COMMON_TIME_INTERVAl == 0) {
        process_timer(false);
        m_count = 1;
    } else {
        m_count++;
    }

    if (m_behind_dump_count % BEHIND_DUMP_INTERVAl == 0)
        dump_behind();
    m_behind_dump_count++;
}

void xsync_gossip_t::on_chain_timer(const mbus::xevent_ptr_t& e) {

    auto bme = dynamic_xobject_ptr_cast<mbus::xevent_chain_timer_t>(e);
    base::xvblock_t* time_block = bme->time_block;

    if (time_block->get_height() <= m_chain_timer_height)
        return;
    m_chain_timer_height = time_block->get_height();

    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    for (const auto &role_it: roles) {

        const common::xnode_address_t &self_addr = role_it.first;
        const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;

        common::xnode_type_t role_type = common::real_part_type(self_addr.type());
        if (common::has<common::xnode_type_t::storage>(role_type) || role_type==common::xnode_type_t::frozen)
            continue;

        if (!m_role_xips_mgr->vrf_gossip_with_archive(time_block, role_type))
            continue;
        
        walk_role(self_addr, role_chains, enum_walk_type_timer);
    }
}

void xsync_gossip_t::add_role(const common::xnode_address_t& addr) {
    common::xnode_type_t type = common::real_part_type(addr.type());

    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    for (const auto &role_it: roles) {
        if (real_part_type(role_it.first.type()) == type) {
            const common::xnode_address_t &self_addr = role_it.first;
            const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;
            walk_role(self_addr, role_chains, enum_walk_type_add_role);
            return;
        }
    }

    update_behind_role();
}

void xsync_gossip_t::remove_role(const common::xnode_address_t& addr) {
    update_behind_role();
}

void xsync_gossip_t::walk_role(const common::xnode_address_t &self_addr, const std::shared_ptr<xrole_chains_t> &role_chains, enum_walk_type walk_type) {

    std::vector<xgossip_chain_info_ptr_t> info_list;

    const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();
    for (const auto &it: chains) {

        if (!m_role_chains_mgr->exists(it.second.address))
            continue;

        if (it.second.sync_policy == enum_chain_sync_policy_fast) {
            continue;
        }

        base::xvaccount_t _vaddr(it.second.address);
        auto zone_id = _vaddr.get_zone_index();
        if (it.second.address != sys_drand_addr) {
            if ((zone_id == base::enum_chain_zone_zec_index) || (zone_id == base::enum_chain_zone_beacon_index)) {
                continue;
            }
        }

        xgossip_chain_info_ptr_t info = std::make_shared<xgossip_chain_info_t>();
        info->owner = it.second.address;
        info->max_height = m_sync_store->get_latest_end_block_height(it.second.address, enum_chain_sync_policy_full);;
        info_list.push_back(info);
    }

    if (!info_list.empty()) {
        if (walk_type == enum_walk_type_add_role)
            xsync_info("xsync_gossip add_role send gossip %s count(%d)", self_addr.to_string().c_str(), info_list.size());
        else if (walk_type == enum_walk_type_timer) {
            //xsync_dbg("xsync_gossip timer send gossip %s count(%d)", self_addr.to_string().c_str(), info_list.size());
        }
        if (common::has<common::xnode_type_t::frozen>(self_addr.type())) {

            uint32_t factor = 0;
            if (walk_type == enum_walk_type_add_role)
                factor = frozen_add_role_gossip_factor;
            else
                factor = frozen_timer_gossip_factor;

            send_frozen_gossip(self_addr, info_list, factor);
        } else {

            uint32_t factor = 0;
            if (walk_type == enum_walk_type_add_role)
                factor = common_add_role_gossip_factor;
            else
                factor = common_timer_gossip_factor;

            send_gossip(self_addr, info_list, factor, sync::enum_gossip_target_type_neighbor);
        }
    }
}

void xsync_gossip_t::process_timer(bool is_frozen) {

    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    for (const auto &role_it: roles) {

        const common::xnode_address_t &self_addr = role_it.first;
        const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;

        if (is_frozen) {
            if (common::has<common::xnode_type_t::frozen>(self_addr.type())) {
                walk_role(self_addr, role_chains, enum_walk_type_timer);
                return;
            }
        } else {
            if (common::has<common::xnode_type_t::frozen>(self_addr.type()))
                continue;
            walk_role(self_addr, role_chains, enum_walk_type_timer);
        }
    }
}

void xsync_gossip_t::handle_message(const std::vector<xgossip_chain_info_ptr_t> &info_list, 
    const common::xnode_address_t &from_address, const common::xnode_address_t &network_self, std::map<std::string, xgossip_behind_info_t> &behind_chain_set) {

    std::vector<xgossip_chain_info_ptr_t> info_list_rsp;

    //xsync_dbg("xsync_gossip recv gossip from %s count(%d)", from_address.to_string().c_str(), info_list.size());

    uint32_t size = info_list.size();
    if (size == 0)
        return;

    uint32_t random_pos = RandomUint32()%size;
    uint32_t behind_block_count = 0;

    for (uint32_t i=0; i<size; i++) {

        // idx->size-1, 0->idx-1
        uint32_t idx = (random_pos+i)%size;

        const xgossip_chain_info_ptr_t &info = info_list[idx];

        if (!m_role_chains_mgr->exists(info->owner)) {
            xsync_warn("xsync_gossip local not exist %s peer(%lu)",
                            info->owner.c_str(), info->max_height);
            continue;
        }

        uint64_t local_height = m_sync_store->get_latest_end_block_height(info->owner, enum_chain_sync_policy_full);
        update_behind(info->owner, local_height, info->max_height);

        auto all_chain = m_role_chains_mgr->get_all_chains();
        auto _chain = all_chain.find(info->owner);
        if (_chain != all_chain.end()) {
            if (all_chain[info->owner].sync_policy != enum_chain_sync_policy_full) {
                xsync_dbg("xsync_gossip  %s is sync(%d).", info->owner.c_str(), all_chain[info->owner].sync_policy);
                continue;
            }
        }

        if (info->max_height > local_height) {

            if (behind_block_count <= max_peer_behind_count) {
                xsync_info("xsync_gossip local is lower %s local(%lu) peer(%lu) %s %s",
                    info->owner.c_str(), local_height, info->max_height, network_self.to_string().c_str(), from_address.to_string().c_str());
                
                xgossip_behind_info_t behind_info(local_height, info->max_height);

                behind_chain_set.insert(std::make_pair(info->owner, behind_info));
                behind_block_count += (info->max_height - local_height);
            } else {
                xsync_dbg("xsync_gossip local is lower(ignore) %s local(%lu) peer(%lu) %s", 
                    info->owner.c_str(), local_height, info->max_height, from_address.to_string().c_str());
            }

        } else if (info->max_height < local_height) {
            xsync_info("xsync_gossip local is higher %s local(%lu) peer(%lu) %s -> %s",
                info->owner.c_str(), local_height, info->max_height, network_self.to_string().c_str(), from_address.to_string().c_str());
            xgossip_chain_info_ptr_t info_rsp = std::make_shared<xgossip_chain_info_t>();
            info_rsp->owner = info->owner;
            info_rsp->max_height = local_height;
            info_list_rsp.push_back(info_rsp);
        } 
    }

    if (!info_list_rsp.empty()) {
        if (common::has<common::xnode_type_t::frozen>(network_self.type())) {
            send_frozen_gossip_to_target(network_self, info_list_rsp, from_address);
        } else {
            send_gossip_to_target(network_self, info_list_rsp, from_address);
        }
    }
}

void xsync_gossip_t::send_gossip(const common::xnode_address_t &self_addr, std::vector<xgossip_chain_info_ptr_t> &info_list, uint32_t max_peers, enum_gossip_target_type target_type) {

    xbyte_buffer_t bloom_data(32, 0);

    for (uint32_t i=0; i<max_peers; i++) {

        m_sync_sender->send_gossip(info_list, bloom_data, self_addr, 1, target_type);

        XMETRICS_GAUGE(metrics::xsync_gossip_send, 1);
    }
}

void xsync_gossip_t::send_gossip_to_target(const common::xnode_address_t &self_addr, std::vector<xgossip_chain_info_ptr_t> &info_list, const common::xnode_address_t &target_addr) {

    xbyte_buffer_t bloom_data(32, 0);

    m_sync_sender->send_gossip_to_target(info_list, bloom_data, self_addr, target_addr);

    XMETRICS_GAUGE(metrics::xsync_gossip_send, 1);
}

void xsync_gossip_t::send_frozen_gossip(const common::xnode_address_t &self_addr, std::vector<xgossip_chain_info_ptr_t> &info_list, uint32_t max_peers) {

    xbyte_buffer_t bloom_data(32, 0);

    for (uint32_t i=0; i<max_peers; i++) {
        m_sync_sender->send_frozen_gossip(info_list, bloom_data, self_addr);
        XMETRICS_GAUGE(metrics::xsync_gossip_send, 1);
    }
}

void xsync_gossip_t::send_frozen_gossip_to_target(const common::xnode_address_t &self_addr, std::vector<xgossip_chain_info_ptr_t> &info_list, const common::xnode_address_t &target_addr) {

    xbyte_buffer_t bloom_data(32, 0);

    m_sync_sender->send_frozen_gossip_to_target(info_list, bloom_data, self_addr, target_addr);

    XMETRICS_GAUGE(metrics::xsync_gossip_send, 1);
}

void xsync_gossip_t::update_behind(const std::string &address, uint64_t local_height, uint64_t peer_height) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto it = m_chain_behind_info.find(address);
    if (it == m_chain_behind_info.end()) {
        xgossip_behind_info_t info(local_height, peer_height);
        m_chain_behind_info.insert(std::make_pair(address, info));
    } else {
        if (peer_height > it->second.peer_height)
            it->second.peer_height = peer_height;

        if (local_height > it->second.local_height)
            it->second.local_height = local_height;

        if (it->second.local_height > it->second.peer_height)
            it->second.peer_height = it->second.local_height;
    }
}

void xsync_gossip_t::dump_behind() {

    uint32_t count = 0;
    uint64_t total_behind_height = 0;

    std::unique_lock<std::mutex> lock(m_lock);

    for (auto &it: m_chain_behind_info) {
        if (it.second.local_height < it.second.peer_height) {
            count++;
            uint64_t behind_height = it.second.peer_height - it.second.local_height;
            total_behind_height += behind_height;
            xsync_info("chain_behind_info %s local(%lu) peer(%lu) behind=%lu", it.first.c_str(), 
                it.second.local_height, it.second.peer_height, behind_height);
        }
    }

    if (count > 0)
        xsync_info("chain_behind_info total chains=%u blocks=%lu", count, total_behind_height);
}

void xsync_gossip_t::update_behind_role() {
    std::unique_lock<std::mutex> lock(m_lock);
    std::map<std::string, xgossip_behind_info_t>::iterator it = m_chain_behind_info.begin();
    for (; it!=m_chain_behind_info.end(); ) {
        const std::string &address = it->first;
        if (!m_role_chains_mgr->exists(address)) {
            m_chain_behind_info.erase(it++);
        } else {
            ++it;
        }
    }
}

NS_END2
