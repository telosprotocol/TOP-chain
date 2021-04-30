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
    bool GetLocalNodeInfo(uint64_t service_type, NetNode & local_node);
    bool GetAdvanceOfConsensus(base::KadmliaKeyPtr con_kad, uint8_t associated_gid, uint64_t & adv_service_type);
    // key is local service_type, value is set of dest service_type
    bool GetAllRelatedServiceType(std::map<uint64_t, std::set<uint64_t>> & smap);
    uint32_t AddNode(NetNode node);
    bool FindNode(const std::string & account, NetNode & Fnode);
    bool FindNode(uint32_t index, NetNode & Fnode, uint64_t service_type);
    bool FindNewNode(NetNode & Fnode, uint64_t service_type);
    bool FindRandomNode(NetNode & Fnode, uint64_t service_type);
    bool FindAllNode(std::vector<NetNode> & node_vec, uint64_t service_type);
    void GetAllNodes(std::vector<NetNode> & node_vec);

    void GetAllNode(std::vector<NetNode> & node_vec);
    bool AllowAdd(const std::string & node_id);  // allow add node from routing table
    bool CheckHasNode(const std::string & node_id, uint64_t service_type);
    bool CheckMarkExpired(const std::string & node_id);  // latest 10 mins mark expired nodes
    void do_clear_and_reset();
    // clear mark expired vector
    void do_clear_for_mark_expired();
    bool HasServiceType(const uint64_t & service_type);

private:
    SmallNetNodes();
    ~SmallNetNodes();
    void AddNodeLimit(uint64_t service_type, std::deque<NetNode> & nodes, const NetNode & node);
    void HandleExpired(std::unordered_map<uint64_t, std::vector<std::string>> & expired_vec,
                       std::vector<uint64_t> & unreg_service_type_vec,
                       std::vector<MarkExpiredNetNode> & mark_expired_netnode_vec);

private:
    std::mutex net_nodes_cache_map_mutex_;
    // key is service_type, value is vector of accounts
    struct NetNodes {
        uint32_t latest_version{0};
        std::deque<NetNode> nodes;
    };
    std::unordered_map<uint64_t, std::shared_ptr<NetNodes>> net_nodes_cache_map_;
    std::mutex local_node_cache_map_mutex_;
    // keep local node info(xip,account,public_key...)
    std::map<uint64_t, NetNode> local_node_cache_map_;
    std::shared_ptr<base::TimerRepeated> clear_timer_{nullptr};
    std::shared_ptr<base::TimerRepeated> clear_markexpired_timer_{nullptr};
    std::mutex mark_expired_netnode_vec_mutex_;
    std::vector<MarkExpiredNetNode> mark_expired_netnode_vec_;
    ServiceNodes * service_nodes_{nullptr};
};

}  // namespace wrouter

}  // namespace top
