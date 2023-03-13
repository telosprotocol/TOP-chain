// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

xrole_xips_manager_t::~xrole_xips_manager_t() {

}

xrole_xips_manager_t::xrole_xips_manager_t(std::string vnode_id):
m_vnode_id(vnode_id) {
}

void xrole_xips_manager_t::add_role(const common::xnode_address_t & self_xip,
                                    const std::vector<common::xnode_address_t> & neighbours,
                                    const std::vector<common::xnode_address_t> & parents,
                                    const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & network_driver,
                                    const std::set<uint16_t> & table_ids) {
    std::unique_lock<std::mutex> lock(m_lock);
    // remove old first : version is different
    for (auto & pair : m_map) {
        if (pair.first.cluster_address() == self_xip.cluster_address()) {
            m_map.erase(pair.first);
            break;
        }
    }
#if defined(DEBUG)
    for (auto const & neighbour : neighbours)
        xsync_dbg("xrole_xips_manager_t::add_role, neighbours: %s", neighbour.to_string().c_str());
    for (auto const & p : parents)
        xsync_dbg("xrole_xips_manager_t::add_role, parents: %s", p.to_string().c_str());
#endif
    m_map[self_xip] = {self_xip,
                       create_xip_vector_ptr(neighbours, self_xip),
                       create_xip_vector_ptr(parents, self_xip),
                       std::make_shared<std::vector<common::xnode_address_t>>(neighbours),
                       table_ids,
                       network_driver};

    m_vnetwork_driver = network_driver;
    m_self_xip = self_xip;
}

void xrole_xips_manager_t::remove_role(const common::xnode_address_t& self_xip) {
    std::unique_lock<std::mutex> lock(m_lock);
    m_map.erase(self_xip);
}

void xrole_xips_manager_t::remove_xips_by_id(const common::xnode_id_t& id) {
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        pair.second.remove_xips_by_id(id);
    }
//    xrole_xips_t::remove_xips_by_id(m_archive_xips, id);
//    xrole_xips_t::remove_xips_by_id(m_edge_archive_xips, id);
}

const common::xnode_address_t xrole_xips_manager_t::get_static_xip() {
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if (real_part_type(pair.first.type()) == common::xnode_type_t::frozen) {
            return pair.first;
        }
    }

    return {};
}

std::vector<common::xnode_address_t> xrole_xips_manager_t::get_rand_neighbors(const common::xnode_address_t& self_xip, uint32_t max_peers) {
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if(pair.first.cluster_address() == self_xip.cluster_address()) {
            return get_rand_peers(pair.second.neighbour_xips, max_peers);
        }
    }
    return {};
}

std::vector<common::xnode_address_t> xrole_xips_manager_t::get_rand_parents(const common::xnode_address_t& self_xip, uint32_t max_peers) {
/*    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if(pair.first.cluster_address() == self_xip.cluster_address()) {
            return get_rand_peers(pair.second.parent_xips, max_peers);
        }
    }
    return {};*/
    if (m_vnetwork_driver == nullptr)
        return {};
    if (!common::has<common::xnode_type_t::consensus_validator>(m_vnetwork_driver->type())) {
        return {};
    }
    auto const & parents_info = m_vnetwork_driver->parents_info2();
    std::vector<common::xnode_address_t> addresses;
    for (auto const & info : parents_info) {
        //addresses.push_back(top::get<data::xnode_info_t>(info).address);
        addresses.push_back(info.second.address);
    }
    xip_vector_ptr parents_xips = create_xip_vector_ptr(addresses, self_xip);
    return get_rand_peers(parents_xips, max_peers);
}

std::vector<common::xnode_address_t> xrole_xips_manager_t::get_rand_full_nodes(uint32_t max_peers) {
    if (m_vnetwork_driver == nullptr)
        return {};
    xip_vector_ptr archive_xips{};
    std::error_code ec;
    archive_xips = create_archive_xip_vector_ptr(m_vnetwork_driver->fullnode_addresses(ec), m_self_xip);
    if (ec) {
        xsync_dbg("xrole_xips_manager_t::get_rand_full_nodes fail to get full nodes");
        return *archive_xips;
    }

    return get_rand_peers(archive_xips, max_peers);
}

std::vector<common::xnode_address_t> xrole_xips_manager_t::get_full_nodes() {
    if (m_vnetwork_driver == nullptr)
        return {};
    xip_vector_ptr archive_xips{};
    std::error_code ec;
    archive_xips = create_archive_xip_vector_ptr(m_vnetwork_driver->fullnode_addresses(ec), m_self_xip);
    if (ec) {
        xsync_dbg("xrole_xips_manager_t::get_rand_full_nodes fail to get full nodes");
    }

    return *archive_xips;
}


std::vector<common::xnode_address_t> xrole_xips_manager_t::get_rand_archives(uint32_t max_peers) {
    if (m_vnetwork_driver == nullptr)
        return {};
    xip_vector_ptr archive_xips{};
    archive_xips = create_archive_xip_vector_ptr(m_vnetwork_driver->archive_addresses(common::xnode_type_t::storage_archive), m_self_xip);
    return get_rand_peers(archive_xips, max_peers);
}

std::vector<common::xnode_address_t> xrole_xips_manager_t::get_archive_list() {
    if (m_vnetwork_driver == nullptr)
        return {};
    xip_vector_ptr archive_xips{};
    archive_xips = create_archive_xip_vector_ptr(m_vnetwork_driver->archive_addresses(common::xnode_type_t::storage_archive), m_self_xip);
    return *archive_xips;
}

std::vector<common::xnode_address_t> xrole_xips_manager_t::get_edge_archive_list() {
    if (m_vnetwork_driver == nullptr)
        return {};
    xip_vector_ptr edge_archive_xips{};
    edge_archive_xips = create_archive_xip_vector_ptr(m_vnetwork_driver->archive_addresses(common::xnode_type_t::storage_exchange), m_self_xip);
    return *edge_archive_xips;
}
std::vector<common::xnode_address_t> xrole_xips_manager_t::get_relay_list() const {
    if (m_vnetwork_driver == nullptr)
        return {};
    xip_vector_ptr edge_archive_xips{};
    std::error_code ec;
    edge_archive_xips = create_archive_xip_vector_ptr(m_vnetwork_driver->relay_addresses(ec), m_self_xip);
    if (ec) {
        xsync_dbg("xrole_xips_manager_t::get_relay_list fail.");
        return *edge_archive_xips;
    }    
    return *edge_archive_xips;
}
bool xrole_xips_manager_t::vrf_gossip_with_archive(base::xvblock_t *time_vblock, common::xnode_type_t role_type) {

    const std::string &hash = time_vblock->get_block_hash();
    uint32_t value = vrf_value(hash);

    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if (real_part_type(pair.first.cluster_address().type()) == role_type) {
            xip_vector_ptr &ptr = pair.second.orig_neighbour_xips;
            uint32_t count = ptr->size();

            if (count != 0) {
                uint32_t idx = value%count;
                if (pair.first == ptr->at(idx))
                    return true;
            }

            return false;  
        }
    }

    return false;
}

std::vector<common::xnode_address_t> xrole_xips_manager_t::get_all_neighbors(const common::xnode_address_t& self_addr) {
    if (m_vnetwork_driver == nullptr)
        return {};
//    if ((m_vnetwork_driver->type() & common::xnode_type_t::edge) != common::xnode_type_t::invalid) {
//        return {};
//    }
    if (m_map.find(self_addr) == m_map.end())
        return {};
    if (m_map[self_addr].m_vnetwork_driver == nullptr)
        return {};
    auto const & neighbors_info = m_map[self_addr].m_vnetwork_driver->neighbors_info2();
    std::vector<common::xnode_address_t> neighbor_addresses;
    for (auto const & info : neighbors_info) {
        if(self_addr != info.second.address) {
            neighbor_addresses.push_back(info.second.address);
        }
    }
    return neighbor_addresses;
}

xip_vector_ptr xrole_xips_manager_t::create_xip_vector_ptr(const std::vector<common::xnode_address_t>& list, const common::xnode_address_t& self_xip) {
    auto ptr = std::make_shared<std::vector<common::xnode_address_t>>();
    for(auto& addr : list) {
        if(self_xip != addr) {  // filter self xip and deceit nodes
            ptr->push_back(addr);
        }
    }
    return ptr;
}

xip_vector_ptr xrole_xips_manager_t::create_archive_xip_vector_ptr(const std::vector<common::xnode_address_t>& list, const common::xnode_address_t& self_xip) const {
    auto ptr = std::make_shared<std::vector<common::xnode_address_t>>();
    for(auto& addr : list) {
        if (self_xip != addr) {
            ptr->push_back(addr);
        }
    }
    return ptr;
}

std::vector<common::xnode_address_t> xrole_xips_manager_t::get_rand_peers(const xip_vector_ptr& list_ptr, uint32_t max_peers) {

    if (list_ptr->size() == 0)
        return {};

    std::vector<common::xnode_address_t> rand_list;
    uint32_t peers = max_peers > list_ptr->size() ? list_ptr->size() : max_peers;
    uint32_t anchor = RandomUint32() % list_ptr->size();
    // uint32_t count = 0;
    for (uint32_t count = 0; count < peers; ++count) {
        rand_list.push_back(list_ptr->at((anchor + count) % list_ptr->size()));
    }
    return rand_list;
}

bool xrole_xips_manager_t::get_self_addr(common::xnode_address_t& self_addr) const {
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if (real_part_type(pair.first.type()) != common::xnode_type_t::frozen) {
            self_addr = pair.first;
            return true;
        }
    }

    return false;
}

bool xrole_xips_manager_t::get_self_addr(common::xnode_type_t node_type, uint16_t table_id, common::xnode_address_t& self_addr) const {
    std::unique_lock<std::mutex> lock(m_lock);
    for (auto& it : m_map) {
        if (common::has(node_type, it.first.type())) {

            if (it.second.set_table_ids.find(table_id) != it.second.set_table_ids.end()) {
                self_addr = it.first;
                return true;
            }
        }
    }

    return false;
}
void xrole_xips_manager_t::set_miner(common::xminer_type_t miner_type, bool genesis) {
    m_miner_type = miner_type;
    m_genesis = genesis;
}
NS_END2
