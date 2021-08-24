// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/small_net_cache.h"

#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_timer.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/multi_routing/service_node_cache.h"

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
    service_nodes_ = ServiceNodes::Instance();

    TOP_INFO("SmallNetNodes Init");
    return true;
}

SmallNetNodes::SmallNetNodes() {
}

SmallNetNodes::~SmallNetNodes() {
    // clear_timer_->Join();
    // clear_timer_ = nullptr;
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

    for (auto const & _p : net_nodes_cache_map_) {
        if (_p.first == service_type && _p.second.size()) {
            auto size = _p.second.size();
            uint32_t index = RandomUint32() % size;
            Fnode = _p.second[index];
            TOP_DEBUG("findnode of index:%d account:%s", index, Fnode.node_id.c_str());
            return true;
        }
    }
    return false;
}

bool SmallNetNodes::FindAllNode(std::vector<WrouterTableNodes> & node_vec, base::ServiceType service_type) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);

    for (auto const & _p : net_nodes_cache_map_) {
        if (_p.first == service_type && _p.second.size()) {
            node_vec.clear();
            for (auto & node : _p.second) {
                node_vec.push_back(node);
            }
            return true;
        }
    }
    return false;
}

void SmallNetNodes::GetAllNode(std::vector<WrouterTableNodes> & node_vec) {
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);
    for (const auto & item : net_nodes_cache_map_) {
        for (auto & node : item.second) {
            node_vec.push_back(node);
        }
    }
    return;
}

void SmallNetNodes::AddNode(std::vector<wrouter::WrouterTableNodes> node) {
    if (node.empty())
        return;
    base::ServiceType comming_service_type = base::GetKadmliaKey(node[0].m_xip2)->GetServiceType();
    std::unique_lock<std::mutex> lock(net_nodes_cache_map_mutex_);

    if (comming_service_type.IsBroadcastService()) {
        if (net_nodes_cache_map_.find(comming_service_type) != net_nodes_cache_map_.end()) {
            xdbg("ElectNetNodes::AddNode update broadcast service election result %s", comming_service_type.info().c_str());
            net_nodes_cache_map_.erase(comming_service_type);
        }
    } else {
        std::vector<base::ServiceType> erase_service_vector;
        for (auto const & _p : net_nodes_cache_map_) {
            base::ServiceType service_type = _p.first;
            if (comming_service_type.IsNewer(service_type, 2)) {
                xdbg("ElectNetNodes::AddNode get new election result %s erase old %s", comming_service_type.info().c_str(), service_type.info().c_str());
                service_nodes_->RemoveExpired(service_type);
                erase_service_vector.push_back(service_type);
            }
        }
        for (auto & erase_service : erase_service_vector) {
            net_nodes_cache_map_.erase(erase_service);
        }
    }

    net_nodes_cache_map_.insert(std::make_pair(comming_service_type, node));
}

}  // end namespace wrouter

}  // end namespace top
