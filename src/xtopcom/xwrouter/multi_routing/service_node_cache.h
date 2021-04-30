// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xkad/routing_table/node_info.h"
#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_timer.h"
#include "xpbase/base/xip_parser.h"

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace top {

namespace wrouter {

class SmallNetNodes;

// class ServiceNodes final {
class ServiceNodes {
public:
    static ServiceNodes * Instance();
    bool Init();

    // search service_type nodes from bootstrap network
    bool GetRootNodes(uint64_t service_type, std::vector<kadmlia::NodeInfoPtr> & node_vec);
    bool GetRootNodes(uint64_t service_type, const std::string & account, std::vector<kadmlia::NodeInfoPtr> & node_vec);
    // for point2point, get des_node_id if exist, or get same service_type node
    bool GetRootNodes(uint64_t service_type, const std::string & des_node_id, kadmlia::NodeInfoPtr & node_ptr);

    bool FindNode(uint64_t service_type, std::vector<kadmlia::NodeInfoPtr> & node_vec);
    bool FindNode(uint64_t service_type, kadmlia::NodeInfoPtr & node_ptr);
    bool FindNode(uint64_t service_type, const std::string & des_node_id, kadmlia::NodeInfoPtr & node_ptr);
    bool CheckHasNode(base::KadmliaKeyPtr kad_key);
    void RemoveExpired(const std::unordered_map<uint64_t, std::vector<std::string>> & expired_node_vec);

    void GetAllServicesNodes(std::vector<kadmlia::NodeInfoPtr> & node_vec);

private:
    ServiceNodes();
    ~ServiceNodes();
    bool AddNode(uint64_t service_type, kadmlia::NodeInfoPtr node);
    void OnGetRootNodesAsync(uint64_t service_type, const std::vector<kadmlia::NodeInfoPtr> & node_vec);

    void do_update();

private:
    std::mutex service_nodes_cache_map_mutex_;
    // key is service_type
    std::unordered_map<uint64_t, std::vector<kadmlia::NodeInfoPtr>> service_nodes_cache_map_;
    std::shared_ptr<base::TimerRepeated> update_timer_{nullptr};
    SmallNetNodes * small_net_nodes_{nullptr};
};

}  // namespace wrouter

}  // namespace top
