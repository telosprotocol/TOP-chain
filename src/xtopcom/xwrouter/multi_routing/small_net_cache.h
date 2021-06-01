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

    void GetAllServiceType(std::set<uint64_t> & svec);
    uint32_t AddNode(NetNode node);
    bool FindNewNode(NetNode & Fnode, uint64_t service_type);
    bool FindRandomNode(NetNode & Fnode, uint64_t service_type);
    bool FindAllNode(std::vector<NetNode> & node_vec, uint64_t service_type);
    void GetAllNode(std::vector<NetNode> & node_vec);

private:
    SmallNetNodes();
    ~SmallNetNodes();
    void AddNodeLimit(uint64_t service_type, std::deque<NetNode> & nodes, const NetNode & node);
    void HandleExpired(std::unordered_map<uint64_t, std::vector<std::string>> & expired_vec, std::vector<uint64_t> & unreg_service_type_vec);
    void do_clear_and_reset();

private:
    std::mutex net_nodes_cache_map_mutex_;
    // key is service_type, value is vector of accounts
    struct NetNodes {
        uint32_t latest_version{0};
        std::deque<NetNode> nodes;
    };
    std::unordered_map<uint64_t, std::shared_ptr<NetNodes>> net_nodes_cache_map_;

    std::shared_ptr<base::TimerRepeated> clear_timer_{nullptr};

    ServiceNodes * service_nodes_{nullptr};
};

}  // namespace wrouter

}  // namespace top
