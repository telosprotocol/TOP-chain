// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/multi_routing.h"

#include <list>

#include "xpbase/base/top_log.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xpbase/base/kad_key/chain_kadmlia_key.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/routing_table.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xpbase/base/top_utils.h"

namespace top {

using namespace kadmlia;

namespace wrouter {

static const int32_t kCheckSingleNodeNetworkPeriod = 5 * 1000 * 1000;
static const uint32_t kCheckSingleNetworkNodesNum = 16;

MultiRouting::MultiRouting()
        : routing_table_map_(),
          routing_table_map_mutex_(),
          root_manager_ptr_(nullptr) {
    auto thread_callback = [this] {
        for (;;) {
            CheckSingleNodeNetwork();
        }
    };
    check_single_network_thread_ = std::make_shared<std::thread>(thread_callback);
    check_single_network_thread_->detach();
    timer_.Start(
            kCheckSingleNodeNetworkPeriod,
            kCheckSingleNodeNetworkPeriod,
            std::bind(&MultiRouting::NotifyCheckSignal, this));
}

MultiRouting::~MultiRouting() {
    timer_.Join();
    TOP_KINFO("MultiRouting destroy");
}

MultiRouting* MultiRouting::Instance() {
    static MultiRouting ins;
    return &ins;
}

void MultiRouting::SetRootRoutingManager(std::shared_ptr<RootRoutingManager> root_manager_ptr) {
    root_manager_ptr_ = root_manager_ptr;
}

void MultiRouting::AddRoutingTable(uint64_t type, RoutingTablePtr routing_table) {
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    if(!routing_table) {
        return;
    }
    auto iter = routing_table_map_.find(type);
    if (iter != routing_table_map_.end()) {
        assert(false);
        return;
    }

    routing_table_map_[type] = routing_table;
}

void MultiRouting::RemoveRoutingTable(uint64_t type) {
    if (root_manager_ptr_) {
        root_manager_ptr_->RemoveRoutingTable(type);
        TOP_KINFO("remove root routing table: %llu", type);
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
        TOP_KINFO("remove service routing table: %llu", type);
    }

    std::vector<uint64_t> vec_type;
    GetAllRegisterType(vec_type);
    for (auto& v : vec_type) {
        TOP_KINFO("after unregister routing table, still have %llu", v);
    }
}

void MultiRouting::RemoveAllRoutingTable() {
    if (root_manager_ptr_) {
        root_manager_ptr_->RemoveAllRoutingTable();
        TOP_KINFO("remove all root routing table");
    }

    decltype(routing_table_map_) rt_map;
    {
        std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
        rt_map = routing_table_map_;
    }

    for (auto it = rt_map.begin(); it != rt_map.end();) {
        it->second->UnInit();
        it = rt_map.erase(it);
        TOP_KINFO("remove all service routing table");
    }
}

RoutingTablePtr MultiRouting::GetRoutingTable(const uint64_t& type, bool root) {
    if (root || type == kRoot) {
        if (!root_manager_ptr_) {
            return nullptr;
        }
        return root_manager_ptr_->GetRoutingTable(type);
    }
    return GetServiceRoutingTable(type);
}

RoutingTablePtr MultiRouting::GetRoutingTable(const std::string& routing_id, bool root) {
    if (root) {
        if (!root_manager_ptr_) {
            return nullptr;
        }
        return root_manager_ptr_->GetRoutingTable(routing_id);
    }
    return GetServiceRoutingTable(routing_id);
}

// base src_service_type and des_service_type, determine which routing table to be choosen
// not considering root routing
RoutingTablePtr MultiRouting::GetSmartRoutingTable(uint64_t type) {
    {
        std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
        auto iter = routing_table_map_.find(type);
        if (iter != routing_table_map_.end()) {
            return iter->second;
        }
    }

    // TODO(Charlie): may be error
    base::KadmliaKeyPtr kad_key = base::GetKadmliaKey(type);
    if (kad_key->network_type() == kRoleService) {
        type = kad_key->GetServiceType(kRoleEdge);
        return GetSmartRoutingTable(type);
    }
    TOP_WARN("get smart object failed![%llu]", type);
    return nullptr;
}

uint64_t MultiRouting::TryGetSmartRoutingTable(uint64_t type) {
    {
        std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
        auto iter = routing_table_map_.find(type);
        if (iter != routing_table_map_.end()) {
            return type;
        }
    }
    // not found dest type, then choose the right one routing_table base src_service_type and des_service_type
    // usually des_service_type only consider server(not edge) in switch
    switch (type) {
        case top::kXVPN : {
                return kEdgeXVPN;
            }
        case top::kTopStorage : {
                return kEdgeTopStorage;
            }
        // TODO(smaug) add more kinds of service/server

        default :
            break;
    } // end switch (des_service_type ...
    return kInvalidType;
}

RoutingTablePtr MultiRouting::GetServiceRoutingTable(const uint64_t& type) {
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    auto iter = routing_table_map_.find(type);
    if (iter == routing_table_map_.end()) {
        return nullptr;
    }
    return iter->second;
}

RoutingTablePtr MultiRouting::GetServiceRoutingTable(const std::string& routing_id) {
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    for (auto& item : routing_table_map_) {
        auto routing_ptr = item.second;
        if (routing_ptr->get_local_node_info()->id() == routing_id) {
            return routing_ptr;
        }
    }
    return nullptr;
}

void MultiRouting::GetAllRegisterType(std::vector<uint64_t>& vec_type) {
    vec_type.clear();
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    for (auto& it : routing_table_map_) {
        vec_type.push_back(it.first);
    }
}

void MultiRouting::GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::RoutingTable>>& vec_rt) {
    vec_rt.clear();
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    for (auto& it : routing_table_map_) {
        vec_rt.push_back(it.second);
    }
}

bool MultiRouting::CheckTypeExist(uint64_t type) {
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    auto iter = routing_table_map_.find(type);
    if (iter != routing_table_map_.end()) {
        return true;
    }
    return false;
}

bool MultiRouting::SetCacheServiceType(uint64_t service_type) {
    if (!root_manager_ptr_) {
        TOP_ERROR("MultiRouting:: root_manager_ptr is null");
        return false;
    }
    return root_manager_ptr_->SetCacheServiceType(service_type);
}


bool MultiRouting::GetServiceBootstrapRootNetwork(
        uint64_t service_type,
        std::set<std::pair<std::string, uint16_t>>& boot_endpoints) {
    if (!root_manager_ptr_) {
        TOP_ERROR("MultiRouting:: root_manager_ptr is null");
        return false;
    }
    return root_manager_ptr_->GetServiceBootstrapRootNetwork(service_type, boot_endpoints);
}

// wrapper of Routingtable::SendToClosestNode
// attention: this function will change src_node_id and src_service_type, be careful
void MultiRouting::SendToNetwork(transport::protobuf::RoutingMessage& message, bool add_hop) {
    RoutingTablePtr routing_table = GetSmartRoutingTable(message.des_service_type());
    LocalNodeInfoPtr local_node = nullptr;
    if (routing_table) {
        local_node = routing_table->get_local_node_info();
        if (!local_node) {
            TOP_WARN("SendToNetwork failed, get_local_node_info null");
            return;
        }
        message.set_src_service_type(local_node->service_type());
        message.set_src_node_id(local_node->id());
        uint32_t des_network_id = GetXNetworkID(message.des_node_id());
        message.set_des_service_type(des_network_id);
        TOP_DEBUG("SendToNetwork:: type(%d) local_service_type(%llu)",
                message.type(),
                local_node->service_type());
        return routing_table->SendToClosestNode(message, add_hop);
    }

    // then choose any one
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    auto iter = routing_table_map_.begin();
    routing_table = iter->second;
    local_node = routing_table->get_local_node_info();
    if (!local_node) {
        TOP_WARN("SendToNetwork failed, get_local_node_info null");
        return;
    }

    message.set_src_service_type(local_node->service_type());
    message.set_src_node_id(local_node->id());
    TOP_DEBUG("SendToNetwork:: type(%d) local_service_type(%llu)",
            message.type(),
            local_node->service_type());
    return routing_table->SendToClosestNode(message, add_hop);
}

// this function will not change anything of message, just choose the right routing table, then sendtoclosestnode
void MultiRouting::SendToNetwork(const transport::protobuf::RoutingMessage& message, bool add_hop) {
    RoutingTablePtr routing_table = GetSmartRoutingTable(message.des_service_type());
    LocalNodeInfoPtr local_node = nullptr;
    if (routing_table) {
        TOP_DEBUG("SendToNetwork:: type(%d)",
                message.type());
        return routing_table->SendToClosestNode(const_cast<transport::protobuf::RoutingMessage&>(message),
                add_hop);
    }

    // then choose any one
    std::unique_lock<std::mutex> lock(routing_table_map_mutex_);
    auto iter = routing_table_map_.begin();
    routing_table = iter->second;
    local_node = routing_table->get_local_node_info();
    if (!local_node) {
        TOP_WARN("SendToNetwork failed, get_local_node_info null");
        return;
    }

    TOP_DEBUG("SendToNetwork:: type(%d)",
            message.type());
    return routing_table->SendToClosestNode(
            const_cast<transport::protobuf::RoutingMessage&>(message),
            add_hop);
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

        TOP_DEBUG("findnode small_node_cache account:%s, action CheckSingleNodeNetWork for service_type:%llu",
                ele_first_node.m_account.c_str(),
                service_type);
        auto tmp_service_type = base::CreateServiceType(ele_first_node.m_xip);
        if (tmp_service_type != service_type) {
            TOP_WARN("small_node_cache find service_type: %llu not equal elect_service_type: %llu", tmp_service_type, service_type);
            continue;
        }
        kad_key = base::GetKadmliaKey(ele_first_node.m_account, true); // kRoot id
        if (!kad_key) {
            TOP_WARN("small_node_cache kad_key nullptr");
            continue;
        }

        std::vector<kadmlia::NodeInfoPtr> ret_nodes;
        int res = GetSameNetworkNodesV2(kad_key->Get(),service_type, ret_nodes);
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
            /*
            std::set<std::pair<std::string, uint16_t>> join_endpoints;
            for (auto& ptr : ret_nodes) {
                join_endpoints.insert(std::make_pair(ptr->public_ip, ptr->public_port));
                TOP_DEBUG("get same network node: %s:%d", (ptr->public_ip).c_str(), ptr->public_port);
            }
            (*iter)->MultiJoinAsync(join_endpoints);
            TOP_DEBUG("find neighbors running ok, get same network and multijoinasync it[%d][%d][%d][%d][%d][%d]",
                    kad_key->xnetwork_id(),
                    kad_key->zone_id(),
                    kad_key->cluster_id(),
                    kad_key->group_id(),
                    kad_key->xip_type(),
                    kad_key->network_type());
                    */

            auto node_ptr = ret_nodes[RandomUint32() % ret_nodes.size()];
            (*iter)->FindCloseNodesWithEndpoint(
                    node_ptr->node_id,
                    std::make_pair(node_ptr->public_ip, node_ptr->public_port));
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

}  // namespace wrouter

}  // namespace top
