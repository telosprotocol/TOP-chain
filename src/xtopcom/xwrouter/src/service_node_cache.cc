// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/service_node_cache.h"

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_timer.h"
#include "xpbase/base/xip_parser.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/root/root_routing_manager.h"

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
bool ServiceNodes::GetRootNodes(uint64_t service_type, std::vector<kadmlia::NodeInfoPtr> & node_vec) {
    if (FindNode(service_type, node_vec)) {
        for (const auto & item : node_vec) {
            TOP_DEBUG("getrootnodes %s %s:%u %llu", HexEncode(item->node_id).c_str(), (item->public_ip).c_str(), item->public_port, item->hash64);
        }
        TOP_DEBUG("getrootnodes of service_type: %llu ok, size: %d", service_type, node_vec.size());
        return true;
    }

    NetNode Fnode;
    if (small_net_nodes_->FindNewNode(Fnode, service_type)) {
        base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(Fnode.m_account, true);  // kRoot id
        assert(kad_key);
        using namespace std::placeholders;
        auto cb = std::bind(&ServiceNodes::OnGetRootNodesAsync, this, _1, _2);
        RootRoutingManager::Instance()->GetRootNodesV2Async(kad_key->Get(), service_type, cb);  // just call
    }
    TOP_WARN("getrootnodes of service_type: %llu failed", service_type);
    return false;
}

bool ServiceNodes::GetRootNodes(uint64_t service_type, const std::string & des_node_id, kadmlia::NodeInfoPtr & node_ptr) {
    if (FindNode(service_type, des_node_id, node_ptr)) {
        TOP_DEBUG("getrootnodes of service_type: %llu ok", service_type);
        return true;
    }

    NetNode Fnode;
    if (small_net_nodes_->FindNewNode(Fnode, service_type)) {
        base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(Fnode.m_account, true);  // kRoot id
        assert(kad_key);
        using namespace std::placeholders;
        auto cb = std::bind(&ServiceNodes::OnGetRootNodesAsync, this, _1, _2);
        RootRoutingManager::Instance()->GetRootNodesV2Async(kad_key->Get(), service_type, cb);  // just call
    }
    TOP_WARN("getrootnodes of service_type: %llu failed", service_type);
    return false;
}

void ServiceNodes::OnGetRootNodesAsync(uint64_t service_type, const std::vector<kadmlia::NodeInfoPtr> & node_vec) {
    for (auto & n : node_vec) {
        AddNode(service_type, n);
    }
}

bool ServiceNodes::GetRootNodes(uint64_t service_type, const std::string & account, std::vector<kadmlia::NodeInfoPtr> & node_vec) {
    if (FindNode(service_type, node_vec)) {
        TOP_DEBUG("getrootnodes of service_type: %llu ok, size: %d", service_type, node_vec.size());
        return true;
    }

    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(account, true);  // kRoot id
    if (!kad_key) {
        return false;
    }

    {
        using namespace std::placeholders;
        auto cb = std::bind(&ServiceNodes::OnGetRootNodesAsync, this, _1, _2);
        RootRoutingManager::Instance()->GetRootNodesV2Async(kad_key->Get(), service_type, cb);  // just call
    }
    TOP_WARN("getrootnodes of service_type:%llu account:%s failed", service_type, account.c_str());
    return false;
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

bool ServiceNodes::FindNode(uint64_t service_type, kadmlia::NodeInfoPtr & node) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    auto ifind = service_nodes_cache_map_.find(service_type);
    if (ifind == service_nodes_cache_map_.end()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type);
        return false;
    }
    if (ifind->second.empty()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type);
        return false;
    }

    auto size = (ifind->second).size();
    uint32_t index = RandomUint32() % size;
    node = (ifind->second)[index];
    TOP_DEBUG("find node:(%s:%d) service_node of service_type: %llu", (node->public_ip).c_str(), node->public_port, service_type);
    return true;
}

bool ServiceNodes::FindNode(uint64_t service_type, const std::string & des_node_id, kadmlia::NodeInfoPtr & node) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    auto ifind = service_nodes_cache_map_.find(service_type);
    if (ifind == service_nodes_cache_map_.end()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type);
        return false;
    }
    if (ifind->second.empty()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type);
        return false;
    }
    for (auto & node_ptr : ifind->second) {
        if (node_ptr->node_id == des_node_id) {
            node = node_ptr;
            TOP_DEBUG("find node of des_node_id:%s directly ok", HexEncode(des_node_id).c_str());
            break;
        }
    }
    if (!node) {
        auto size = (ifind->second).size();
        uint32_t index = RandomUint32() % size;
        node = (ifind->second)[index];  // random
    }

    TOP_DEBUG("find node:(%s:%d) service_node of service_type: %llu", (node->public_ip).c_str(), node->public_port, service_type);
    return true;
}

bool ServiceNodes::FindNode(uint64_t service_type, std::vector<kadmlia::NodeInfoPtr> & node_vec) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    auto ifind = service_nodes_cache_map_.find(service_type);
    if (ifind == service_nodes_cache_map_.end()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type);
        return false;
    }
    if (ifind->second.empty()) {
        TOP_WARN("can't find service_node of service_type: %llu", service_type);
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
    TOP_DEBUG("find  %d service_node of service_type: %llu", size, service_type);
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

bool ServiceNodes::AddNode(uint64_t service_type, kadmlia::NodeInfoPtr node) {
    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(node->node_id);
    uint64_t node_service_type = kad_key->GetServiceType();
    if (node_service_type != service_type) {
        TOP_WARN("node[%s](%s:%d)node_service_type: %llu not equal service_type: %llu",
                 HexEncode(node->node_id).c_str(),
                 (node->public_ip).c_str(),
                 node->public_port,
                 node_service_type,
                 service_type);
        return false;
    }

    // TODO(smaug) set limit of size
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    auto ifind = service_nodes_cache_map_.find(service_type);
    if (ifind == service_nodes_cache_map_.end()) {
        std::vector<kadmlia::NodeInfoPtr> tmp_nodes;
        tmp_nodes.push_back(node);
        service_nodes_cache_map_.insert(std::pair<uint64_t, std::vector<kadmlia::NodeInfoPtr>>(service_type, tmp_nodes));
    } else {
        for (auto & nptr : ifind->second) {
            if (nptr->node_id == node->node_id) {
                TOP_DEBUG("already has node:%s %s:%d for service_type: %llu", HexEncode(node->node_id).c_str(), (node->public_ip).c_str(), node->public_port, service_type);
                return false;
            }
        }
        (ifind->second).push_back(node);
    }
    TOP_DEBUG("addnode service_type: %llu, service_node:%s %s:%d  now size: %d",
              service_type,
              HexEncode(node->node_id).c_str(),
              (node->public_ip).c_str(),
              node->public_port,
              service_nodes_cache_map_[service_type].size());
    return true;
}

void ServiceNodes::RemoveExpired(const std::unordered_map<uint64_t, std::vector<std::string>> & expired_node_vec) {
    std::unique_lock<std::mutex> lock(service_nodes_cache_map_mutex_);
    for (const auto & exitem : expired_node_vec) {
        auto ifind = service_nodes_cache_map_.find(exitem.first);
        if (ifind == service_nodes_cache_map_.end()) {
            continue;
        }
        for (auto iter = service_nodes_cache_map_[exitem.first].begin(); iter != service_nodes_cache_map_[exitem.first].end();) {
            auto tfind = std::find(exitem.second.begin(), exitem.second.end(), (*iter)->node_id);
            if (tfind != exitem.second.end()) {
                // find expired node
                TOP_DEBUG("remove expired service node service_type:%llu id:%s %s:%d",
                          exitem.first,
                          HexEncode((*iter)->node_id).c_str(),
                          ((*iter)->public_ip).c_str(),
                          (*iter)->public_port);
                iter = service_nodes_cache_map_[exitem.first].erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

void ServiceNodes::do_update() {
    std::set<uint64_t> service_type_vec;
    small_net_nodes_->GetAllServiceType(service_type_vec);
    TOP_DEBUG("small net nodes getallservicetype size: %d", service_type_vec.size());
    for (auto & item : service_type_vec) {
        uint64_t service_type = item;
        TOP_DEBUG("begin do_update service_type: %llu", service_type);
        std::vector<NetNode> node_vec;
        if (!small_net_nodes_->FindAllNode(node_vec, service_type) || node_vec.empty()) {
            TOP_WARN("can't find nodes of service_type: %llu", service_type);
            continue;
        }

        const auto rand_index = RandomUint32() % node_vec.size();
        std::string account = node_vec[rand_index].m_account;
        base::KadmliaKeyPtr kad_key = GetKadmliaKey(node_vec[rand_index].m_xip, account);
        if (CheckHasNode(kad_key)) {
            continue;
        }
        TOP_DEBUG("blueroot do update by account:%s, index:%u", account.c_str(), rand_index);
        base::KadmliaKeyPtr root_kad_key = base::GetKadmliaKey(account, true);  // kRoot id
        assert(root_kad_key);
        using namespace std::placeholders;
        auto cb = std::bind(&ServiceNodes::OnGetRootNodesAsync, this, _1, _2);
        RootRoutingManager::Instance()->GetRootNodesV2Async(root_kad_key->Get(), service_type, cb);  // just call
    }                                                                                                // end for (auto& item
}

}  // end namespace wrouter

}  // end namespace top
