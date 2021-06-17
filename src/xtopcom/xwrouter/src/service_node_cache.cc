// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/service_node_cache.h"

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_timer.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/multi_routing/small_net_cache.h"

#include <cassert>

namespace top {

namespace wrouter {
static const uint64_t kUpdatePeriod = 15ll * 1000ll * 1000ll;            // 2 min check timer
static const uint64_t kUpdateSuperNodesPeriod = 60ll * 1000ll * 1000ll;  // 2 min check timer

ServiceNodes * ServiceNodes::Instance() {
    static ServiceNodes ins;
    return &ins;
}

bool ServiceNodes::Init() {
    update_timer_ = std::make_shared<base::TimerRepeated>(base::TimerManager::Instance(), "ServiceNodes::do_update");
    update_timer_->Start(500ll * 1000ll, kUpdatePeriod, std::bind(&ServiceNodes::do_update, this));

    small_net_nodes_ = SmallNetNodes::Instance();
    TOP_INFO("ServiceNodes Init");
    return true;
}

ServiceNodes::ServiceNodes() {
}

ServiceNodes::~ServiceNodes() {
    update_timer_->Join();
    update_timer_ = nullptr;
    TOP_INFO("ServiceNodes destroy");
}

// get service_type nodes from cache
bool ServiceNodes::GetRootNodes(base::ServiceType service_type, std::vector<kadmlia::NodeInfoPtr> & node_vec) {
    if (FindNode(service_type, node_vec)) {
        for (const auto & item : node_vec) {
            TOP_DEBUG("getrootnodes %s %s:%u %llu", item->node_id.c_str(), (item->public_ip).c_str(), item->public_port, item->hash64);
        }
        TOP_DEBUG("getrootnodes of service_type: %llu ok, size: %d", service_type.value(), node_vec.size());
        return true;
    }

    WrouterTableNodes Fnode;
    if (small_net_nodes_->FindRandomNode(Fnode, service_type)) {
        base::KadmliaKeyPtr kad_key = base::GetRootKadmliaKey(Fnode.node_id);  // kRoot id
        assert(kad_key);
        using namespace std::placeholders;
        auto cb = std::bind(&ServiceNodes::OnCacheElectNodesAsync, this, _1, _2);
        auto root_routing_table = std::dynamic_pointer_cast<RootRouting>(MultiRouting::Instance()->GetRootRoutingTable());
        root_routing_table->CacheElectNodesAsync(kad_key->Get(), service_type, cb);
    }
    TOP_WARN("getrootnodes of service_type: %llu failed", service_type.value());
    return false;
}

bool ServiceNodes::GetRootNodes(base::ServiceType service_type, const std::string & des_node_id, kadmlia::NodeInfoPtr & node_ptr) {
    if (FindNode(service_type, des_node_id, node_ptr)) {
        TOP_DEBUG("getrootnodes of service_type: %llu ok", service_type.value());
        return true;
    }

    WrouterTableNodes Fnode;
    if (small_net_nodes_->FindRandomNode(Fnode, service_type)) {
        base::KadmliaKeyPtr kad_key = base::GetRootKadmliaKey(Fnode.node_id);  // kRoot id
        assert(kad_key);
        using namespace std::placeholders;
        auto cb = std::bind(&ServiceNodes::OnCacheElectNodesAsync, this, _1, _2);
        auto root_routing_table = std::dynamic_pointer_cast<RootRouting>(MultiRouting::Instance()->GetRootRoutingTable());
        root_routing_table->CacheElectNodesAsync(kad_key->Get(), service_type, cb);
    }
    TOP_WARN("getrootnodes of service_type: %llu failed", service_type.value());
    return false;
}

void ServiceNodes::OnCacheElectNodesAsync(base::ServiceType service_type, const std::vector<kadmlia::NodeInfoPtr> & node_vec) {
    for (auto & n : node_vec) {
        AddNode(service_type, n);
    }
}

bool ServiceNodes::CheckHasNode(base::KadmliaKeyPtr kad_key) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    auto ifind = service_nodes_cache_map_.find(kad_key->GetServiceType());
    if (ifind == service_nodes_cache_map_.end()) {
        return false;
    }
    if (ifind->second.empty()) {
        return false;
    }
    for (const auto & item : ifind->second) {
        if (item->node_id == kad_key->Get()) {
            return true;
        }
    }
    return false;
}

bool ServiceNodes::FindNode(base::ServiceType service_type, const std::string & des_node_id, kadmlia::NodeInfoPtr & node) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);

    for (auto const & _p : service_nodes_cache_map_) {
        if (_p.first == service_type && _p.second.size()) {
            for (auto & node_ptr : _p.second) {
                if (node_ptr->node_id == des_node_id) {
                    node = node_ptr;
                    TOP_DEBUG("find node of des_node_id:%s directly ok", des_node_id.c_str());
                    break;
                }
            }
            if (!node) {
                auto size = (_p.second).size();
                uint32_t index = RandomUint32() % size;
                node = (_p.second)[index];  // random
            }
            TOP_DEBUG("find node:(%s:%d) service_node of service_type: %llu", (node->public_ip).c_str(), node->public_port, service_type.value());
            return true;
        }
    }
    return false;

    auto ifind = service_nodes_cache_map_.find(service_type);
    if (ifind == service_nodes_cache_map_.end()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type.value());
        return false;
    }
    if (ifind->second.empty()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type.value());
        return false;
    }
    for (auto & node_ptr : ifind->second) {
        if (node_ptr->node_id == des_node_id) {
            node = node_ptr;
            TOP_DEBUG("find node of des_node_id:%s directly ok", des_node_id.c_str());
            break;
        }
    }
    if (!node) {
        auto size = (ifind->second).size();
        uint32_t index = RandomUint32() % size;
        node = (ifind->second)[index];  // random
    }

    TOP_DEBUG("find node:(%s:%d) service_node of service_type: %llu", (node->public_ip).c_str(), node->public_port, service_type.value());
    return true;
}

bool ServiceNodes::FindNode(base::ServiceType service_type, std::vector<kadmlia::NodeInfoPtr> & node_vec) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    for (auto const & _p : service_nodes_cache_map_) {
        if (_p.first == service_type && _p.second.size()) {
            auto size = _p.second.size();
            // just return the last 3 nodes
            if (size <= 3) {
                node_vec = _p.second;
            } else {
                uint32_t index = RandomUint32() % size;
                if (index >= (size - 3)) {
                    index = size - 3;
                }
                for (auto i = 0; i < 3; ++i) {
                    node_vec.push_back((_p.second)[index + i]);
                }
            }
            TOP_DEBUG("find  %d service_node of service_type: %llu", size, service_type.value());
            return true;
        }
    }
    return false;

    auto ifind = service_nodes_cache_map_.find(service_type);
    if (ifind == service_nodes_cache_map_.end()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type.value());
        return false;
    }
    if (ifind->second.empty()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type.value());
        return false;
    }

    auto size = (ifind->second).size();
    // just return the last 3 nodes
    if (size <= 3) {
        node_vec = ifind->second;
    } else {
        uint32_t index = RandomUint32() % size;
        if (index >= (size - 3)) {
            // max value of index is size - 3
            index = size - 3;
        }
        for (auto i = 0; i < 3; ++i) {
            node_vec.push_back((ifind->second)[index + i]);
        }
    }
    TOP_DEBUG("find  %d service_node of service_type: %llu", size, service_type.value());
    return true;
}

void ServiceNodes::GetAllServicesNodes(std::vector<kadmlia::NodeInfoPtr> & node_vec) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    for (const auto & item : service_nodes_cache_map_) {
        if (item.second.size() > 0) {
            for (const auto & sitem : item.second) {
                node_vec.push_back(sitem);
            }
        }
    }
    return;
}

bool ServiceNodes::AddNode(base::ServiceType service_type, kadmlia::NodeInfoPtr node) {
    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(node->node_id);
    base::ServiceType node_service_type = kad_key->GetServiceType();
    if (node_service_type != service_type) {
        TOP_WARN("node[%s](%s:%d)node_service_type: %llu not equal service_type: %llu",
                 (node->node_id).c_str(),
                 (node->public_ip).c_str(),
                 node->public_port,
                 node_service_type.value(),
                 service_type.value());
        return false;
    }

    // TODO(smaug) set limit of size
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    // for add_node find must be precise
    auto ifind = service_nodes_cache_map_.find(service_type);
    if (ifind == service_nodes_cache_map_.end()) {
        std::vector<kadmlia::NodeInfoPtr> tmp_nodes;
        tmp_nodes.push_back(node);
        service_nodes_cache_map_.insert(std::pair<base::ServiceType, std::vector<kadmlia::NodeInfoPtr>>(service_type, tmp_nodes));
    } else {
        for (auto & nptr : ifind->second) {
            if (nptr->node_id == node->node_id) {
                TOP_DEBUG("already has node:%s %s:%d for service_type: %llu", node->node_id.c_str(), (node->public_ip).c_str(), node->public_port, service_type.value());
                return false;
            }
        }
        (ifind->second).push_back(node);
    }
    TOP_DEBUG("addnode service_type: %llu, service_node:%s %s:%d  now size: %d",
              service_type.value(),
              (node->node_id).c_str(),
              (node->public_ip).c_str(),
              node->public_port,
              service_nodes_cache_map_[service_type].size());
    return true;
}

void ServiceNodes::RemoveExpired(base::ServiceType const & service_type) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    auto ifind = service_nodes_cache_map_.find(service_type);
    if (ifind != service_nodes_cache_map_.end()) {
        service_nodes_cache_map_.erase(ifind);
    }
}

void ServiceNodes::do_update() {
    std::set<base::ServiceType> service_type_vec;
    small_net_nodes_->GetAllServiceType(service_type_vec);
    TOP_DEBUG("small net nodes getallservicetype size: %d", service_type_vec.size());
    for (auto & item : service_type_vec) {
        base::ServiceType service_type = item;
        TOP_DEBUG("begin do_update service_type: %llu", service_type.value());
        std::vector<WrouterTableNodes> node_vec;
        if (!small_net_nodes_->FindAllNode(node_vec, service_type) || node_vec.empty()) {
            TOP_WARN("can't find nodes of service_type: %llu", service_type.value());
            continue;
        }

        const auto rand_index = RandomUint32() % node_vec.size();
        std::string account = node_vec[rand_index].node_id;
        base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(node_vec[rand_index].m_xip2);
        if (CheckHasNode(kad_key)) {
            continue;
        }
        TOP_DEBUG("blueroot do update by account:%s, index:%u", account.c_str(), rand_index);
        base::KadmliaKeyPtr root_kad_key = base::GetRootKadmliaKey(account);  // kRoot id
        assert(root_kad_key);
        using namespace std::placeholders;
        auto cb = std::bind(&ServiceNodes::OnCacheElectNodesAsync, this, _1, _2);
        auto root_routing_table = std::dynamic_pointer_cast<RootRouting>(MultiRouting::Instance()->GetRootRoutingTable());
        if (!root_routing_table)
            return;
        root_routing_table->CacheElectNodesAsync(root_kad_key->Get(), service_type, cb);
    }
}

}  // end namespace wrouter

}  // end namespace top
