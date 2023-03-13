// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_pusher.h"

#include "xdata/xnative_contract_address.h"
#include "xsync/xsync_message.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"
#include "xcommon/xnode_type.h"
#include <unordered_set>
NS_BEG2(top, sync)

using namespace data;

// src_count < dst_count, one to many
// src_count == dst_count, one to one
// src_count > dst_count , (null or one) to one
std::vector<uint32_t>  calc_push_mapping(uint32_t src_count, uint32_t dst_count, uint32_t src_self_position, uint32_t random) {

    if (src_count == 0 || dst_count == 0)
        return {};

    if (src_self_position >= src_count)
        return {};

    // 1. calc new src size
    uint32_t new_src_count = src_count;
    if (src_count > dst_count)
        new_src_count = dst_count;

    // 2. select random position
    uint32_t start_position = random % src_count;

    // 3. check range
    uint32_t end = src_self_position;
    if (src_self_position < start_position) {
        end += src_count;
    }
    if ((start_position + new_src_count) < end) {
        return {};
    }

    std::vector<uint32_t> self_to_dst;
    for (auto offset = end - start_position; offset < dst_count; offset += new_src_count) {
        self_to_dst.push_back(offset);
    }

    return self_to_dst;
}

std::set<uint32_t>  calc_push_select(uint32_t dst_count, uint32_t random) {

    uint32_t start_position = random % dst_count;

    std::set<uint32_t> results;
    uint32_t sqrt_size = sqrt(dst_count);
    for (uint32_t i=0; i<sqrt_size; i++) {
        uint32_t idx = (start_position+i) % dst_count;
        results.insert(idx);
    }

    return results;
}

xsync_pusher_t::xsync_pusher_t(std::string vnode_id,
    xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender, xrole_chains_mgr_t *role_chains_mgr, xsync_store_face_t *sync_store):
m_vnode_id(vnode_id),
m_role_xips_mgr(role_xips_mgr),
m_sync_sender(sync_sender),
m_role_chains_mgr(role_chains_mgr),
m_sync_store(sync_store) {

}

void xsync_pusher_t::push_newblock_to_archive(const xblock_ptr_t &block) {

    if (block == nullptr)
        return;

    const std::string address = block->get_block_owner();
    vnetwork::xvnode_address_t self_addr;
    xsync_dbg("push_newblock_to_archive: %s", address.c_str());

    // bool is_table_address = data::is_table_address(common::xaccount_address_t{address});
    // if (!is_table_address) {
    //     xsync_warn("xsync_pusher_t push_newblock_to_archive is not table %s", block->dump().c_str());
    //     return;
    // }

    common::xnode_type_t node_type = common::xnode_type_t::invalid;
    std::string address_prefix;
    uint32_t table_id = 0;

    if (!data::xdatautil::extract_parts(address, address_prefix, table_id))
        return;

    if (address_prefix == common::rec_table_base_address.to_string()) {
        node_type = common::xnode_type_t::rec;
    } else if (address_prefix == common::zec_table_base_address.to_string()) {
        node_type = common::xnode_type_t::zec;
    } else if (address_prefix == common::con_table_base_address.to_string()) {
        node_type = common::xnode_type_t::consensus;
    } else if (address_prefix == common::eth_table_base_address.to_string()) {
        node_type = common::xnode_type_t::evm;
    } else if (address_prefix == common::relay_table_base_address.to_string()) {
        node_type = common::xnode_type_t::relay;
    } else {
        assert(0);
    }

    // check table id
    if (!m_role_xips_mgr->get_self_addr(node_type, table_id, self_addr)) {
        xsync_warn("xsync_pusher_t push_newblock_to_archive get self addr failed %s", block->dump().c_str());
        return;
    }

    std::vector<vnetwork::xvnode_address_t> all_neighbors = m_role_xips_mgr->get_all_neighbors(self_addr);
    all_neighbors.push_back(self_addr);
    std::sort(all_neighbors.begin(), all_neighbors.end());
    uint32_t neighbor_number = all_neighbors.size();

    int32_t self_position = -1;
    for (uint32_t i=0; i<all_neighbors.size(); i++) {
        if (all_neighbors[i] == self_addr) {
            self_position = i;
            break;
        }
    }

    uint32_t random = vrf_value(block->get_block_hash());
    // uint32_t overlap_count = 0;
    // uint32_t overlap_quota = 3;
    std::vector<std::shared_ptr<std::vector<vnetwork::xvnode_address_t>>> objects;
    objects.push_back(std::make_shared<std::vector<vnetwork::xvnode_address_t>>(m_role_xips_mgr->get_archive_list()));
    objects.push_back(std::make_shared<std::vector<vnetwork::xvnode_address_t>>(m_role_xips_mgr->get_full_nodes()));

    std::unordered_set<common::xaccount_address_t> validator_auditor_neighbours;
    for (auto neighbor:all_neighbors) {
        validator_auditor_neighbours.insert(neighbor.account_address());
    }

    if(common::has<common::xnode_type_t::consensus_validator>(self_addr.type())) {
        std::vector<vnetwork::xvnode_address_t> parents = m_role_xips_mgr->get_rand_parents(self_addr, 0xffffffff);
        for (auto neighbor:parents) {
            validator_auditor_neighbours.insert(neighbor.account_address());
        }
    }

    for (auto object:objects) {
        //if (!object->empty() && !common::has<common::xnode_type_t::consensus_auditor>(self_addr.type())) {
        if (object->empty())
            continue;
        if (common::has<common::xnode_type_t::consensus_validator>(self_addr.type()) || common::has<common::xnode_type_t::evm_validator>(self_addr.type()) ||
            common::has<common::xnode_type_t::relay>(self_addr.type())) {
            std::vector<uint32_t> push_arcs = calc_push_mapping(neighbor_number, object->size(), self_position, random);
            xsync_info("push_newblock_to_archive, src=%u dst=%u push_arcs=%u src %s %s", neighbor_number, object->size(),
                push_arcs.size(), self_addr.to_string().c_str(), block->dump().c_str());
            for (auto &dst_idx: push_arcs) {
                vnetwork::xvnode_address_t &target_addr = (*object)[dst_idx];
                auto found = validator_auditor_neighbours.find(target_addr.account_address());
                if (found == validator_auditor_neighbours.end()) {
                    xsync_info("push_newblock_to_archive src=%s dst=%s, %s, block_height = %llu",
                        self_addr.to_string().c_str(),
                        target_addr.to_string().c_str(),
                        address.c_str(),
                        block->get_height());
                    m_sync_sender->push_newblock(block, self_addr, target_addr);
                }
            }
        }
    }

    // push edge archive
    std::vector<vnetwork::xvnode_address_t> edge_archive_list = m_role_xips_mgr->get_edge_archive_list();
    if (!edge_archive_list.empty()) {
        std::vector<uint32_t> push_edge_arcs = calc_push_mapping(neighbor_number, edge_archive_list.size(), self_position, random);
        xsync_info("push_newblock_to_archive, edge_archive: src=%u dst=%u push_edge_arcs= %u src %s %s", neighbor_number, edge_archive_list.size(), 
            push_edge_arcs.size(), self_addr.to_string().c_str(), block->dump().c_str());
        for (auto &dst_idx: push_edge_arcs) {
            m_sync_sender->push_newblock(block, self_addr, edge_archive_list[dst_idx]);
        }
    }


}
int xsync_pusher_t::get_chain_info(const vnetwork::xvnode_address_t &network_self, std::vector<xchain_state_info_t>& info_list) {
    const std::shared_ptr<xrole_chains_t> &role_chains = m_role_chains_mgr->get_role(network_self);
    const map_chain_info_t & chains = role_chains->get_chains_wrapper().get_chains();
    
    for (const auto & it : chains) {
        enum_chain_sync_policy sync_policy = it.second.sync_policy;
        const std::string & address = it.first;
        // const xchain_info_t & chain_info = it.second;

        xchain_state_info_t info;
        info.address = address;
        info.start_height = m_sync_store->get_latest_start_block_height(address, sync_policy);
        info.end_height = m_sync_store->get_latest_end_block_height(address, sync_policy);
        info_list.push_back(info);
    }

    xsync_info("xsync_pusher_t::get_chain_info: %s count(%d)", network_self.to_string().c_str(), info_list.size());
    if (info_list.empty()) {
        return 1;
    }
    return 0;
}
void xsync_pusher_t::on_timer() {
    if (m_time_rejecter.reject()){
        return;
    }
    
    m_counter++;
    if (m_counter % 60 != 0)
        return;

    std::string address;
    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    vnetwork::xvnode_address_t self_addr;
    common::xnode_type_t node_type;
    for (const auto &role_it: roles) {
        self_addr = role_it.first;
        // const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;
        node_type = self_addr.type();

        if (common::has<common::xnode_type_t::rec>(node_type) || common::has<common::xnode_type_t::zec>(node_type) ||
            common::has<common::xnode_type_t::consensus>(node_type) || common::has<common::xnode_type_t::evm>(node_type) ||
            common::has<common::xnode_type_t::relay>(node_type)) {
            address = self_addr.to_string();
            break;
        } else if (common::has<common::xnode_type_t::storage_archive>(node_type)) {
            continue;
        } else {
            continue;
        }
    }
    if (address.empty()) {
        xsync_info("xsync_pusher_t::on_timer, not find self_addr");
        return;
    }

    std::vector<vnetwork::xvnode_address_t> all_neighbors = m_role_xips_mgr->get_all_neighbors(self_addr);
    all_neighbors.push_back(self_addr);
    std::sort(all_neighbors.begin(), all_neighbors.end());
    uint32_t neighbor_number = all_neighbors.size();

    int32_t self_position = -1;
    for (uint32_t i=0; i<all_neighbors.size(); i++) {
        if (all_neighbors[i] == self_addr) {
            self_position = i;
            break;
        }
    }

    if (self_position < 0) {
        xsync_info("xsync_pusher_t::on_timer, not this validator, %s", self_addr.to_string().c_str());
        return;
    }

    // uint32_t overlap_count = 0;
    // uint32_t overlap_quota = 3;
    std::vector<std::shared_ptr<std::vector<vnetwork::xvnode_address_t>>> objects;
    objects.push_back(std::make_shared<std::vector<vnetwork::xvnode_address_t>>(m_role_xips_mgr->get_archive_list()));
    objects.push_back(std::make_shared<std::vector<vnetwork::xvnode_address_t>>(m_role_xips_mgr->get_full_nodes()));

    std::unordered_set<common::xaccount_address_t> validator_auditor_neighbours;
    for (auto neighbor:all_neighbors) {
        validator_auditor_neighbours.insert(neighbor.account_address());
    }

    if(common::has<common::xnode_type_t::consensus_validator>(self_addr.type())) {
        std::vector<vnetwork::xvnode_address_t> parents = m_role_xips_mgr->get_rand_parents(self_addr, 0xffffffff);
        for (auto neighbor:parents) {
            validator_auditor_neighbours.insert(neighbor.account_address());
        }
    }
    for (auto n:validator_auditor_neighbours)
        xsync_dbg("xsync_pusher_t, neighbours:%s", n.to_string().c_str());

    for (auto object:objects) {
        //if (!object->empty() && !common::has<common::xnode_type_t::consensus_auditor>(self_addr.type())) {
        if (object->empty())
            continue;
        if (common::has<common::xnode_type_t::consensus_validator>(self_addr.type()) || common::has<common::xnode_type_t::evm_validator>(self_addr.type()) ||
            common::has<common::xnode_type_t::relay>(self_addr.type())) {
            std::vector<uint32_t> push_arcs = calc_push_mapping(neighbor_number, object->size(), self_position, 0);
            xsync_dbg("xsync_pusher_t, maybe send_query_archive_height src=%u dst=%u push_arcs=%u src %s", neighbor_number, object->size(),
                push_arcs.size(), self_addr.to_string().c_str());
            if (push_arcs.size() == 1 && object->size() > 1)  // mapping to 2 arc
                push_arcs.push_back((push_arcs[0] + 1) % object->size());

            for (auto &dst_idx: push_arcs) {
                vnetwork::xvnode_address_t &target_addr = (*object)[dst_idx];
                auto found = validator_auditor_neighbours.find(target_addr.account_address());
                if (found == validator_auditor_neighbours.end()) {
                    xsync_info("xsync_pusher_t, send_query_archive_height, send, src=%s dst=%s",
                        self_addr.to_string().c_str(), target_addr.to_string().c_str());
                    //xsync_query_height_t info;
                    std::vector<xchain_state_info_t> info_list;
                    get_chain_info(self_addr, info_list);
                    if (info_list.empty())
                        break;
                    m_sync_sender->send_query_archive_height(info_list, self_addr, target_addr);
                } else {
                    xsync_dbg("xsync_pusher_t, send_query_archive_height, not find, src=%s dst=%s",
                        self_addr.to_string().c_str(), target_addr.to_string().c_str());
                }
            }
        }
    }
}
NS_END2
