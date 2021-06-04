// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/multi_routing.h"

#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/routing_table.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/root/root_routing_manager.h"

#include <list>

namespace top {

using namespace kadmlia;

namespace wrouter {

// static const int32_t kCheckSingleNodeNetworkPeriod = 5 * 1000 * 1000;
static const int32_t kCheckElectRoutingTableNodesPeriod = 5 * 1000 * 1000;
// static const uint32_t kCheckSingleNetworkNodesNum = 16;

MultiRouting::MultiRouting() : routing_table_map_(), routing_table_map_mutex_(), root_manager_ptr_(nullptr) {
    // auto thread_callback = [this] {
    //     for (;;) {
    //         CheckSingleNodeNetwork();
    //     }
    // };
    // check_single_network_thread_ = std::make_shared<std::thread>(thread_callback);
    // check_single_network_thread_->detach();
    // timer_.Start(kCheckSingleNodeNetworkPeriod, kCheckSingleNodeNetworkPeriod, std::bind(&MultiRouting::NotifyCheckSignal, this));

    check_elect_routing_ = std::make_shared<base::TimerRepeated>(timer_manager_, "MultiRouting::CheckElectRoutingTable");
    check_elect_routing_->Start(kCheckElectRoutingTableNodesPeriod, kCheckElectRoutingTableNodesPeriod, std::bind(&MultiRouting::CheckElectRoutingTable, this));
}

MultiRouting::~MultiRouting() {
    // timer_.Join();
    TOP_KINFO("MultiRouting destroy");
}

MultiRouting * MultiRouting::Instance() {
    static MultiRouting ins;
    return &ins;
}

void MultiRouting::SetRootRoutingManager(std::shared_ptr<RootRoutingManager> root_manager_ptr) {
    root_manager_ptr_ = root_manager_ptr;
}

void MultiRouting::AddRoutingTable(base::ServiceType type, RoutingTablePtr routing_table) {
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    if (!routing_table) {
        return;
    }
    auto iter = routing_table_map_.find(type);
    if (iter != routing_table_map_.end()) {
        assert(false);
        return;
    }

    routing_table_map_[type] = routing_table;
}

void MultiRouting::RemoveRoutingTable(base::ServiceType type) {
    if (type == base::ServiceType{kRoot} && root_manager_ptr_) {
        root_manager_ptr_->Destory();
        TOP_KINFO("remove root routing table: %llu", type.value());
    }

    RoutingTablePtr remove_routing_table = nullptr;
    {
        std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
        auto iter = routing_table_map_.find(type);
        if (iter != routing_table_map_.end()) {
            remove_routing_table = iter->second;
            routing_table_map_.erase(iter);
        }
    }
    if (remove_routing_table) {
        remove_routing_table->UnInit();
        TOP_KINFO("remove service routing table: %llu", type.value());
    }

    std::vector<base::ServiceType> vec_type;
    GetAllRegisterType(vec_type);
    for (auto & v : vec_type) {
        TOP_KINFO("after unregister routing table, still have %llu", v.value());
    }
}

RoutingTablePtr MultiRouting::GetRoutingTable(const base::ServiceType & type, bool root) {
    if (root || type.value() == kRoot) {
        if (!root_manager_ptr_) {
            return nullptr;
        }
        return root_manager_ptr_->GetRoutingTable(type);
    }
    return GetServiceRoutingTable(type);
}

RoutingTablePtr MultiRouting::GetRoutingTable(const std::string & routing_id, bool root) {
    if (root) {
        if (!root_manager_ptr_) {
            return nullptr;
        }
        return root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot});
    }
    return GetServiceRoutingTable(routing_id);
}

RoutingTablePtr MultiRouting::GetServiceRoutingTable(const base::ServiceType & type) {
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);

    for (auto const & _p : routing_table_map_) {
        if (_p.first == type) {
            return _p.second;
        }
    }
    return nullptr;

    // auto iter = routing_table_map_.find(type);
    
    // if (iter == routing_table_map_.end()) {
    //     return nullptr;
    // }
    // return iter->second;
}

RoutingTablePtr MultiRouting::GetServiceRoutingTable(const std::string & routing_id) {
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    for (auto & item : routing_table_map_) {
        auto routing_ptr = item.second;
        if (routing_ptr->get_local_node_info()->id() == routing_id) {
            return routing_ptr;
        }
    }
    return nullptr;
}

void MultiRouting::GetAllRegisterType(std::vector<base::ServiceType> & vec_type) {
    vec_type.clear();
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    for (auto & it : routing_table_map_) {
        vec_type.push_back(it.first);
    }
}

void MultiRouting::GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::RoutingTable>> & vec_rt) {
    vec_rt.clear();
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    for (auto & it : routing_table_map_) {
        vec_rt.push_back(it.second);
    }
}

/*

bool MultiRouting::SetCacheServiceType(uint64_t service_type) {
    if (!root_manager_ptr_) {
        TOP_ERROR("MultiRouting:: root_manager_ptr is null");
        return false;
    }
    return root_manager_ptr_->SetCacheServiceType(service_type);
}

bool MultiRouting::GetServiceBootstrapRootNetwork(uint64_t service_type, std::set<std::pair<std::string, uint16_t>> & boot_endpoints) {
    if (!root_manager_ptr_) {
        TOP_ERROR("MultiRouting:: root_manager_ptr is null");
        return false;
    }
    return root_manager_ptr_->GetServiceBootstrapRootNetwork(service_type, boot_endpoints);
}

void MultiRouting::WaitCheckSignal() {
    TOP_DEBUG("bluesig wait");
    std::unique_lock<std::mutex> lock(check_single_network_mutex_);
    check_single_network_condv_.wait(lock);
}

void MultiRouting::NotifyCheckSignal() {
    TOP_DEBUG("bluesig notify");
    std::unique_lock<std::mutex> lock(check_single_network_mutex_);
    check_single_network_condv_.notify_one();
}

void MultiRouting::CheckSingleNodeNetwork() {
    WaitCheckSignal();
    // todo charles add it back!
    TOP_DEBUG("bluesig check");
    std::vector<kadmlia::RoutingTablePtr> routing_vec;
    {
        std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
        for (auto iter = routing_table_map_.begin(); iter != routing_table_map_.end(); ++iter) {
            if (iter->second->nodes_size() <= kCheckSingleNetworkNodesNum) {
                routing_vec.push_back(iter->second);
            }
        }
    }

    for (auto iter = routing_vec.begin(); iter != routing_vec.end(); ++iter) {
        auto service_type = (*iter)->get_local_node_info()->kadmlia_key()->GetServiceType();
        base::KadmliaKeyPtr kad_key = nullptr;
        wrouter::NetNode ele_first_node;
        if (!(SmallNetNodes::Instance()->FindRandomNode(ele_first_node, service_type))) {
            TOP_WARN("Findnode small_node_cache invalid");
            continue;
        }

        TOP_DEBUG("findnode small_node_cache account:%s, action CheckSingleNodeNetWork for service_type:%llu", ele_first_node.m_account.c_str(), service_type);
        auto tmp_service_type = base::CreateServiceType(ele_first_node.m_xip);
        if (tmp_service_type != service_type) {
            TOP_WARN("small_node_cache find service_type: %llu not equal elect_service_type: %llu", tmp_service_type, service_type);
            continue;
        }
        kad_key = base::GetKadmliaKey(ele_first_node.m_account, true);  // kRoot id
        if (!kad_key) {
            TOP_WARN("small_node_cache kad_key nullptr");
            continue;
        }

        std::vector<kadmlia::NodeInfoPtr> ret_nodes;
        int res = GetSameNetworkNodesV2(kad_key->Get(), service_type, ret_nodes);
        TOP_DEBUG("find neighbors running.[res:%d][size:%d][%d][%d][%d][%d][%d] [%s]",
                  res,
                  ret_nodes.size(),
                  kad_key->xnetwork_id(),
                  kad_key->zone_id(),
                  kad_key->cluster_id(),
                  kad_key->group_id(),
                  kad_key->xip_type(),
                  HexEncode(kad_key->Get()).c_str());
        if (res == kadmlia::kKadSuccess) {
            if (ret_nodes.empty()) {
                continue;
            }

            auto node_ptr = ret_nodes[RandomUint32() % ret_nodes.size()];
            (*iter)->FindCloseNodesWithEndpoint(node_ptr->node_id, std::make_pair(node_ptr->public_ip, node_ptr->public_port));
            TOP_DEBUG("find neighbors running ok, get same network and join it[%d][%d][%d][%d][%d][%d][%s][ip:%s][port:%d]",
                      kad_key->xnetwork_id(),
                      kad_key->zone_id(),
                      kad_key->cluster_id(),
                      kad_key->group_id(),
                      kad_key->xip_type(),
                      kad_key->network_type(),
                      HexEncode(node_ptr->node_id).c_str(),
                      node_ptr->public_ip.c_str(),
                      node_ptr->public_port);
        }
    }
}
    */


void MultiRouting::CheckElectRoutingTable(){
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    for (auto _p : routing_table_map_) {
        kadmlia::RoutingTablePtr routing_table = _p.second;
        auto kad_key_ptrs = routing_table->GetElectionNodesExpected();
        if (!kad_key_ptrs.empty()) {
            std::map<std::string, kadmlia::NodeInfoPtr>  res_nodes; // election_node_id, NodeInfoPtr
            root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot})->FindElectionNodesInfo(kad_key_ptrs, res_nodes);
            routing_table->HandleElectionNodesInfoFromRoot(res_nodes);
        }
    }
}
}  // namespace wrouter

}  // namespace top
