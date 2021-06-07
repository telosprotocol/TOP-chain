// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/multi_routing.h"

#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/routing_table_base.h"
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

MultiRouting::MultiRouting() : elect_routing_table_map_(), elect_routing_table_map_mutex_(), root_manager_ptr_(nullptr) {
    // auto thread_callback = [this] {
    //     for (;;) {
    //         CheckSingleNodeNetwork();
    //     }
    // };
    // check_single_network_thread_ = std::make_shared<std::thread>(thread_callback);
    // check_single_network_thread_->detach();
    // timer_.Start(kCheckSingleNodeNetworkPeriod, kCheckSingleNodeNetworkPeriod, std::bind(&MultiRouting::NotifyCheckSignal, this));

    check_elect_routing_ = std::make_shared<base::TimerRepeated>(timer_manager_, "MultiRouting::CheckElectRoutingTableTimer");
    check_elect_routing_->Start(kCheckElectRoutingTableNodesPeriod, kCheckElectRoutingTableNodesPeriod, std::bind(&MultiRouting::CompleteElectRoutingTable, this));
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

kadmlia::RoutingTablePtr MultiRouting::GetRoutingTable(base::ServiceType const &service_type,bool is_root){
    if(is_root){
        if (!root_manager_ptr_) {
            return nullptr;
        }
        return root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot});
    }else{
        for (auto const & _p : elect_routing_table_map_) {
            if (_p.first == service_type) {
                return _p.second;
            }
        }
        return nullptr;
    }
}

kadmlia::ElectRoutingTablePtr MultiRouting::GetElectRoutingTable(base::ServiceType const & service_type) {
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    for (auto const & _p : elect_routing_table_map_) {
        if (_p.first == service_type) {
            return _p.second;
        }
    }
    return nullptr;
}

kadmlia::RootRoutingTablePtr MultiRouting::GetRootRoutingTable() {
    if (!root_manager_ptr_) {
        return nullptr;
    }
    return root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot});
}

void MultiRouting::AddElectRoutingTable(base::ServiceType service_type, kadmlia::ElectRoutingTablePtr routing_table) {
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    if (!routing_table) {
        return;
    }
    auto iter = elect_routing_table_map_.find(service_type);
    if (iter != elect_routing_table_map_.end()) {
        assert(false);
        return;
    }

    elect_routing_table_map_[service_type] = routing_table;
}

void MultiRouting::RemoveElectRoutingTable(base::ServiceType service_type) {
    if (service_type == base::ServiceType{kRoot} && root_manager_ptr_) {
        root_manager_ptr_->Destory();
        TOP_KINFO("remove root routing table: %llu", service_type.value());
    }

    ElectRoutingTablePtr remove_routing_table = nullptr;
    {
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
        auto iter = elect_routing_table_map_.find(service_type);
        if (iter != elect_routing_table_map_.end()) {
            remove_routing_table = iter->second;
            elect_routing_table_map_.erase(iter);
        }
    }
    if (remove_routing_table) {
        remove_routing_table->UnInit();
        TOP_KINFO("remove service routing table: %llu", service_type.value());
    }

    std::vector<base::ServiceType> vec_type;
    GetAllRegisterType(vec_type);
    for (auto & v : vec_type) {
        TOP_KINFO("after unregister routing table, still have %llu", v.value());
    }
}

// RoutingTablePtr MultiRouting::GetRoutingTable(const base::ServiceType & type, bool root) {
//     if (root || type.value() == kRoot) {
//         if (!root_manager_ptr_) {
//             return nullptr;
//         }
//         return root_manager_ptr_->GetRoutingTable(type);
//     }
//     return GetServiceRoutingTable(type);
// }

// RoutingTablePtr MultiRouting::GetRoutingTable(const std::string & routing_id, bool root) {
//     if (root) {
//         if (!root_manager_ptr_) {
//             return nullptr;
//         }
//         return root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot});
//     }
//     return GetServiceRoutingTable(routing_id);
// }

// RoutingTablePtr MultiRouting::GetServiceRoutingTable(const base::ServiceType & type) {
//     std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);

//     for (auto const & _p : elect_routing_table_map_) {
//         if (_p.first == type) {
//             return _p.second;
//         }
//     }
//     return nullptr;

//     // auto iter = elect_routing_table_map_.find(type);

//     // if (iter == elect_routing_table_map_.end()) {
//     //     return nullptr;
//     // }
//     // return iter->second;
// }

// RoutingTablePtr MultiRouting::GetServiceRoutingTable(const std::string & routing_id) {
//     std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
//     for (auto & item : elect_routing_table_map_) {
//         auto routing_ptr = item.second;
//         if (routing_ptr->get_local_node_info()->id() == routing_id) {
//             return routing_ptr;
//         }
//     }
//     return nullptr;
// }

void MultiRouting::GetAllRegisterType(std::vector<base::ServiceType> & vec_type) {
    vec_type.clear();
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    for (auto & it : elect_routing_table_map_) {
        vec_type.push_back(it.first);
    }
}

void MultiRouting::GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::ElectRoutingTable>> & vec_rt) {
    vec_rt.clear();
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    for (auto & it : elect_routing_table_map_) {
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
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
        for (auto iter = elect_routing_table_map_.begin(); iter != elect_routing_table_map_.end(); ++iter) {
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

void MultiRouting::CheckElectRoutingTable(base::ServiceType service_type) {
    kadmlia::ElectRoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
        assert(elect_routing_table_map_.find(service_type) != elect_routing_table_map_.end());
        routing_table = elect_routing_table_map_[service_type];
    }
    auto kad_key_ptrs = routing_table->GetElectionNodesExpected();
    if (!kad_key_ptrs.empty()) {
        std::map<std::string, kadmlia::NodeInfoPtr> res_nodes;  // election_node_id, NodeInfoPtr
        root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot})->FindElectionNodesInfo(kad_key_ptrs, res_nodes);
        routing_table->HandleElectionNodesInfoFromRoot(res_nodes);
    }
}

void MultiRouting::CheckElectRoutingTableTimer() {
    std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
    for (auto _p : elect_routing_table_map_) {
        kadmlia::ElectRoutingTablePtr routing_table = _p.second;
        auto kad_key_ptrs = routing_table->GetElectionNodesExpected();
        if (!kad_key_ptrs.empty()) {
            std::map<std::string, kadmlia::NodeInfoPtr> res_nodes;  // election_node_id, NodeInfoPtr
            root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot})->FindElectionNodesInfo(kad_key_ptrs, res_nodes);
            routing_table->HandleElectionNodesInfoFromRoot(res_nodes);
        }
    }
}

void MultiRouting::CompleteElectRoutingTable() {
    bool flag{false};
    {
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);

        for (auto const & routing_table_pair : elect_routing_table_map_) {
            kadmlia::ElectRoutingTablePtr routing_table = routing_table_pair.second;
            // map<election_xip2_str,node_id_root_kad_key>
            auto kad_key_ptrs = routing_table->GetElectionNodesExpected();
            if (!kad_key_ptrs.empty()) {
                for (auto const & _p : kad_key_ptrs) {
                    OnCompleteElectRoutingTableCallback cb =
                        std::bind(&MultiRouting::OnCompleteElectRoutingTable, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
                    // OnCompleteElectRoutingTableCallback cb = std::bind(&MultiRouting::OnCompleteElectRoutingTable, this, routing_table_pair.first, _p.first,
                    // std::placeholders::_1);
                    if (std::dynamic_pointer_cast<RootRouting>(root_manager_ptr_->GetRoutingTable(base::ServiceType{kRoot}))
                            ->FindNodesFromOthers(routing_table_pair.first, _p.first, cb, _p.second) == false) {
                        flag = true;
                        break;
                    }
                }
            }
            if (flag)
                break;
        }
    }
    if (flag) {
        CheckElectRoutingTableTimer();
    }
}

void MultiRouting::OnCompleteElectRoutingTable(base::ServiceType const service_type, std::string const election_xip2, kadmlia::NodeInfoPtr const & node_info) {
    xdbg("Charles Debug MultiRouting::OnCompleteElectRoutingTable %s", election_xip2.c_str());
    kadmlia::ElectRoutingTablePtr routing_table;
    {
        std::unique_lock<std::mutex> lock(elect_routing_table_map_mutex_);
        if (elect_routing_table_map_.find(service_type) == elect_routing_table_map_.end()) {
            return;
        }
        assert(elect_routing_table_map_.find(service_type) != elect_routing_table_map_.end());
        routing_table = elect_routing_table_map_[service_type];
    }
    routing_table->OnFindNodesFromRootRouting(election_xip2, node_info);
}

}  // namespace wrouter

}  // namespace top
