// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

xrole_xips_manager_t::~xrole_xips_manager_t() {

}

xrole_xips_manager_t::xrole_xips_manager_t(std::string vnode_id, const observer_ptr<base::xvnodesrv_t> &nodesrv, xdeceit_node_manager_t *blacklist):
m_vnode_id(vnode_id),
m_nodesrv(nodesrv),
m_blacklist(blacklist) {
}

void xrole_xips_manager_t::add_role(const vnetwork::xvnode_address_t& self_xip, const std::vector<vnetwork::xvnode_address_t>& neighbours,
                const std::vector<vnetwork::xvnode_address_t>& parents, const std::vector<vnetwork::xvnode_address_t>& archives) {
    std::unique_lock<std::mutex> lock(m_lock);
    // remove old first : version is different
    for(auto& pair : m_map) {
        if(pair.first.cluster_address() == self_xip.cluster_address()) {
            m_map.erase(pair.first);
            break;
        }
    }


    m_map[self_xip] = {self_xip, create_xip_vector_ptr(neighbours, self_xip), create_xip_vector_ptr(parents, self_xip),
                        std::make_shared<std::vector<vnetwork::xvnode_address_t>>(neighbours)};

    // only the same archive view can obtain the same vrf and hash result
    m_archive_xips = create_archive_xip_vector_ptr(archives, self_xip);
}

void xrole_xips_manager_t::remove_role(const vnetwork::xvnode_address_t& self_xip) {
    std::unique_lock<std::mutex> lock(m_lock);
    m_map.erase(self_xip);
}

void xrole_xips_manager_t::remove_xips_by_id(const common::xnode_id_t& id) {
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        pair.second.remove_xips_by_id(id);
    }
    xrole_xips_t::remove_xips_by_id(m_archive_xips, id);
}

const vnetwork::xvnode_address_t xrole_xips_manager_t::get_static_xip() {
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if (real_part_type(pair.first.type()) == common::xnode_type_t::frozen) {
            return pair.first;
        }
    }

    return {};
}

std::vector<vnetwork::xvnode_address_t> xrole_xips_manager_t::get_rand_neighbors(const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers) {
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if(pair.first.cluster_address() == self_xip.cluster_address()) {
            return get_rand_peers(pair.second.neighbour_xips, max_peers);
        }
    }
    return {};
}

std::vector<vnetwork::xvnode_address_t> xrole_xips_manager_t::get_rand_parents(const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers) {
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if(pair.first.cluster_address() == self_xip.cluster_address()) {
            return get_rand_peers(pair.second.parent_xips, max_peers);
        }
    }
    return {};
}

std::vector<vnetwork::xvnode_address_t> xrole_xips_manager_t::get_rand_archives(const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers) {
    std::unique_lock<std::mutex> lock(m_lock);
    return get_rand_peers(m_archive_xips, max_peers);
}

std::vector<vnetwork::xvnode_address_t> xrole_xips_manager_t::get_vrf_sqrt_archive(const std::string& hash) {

    std::vector<vnetwork::xvnode_address_t> archive_nodes;

    std::unique_lock<std::mutex> lock(m_lock);

    const xip_vector_ptr& ptr = m_archive_xips;

    if (ptr->size() == 0)
        return {};

    uint32_t max_peers = sqrt(ptr->size());
    uint32_t value = vrf_value(hash);

    bool found = false;

    uint32_t base_index = value % ptr->size();
    for (uint32_t i = 0; i < max_peers; i++) {
        const vnetwork::xvnode_address_t &xip = ptr->at((base_index + i) % ptr->size());
        archive_nodes.push_back(xip);
    }

    return archive_nodes;
}

std::vector<vnetwork::xvnode_address_t> xrole_xips_manager_t::get_archive_list() {
    std::unique_lock<std::mutex> lock(m_lock);

    std::vector<vnetwork::xvnode_address_t> nodes;
    uint32_t count = m_archive_xips->size();
    for (uint32_t i=0; i<count; i++) {
        nodes.push_back(m_archive_xips->at(i));
    }

    return nodes;
}

bool xrole_xips_manager_t::get_rand_neighbor_by_block(base::xvblock_t *vblock, vnetwork::xvnode_address_t& self_xip, vnetwork::xvnode_address_t& target_xip) {

    const base::xvqcert_t *test_for_cert = vblock->get_cert();
    base::xauto_ptr<base::xvnodegroup_t> node_group = m_nodesrv->get_group(test_for_cert->get_validator());
    assert(node_group != nullptr);

    const xvip2_t &group_xip2 = node_group->get_xip2_addr();

    int32_t self_slot_id = -1;
    int32_t target_slot_id = -1;
    uint32_t group_size = node_group->get_size();

    const std::vector<base::xvnode_t*> &nodes = node_group->get_nodes();
    for (const auto &it: nodes) {
        base::xvnode_t* node = it;
        const std::string& prikey = node->get_sign_prikey();
        if (!prikey.empty()) {
            const xvip2_t & node_xip2 = node->get_xip2_addr();
            self_slot_id = get_node_id_from_xip2(node_xip2);
            break;
        }
    }

    if (self_slot_id == -1) {
        //assert(0);
        return false;
    }

    while (1) {
        target_slot_id = RandomUint32()%group_size;
        if (target_slot_id != self_slot_id)
            break;
    }

    self_xip = build_address_from_vnode(group_xip2, nodes, self_slot_id);
    target_xip = build_address_from_vnode(group_xip2, nodes, target_slot_id);

    return true;
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

bool xrole_xips_manager_t::vrf_send_newblock(base::xvblock_t *vblock, vnetwork::xvnode_address_t& self_xip, uint32_t &self_position, uint32_t &deliver_node_count) {

    const std::string& hash = vblock->get_block_hash();

    const base::xvqcert_t *test_for_cert = vblock->get_cert();
    base::xauto_ptr<base::xvnodegroup_t> node_group = m_nodesrv->get_group(test_for_cert->get_validator());

    uint32_t group_size = node_group->get_size();
    uint32_t max_senders = sqrt(group_size);

    // 1. get my position
    int32_t self_slot_id = -1;

    const std::vector<base::xvnode_t*> &nodes = node_group->get_nodes();
    for (const auto &it: nodes) {
        base::xvnode_t* node = it;
        const std::string& prikey = node->get_sign_prikey();
        if (!prikey.empty()) {
            const xvip2_t & node_xip2 = node->get_xip2_addr();
            self_slot_id = get_node_id_from_xip2(node_xip2);
            break;
        }
    }

    if (self_slot_id == -1)
        return false;

    // 2. calc if contain my position
    uint32_t value = vrf_value(hash);
    uint32_t vrf_index = value % group_size;
    bool found = false;

    for (uint32_t i = 0; i < max_senders; i++) {
        uint32_t idx = (vrf_index + i) % group_size;
        if (idx == (uint32_t)self_slot_id) {
            self_position = i;
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    // 3. build address
    const xvip2_t &group_xip2 = node_group->get_xip2_addr();
    self_xip = build_address_from_vnode(group_xip2, nodes, self_slot_id);
    deliver_node_count = max_senders;

    return true;
}

xip_vector_ptr xrole_xips_manager_t::create_xip_vector_ptr(const std::vector<vnetwork::xvnode_address_t>& list, const vnetwork::xvnode_address_t& self_xip) {
    auto ptr = std::make_shared<std::vector<vnetwork::xvnode_address_t>>();
    m_blacklist->filter_deceit_nodes([&](const std::set<vnetwork::xaccount_address_t>& _set) {
        for(auto& addr : list) {
            if(self_xip != addr && _set.find(addr.account_address()) == _set.end()) {  // filter self xip and deceit nodes
                ptr->push_back(addr);
            }
        }
    });
    return ptr;
}

xip_vector_ptr xrole_xips_manager_t::create_archive_xip_vector_ptr(const std::vector<vnetwork::xvnode_address_t>& list, const vnetwork::xvnode_address_t& self_xip) {
    auto ptr = std::make_shared<std::vector<vnetwork::xvnode_address_t>>();
    for(auto& addr : list) {
        ptr->push_back(addr);
    }
    return ptr;
}

std::vector<vnetwork::xvnode_address_t> xrole_xips_manager_t::get_rand_peers(const xip_vector_ptr& list_ptr, uint32_t max_peers) {

    if (list_ptr->size() == 0)
        return {};

    std::vector<vnetwork::xvnode_address_t> rand_list;
    uint32_t peers = max_peers > list_ptr->size() ? list_ptr->size() : max_peers;
    uint32_t anchor = RandomUint32() % list_ptr->size();
    uint32_t count = 0;
    for (uint32_t count = 0; count < peers; ++count) {
        rand_list.push_back(list_ptr->at((anchor + count) % list_ptr->size()));
    }
    return rand_list;
}

uint32_t xrole_xips_manager_t::vrf_value(const std::string& hash) {

    uint32_t value = 0;
    const uint8_t* data = (const uint8_t*)hash.data();
    for (size_t i = 0; i < hash.size(); i++) {
        value += data[i];
    }

    return value;
}

bool xrole_xips_manager_t::is_consensus_role_exist(base::xvblock_t *vblock) const {

    const std::string& account = vblock->get_account();

    uint32_t table_id{UINT32_MAX};
    common::xnode_type_t role = data::get_node_role_from_account(common::xaccount_address_t{ account }, table_id);

    common::xnode_type_t type = common::xnode_type_t::consensus_validator;
    if (common::has<common::xnode_type_t::committee>(role) || common::has<common::xnode_type_t::zec>(role)) {
        type = role;
    }

    bool found = false;
    std::unique_lock<std::mutex> lock(m_lock);
    for(auto& pair : m_map) {
        if (real_part_type(pair.first.type()) == type) {
            found = true;
            break;
        }
    }

    return found;
}

vnetwork::xvnode_address_t xrole_xips_manager_t::build_address_from_vnode(const xvip2_t &group_xip2, const std::vector<base::xvnode_t*> &nodes, int32_t slot_id) {

    assert(slot_id >= 0);
    assert((uint32_t)slot_id < nodes.size());

    common::xzone_id_t zone_id = common::xzone_id_t{uint8_t(get_zone_id_from_xip2(group_xip2))};
    common::xcluster_id_t cluster_id = common::xcluster_id_t{uint8_t(get_cluster_id_from_xip2(group_xip2))};
    common::xgroup_id_t group_id = common::xgroup_id_t{uint8_t(get_group_id_from_xip2(group_xip2))};
    common::xversion_t ver = common::xversion_t{uint8_t(get_network_ver_from_xip2(group_xip2))};
    common::xnetwork_id_t nid = common::xnetwork_id_t{uint32_t(get_network_id_from_xip2(group_xip2))};
    uint64_t elect_height = get_network_height_from_xip2(group_xip2);

    uint32_t group_size = nodes.size();

    auto const cluster_addr = common::xsharding_address_t{
                                    nid,
                                    zone_id,
                                    cluster_id,
                                    group_id
                                };

    base::xvnode_t* node = nodes[slot_id];
    common::xaccount_election_address_t account_election_address{ common::xnode_id_t { node->get_account() }, common::xslot_id_t{(uint16_t)slot_id} };
    vnetwork::xvnode_address_t addr = vnetwork::xvnode_address_t{cluster_addr, account_election_address, ver, (uint16_t)group_size, elect_height};

    return addr;
}

NS_END2
