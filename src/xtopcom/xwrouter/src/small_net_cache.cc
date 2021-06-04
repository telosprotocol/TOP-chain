// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/small_net_cache.h"

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

void SmallNetNodes::GetAllServiceType(std::set<base::ServiceType> & svec) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    for (auto & item : net_nodes_cache_map_) {
        svec.insert(item.first);
    }
    TOP_DEBUG("getallservicetype size: %d", svec.size());
    return;
}

bool SmallNetNodes::FindRandomNode(WrouterTableNodes & Fnode, base::ServiceType service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);

    for(auto const & _p : net_nodes_cache_map_){
        if(_p.first==service_type && _p.second->nodes.size()){
            auto size = _p.second->nodes.size();
            uint32_t index = RandomUint32() % size;
            Fnode = _p.second->nodes[index];
            TOP_DEBUG("findnode of index:%d account:%s", index, Fnode.node_id.c_str());
            return true;
        }
    }
    return false;


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
    TOP_DEBUG("findnode of index:%d account:%s", index, Fnode.node_id.c_str());
    return true;
}

// always find the last node of vector, usually this is the recent elect-node
bool SmallNetNodes::FindNewNode(WrouterTableNodes & Fnode, base::ServiceType service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    // xdbg("Charles Debug SmallNetNodes::FindNewNode size: %zu",net_nodes_cache_map_.size());
    // for(auto const & _p:net_nodes_cache_map_){
    //     xdbg("Charles Debug service: %lld , queue size: %zu", _p.first.value(), _p.second->nodes.size());
    // }

    for(auto _p : net_nodes_cache_map_){
        if(_p.first==service_type && _p.second->nodes.size()){
            Fnode = _p.second->nodes.back();
            xdbg("SmallNetNodes::FindNewNode ori service_type: %lld ,from service_type: %lld ,size: %zu", service_type.value(), _p.first.value(), _p.second->nodes.size());
            return true;
        }
    }
    return false;

    // auto ifind = net_nodes_cache_map_.find(service_type);
    // if (ifind == net_nodes_cache_map_.end()) {
    //     return false;
    // }

    // auto size = (ifind->second)->nodes.size();
    // if (size == 0) {
    //     return false;
    // }
    // Fnode = ifind->second->nodes[size - 1];
    // TOP_DEBUG("findnode of index:%d account:%s", size - 1, Fnode.node_id.c_str());
    // return true;
}

bool SmallNetNodes::FindAllNode(std::vector<WrouterTableNodes> & node_vec, base::ServiceType service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    
    for(auto const & _p : net_nodes_cache_map_){
        if(_p.first==service_type && _p.second->nodes.size()){
            node_vec.clear();
            for (auto & node : _p.second->nodes) {
                node_vec.push_back(node);
            }
            return true;
        }
    }
    return false;
    
    
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

void SmallNetNodes::GetAllNode(std::vector<WrouterTableNodes> & node_vec) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    for (const auto & item : net_nodes_cache_map_) {
        for (auto & node : (item.second)->nodes) {
            node_vec.push_back(node);
        }
    }
    return;
}

void SmallNetNodes::AddNodeLimit(base::ServiceType service_type, std::deque<WrouterTableNodes> & nodes, const WrouterTableNodes & node) {
    nodes.push_back(node);
    // pop the old nodes
    while (nodes.size() > kMaxSizePerServiceType) {
        const auto & node = nodes.front();
        TOP_INFO("bluever %ld limit drop node %s version(%u)", (long)service_type.value(), node.node_id.c_str());
        nodes.pop_front();
    }
}

uint32_t SmallNetNodes::AddNode(WrouterTableNodes node) {
    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(node.m_xip2);
    // node.m_node_id = kad_key->Get();
    base::ServiceType service_type = kad_key->GetServiceType();

    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    // for add_node find must be precise
    auto ifind = net_nodes_cache_map_.find(service_type);
    if (ifind == net_nodes_cache_map_.end()) {
        auto net_nodes = std::make_shared<WrouterTableNodes_queue>();
        net_nodes->latest_version = node.m_xip2.height();
        net_nodes->nodes.push_back(node);
        net_nodes_cache_map_[service_type] = net_nodes;
        TOP_DEBUG("bluever %ld add node(%s) version(%llu)", (long)service_type.value(), node.node_id.c_str(), node.m_xip2.height());
    } else {
        // update latest version
        auto & latest_version = ifind->second->latest_version;
        if (node.m_xip2.height() > latest_version) {
            latest_version = node.m_xip2.height();
            TOP_DEBUG("bluever %ld update version to %llu", (long)service_type.value(), latest_version);
        }

        // filter old version
        if (node.m_xip2.height() < latest_version) {
            TOP_DEBUG("bluever %ld old version(%llu) node(%s) is ignore, latest version is %llu", (long)service_type.value(), node.m_xip2.height(), node.node_id.c_str(), latest_version);
            return 0;
        }

        for (auto & n : ifind->second->nodes) {
            if (n.node_id == node.node_id) {
                if (node.m_xip2.height() > n.m_xip2.height()) {
                    TOP_INFO("bluever update node(%s) version(%llu) to %llu", n.node_id.c_str(), n.m_xip2.height(), node.m_xip2.height());
                    n.m_xip2 = node.m_xip2;
                }
                return net_nodes_cache_map_[service_type]->nodes.size();
            }
        }
        AddNodeLimit(service_type, net_nodes_cache_map_[service_type]->nodes, node);
        TOP_DEBUG("bluever %ld add node(%s) version(%llu)", (long)service_type.value(), node.node_id.c_str(), node.m_xip2.height());
    }
    auto size = net_nodes_cache_map_[service_type]->nodes.size();
    TOP_DEBUG("addnode account:%s service_type:%llu xip2:%s,now size:%u",
              node.node_id.c_str(),
              // HexEncode(node.m_public_key).c_str(),
              service_type.value(),
              node.m_xip2.to_string().c_str(),
              size);
    return size;
    // return 0;
}

void SmallNetNodes::HandleExpired(std::unordered_map<base::ServiceType, std::vector<std::string>> & expired_vec,
                                  std::vector<base::ServiceType> & unreg_service_type_vec) {
    service_nodes_->RemoveExpired(expired_vec);

    // unregister routing table
    for (auto & service_type : unreg_service_type_vec) {
        // TODO(smaug)
        TOP_KINFO("unregister routing table %llu", service_type.value());
        MultiRouting::Instance()->RemoveRoutingTable(service_type);
        TOP_INFO("smaug quit routing table for service_type:%llu", service_type.value());
    }

    // drop node of routing table
    for (auto & item : expired_vec) {
        auto service_routing = wrouter::GetRoutingTable(item.first, false);
        if (!service_routing) {
            TOP_WARN("no routing table of service_type:%llu found", item.first.value());
            continue;
        }
        service_routing->BulkDropNode(item.second);
        TOP_INFO("service_node_cache bulkdropnode %u nodes of service_type:%llu", (item.second).size(), item.first.value());
    }
}

void SmallNetNodes::do_clear_and_reset() {
    // clear old versions
    std::unordered_map<base::ServiceType, std::vector<std::string>> expired_nodeid_vec;
    std::vector<base::ServiceType> unreg_service_type_vec;
    {
        std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
        for (auto & mitem : net_nodes_cache_map_) {
            const auto & latest_version = mitem.second->latest_version;
            if (latest_version < 1) {
                continue;
            }
            TOP_DEBUG("%ld check version %u", (long)mitem.first.value(), latest_version);
            for (auto iter = (mitem.second)->nodes.begin(); iter != (mitem.second)->nodes.end();) {
                if (iter->m_xip2.height() < latest_version - 1) {  // keep the latest 2 versions
                    // elect nodes expired
                    TOP_DEBUG("bluever %ld remove expired node(%s) version(%llu)", (long)mitem.first.value(), iter->node_id.c_str(), iter->m_xip2.height());
                    if (iter->node_id == global_node_id) {
                        TOP_DEBUG("add unreg service_type:%llu", mitem.first.value());
                        unreg_service_type_vec.push_back(mitem.first);
                    }

                    // base::KadmliaKeyPtr tmp_kad_key = base::GetKadmliaKey(iter->m_xip, iter->node_id);
                    // expired_nodeid_vec[mitem.first].push_back(tmp_kad_key->Get());
                    expired_nodeid_vec[mitem.first].push_back(iter->node_id);
                    iter = (mitem.second)->nodes.erase(iter);
                } else {
                    ++iter;
                }
            }
        }
    }

    HandleExpired(expired_nodeid_vec, unreg_service_type_vec);
}

}  // end namespace wrouter

}  // end namespace top
