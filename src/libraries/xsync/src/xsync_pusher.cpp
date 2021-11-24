// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_pusher.h"
#include "xsync/xsync_message.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"
#include "xcommon/xnode_type.h"
#include <unordered_set>
NS_BEG2(top, sync)

using namespace data;

// map dst to src, return my mapping
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
    xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_role_xips_mgr(role_xips_mgr),
m_sync_sender(sync_sender) {

}

void xsync_pusher_t::push_newblock_to_archive(const xblock_ptr_t &block) {

    if (block == nullptr)
        return;

    const std::string address = block->get_block_owner();
    vnetwork::xvnode_address_t self_addr;
    xsync_dbg("push_newblock_to_archive: %s", address.c_str());

    bool is_table_address = data::is_table_address(common::xaccount_address_t{address});
    if (!is_table_address) {
        xsync_warn("xsync_pusher_t push_newblock_to_archive is not table %s", block->dump().c_str());
        return;
    }

    common::xnode_type_t node_type = common::xnode_type_t::invalid;
    std::string address_prefix;
    uint32_t table_id = 0;

    if (!data::xdatautil::extract_parts(address, address_prefix, table_id))
        return;

    if (address_prefix == sys_contract_beacon_table_block_addr) {
        node_type = common::xnode_type_t::rec;
    } else if (address_prefix == sys_contract_zec_table_block_addr) {
        node_type = common::xnode_type_t::zec;
    } else if (address_prefix == sys_contract_sharding_table_block_addr) {
        node_type = common::xnode_type_t::consensus;
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
    uint32_t overlap_count = 0;
    uint32_t overlap_quota = 3;
    std::vector<vnetwork::xvnode_address_t> archive_list = m_role_xips_mgr->get_archive_list();

    std::unordered_set<common::xaccount_address_t> validator_auditor_neighbours;
    for (auto neighbor:all_neighbors) {
        validator_auditor_neighbours.insert(neighbor.account_address());
    }

    if(common::has<common::xnode_type_t::validator>(self_addr.type())) {
        std::vector<vnetwork::xvnode_address_t> parents = m_role_xips_mgr->get_rand_parents(self_addr, 0xffffffff);
        for (auto neighbor:parents) {
            validator_auditor_neighbours.insert(neighbor.account_address());
        }
    }

    if (!archive_list.empty() && !common::has<common::xnode_type_t::auditor>(self_addr.type())) {
        std::vector<uint32_t> push_arcs = calc_push_mapping(neighbor_number, archive_list.size(), self_position, random);
        xsync_dbg("push_newblock_to_archive src=%u dst=%u push_arcs=%u src %s %s", neighbor_number, archive_list.size(),
            push_arcs.size(), self_addr.to_string().c_str(), block->dump().c_str());
        for (auto &dst_idx: push_arcs) {
            vnetwork::xvnode_address_t &target_addr = archive_list[dst_idx];
            auto found = validator_auditor_neighbours.find(target_addr.account_address());
            if (found == validator_auditor_neighbours.end()) {
                xsync_dbg("push_newblock_to_archive,send, src=%s dst=%s, block_height = %llu",
                    self_addr.to_string().c_str(),
                    target_addr.to_string().c_str(),
                    block->get_height());
                m_sync_sender->push_newblock(block, self_addr, target_addr);
            }
        }
    }

    #if 0
    if(common::has<common::xnode_type_t::auditor>(self_addr.type())) {
        // push archive node
        overlap_quota = 3;

        if (overlap_quota > archive_list.size()) {
            overlap_quota = archive_list.size();
        } else {
            uint32_t tmp = archive_list.size() / 3;
            if (tmp >= overlap_quota) {
                overlap_quota = tmp;
            }
        }

        for (auto neighbor = all_neighbors.begin(); (neighbor != all_neighbors.end()) && (overlap_count < overlap_quota); ++neighbor) {
            for (auto archive_addr:archive_list) {
                if (neighbor->account_address() == archive_addr.account_address()) {
                    overlap_count++;
                    break;
                }
            }
        }
    } else if (common::has<common::xnode_type_t::zec>(self_addr.type()) || common::has<common::xnode_type_t::rec>(self_addr.type())) {
        overlap_count = overlap_quota - 1;
    } else {
        overlap_count = overlap_quota + 1;
    }


    xsync_dbg("push_newblock_to_archive overlap_count over overlap_quota %u", overlap_quota <= overlap_count);

    if (!archive_list.empty() && (overlap_count < overlap_quota)) {
        std::vector<uint32_t> push_arcs = calc_push_mapping(neighbor_number, archive_list.size(), self_position, random);
        xsync_dbg("push_newblock_to_archive src=%u dst=%u push_arcs=%u src %s %s", neighbor_number, archive_list.size(),
            push_arcs.size(), self_addr.to_string().c_str(), block->dump().c_str());
        for (auto &dst_idx: push_arcs) {
            vnetwork::xvnode_address_t &target_addr = archive_list[dst_idx];
            bool found = false;
            for (auto neighbor:all_neighbors) {
                if (neighbor.account_address() == target_addr.account_address()) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                xsync_dbg("push_newblock_to_archive src=%s dst=%s, block_height = %llu",
                    self_addr.to_string().c_str(),
                    target_addr.to_string().c_str(),
                    block->get_height());
                m_sync_sender->push_newblock(block, self_addr, target_addr);
            }
        }
    }
    #endif

    // push edge archive
    std::vector<vnetwork::xvnode_address_t> edge_archive_list = m_role_xips_mgr->get_edge_archive_list();
    if (!edge_archive_list.empty()) {
        std::vector<uint32_t> push_edge_arcs = calc_push_mapping(neighbor_number, edge_archive_list.size(), self_position, random);
        xsync_dbg("push_newblock_to_edge_archive src=%u dst=%u push_edge_arcs= %u src %s %s", neighbor_number, edge_archive_list.size(), 
            push_edge_arcs.size(), self_addr.to_string().c_str(), block->dump().c_str());
        for (auto &dst_idx: push_edge_arcs) {
            m_sync_sender->push_newblock(block, self_addr, edge_archive_list[dst_idx]);
        }
    }
}

NS_END2
