// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/root/root_routing_manager.h"

#include "xpbase/base/check_cast.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/endpoint_util.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/root/root_routing.h"

namespace top {

using namespace kadmlia;

namespace wrouter {

RootRoutingManagerPtr RootRoutingManager::Instance() {
    static auto ins = std::make_shared<RootRoutingManager>();
    return ins;
}

RootRoutingManager::RootRoutingManager()
        : root_routing_map_(),
          root_routing_map_mutex_() {}

RootRoutingManager::~RootRoutingManager() {
    std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
    for (auto iter = root_routing_map_.begin(); iter != root_routing_map_.end(); ++iter) {
        iter->second->UnInit();
    }
    root_routing_map_.clear();
}

int RootRoutingManager::GetRootNodes(
        uint32_t network_id,
        std::vector<NodeInfoPtr>& root_nodes) {
    RoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        auto iter = root_routing_map_.find(kRoot);
        if (iter == root_routing_map_.end()) {
            TOP_WARN("service type[%d] has not added!", network_id);
            return kKadFailed;
        }
        routing_table = iter->second;
    }
    if (!routing_table) {
        TOP_WARN("get routing table failed!");
        return kKadFailed;
    }
    RootRouting* root_table = static_cast<RootRouting*>(routing_table.get());
    if (!routing_table) {
        TOP_WARN("get routing table failed!");
        return kKadFailed;
    }
    return root_table->GetRootNodes(network_id, root_nodes);
}

int RootRoutingManager::GetRootNodes(
        const std::string& des_id,
        std::vector<NodeInfoPtr>& root_nodes) {
    RoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        auto iter = root_routing_map_.find(kRoot);
        if (iter == root_routing_map_.end()) {
            TOP_WARN("service type[%s] has not added!", HexEncode(des_id).c_str());
            return kKadFailed;
        }
        routing_table = iter->second;
    }
    if (!routing_table) {
        TOP_WARN("get routing table failed!");
        return kKadFailed;
    }
    RootRouting* root_table = static_cast<RootRouting*>(routing_table.get());
    return root_table->GetRootNodes(des_id, root_nodes);
}

int RootRoutingManager::GetRootNodesV2(
        const std::string& des_id,
        uint64_t service_type,
        std::vector<NodeInfoPtr>& root_nodes) {
    RoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        auto iter = root_routing_map_.find(kRoot);
        if (iter == root_routing_map_.end()) {
            TOP_WARN("service type[%s] has not added!", HexEncode(des_id).c_str());
            return kKadFailed;
        }
        routing_table = iter->second;
    }
    if (!routing_table) {
        TOP_WARN("get routing table failed!");
        return kKadFailed;
    }
    RootRouting* root_table = static_cast<RootRouting*>(routing_table.get());
    return root_table->GetRootNodesV2(des_id, service_type, root_nodes);
}

int RootRoutingManager::GetRootBootstrapCache(
        std::set<std::pair<std::string, uint16_t>>& boot_endpoints) {
    auto routing_table_ptr = GetRoutingTable(kRoot);
    if (!routing_table_ptr) {
        TOP_INFO("<blueshi> no routing_table(kRoot)");
        return kKadFailed;
    }

    routing_table_ptr->GetBootstrapCache(boot_endpoints);
    return kKadSuccess;
}

int RootRoutingManager::AddRoutingTable(
        std::shared_ptr<transport::Transport> transport,
        const base::Config& config,
        base::KadmliaKeyPtr kad_key_ptr,
        on_bootstrap_cache_get_callback_t get_cache_callback,
        on_bootstrap_cache_set_callback_t set_cache_callback,
        bool wait_for_joined) {
    if (!transport) {
        TOP_FATAL("invalid udp transport!");
        return kKadFailed;
    }

    if (CreateRoutingTable(
            transport,
            config,
            kad_key_ptr,
            get_cache_callback,
            set_cache_callback,
            wait_for_joined) != kKadSuccess) {
        TOP_FATAL("create routing table failed!");
        return kKadFailed;
    }

    return kKadSuccess;
}
    
void RootRoutingManager::RemoveRoutingTable(uint64_t service_type) {
    RoutingTablePtr root_routing = nullptr;
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        auto iter = root_routing_map_.find(service_type);
        if (iter != root_routing_map_.end()) {
            root_routing = iter->second;
            root_routing_map_.erase(iter);
        }
    }

    if (root_routing) {
        root_routing->UnInit();
    }
}

void RootRoutingManager::RemoveAllRoutingTable() {
    decltype(root_routing_map_) rt_map;
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        rt_map = root_routing_map_;
    }

    for (auto it = rt_map.begin(); it != rt_map.end();) {
        it->second->UnInit();
        it = rt_map.erase(it);
        TOP_KINFO("remove all root service routing table");
    }


    /*
    std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
    for (auto it = root_routing_map_.begin(); it != root_routing_map_.end();) {
        it->second->UnInit();
        it = root_routing_map_.erase(it);
    }
    */
}


std::shared_ptr<RoutingTable> RootRoutingManager::GetRoutingTable(uint64_t service_type) {
    std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
    auto iter = root_routing_map_.find(service_type);
    if (iter != root_routing_map_.end()) {
        return iter->second;
    }
    return nullptr;
}

std::shared_ptr<RoutingTable> RootRoutingManager::GetRoutingTable(const std::string& routing_id) {
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        for (auto& item : root_routing_map_) {
            auto routing_ptr = item.second;
            if (routing_ptr->get_local_node_info()->id() == routing_id) {
                return routing_ptr;
            }
        }
    }
    // handle kRoot nodeid
    auto kad_key_ptr = base::GetKadmliaKey(routing_id);
    auto xnetwork_id = kad_key_ptr->xnetwork_id();
    if (xnetwork_id == kRoot) {
        return this->GetRoutingTable(kRoot);
    }
    return nullptr;
}

int RootRoutingManager::CreateRoutingTable(
        std::shared_ptr<transport::Transport> transport,
        const base::Config& config,
        base::KadmliaKeyPtr kad_key_ptr,
        on_bootstrap_cache_get_callback_t get_cache_callback,
        on_bootstrap_cache_set_callback_t set_cache_callback,
        bool wait_for_joined) {
    uint64_t service_type = kRoot;
    if (kad_key_ptr->Xip().xnetwork_id() != kRoot) {
        service_type = kad_key_ptr->GetServiceType();
    }
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        auto iter = root_routing_map_.find(service_type);
        if (iter != root_routing_map_.end()) {
            TOP_WARN("service type[%lu] has added!", service_type);
            return kKadSuccess;
        }
    }

    std::set<std::pair<std::string, uint16_t>> public_endpoints_config;
    GetPublicEndpointsConfig(config, public_endpoints_config);
    TOP_INFO("enter CreateRoutingTable:%lu", service_type);
    kadmlia::LocalNodeInfoPtr local_node_ptr = kadmlia::CreateLocalInfoFromConfig(
            config,
            kad_key_ptr);
    if (!local_node_ptr) {
        TOP_FATAL("create local_node_ptr for service_type(%ld) failed", (long)service_type);
        return kKadFailed;
    }
    RoutingTablePtr routing_table_ptr;
    routing_table_ptr.reset(new RootRouting(transport, local_node_ptr));

    if (!routing_table_ptr->Init()) {
        TOP_FATAL("init edge bitvpn routing table failed!");
        return kKadFailed;
    }

    // enable bootstrap public endpoints cache
    if (service_type == kRoot) {
        routing_table_ptr->RegisterBootstrapCacheCallback(get_cache_callback, set_cache_callback);
        TOP_INFO("kroot routing table enable bootstrapcache, register set and get");
    }

    routing_table_ptr->get_local_node_info()->set_service_type(service_type);
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        root_routing_map_[service_type] = routing_table_ptr;
    }

    if (service_type == kRoot) {
        bool first_node = false;
        if (config.Get("node", "first_node", first_node)) {
            if (first_node) {
                TOP_INFO("first node started!");
                return kKadSuccess;
            }
        }
    }

    TOP_INFO("before public_endpoints_all");
    std::set<std::pair<std::string, uint16_t>> public_endpoints_all;
    if (service_type == kRoot) {
        routing_table_ptr->GetBootstrapCache(public_endpoints_all);
    } else {
        GetRootBootstrapCache(public_endpoints_all);
    }

    TOP_INFO("before MergeEndpoints");
    base::MergeEndpoints(public_endpoints_config, public_endpoints_all);
    if (public_endpoints_all.empty()) {
        TOP_FATAL("node join must has bootstrap endpoints!");
        return kKadFailed;
    }

    if (wait_for_joined) {
        if (routing_table_ptr->MultiJoin(public_endpoints_all) != kKadSuccess) {
            TOP_FATAL("MultiJoin failed");
            return kKadFailed;
        }
        TOP_INFO("MultiJoin success.");
    } else {
        routing_table_ptr->MultiJoinAsync(public_endpoints_all);
    }

    return kKadSuccess;
}

int RootRoutingManager::GetBootstrapRootNetwork(
        uint64_t service_type,
        std::set<std::pair<std::string, uint16_t>>& boot_endpoints) {
    TOP_INFO("get bootstrap from root network, service_type=%llu ...", service_type);
    std::vector<top::kadmlia::NodeInfoPtr> nodes;
    for (int i = 0; i < 3; ++i) {  // try 3 times
        TOP_INFO("get public nodes, try %d ...", i);
        std::vector<top::kadmlia::NodeInfoPtr> nodes_tmp;
        if (GetRootNodes(service_type, nodes_tmp) != top::kadmlia::kKadSuccess) {
            TOP_INFO("get public nodes, try %d failed", i);
            continue;
        }

        for (auto& node_ptr : nodes_tmp) {
            if (node_ptr->IsPublicNode()) {
                nodes.push_back(node_ptr);
            } else {
                TOP_INFO("ignore local node(%s:%d)",
                    node_ptr->public_ip.c_str(), (int)node_ptr->public_port);
            }
        }

        // one public node at least
        if (nodes.empty()) {
            TOP_INFO("get public nodes failed");
            return kKadFailed;
        }
    }

    boot_endpoints.clear();
    for (auto& node_ptr : nodes) {
        boot_endpoints.insert(std::make_pair(node_ptr->public_ip, node_ptr->public_port));
        TOP_INFO("boostrap node(%s:%d)",
            node_ptr->public_ip.c_str(), node_ptr->public_port);
    }

    return kKadSuccess;
}

bool RootRoutingManager::GetServiceBootstrapRootNetwork(
        uint64_t service_type,
        std::set<std::pair<std::string, uint16_t>>& boot_endpoints) {
    RoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        auto iter = root_routing_map_.find(kRoot);
        if (iter == root_routing_map_.end()) {
            TOP_WARN("service type[%d] has not added!", service_type);
            return false;
        }
        routing_table = iter->second;
    }
    if (!routing_table) {
        TOP_WARN("get routing table failed!");
        return false;
    }
    RootRouting* root_table = static_cast<RootRouting*>(routing_table.get());
    return root_table->GetCacheServicePublicNodes(service_type, boot_endpoints);
}


bool RootRoutingManager::SetCacheServiceType(uint64_t service_type) {
    RoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(root_routing_map_mutex_);
        auto iter = root_routing_map_.find(kRoot);
        if (iter == root_routing_map_.end()) {
            TOP_WARN("service type[%d] has not added!", service_type);
            return false;
        }
        routing_table = iter->second;
    }
    if (!routing_table) {
        TOP_WARN("get routing table failed!");
        return false;
    }
    RootRouting* root_table = static_cast<RootRouting*>(routing_table.get());
    return root_table->SetCacheServiceType(service_type);
}

int RootRoutingManager::GetRootNodesV2Async(
        const std::string& des_kroot_id,
        uint64_t des_service_type,
        GetRootNodesV2AsyncCallback cb) {
    TOP_DEBUG("bluerootasync start");
    auto root_routing = std::dynamic_pointer_cast<RootRouting>(GetRoutingTable(kRoot));
    if (!root_routing) {
        TOP_WARN("get routing table failed!");
        return kadmlia::kKadFailed;
    }

    std::vector<kadmlia::NodeInfoPtr> nodes;
    using namespace std::placeholders;
    auto callback = std::bind(&RootRoutingManager::OnGetRootNodesV2Async, this, cb, des_service_type, _1);
    if (root_routing->GetRootNodesV2Async(
                des_kroot_id,
                des_service_type,
                callback) != kadmlia::kKadSuccess) {
        /*
        TOP_WARN("getrootnodes failed,des_kroot_id: %s des_service_type: %llu",
                HexEncode(des_kroot_id).c_str(),
                des_service_type);
                */
        return kadmlia::kKadFailed;
    }

    return kadmlia::kKadSuccess;
}

void RootRoutingManager::OnGetRootNodesV2Async(
        GetRootNodesV2AsyncCallback cb,
        uint64_t service_type,
        const std::vector<kadmlia::NodeInfoPtr>& nodes) {
    TOP_DEBUG("bluerootasync callback");
    std::vector<kadmlia::NodeInfoPtr> ret_nodes;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        auto tmp_kad_key = base::GetKadmliaKey(nodes[i]->node_id);
        uint64_t node_service_type = tmp_kad_key->GetServiceType();
        if (service_type != node_service_type) {
            continue;
        }
        ret_nodes.push_back(nodes[i]);
    }

    cb(service_type, ret_nodes);
}

}  // namespace wrouter

}  // namespace top
