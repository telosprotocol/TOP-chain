// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/small_net_cache.h"

#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_timer.h"
#include "xpbase/base/xip_parser.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/register_routing_table.h"

#include <array>
#include <cassert>

namespace top {

namespace wrouter {
static const uint64_t kClearPeriod = 30ll * 1000ll * 1000ll;                   // 30s check timer
static const uint64_t kClearMarkExpiredPeriod = 3ll * 60ll * 1000ll * 1000ll;  // 3 min check timer
static const uint32_t kKeepMarkExpiredMaxTime = 10 * 60 * 1000;                // 10min
static const size_t kMaxSizePerServiceType = 512;

SmallNetNodes * SmallNetNodes::Instance() {
    static SmallNetNodes ins;
    return &ins;
}

bool SmallNetNodes::Init() {
    clear_timer_ = std::make_shared<base::TimerRepeated>(base::TimerManager::Instance(), "SmallNetNodes::Clear");
    clear_timer_->Start(500ll * 1000ll, kClearPeriod, std::bind(&SmallNetNodes::do_clear_and_reset, this));

    clear_markexpired_timer_ = std::make_shared<base::TimerRepeated>(base::TimerManager::Instance(), "SmallNetNodes::ClearMarkExpired");
    clear_markexpired_timer_->Start(500ll * 1000ll, kClearMarkExpiredPeriod, std::bind(&SmallNetNodes::do_clear_for_mark_expired, this));

    service_nodes_ = ServiceNodes::Instance();

    TOP_INFO("SmallNetNodes Init");
    return true;
}

SmallNetNodes::SmallNetNodes() {
}

SmallNetNodes::~SmallNetNodes() {
    // clear_timer_->Join();
    clear_timer_ = nullptr;
    TOP_INFO("SmallNetNodes destroy");
}

void SmallNetNodes::GetAllServiceType(std::set<uint64_t> & svec) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    for (auto & item : net_nodes_cache_map_) {
        svec.insert(item.first);
    }
    TOP_DEBUG("getallservicetype size: %d", svec.size());
    return;
}

bool SmallNetNodes::GetLocalNodeInfo(uint64_t service_type, NetNode & local_node) {
    std::unique_lock<std::mutex> lockself(local_node_cache_map_mutex_);
    auto ifind = local_node_cache_map_.find(service_type);
    if (ifind == local_node_cache_map_.end()) {
        TOP_WARN("find local node info failed, no elect data caching");
        return false;
    }
    local_node = ifind->second;
    TOP_DEBUG("GetLocalNodeInfo %llu", service_type);
    return true;
}

// for shard, get service type of adv which is the super-up level
bool SmallNetNodes::GetAdvanceOfConsensus(base::KadmliaKeyPtr con_kad, uint8_t associated_gid, uint64_t & adv_service_type) {
    base::KadmliaKeyPtr adv_kad_key = base::GetKadmliaKey();
    if (!adv_kad_key) {
        TOP_WARN("GetKadmliaKey failed");
        return false;
    }
    adv_kad_key->set_xnetwork_id(kChainAdvanceGroup);
    adv_kad_key->set_zone_id(con_kad->Xip().zone_id());
    adv_kad_key->set_cluster_id(con_kad->Xip().cluster_id());
    adv_kad_key->set_group_id(associated_gid);  // attention
    // adv_kad_key->set_xip_type(elect::kElectionCommittee);
    adv_service_type = adv_kad_key->GetServiceType();
    TOP_DEBUG("GetAdvance %llu of con: %s %llu", adv_service_type, HexEncode(con_kad->Get()).c_str(), con_kad->GetServiceType());
    return true;
}

bool SmallNetNodes::HasServiceType(const uint64_t & service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    auto ifind = net_nodes_cache_map_.find(service_type);
    if (ifind == net_nodes_cache_map_.end()) {
        TOP_WARN("service type: %llu not caching", service_type);
        return false;
    }
    return true;
}

// TODO(smaug)
bool SmallNetNodes::GetAllRelatedServiceType(std::map<uint64_t, std::set<uint64_t>> & smap) {
    std::set<uint64_t> svec;
    GetAllServiceType(svec);
    if (svec.empty()) {
        TOP_INFO("no elect data caching");
        return false;
    }

    std::vector<std::shared_ptr<kadmlia::RoutingTable>> vec_rt;
    wrouter::GetAllRegisterRoutingTable(vec_rt);
    TOP_DEBUG("GetAllRegisterRoutingTable size: %d", vec_rt.size());
    if (vec_rt.empty()) {
        TOP_INFO("no service routing table registered");
        return false;
    }

    for (auto & rt : vec_rt) {
        auto xnetwork_id = rt->get_local_node_info()->kadmlia_key()->xnetwork_id();
        auto local_service_type = rt->get_local_node_info()->kadmlia_key()->GetServiceType();
        NetNode local_node;
        if (!GetLocalNodeInfo(local_service_type, local_node)) {
            TOP_WARN("get relationship of local service type: %llu failed", local_service_type);
            continue;
        }
        switch (xnetwork_id) {
        case kChainConsensusGroup:
            // for shard, concerned with higher-up cluster(adv)
            uint64_t adv_service_type;
            if (!GetAdvanceOfConsensus(rt->get_local_node_info()->kadmlia_key(), local_node.m_associated_gid, adv_service_type)) {
                break;
            }
            if (!HasServiceType(adv_service_type)) {
                break;
            }
            smap[local_service_type].insert(adv_service_type);
            TOP_DEBUG("insert relationship service type, local: %llu des: %llu", local_service_type, adv_service_type);
            break;
        case kChainAdvanceGroup:
            // for adv, concerned with beacon(rec/zec) and other adv
            break;
        case kChainArchiveNet:
            // for archive, concerned with self
            break;
        case kChainEdgeNet:
            // for edge, concerned with self
            break;
        case kChainZecNet:
        case kChainRecNet:
            // for beacon(rec/zec), concerned with one adv and backup and edge, and archive
            break;
        default:
            TOP_WARN("invalid service_type, xnetwork_id: %d", xnetwork_id);
            break;
        }
    }

    return true;
}

bool SmallNetNodes::AllowAdd(const std::string & node_id) {
    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(node_id);
    if (!kad_key) {
        TOP_WARN("kad_key nullptr in AllowAdd, node_id:%s", HexEncode(node_id).c_str());
        return false;
    }
    auto service_type = kad_key->GetServiceType();
    // not found in cache elect data and mark expired in latest 10 min, then forbidden addnode
    if (!CheckHasNode(node_id, service_type) && CheckMarkExpired(node_id)) {
        TOP_WARN("no permission to addnode for node_id:%s", HexEncode(node_id).c_str());
        return false;
    }
    TOP_DEBUG("allow to addnode for node_id:%s", HexEncode(node_id).c_str());
    return true;
}

// latest 10 mins mark expired nodes
bool SmallNetNodes::CheckMarkExpired(const std::string & node_id) {
    std::vector<MarkExpiredNetNode> mark_expired_netnode_vec;
    {
        std::unique_lock<std::mutex> mlock(mark_expired_netnode_vec_mutex_);
        // avoid dead lock for now TODO(smaug) better performance
        mark_expired_netnode_vec = mark_expired_netnode_vec_;
    }
    for (auto & item : mark_expired_netnode_vec) {
        if (item.node_id == node_id) {
            TOP_DEBUG("check elect data mark_expired true of node_id:%s", HexEncode(node_id).c_str());
            return true;
        }
    }
    TOP_DEBUG("check elect data mark_expired false of node_id:%s", HexEncode(node_id).c_str());
    return false;
}

// check has node id in cache
bool SmallNetNodes::CheckHasNode(const std::string & node_id, uint64_t service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    auto ifind = net_nodes_cache_map_.find(service_type);
    if (ifind == net_nodes_cache_map_.end()) {
        TOP_WARN("check elect data of node_id:%s failed", HexEncode(node_id).c_str());
        return false;
    }

    for (auto & item : ifind->second->nodes) {
        if (item.m_node_id == node_id) {
            return true;
        }
    }

    TOP_WARN("check elect data of node_id:%s failed", HexEncode(node_id).c_str());
    return false;
}

bool SmallNetNodes::FindNode(const std::string & account, NetNode & Fnode) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    for (auto mitem : net_nodes_cache_map_) {
        for (auto & item : mitem.second->nodes) {
            if (item.m_account == account) {
                Fnode = item;
                return true;
            }
        }
    }
    TOP_WARN("findnode of account:%s failed", account.c_str());
    return false;
}

bool SmallNetNodes::FindNode(uint32_t index, NetNode & Fnode, uint64_t service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    auto ifind = net_nodes_cache_map_.find(service_type);
    if (ifind == net_nodes_cache_map_.end()) {
        return false;
    }
    auto size = (ifind->second)->nodes.size();
    if (size <= index) {
        TOP_WARN("index:%d beyond vector.size:%d", index, size);
        return false;
    }

    Fnode = (ifind->second)->nodes[index];
    TOP_DEBUG("findnode of index:%d account:%s", index, Fnode.m_account.c_str());
    return true;
}

bool SmallNetNodes::FindRandomNode(NetNode & Fnode, uint64_t service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    auto ifind = net_nodes_cache_map_.find(service_type);
    if (ifind == net_nodes_cache_map_.end()) {
        return false;
    }

    auto size = (ifind->second)->nodes.size();
    if (size == 0) {
        return false;
    }
    uint32_t index = RandomUint32() % size;
    Fnode = ifind->second->nodes[index];
    TOP_DEBUG("findnode of index:%d account:%s", index, Fnode.m_account.c_str());
    return true;
}

// always find the last node of vector, usually this is the recent elect-node
bool SmallNetNodes::FindNewNode(NetNode & Fnode, uint64_t service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    auto ifind = net_nodes_cache_map_.find(service_type);
    if (ifind == net_nodes_cache_map_.end()) {
        return false;
    }

    auto size = (ifind->second)->nodes.size();
    if (size == 0) {
        return false;
    }
    Fnode = ifind->second->nodes[size - 1];
    TOP_DEBUG("findnode of index:%d account:%s", size - 1, Fnode.m_account.c_str());
    return true;
}

bool SmallNetNodes::FindAllNode(std::vector<NetNode> & node_vec, uint64_t service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    auto ifind = net_nodes_cache_map_.find(service_type);
    if (ifind == net_nodes_cache_map_.end()) {
        return false;
    }

    node_vec.clear();
    for (auto & node : ifind->second->nodes) {
        node_vec.push_back(node);
    }

    return true;
}

void SmallNetNodes::GetAllNode(std::vector<NetNode> & node_vec) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    for (const auto & item : net_nodes_cache_map_) {
        for (auto & node : (item.second)->nodes) {
            node_vec.push_back(node);
        }
    }
    return;
}

void SmallNetNodes::AddNodeLimit(uint64_t service_type, std::deque<NetNode> & nodes, const NetNode & node) {
    nodes.push_back(node);
    // pop the old nodes
    while (nodes.size() > kMaxSizePerServiceType) {
        const auto & node = nodes.front();
        TOP_INFO("bluever %ld limit drop node %s version(%u)", (long)service_type, node.m_account.c_str(), node.m_version);
        nodes.pop_front();
    }
}

uint32_t SmallNetNodes::AddNode(NetNode node) {
    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(node.m_xip, node.m_account);
    node.m_node_id = kad_key->Get();
    uint64_t service_type = kad_key->GetServiceType();
    if (node.m_account == global_node_id) {
        std::unique_lock<std::mutex> lockself(local_node_cache_map_mutex_);
        local_node_cache_map_[service_type] = node;
        TOP_DEBUG("self addnode account: %s service_type: %llu", node.m_account.c_str(), service_type);
    }

    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    auto ifind = net_nodes_cache_map_.find(service_type);
    if (ifind == net_nodes_cache_map_.end()) {
        auto net_nodes = std::make_shared<NetNodes>();
        net_nodes->latest_version = node.m_version;
        AddNodeLimit(service_type, net_nodes->nodes, node);
        net_nodes_cache_map_[service_type] = net_nodes;
        TOP_DEBUG("bluever %ld add node(%s) version(%llu)", (long)service_type, node.m_account.c_str(), node.m_version);
    } else {
        // update latest version
        auto & latest_version = ifind->second->latest_version;
        if (node.m_version > latest_version) {
            latest_version = node.m_version;
            TOP_DEBUG("bluever %ld update version to %llu", (long)service_type, latest_version);
        }

        // filter old version
        if (node.m_version < latest_version) {
            TOP_DEBUG("bluever %ld old version(%llu) node(%s) is ignore, latest version is %llu", (long)service_type, node.m_version, node.m_account.c_str(), latest_version);
            return 0;
        }

        for (auto & n : ifind->second->nodes) {
            if (n.m_account == node.m_account) {
                if (node.m_version > n.m_version) {
                    TOP_INFO("bluever update node(%s) version(%llu) to %llu", n.m_account.c_str(), n.m_version, node.m_version);
                    n.m_version = node.m_version;
                }
                return net_nodes_cache_map_[service_type]->nodes.size();
            }
        }
        AddNodeLimit(service_type, net_nodes_cache_map_[service_type]->nodes, node);
        TOP_DEBUG("bluever %ld add node(%s) version(%llu)", (long)service_type, node.m_account.c_str(), node.m_version);
    }
    auto size = net_nodes_cache_map_[service_type]->nodes.size();
    TOP_DEBUG(
        "addnode account:%s public_key:%s xip:%s service_type:%llu xnetwork_id:%u,"
        "now size:%u",
        node.m_account.c_str(),
        HexEncode(node.m_public_key).c_str(),
        HexEncode(node.m_xip.xip()).c_str(),
        service_type,
        node.m_xip.xnetwork_id(),
        size);
    return size;
}

void SmallNetNodes::HandleExpired(std::unordered_map<uint64_t, std::vector<std::string>> & expired_vec,
                                  std::vector<uint64_t> & unreg_service_type_vec,
                                  std::vector<MarkExpiredNetNode> & mark_expired_netnode_vec) {
    // remove service expired node
    TOP_DEBUG("call service_node handleexpired");
    {
        std::unique_lock<std::mutex> mlock(mark_expired_netnode_vec_mutex_);
        mark_expired_netnode_vec_.insert(mark_expired_netnode_vec_.end(), mark_expired_netnode_vec.begin(), mark_expired_netnode_vec.end());
        TOP_DEBUG("append %u nodes mark_expired", mark_expired_netnode_vec.size());
    }

    service_nodes_->RemoveExpired(expired_vec);

    // unregister routing table
    for (auto & service_type : unreg_service_type_vec) {
        // TODO(smaug)
        TOP_KINFO("unregister routing table %llu", service_type);
        MultiRouting::Instance()->RemoveRoutingTable(service_type);
        TOP_INFO("smaug quit routing table for service_type:%llu", service_type);
    }

    // drop node of routing table
    for (auto & item : expired_vec) {
        auto service_routing = wrouter::GetRoutingTable(item.first, false);
        if (!service_routing) {
            TOP_WARN("no routing table of service_type:%llu found", item.first);
            continue;
        }
        service_routing->BulkDropNode(item.second);
        TOP_INFO("service_node_cache bulkdropnode %u nodes of service_type:%llu", (item.second).size(), item.first);
    }
}

void SmallNetNodes::do_clear_and_reset() {
    // clear old versions
    std::unordered_map<uint64_t, std::vector<std::string>> expired_nodeid_vec;
    std::vector<uint64_t> unreg_service_type_vec;
    std::vector<MarkExpiredNetNode> mark_expired_netnode_vec;
    {
        std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
        for (auto & mitem : net_nodes_cache_map_) {
            const auto & latest_version = mitem.second->latest_version;
            if (latest_version < 1) {
                continue;
            }
            TOP_DEBUG("%ld check version %u", (long)mitem.first, latest_version);
            for (auto iter = (mitem.second)->nodes.begin(); iter != (mitem.second)->nodes.end();) {
                if (iter->m_version < latest_version - 1) {  // keep the latest 2 versions
                    // elect nodes expired
                    TOP_DEBUG("bluever %ld remove expired node(%s) version(%llu)", (long)mitem.first, iter->m_account.c_str(), iter->m_version);
                    if (iter->m_account == global_node_id) {
                        TOP_DEBUG("add unreg service_type:%llu", mitem.first);
                        unreg_service_type_vec.push_back(mitem.first);
                    }

                    // base::KadmliaKeyPtr tmp_kad_key = base::GetKadmliaKey(iter->m_xip, iter->m_account);
                    // expired_nodeid_vec[mitem.first].push_back(tmp_kad_key->Get());
                    expired_nodeid_vec[mitem.first].push_back(iter->m_node_id);
                    MarkExpiredNetNode mnode{iter->m_node_id, std::chrono::steady_clock::now()};
                    mark_expired_netnode_vec.push_back(mnode);
                    iter = (mitem.second)->nodes.erase(iter);
                } else {
                    ++iter;
                }
            }
        }
    }

    HandleExpired(expired_nodeid_vec, unreg_service_type_vec, mark_expired_netnode_vec);
}

void SmallNetNodes::do_clear_for_mark_expired() {
    {
        std::unique_lock<std::mutex> mlock(mark_expired_netnode_vec_mutex_);
        auto tp_now = std::chrono::steady_clock::now();
        for (auto it = mark_expired_netnode_vec_.begin(); it != mark_expired_netnode_vec_.end();) {
            if (it->time_point + std::chrono::milliseconds(kKeepMarkExpiredMaxTime) < tp_now) {
                // just keep latest 10 min
                TOP_DEBUG("remove mark expired node:%s", HexEncode(it->node_id).c_str());
                it = mark_expired_netnode_vec_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

}  // end namespace wrouter

}  // end namespace top
