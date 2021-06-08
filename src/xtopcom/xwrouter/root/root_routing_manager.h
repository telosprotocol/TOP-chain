#if 0
// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xkad/routing_table/bootstrap_cache.h"
#include "xkad/routing_table/node_info.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/top_utils.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>

namespace top {

namespace transport {
class Transport;
}

namespace wrouter {
class RootRouting;
class RootRoutingManager;
using RootRoutingManagerPtr = std::shared_ptr<RootRoutingManager>;

class RootRoutingManager {
public:
    static RootRoutingManagerPtr Instance();
    RootRoutingManager();
    ~RootRoutingManager();
    void Destory();
    int InitRootRoutingTable(std::shared_ptr<transport::Transport> transport,
                             const base::Config & config,
                             base::KadmliaKeyPtr kad_key_ptr);
    std::shared_ptr<wrouter::RootRouting> GetRootRoutingTable();
    // int GetRootNodes(uint32_t network_id, std::vector<kadmlia::NodeInfoPtr> & root_nodes);
    // int GetRootNodesV2(const std::string & des_id, base::ServiceType service_type, std::vector<kadmlia::NodeInfoPtr> & root_nodes);
    // int GetRootBootstrapCache(std::set<std::pair<std::string, uint16_t>> & boot_endpoints);
    // int GetBootstrapRootNetwork(uint64_t service_type, std::set<std::pair<std::string, uint16_t>> & boot_endpoints);

    // bool GetServiceBootstrapRootNetwork(uint64_t service_type, std::set<std::pair<std::string, uint16_t>> & boot_endpoints);
    // bool SetCacheServiceType(uint64_t service_type);

    // using GetRootNodesV2AsyncCallback = std::function<void(base::ServiceType, const std::vector<kadmlia::NodeInfoPtr> &)>;
    // int GetRootNodesV2Async(const std::string & des_kroot_id, base::ServiceType des_service_type, GetRootNodesV2AsyncCallback cb);

private:
    int CreateRoutingTable(std::shared_ptr<transport::Transport> transport,
                           const base::Config & config,
                           base::KadmliaKeyPtr kad_key_ptr);

    // void OnGetRootNodesV2Async(GetRootNodesV2AsyncCallback cb, base::ServiceType service_type, const std::vector<kadmlia::NodeInfoPtr> & nodes);

    std::shared_ptr<wrouter::RootRouting> root_routing_table_;
    std::mutex root_routing_table_mutex_;

    DISALLOW_COPY_AND_ASSIGN(RootRoutingManager);
};

}  // namespace wrouter

}  // namespace top
#endif