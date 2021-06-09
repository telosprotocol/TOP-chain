// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xpbase/base/top_timer.h"
#include "xwrouter/multi_routing/net_node.h"

#include <chrono>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace top {

namespace base {
class KadmliaKey;
typedef std::shared_ptr<KadmliaKey> KadmliaKeyPtr;
}  // namespace base

namespace wrouter {
class ServiceNodes;

class SmallNetNodes final {
public:
    static SmallNetNodes * Instance();
    bool Init();

    void GetAllServiceType(std::set<base::ServiceType> & svec);
    void AddNode(std::vector<wrouter::WrouterTableNodes> node);
    // bool FindNewNode(WrouterTableNodes & Fnode, base::ServiceType service_type);
    bool FindRandomNode(WrouterTableNodes & Fnode, base::ServiceType service_type);
    bool FindAllNode(std::vector<WrouterTableNodes> & node_vec, base::ServiceType service_type);
    void GetAllNode(std::vector<WrouterTableNodes> & node_vec);

private:
    SmallNetNodes();
    ~SmallNetNodes();
    // void AddNodeLimit(base::ServiceType service_type, std::deque<WrouterTableNodes> & nodes, const WrouterTableNodes & node);
    // void HandleExpired(std::unordered_map<base::ServiceType, std::vector<std::string>> & expired_vec, std::vector<base::ServiceType> & unreg_service_type_vec);
    // void do_clear_and_reset();

private:
    std::mutex net_nodes_cache_map_mutex_;
    // key is service_type, value is vector of accounts
    struct WrouterTableNodes_queue {
        uint32_t latest_version{0};
        std::deque<WrouterTableNodes> nodes;
    };
    // std::unordered_map<base::ServiceType, std::shared_ptr<WrouterTableNodes_queue>> net_nodes_cache_map_;
    std::unordered_map<base::ServiceType, std::vector<wrouter::WrouterTableNodes>> net_nodes_cache_map_;

    // std::shared_ptr<base::TimerRepeated> clear_timer_{nullptr};

    ServiceNodes * service_nodes_{nullptr};
};

}  // namespace wrouter

}  // namespace top
