// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/super_routing_node.h"
#include "xpbase/base/top_log.h"

namespace top {

namespace wrouter {

void SuperNode::DumpSuperNode(
        const uint32_t& xnetwork_id,
        const uint64_t& service_type) const noexcept {
    std::unique_lock<std::mutex> lock(super_node_cache_map_mutex_);
    const auto xfind = super_node_cache_map_.find(xnetwork_id);
    if (xfind == super_node_cache_map_.end()) {
        return ;
    }
    const auto sfind = (xfind->second).find(service_type);
    if (sfind == (xfind->second).end()) {
        return ;
    }


#ifdef DEBUG
    TOP_DEBUG("begin dump supernode xnetwork_id:%u service_type:%llu",
            xnetwork_id,
            service_type);
    for (const auto& node : sfind->second) {
        if (!node) {
            continue;
        }
        TOP_DEBUG("dump supernode:%s xnetwork_id:%u service_type:%llu",
                HexEncode(node->node_id).c_str(),
                xnetwork_id,
                service_type);
    }
    TOP_DEBUG("end dump supernode xnetwork_id:%u service_type:%llu",
            xnetwork_id,
            service_type);
#endif
}

bool SuperNode::AddNode(
        const uint32_t& xnetwork_id,
        const uint64_t& service_type,
        const kadmlia::NodeInfoPtr& super_node) noexcept {
    bool ret = false;
    {
        std::unique_lock<std::mutex> lock(super_node_cache_map_mutex_);
        auto& value_super_node_map = super_node_cache_map_[xnetwork_id];
        auto& nodes_ary = value_super_node_map[service_type];

        TOP_DEBUG("SuperNode::AddNode xnetwork_id:%u service_type:%llu in nodeid[%s]",
                   xnetwork_id,
                   service_type,
                   HexEncode(super_node->node_id).c_str());

        for (auto& node : nodes_ary) {
            if (!node) {
                node = super_node;
                node->service_type = service_type;
                ret = true;
                break;
            }
        }
    }

    DumpSuperNode(xnetwork_id, service_type);
    if (!ret) {
        TOP_WARN("addnode supernode goes wrong");
    }
    return ret;
}

bool SuperNode::CheckSuperNode(
        const uint32_t& xnetwork_id,
        const uint64_t& service_type,
        const std::string& super_node_id) const noexcept {
    std::unique_lock<std::mutex> lock(super_node_cache_map_mutex_);
    const auto xfind = super_node_cache_map_.find(xnetwork_id);
    if (xfind == super_node_cache_map_.end()) {
        return false;
    }
    const auto sfind = (xfind->second).find(service_type);
    if (sfind == (xfind->second).end()) {
        TOP_INFO("CheckSuperNode xnetwork_id:%u service_type:%llu have no super nodes",
                xnetwork_id,
                service_type);
        return false;
    }

    for (const auto& node : sfind->second) {
        if (!node) {
            continue;
        }
        TOP_DEBUG("CheckSuperNode xnetwork_id:%u service_type:%llu in nodeid:%s map nodeid:%s",
                   xnetwork_id,
                   service_type,
                   HexEncode(super_node_id).c_str(),
                   HexEncode(node->node_id).c_str());
        if (node->node_id == super_node_id) {
            return true;
        }
    }
    return false;
}

// new vrf supernodes born, remove nodes not in new vrf supernodes
void SuperNode::CheckReplace(
        const uint32_t& xnetwork_id,
        const uint64_t& service_type,
        const std::array<NetNode, SingleNetSuperNodeSize>& node_ary,
        std::set<uint32_t>& exclude_index) {
    std::unique_lock<std::mutex> lock(super_node_cache_map_mutex_);
    auto& value_super_node_map = super_node_cache_map_[xnetwork_id];
    auto& nodes_deq = value_super_node_map[service_type];
    for (auto& node : nodes_deq) {
        if (!node) {
            continue;
        }
        bool already_has = false;
        for (uint32_t i = 0; i < node_ary.size(); ++i) {
            if (node->node_id == node_ary[i].m_node_id) {
                already_has = true;
                exclude_index.insert(i);
                break;
            }
        }
        if (!already_has) {
            // remove this supernode because not in new vrf supernode
            node = nullptr;
        }
    }
    return;
}


void SuperNode::SuperRoutingNodes(
        const uint32_t& xnetwork_id,
        std::vector<kadmlia::NodeInfoPtr>& node_vec) noexcept {
    std::unique_lock<std::mutex> lock(super_node_cache_map_mutex_);
    auto& value_super_node_map = super_node_cache_map_[xnetwork_id];
    for (const auto& item : value_super_node_map) {
        for (const auto& node : item.second) {
            if (!node) {
                continue;
            }
            if ((node->public_ip).empty() && (node->local_ip).empty()) {
                // usually this is for localnode (using this way to filter localnode just for now)
                continue;
            }
            node_vec.push_back(node);
        }
    }
    TOP_DEBUG("supernode for broadcast size:%u", node_vec.size());
}

} // end namespace wrouter

} // end namespace top
